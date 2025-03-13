#define HELTEC_POWER_BUTTON
#include <heltec_unofficial.h>
#include <Wire.h>

// LoRa Settings
#define FREQUENCY 866.3 // MHz
#define BANDWIDTH 250.0 // kHz
#define SPREADING_FACTOR 9
#define TRANSMIT_POWER 0

String rxdata;      // Stores received time data
volatile bool rxFlag = false;
int screenW = 128, screenH = 64;
int clockCenterX = screenW / 2, clockCenterY = (screenH - 16) / 2 + 16;

int hh = 0, mm = 0, ss = 0; // Store received time
bool timeReceived = false;   // Flag to check if time was received

// LoRa Setup
void radio_setup() {
  both.println("Radio init");
  RADIOLIB_OR_HALT(radio.begin());
  radio.setDio1Action(rx); // Set the callback function for received packets
  RADIOLIB_OR_HALT(radio.setFrequency(FREQUENCY));
  RADIOLIB_OR_HALT(radio.setBandwidth(BANDWIDTH));
  RADIOLIB_OR_HALT(radio.setSpreadingFactor(SPREADING_FACTOR));
  RADIOLIB_OR_HALT(radio.setOutputPower(TRANSMIT_POWER));
  RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
}

// Display update function
void updateDisplay() {
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  
  if (!timeReceived) {
    // Display waiting message if time hasn't been received
    display.setFont(ArialMT_Plain_18);
    display.drawString(clockCenterX, clockCenterY, "Waiting for");
    display.drawString(clockCenterX, clockCenterY + 26, "time...");
  } else {
    // Display received time once it's available
    display.setFont(ArialMT_Plain_24);
    char timenow[9];
    sprintf(timenow, "%02d:%02d:%02d", hh, mm, ss);
    display.drawString(clockCenterX, clockCenterY, timenow);
  }

  display.display();
}

void setup() {
  heltec_setup();
  Serial.begin(115200);
  Wire.begin();
  radio_setup();
  display.init();
  display.flipScreenVertically();
  updateDisplay();
}

void loop() {
  heltec_loop();

  // **Handle incoming LoRa message**
  if (rxFlag) {
    rxFlag = false;
    radio.readData(rxdata);
    
    if (_radiolib_status == RADIOLIB_ERR_NONE) {
      both.printf("RX [%s]\n", rxdata.c_str());
      
      // **Parse received time string (format HH:MM:SS)**
      int new_hh, new_mm, new_ss;
      if (sscanf(rxdata.c_str(), "%d:%d:%d", &new_hh, &new_mm, &new_ss) == 3) {
        hh = new_hh;
        mm = new_mm;
        ss = new_ss;
        timeReceived = true; // Mark time as received
        updateDisplay(); // Immediately update display with the received time
      }
    }
    
    // Ready for next message
    RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
  }
}

// LoRa interrupt (executed when message is received)
void rx() {
  rxFlag = true;
}
