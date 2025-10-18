/*
  ESP8266 Self-Study Lamp with:
  - Web-based On/Off control
  - Adjustable Pomodoro timer (1, 5, 25, 50 minutes) with LCD display
  - Buzzer alarm when timer completes
  - Improved web UI with clear LCD display for selected time
  - Separated timer selection and start button for better user experience
*/

#include <ESP8266WiFi.h>
#include <LiquidCrystal_I2C.h>

// Wi-Fi credentials
const char* ssid = "HUAWEI Y7 Pro 2019";  // Enter your Wi-Fi SSID
const char* password = "478478478";       // Enter your Wi-Fi password

WiFiServer server(80);
String header;

// Pin definitions
const int relayPin = D0;
const int buzzerPin = D5;

// LCD setup (Check I2C address, 0x27 or 0x3F are common)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Timer variables
unsigned long startTime;
unsigned long duration = 0;
bool timerRunning = false;
bool timeSelected = false;

void setup() {
  Serial.begin(115200);
  pinMode(relayPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  digitalWrite(relayPin, HIGH);  // Lamp OFF
  digitalWrite(buzzerPin, LOW);  // Buzzer OFF

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Select Timer");
  lcd.setCursor(0, 1);
  lcd.print("1/5/25/50 min");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");
  Serial.println(WiFi.localIP());
  server.begin();
}

void displaySelectedTime() {
  unsigned int mins = duration / 60000UL;
  lcd.setCursor(0, 0);
  lcd.print("Selected:       ");
  lcd.setCursor(0, 1);
  lcd.printf("%02d:00           ", mins);
}

void displayTimer(unsigned long remaining) {
  unsigned int mins = (remaining / 60000UL) % 60;
  unsigned int secs = (remaining / 1000UL) % 60;
  lcd.setCursor(0, 1);
  lcd.printf("%02d:%02d           ", mins, secs);
}

void triggerBuzzer() {
  for (int i = 0; i < 5; i++) {
    digitalWrite(buzzerPin, HIGH);
    delay(500);
    digitalWrite(buzzerPin, LOW);
    delay(500);
  }
}

void handleTimer() {
  if (timerRunning) {
    unsigned long elapsed = millis() - startTime;
    if (elapsed >= duration) {
      timerRunning = false;
      lcd.setCursor(0, 0);
      lcd.print("Timer Done      ");
      displayTimer(0);
      triggerBuzzer();
    } else {
      displayTimer(duration - elapsed);
    }
  }
}

void handleWebRequest(WiFiClient& client) {
  String currentLine = "";
  while (client.connected()) {
    if (client.available()) {
      char c = client.read();
      header += c;
      if (c == '\n') {
        if (currentLine.length() == 0) {
          client.println("HTTP/1.1 200 OK");
          client.println("Content-type:text/html");
          client.println("Connection: close\n");

          // Handle relay control
          if (header.indexOf("GET /lamp/on") >= 0) {
            digitalWrite(relayPin, LOW);
          } else if (header.indexOf("GET /lamp/off") >= 0) {
            digitalWrite(relayPin, HIGH);
          }

          // Handle timer selection
          if (header.indexOf("GET /timer/select/1") >= 0) {
            duration = 1 * 60 * 1000;
            timeSelected = true;
            timerRunning = false;
            displaySelectedTime();
          } else if (header.indexOf("GET /timer/select/5") >= 0) {
            duration = 5 * 60 * 1000;
            timeSelected = true;
            timerRunning = false;
            displaySelectedTime();
          } else if (header.indexOf("GET /timer/select/25") >= 0) {
            duration = 25 * 60 * 1000;
            timeSelected = true;
            timerRunning = false;
            displaySelectedTime();
          } else if (header.indexOf("GET /timer/select/50") >= 0) {
            duration = 50 * 60 * 1000;
            timeSelected = true;
            timerRunning = false;
            displaySelectedTime();
          }

          // Handle start and reset actions
          if (header.indexOf("/timer/start") >= 0 && timeSelected) {
            timerRunning = true;
            startTime = millis();
            lcd.setCursor(0, 0);
            lcd.print("Running:        ");
            displayTimer(duration);
          } else if (header.indexOf("/timer/reset") >= 0) {
            timerRunning = false;
            timeSelected = false;
            lcd.setCursor(0, 0);
            lcd.print("Select Timer    ");
            lcd.setCursor(0, 1);
            lcd.print("1/5/25/50 min   ");
          }

          // Updated Web UI
          client.println("<!DOCTYPE html><html><head><title>Study Lamp</title>");
          client.println("<style>body{font-family:sans-serif;text-align:center;}button{padding:15px;margin:10px;font-size:18px;border-radius:10px;}</style></head>");
          client.println("<body><h1>Study Lamp Control</h1>");
          client.println("<p><a href=\"/lamp/on\"><button style=\"background-color:lightgreen;\">Lamp ON</button></a>");
          client.println("<a href=\"/lamp/off\"><button style=\"background-color:lightcoral;\">Lamp OFF</button></a></p>");
          client.println("<h2>Pomodoro Timer</h2>");
          client.println("<p>");
          client.println("<a href=\"/timer/select/1\"><button>1 Minute</button></a>");
          client.println("<a href=\"/timer/select/5\"><button>5 Minutes</button></a>");
          client.println("<a href=\"/timer/select/25\"><button>25 Minutes</button></a>");
          client.println("<a href=\"/timer/select/50\"><button>50 Minutes</button></a>");
          client.println("</p>");
          if (timeSelected && !timerRunning) {
            client.println("<p><a href=\"/timer/start\"><button style=\"background-color:lightblue;\">Start Timer</button></a></p>");
          }
          client.println("<p><a href=\"/timer/reset\"><button style=\"background-color:gold;\">Reset Timer</button></a></p>");
          client.println("</body></html>");

          client.println();
          break;
        } else {
          currentLine = "";
        }
      } else if (c != '\r') {
        currentLine += c;
      }
    }
  }
  header = "";
  client.stop();
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    handleWebRequest(client);
  }
  handleTimer();
}


