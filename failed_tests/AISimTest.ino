#include <TinyGsmClient.h>
#include <UniversalTelegramBot.h>



// Define GSM modem model
#define GSM_MODEM_MODEL_SIM7000 

// Pin definitions for GSM module (adjust if needed)
#define GSM_TX 27
#define GSM_RX 26

// SIM card & GSM Config
#define GSM_SERIAL Serial2
#define GSM_BAUD 115200 
const char apn[] = "prepaid.dna.fi"; 
const char user[] = ""; 
const char pass[] = "";

// Telegram Bot Token
const char* botToken = "7568394111:AAEc673Oxmk8AeYf_Pad_P_2dkgpvWaxF3Y"; 

// Global Variables
TinyGsm modem(GSM_SERIAL);
TinyGsmClient client(modem);
UniversalTelegramBot bot(botToken, client);

void setup() {
  // Initialize Serial
  Serial.begin(115200);
  gsmSerial.begin(115200);
  GSM_SERIAL.begin(GSM_BAUD);

  // Initialize GSM module
  Serial.print("Initializing GSM module...");
  if (!modem.begin()) {
    Serial.println("Failed to initialize modem. Restarting...");
    delay(5000);
    ESP.restart();
  }
  Serial.println("GSM initialized.");

  // Connect to cellular network
  Serial.print("Connecting to cellular network...");
  if (!modem.gprsConnect(apn, user, pass)) {
    Serial.println("Failed to connect to network. Restarting...");
    delay(5000);
    ESP.restart();
  }
  Serial.println("Connected to cellular network.");

  // Initialize Telegram Bot
  bot.updateToken(botToken); 
}

void loop() {
  // Check for Telegram messages
  int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
  if (numNewMessages > 0) {
    // Handle messages (if needed)
  }

  delay(1000); // Check periodically
}
