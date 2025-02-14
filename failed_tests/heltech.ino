#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <Servo.h>

// Replace with your WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Replace with your bot token
const char* botToken = "YOUR_TELEGRAM_BOT_TOKEN";

// Replace with your chat ID
const String chat_id = "YOUR_CHAT_ID";

WiFiClientSecure client;
UniversalTelegramBot bot(botToken, client);

Servo parkingServo;  // Servo object
const int servoPin = 13;  // Change to your GPIO pin

unsigned long lastTimeBotRan = 0;
const int botRequestDelay = 1000;

void setup() {
    Serial.begin(115200);

    // Connect to WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");

    client.setInsecure();  // Avoid certificate validation

    parkingServo.attach(servoPin);
    parkingServo.write(0);  // Reset to default position
}

void handleNewMessages(int numNewMessages) {
    for (int i = 0; i < numNewMessages; i++) {
        String chatID = bot.messages[i].chat_id;
        String text = bot.messages[i].text;

        Serial.println("Received command: " + text);

        if (chatID == chat_id) {
            if (text.startsWith("/set ")) {
                int angle = text.substring(5).toInt();
                if (angle >= 0 && angle <= 180) {
                    parkingServo.write(angle);
                    bot.sendMessage(chatID, "Parking disc set to " + String(angle) + " degrees.", "");
                } else {
                    bot.sendMessage(chatID, "Invalid angle! Use 0-180.", "");
                }
            } else {
                bot.sendMessage(chatID, "Use /set [0-180] to set the parking time.", "");
            }
        }
    }
}

void loop() {
    if (millis() - lastTimeBotRan > botRequestDelay) {
        int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
        while (numNewMessages) {
            handleNewMessages(numNewMessages);
            numNewMessages = bot.getUpdates(bot.last_message_received + 1);
        }
        lastTimeBotRan = millis();
    }
}
