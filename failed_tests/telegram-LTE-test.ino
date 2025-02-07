//WIRING INSTRUCTIONS FROM DISPLAY TO ESP32
// 1 to GROUND
// 2 to 3V3
// 3 to 18
// 4 to 23
// 5 to 4
// 6 to 5
// 7 to 15
// 8 to 3.3
// Additionally, SMS sending switch between 19 and GROUND


#include <Adafruit_GFX.h>    // Core graphics librar
#include <Adafruit_ST7735.h> // Hardware-specific library
//#include <Fonts/FreeSans9pt7b.h>//If you want to use fonts,
// Pin definitions for ESP32 display
#define TFT_CS    15
#define TFT_RST   4
#define TFT_DC    5
#define TFT_SCLK  18
#define TFT_MOSI  23

// Initialize ST7735 TFT library
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

#define TINY_GSM_MODEM_SIM7000
#define TINY_GSM_RX_BUFFER 1024 // Set RX buffer to 1Kb
#define SerialAT Serial1
#define DUMP_AT_COMMANDS // See all AT commands, if wanted
#define GSM_PIN "1234" // set GSM PIN, if any

// Your GPRS credentials, if any
const char apn[]  = "prepaid.dna.fi";     //SET TO YOUR APN
const char gprsUser[] = "";
const char gprsPass[] = "";
bool sent = false;
#define SMS_TARGET  "" // Set phone number, if you want to test SMS

#include <TinyGsmClient.h>

#ifdef DUMP_AT_COMMANDS  // if enabled it requires the streamDebugger lib
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, Serial);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

#define uS_TO_S_FACTOR 1000000ULL  // Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP  60          // Time ESP32 will go to sleep (in seconds)

#define UART_BAUD   115200
#define PIN_DTR     25
#define PIN_TX      27
#define PIN_RX      26
#define PWR_PIN     4

#include <UniversalTelegramBot.h>
#define BOT_TOKEN "7568394111:AAEc673Oxmk8AeYf_Pad_P_2dkgpvWaxF3Y"
const unsigned long BOT_MTBS = 1000;
TinyGsmClient client(modem);
UniversalTelegramBot bot(BOT_TOKEN, client);
unsigned long bot_lasttime; 


int counter, lastIndex, numberOfPieces = 24;
String pieces[24], input;
const int smsSwitchPin = 19;
int smsSwitch;
String gps_raw;
String myArray[20];
String strYear;
String strDate;
String strMonth;
String strDay;
String strHour;
String strMin;
String strSec;
String strTime;
String strVsat;
String res;
String strLat, strLon;
float lat, lon, year, month, day, hour;


int i = 0, j = 0;

void setup() {
  pinMode(smsSwitchPin, INPUT_PULLUP);
  tft.initR(INITR_BLACKTAB);
  tft.fillScreen(ST7735_BLUE);
  tft.setRotation(3);
  Serial.begin(115200);
  delay(10);
  SerialAT.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX);

  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  //tft.setFont(&FreeSans9pt7b);
  tft.setTextSize(0);
  tft.setCursor(0, 20);
  tft.print("Initializing system...");
  Serial.println("Initializing system...");
  if (!modem.restart()) {
    Serial.println("Failed to restart modem, attempting to continue without restarting");
  }
  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  Serial.println("Initializing modem...");
  
  tft.setCursor(0, 20);
  tft.print("Initializing modem...");
  if (!modem.init()) {
    Serial.println("Failed to restart modem, attempting to continue without restarting");
  }

  String name = modem.getModemName();
  delay(500);
  blankBox(0);
  Serial.println("Modem Name: " + name);
  tft.setCursor(0, 20);
  tft.print("Modem:\n" + name);

  //  drawBox(0, 3, "Modem name:\n" + name);
  String modemInfo = modem.getModemInfo();
  delay(500);
  Serial.println("Modem Info:\n" + modemInfo);


  // Unlock your SIM card with a PIN if needed
  if ( GSM_PIN && modem.getSimStatus() != 3 ) {
    modem.simUnlock(GSM_PIN);
  }
  modem.sendAT(" + CFUN = 0 ");
  if (modem.waitResponse(10000L) != 1) {
    DBG(" + CFUN = 0  false ");
  }
  delay(2000);
  blankBox(0);

  /*
    2 Automatic
    13 GSM only
    38 LTE only
    51 GSM and LTE only
  * * * */
  // CHANGE NETWORK MODE, IF NEEDED
  res = modem.setNetworkMode(38);
  if (res != "1") {
    DBG("setNetworkMode  false ");
    return ;
  }
  delay(200);

  /*
    1 CAT-M
    2 NB-Iot
    3 CAT-M and NB-IoT
  * * */
  // CHANGE PREFERRED MODE, IF NEEDED
  res = modem.setPreferredMode(1);
  if (res != "1") {
    DBG("setPreferredMode  false ");
    return ;
  }

  blankBox(0);
  tft.setCursor(0, 20);
  tft.print("Waiting for");
  tft.setCursor(0, 40);
  tft.print("GPRS network");

  Serial.println("\n\n\nWaiting for network...");
  //drawBox(0, 0, "Wait for network....");

  if (!modem.waitForNetwork()) {
    delay(10000);
    return;
  }

  if (modem.isNetworkConnected()) {
    blankBox(0);
    tft.setCursor(0, 20);
    tft.print("Connected to");
    tft.setCursor(0, 40);
    tft.print("GPRS network");
    Serial.println("Network connected");
    //    drawBox(1, 2, "Connected to GPRS network");
    delay(2000);
  }

  modem.gprsDisconnect();
  if (!modem.isGprsConnected()) {
    Serial.println("GPRS disconnected");
  } else {
    Serial.println("GPRS disconnect: Failed.");
  }
  modem.enableGPS();
}

bool readSMSswitch() {
  smsSwitch = digitalRead(smsSwitchPin);
  if (smsSwitch == 0) {

    Serial.println("Switch is off");
    drawBox(3, 1, "SMS NOT sent.");
  }
  else {
    Serial.println("Switch is on");
    drawBox(3, 1, "SMS IS sent.");
  }
  return smsSwitch;
}

void drawBox(int boxLoc, int txtColor, String strMsg) {
  tft.fillRect(0, boxLoc * 32, 160, 32, ST7735_BLUE);
  tft.setCursor(0, boxLoc * 32);
  if (txtColor == 1) {
    tft.setTextColor(ST7735_RED);
  }
  else if (txtColor == 2) {
    tft.setTextColor(ST7735_GREEN);
  }
  else if (txtColor == 3) {
    tft.setTextColor(ST7735_YELLOW);
  }
  else {
    tft.setTextColor(ST7735_WHITE);
  }
  tft.println(strMsg);
}

void blankBox(int boxLoc) {
  tft.fillRect(0, boxLoc * 32, 160, 32, ST7735_BLUE);
}

void gpsParse(String passedGPS) {
  Serial.println("Parsing string:");
  Serial.println(passedGPS);
  String tempStr;

  while ( j < passedGPS.length())
  {
    if (passedGPS.charAt(j) != ',')
    {
      tempStr += passedGPS.charAt(j);
    }
    else
    {
      myArray[i] = tempStr;
      tempStr = "";
      i++;
    }
    j++;
  }
  for (int j = 0; j < i; j++)
  {
    Serial.println(myArray[j]);
    Serial.print(' ');
  }
}

void timeDateParse() {
  String strTemp;
  strTemp = myArray[2];
  Serial.print("strTemp: ");
  Serial.println(strTemp);
  strYear = strTemp.substring(0, 4);
  strMonth = strTemp.substring(4, 6);
  strDay = strTemp.substring(6, 8);
  strHour = strTemp.substring(8, 10);
  strMin = strTemp.substring(10, 12);
  strSec = strTemp.substring(12, 14);
  strDate = (strDay + "." + strMonth + "." + strYear);
  strTime = (strHour + ":" + strMin + ":" + strSec);
  delay(5000);
  strTemp = "";
}

void handleNewMessages(int numNewMessages)
{
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));

  for (int i = 0; i < numNewMessages; i++)
  {
    String chat_id = bot.messages[i].chat_id;
    String text = bot.messages[i].text;

    String from_name = bot.messages[i].from_name;
    if (from_name == "")
      from_name = "Guest";

    if (text == "/send_test_action")
    {
      bot.sendChatAction(chat_id, "typing");
      delay(4000);
      bot.sendMessage(chat_id, "Did you see the action message?");

      // You can't use own message, just choose from one of bellow

      //typing for text messages
      //upload_photo for photos
      //record_video or upload_video for videos
      //record_audio or upload_audio for audio files
      //upload_document for general files
      //find_location for location data

      //more info here - https://core.telegram.org/bots/api#sendchataction
    }

    if (text == "/start")
    {
      String welcome = "Welcome to Universal Arduino Telegram Bot library, " + from_name + ".\n";
      welcome += "This is Chat Action Bot example.\n\n";
      welcome += "/send_test_action : to send test chat action message\n";
      bot.sendMessage(chat_id, welcome);
    }
  }
}

void loop() {
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
