  /**
     Send and receive LoRa-modulation packets with a sequence number, showing RSSI
     and SNR for received packets on the little display.
  
     Note that while this send and received using LoRa modulation, it does not do
     LoRaWAN. For that, see the LoRaWAN_TTN example.
  
     This works on the stick, but the output on the screen gets cut off.
  */
  
  
  
  // Turns the 'PRG' button into the power button, long press is off
  #define HELTEC_POWER_BUTTON   // must be before "#include <heltec_unofficial.h>"
  #include <heltec_unofficial.h>
  
  // Pause between transmited packets in seconds.
  // Set to zero to only transmit a packet when pressing the user button
  // Will not exceed 1% duty cycle, even if you set a lower value.
  #define PAUSE               300
  
  // Frequency in MHz. Keep the decimal point to designate float.
  // Check your own rules and regulations to see what is legal where you are.
  #define FREQUENCY           866.3       // for Europe
  // #define FREQUENCY           905.2       // for US
  
  // LoRa bandwidth. Keep the decimal point to designate float.
  // Allowed values are 7.8, 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125.0, 250.0 and 500.0 kHz.
  #define BANDWIDTH           250.0
  
  // Number from 5 to 12. Higher means slower but higher "processor gain",
  // meaning (in nutshell) longer range and more robust against interference.
  #define SPREADING_FACTOR    9
  
  // Transmit power in dBm. 0 dBm = 1 mW, enough for tabletop-testing. This value can be
  // set anywhere between -9 dBm (0.125 mW) to 22 dBm (158 mW). Note that the maximum ERP
  // (which is what your antenna maximally radiates) on the EU ISM band is 25 mW, and that
  // transmissting without an antenna can damage your hardware.
  #define TRANSMIT_POWER      0
  
  String rxdata;
  volatile bool rxFlag = false;
  long counter = 0;
  uint64_t last_tx = 0;
  uint64_t tx_time;
  uint64_t minimum_pause;
  
  #include <WiFi.h>
  #include <NTPClient.h>
  #include <WiFiClientSecure.h>
  #include <UniversalTelegramBot.h>
  
  // Wifi network station credentials
  #define WIFI_SSID ""
  #define WIFI_PASSWORD ""
  
  
  const long utcOffsetWinter = 7200; // Offset from UTC in seconds (3600 seconds = 1h) -- UTC+1 (Central European Winter Time)
  const long utcOffsetSummer = 10800;
  
  WiFiUDP udpSocket;
  NTPClient ntpClient(udpSocket, "pool.ntp.org", utcOffsetWinter);
  
  // Telegram BOT Token (Get from Botfather)
  #define BOT_TOKEN ""
  
  const unsigned long BOT_MTBS = 1000; // mean time between scan messages
  
  WiFiClientSecure secured_client;
  UniversalTelegramBot bot(BOT_TOKEN, secured_client);
  unsigned long bot_lasttime;          // last time messages' scan has been done
  String currentTime;
  char nextHalfHour[6]; // "HH:MM\0" (5 chars + null terminator)
  
  
  void radio_setup() {
    both.println("Radio init");
    RADIOLIB_OR_HALT(radio.begin());
    // Set the callback function for received packets
    radio.setDio1Action(rx);
    // Set radio parameters
    both.printf("Frequency: %.2f MHz\n", FREQUENCY);
    RADIOLIB_OR_HALT(radio.setFrequency(FREQUENCY));
    both.printf("Bandwidth: %.1f kHz\n", BANDWIDTH);
    RADIOLIB_OR_HALT(radio.setBandwidth(BANDWIDTH));
    both.printf("Spreading Factor: %i\n", SPREADING_FACTOR);
    RADIOLIB_OR_HALT(radio.setSpreadingFactor(SPREADING_FACTOR));
    both.printf("TX power: %i dBm\n", TRANSMIT_POWER);
    RADIOLIB_OR_HALT(radio.setOutputPower(TRANSMIT_POWER));
    // Start receiving
    RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
  }
  
  void handleNewMessages(int numNewMessages)
  {
    for (int i = 0; i < numNewMessages; i++)
    {
      // Inline buttons with callbacks when pressed will raise a callback_query message
      if (bot.messages[i].type == "callback_query")
      {
        Serial.print("Call back button pressed by: ");
        Serial.println(bot.messages[i].from_id);
        Serial.print("Data on the button: ");
        Serial.println(bot.messages[i].text);
        ntpClient.update();
        int hh = ntpClient.getHours();
        int mm = ntpClient.getMinutes();
        currentTime = String(hh) + ":" + String(mm);
        
        if (bot.messages[i].text == "Get Time") {
          bot.sendMessage(bot.messages[i].from_id, "it is: " + currentTime, "");
        } else if (bot.messages[i].text == "Reset") {
          get_next_half_hour(hh, mm);
          char message[32];  // Ensure it's large enough
          snprintf(message, sizeof(message), "Time set to: %s", nextHalfHour);
          radio_loop(nextHalfHour);
          bot.sendMessage(bot.messages[i].from_id, message, "");
        }
  
        Serial.println(currentTime);
  
      }
      else
      {
        String chat_id = bot.messages[i].chat_id;
        String text = bot.messages[i].text;
  
        String from_name = bot.messages[i].from_name;
        if (from_name == "")
          from_name = "Guest";
  
        if (text == "/options")
        {
          String keyboardJson = "[[{ \"text\" : \"Get Time\", \"callback_data\" : \"Get Time\" }],[{ \"text\" : \"Reset\", \"callback_data\" : \"Reset\" }]]";
          bot.sendMessageWithInlineKeyboard(chat_id, "Choose from one of the following options", "", keyboardJson);
        }
  
        if (text == "/start")
        {
          String welcome = "Welcome to the remote parking disc utility, " + from_name + ".\n";
          welcome += "/options : returns the inline keyboard with the commands\n";
  
          bot.sendMessage(chat_id, welcome, "Markdown");
        }
      }
    }
  }
  
  void get_next_half_hour(int hh, int mm) {
    if (mm < 30) {
      mm = 30;
    } else {
      mm = 0;
      hh = (hh + 1) % 24; // Ensure it rolls over at midnight
    }
  
    sprintf(nextHalfHour, "%02d:%02d", hh, mm);
  }
  
  void telegram_setup()
  {
    Serial.begin(115200);
    Serial.println();
  
    // attempt to connect to Wifi network:
    Serial.print("Connecting to Wifi SSID ");
    Serial.print(WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
    while (WiFi.status() != WL_CONNECTED)
    {
      Serial.print(".");
      delay(500);
    }
    Serial.print("\nWiFi connected. IP address: ");
    Serial.println(WiFi.localIP());
  
  }
  
  void setup() {
    heltec_setup();
    radio_setup();
    telegram_setup();
    ntpClient.begin();
  }
  
  void radio_loop(String data) {
      // Transmit data
      both.printf("TX [%s] ", String(counter).c_str());
      radio.clearDio1Action();
      heltec_led(50);
      tx_time = millis();
      RADIOLIB(radio.transmit(data));
      tx_time = millis() - tx_time;
      heltec_led(0);
  
      if (_radiolib_status == RADIOLIB_ERR_NONE) {
        both.printf("OK (%i ms)\n", (int)tx_time);
      } else {
        both.printf("fail (%i)\n", _radiolib_status);
        return; // If TX fails, don't proceed to RX
      }
  
      // Set the wait time to comply with duty cycle regulations
      minimum_pause = tx_time * 100;
      last_tx = millis();
  
      // **Immediately switch to receive mode to wait for a response**
      radio.setDio1Action(rx);
      RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
    // }
  
    // **Handle response reception**
    if (rxFlag) {
      rxFlag = false;
      radio.readData(rxdata);
      
      if (_radiolib_status == RADIOLIB_ERR_NONE) {
        both.printf("RX [%s]\n", rxdata.c_str());
        both.printf("  RSSI: %.2f dBm\n", radio.getRSSI());
        both.printf("  SNR: %.2f dB\n", radio.getSNR());
  
        // **Now that response is received, allow another TX cycle**
        last_tx = millis(); // Reset last_tx to allow next message after waiting
      }
  
      // Ready to receive another message
      RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
    }
  }
  
  void loop() {
    heltec_loop();
  
    if (millis() - bot_lasttime > BOT_MTBS)
    {
      int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
  
      while (numNewMessages)
      {
        Serial.println("got response");
        handleNewMessages(numNewMessages);
        numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      }
      bot_lasttime = millis();
    }
  }
  
  // Can't do Serial or display things here, takes too much time for the interrupt
  void rx() {
    rxFlag = true;
  }
