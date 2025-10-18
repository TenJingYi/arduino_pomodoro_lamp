/*
  ESP8266 Self-Study Lamp with:
  - Web-based On/Off control
  - Pomodoro timer with LCD display
  - Buzzer alarm when timer completes
*/

#include <ESP8266WiFi.h>
#include <LiquidCrystal_I2C.h>

// Wi-Fi credentials
const char* ssid = "HUAWEI Y7 Pro 2019";  // Enter your Wi-Fi SSID
const char* password = "478478478";  // Enter your Wi-Fi password

WiFiServer server(80);
String header;

// Pin definitions
const int relayPin = D0;
const int buzzerPin = D5;

// LCD setup (Check I2C address, 0x27 or 0x3F are common)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Timer variables
unsigned long startTime;
unsigned long duration = 25 * 60 * 1000;  // 25 minutes Pomodoro
bool timerRunning = false;

void setup() {
  Serial.begin(115200);
  pinMode(relayPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  digitalWrite(relayPin, HIGH);  // Lamp OFF
  digitalWrite(buzzerPin, LOW);  // Buzzer OFF

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Timer: 25:00");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");
  Serial.println(WiFi.localIP());
  server.begin();
}

void displayTimer(unsigned long remaining) {
  unsigned int mins = (remaining / 60000UL) % 60;
  unsigned int secs = (remaining / 1000UL) % 60;
  lcd.setCursor(0, 1);
  lcd.printf("%02d:%02d", mins, secs);
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
      lcd.print("Timer Done     ");
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

          // Handle timer control
          if (header.indexOf("GET /timer/start") >= 0) {
            timerRunning = true;
            startTime = millis();
            lcd.setCursor(0, 0);
            lcd.print("Timer Running  ");
          } else if (header.indexOf("GET /timer/reset") >= 0) {
            timerRunning = false;
            lcd.setCursor(0, 0);
            lcd.print("Timer: 25:00   ");
            displayTimer(duration);
          }

          // Web interface
          client.println("<!DOCTYPE html><html><head><title>Study Lamp</title>");
          client.println("<style>button{padding:20px;font-size:20px;}</style></head>");
          client.println("<body><h1>Study Lamp Control</h1>");
          client.println("<p><a href=\"/lamp/on\"><button>Lamp ON</button></a>");
          client.println("<a href=\"/lamp/off\"><button>Lamp OFF</button></a></p>");
          client.println("<h2>Pomodoro Timer</h2>");
          client.println("<p><a href=\"/timer/start\"><button>Start Timer</button></a>");
          client.println("<a href=\"/timer/reset\"><button>Reset Timer</button></a></p>");
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
