/*
 * ESP32 IoT OS - ALTERA to Firebase
 *
 * ספריות תומכות (יש להתקין דרך ARDUINO IDE):
 * - ArduinoJson by Benoit Blanchon (v6.x)
 * - Adafruit SH110X (for OLED display)
 * 
 * חיבורי חומרה:
 * ESP32 GPIO16 (RX2) ← ALTERA TX
 * ESP32 GPIO17 (TX2) → ALTERA RX
 * ESP32 GND         ← ALTERA GND
 */

#include <WiFi.h>
#include <Wire.h>
#include <WebServer.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_SH110X.h>
#include <WiFiClientSecure.h>
#include <HardwareSerial.h>

// ==================== הגדרות ====================

//OLED display
#define i2c_Address 0x3c 
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define ledpin 2

// Serial Configuration
#define SERIAL_BAUD 115200
#define SERIAL2_BAUD 9600

// Define UART2 pins
#define RXD2 16
#define TXD2 17

HardwareSerial lidarSerial(2); // Use UART2

//#define SERIAL2_RX 16
//#define SERIAL2_TX 17

// WiFi AP Configuration
String AP_SSID = "ESP-IOT-TF-LUNA";
#define AP_PASSWORD "12345678"

// Web Server
#define WEB_SERVER_PORT 80

// WiFi Connection
#define WIFI_CONNECT_TIMEOUT 10000
#define MAX_WIFI_NETWORKS 3

// Firebase
#define FIREBASE_SEND_INTERVAL 1000
#define FIREBASE_READ_INTERVAL 2000
#define FIREBASE_TIMEOUT 5000

// NVS Keys
#define PREF_NAMESPACE "esp32_config"
#define PREF_SSID_1 "ssid1"
#define PREF_PASS_1 "pass1"
#define PREF_SSID_2 "ssid2"
#define PREF_PASS_2 "pass2"
#define PREF_SSID_3 "ssid3"
#define PREF_PASS_3 "pass3"
#define PREF_FB_URL "fb_url"
#define PREF_FB_KEY "fb_key"

// Buffer sizes
#define SERIAL_BUFFER_SIZE 19
#define JSON_BUFFER_SIZE 256

// Device ID
#define DEVICE_ID_PREFIX "ESP32_"

// ==================== ממשק HTML ====================

const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html dir="rtl" lang="he">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 IoT Configuration</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { font-family: Arial, sans-serif; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); min-height: 100vh; padding: 20px; direction: rtl; }
        .container { max-width: 600px; margin: 0 auto; background: white; border-radius: 15px; box-shadow: 0 10px 40px rgba(0,0,0,0.2); overflow: hidden; }
        .header { background: linear-gradient(135deg, #5a67d8 0%, #6b46c1 100%); color: white; padding: 30px; text-align: center; }
        .header h1 { font-size: 28px; margin-bottom: 10px; }
        .header p { opacity: 0.9; font-size: 14px; }
        .content { padding: 30px; }
        .section { margin-bottom: 30px; }
        .section-title { font-size: 20px; color: #5a67d8; margin-bottom: 15px; padding-bottom: 10px; border-bottom: 2px solid #e2e8f0; }
        .form-group { margin-bottom: 20px; }
        label { display: block; margin-bottom: 8px; color: #4a5568; font-weight: 500; }
        input[type="text"], input[type="password"] { width: 100%; padding: 12px; border: 2px solid #e2e8f0; border-radius: 8px; font-size: 14px; transition: border-color 0.3s; }
        input[type="text"]:focus, input[type="password"]:focus { outline: none; border-color: #5a67d8; }
        .wifi-networks { background: #f7fafc; padding: 20px; border-radius: 8px; margin-bottom: 20px; }
        .wifi-network { background: white; padding: 15px; border-radius: 8px; margin-bottom: 15px; border: 1px solid #e2e8f0; }
        .wifi-network h4 { color: #5a67d8; margin-bottom: 10px; }
        button { background: linear-gradient(135deg, #5a67d8 0%, #6b46c1 100%); color: white; border: none; padding: 15px 30px; font-size: 16px; border-radius: 8px; cursor: pointer; width: 100%; font-weight: 600; }
        button:hover { transform: translateY(-2px); box-shadow: 0 5px 15px rgba(90, 103, 216, 0.4); }
        .status { background: #f0fff4; border: 1px solid #9ae6b4; color: #276749; padding: 15px; border-radius: 8px; margin-bottom: 20px; display: none; }
        .status.show { display: block; }
        .status.error { background: #fff5f5; border-color: #fc8181; color: #742a2a; }
        .info-box { background: #ebf8ff; border: 1px solid #90cdf4; color: #2c5282; padding: 15px; border-radius: 8px; margin-bottom: 20px; }
        .small-text { font-size: 12px; color: #718096; margin-top: 5px; }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>ESP32 IoT Configuration</h1>
            <p>ניהול רשתות WiFi והגדרות Firebase</p>
        </div>
        <div class="content">
            <div id="status" class="status"></div>
            <div class="info-box"><strong>מידע:</strong> הזן את פרטי הרשתות ו-Firebase ולחץ על שמור</div>
            <form id="configForm" method="POST" action="/save">
                <div class="section">
                    <div class="section-title">WiFi Networks</div>
                    <div class="wifi-networks">
                        <div class="wifi-network">
                            <h4>רשת #1</h4>
                            <div class="form-group"><label>שם רשת (SSID)</label><input type="text" name="ssid1" id="ssid1" placeholder="שם הרשת"></div>
                            <div class="form-group"><label>סיסמה (אופציונלי)</label><input type="password" name="pass1" id="pass1" placeholder="השאר ריק לרשת פתוחה"></div>
                        </div>
                        <div class="wifi-network">
                            <h4>רשת #2 (אופציונלי)</h4>
                            <div class="form-group"><label>שם רשת (SSID)</label><input type="text" name="ssid2" id="ssid2" placeholder="שם הרשת"></div>
                            <div class="form-group"><label>סיסמה (אופציונלי)</label><input type="password" name="pass2" id="pass2" placeholder="השאר ריק לרשת פתוחה"></div>
                        </div>
                        <div class="wifi-network">
                            <h4>רשת #3 (אופציונלי)</h4>
                            <div class="form-group"><label>שם רשת (SSID)</label><input type="text" name="ssid3" id="ssid3" placeholder="שם הרשת"></div>
                            <div class="form-group"><label>סיסמה (אופציונלי)</label><input type="password" name="pass3" id="pass3" placeholder="השאר ריק לרשת פתוחה"></div>
                        </div>
                    </div>
                </div>
                <div class="section">
                    <div class="section-title">Firebase Settings</div>
                    <div class="form-group">
                        <label>Firebase Database URL</label>
                        <input type="text" name="fb_url" id="fb_url" placeholder="https://your-project.firebaseio.com">
                        <div class="small-text">לדוגמה: https://my-project-123.firebaseio.com</div>
                    </div>
                    <div class="form-group">
                        <label>Firebase API Key</label>
                        <input type="text" name="fb_key" id="fb_key" placeholder="Database Secret או השאר ריק">
                        <div class="small-text">Database Secret או השאר ריק לשימוש ב-Public Rules</div>
                    </div>
                </div>
                <button type="submit">שמור הגדרות</button>
            </form>
        </div>
    </div>
    <script>
        fetch('/get').then(r => r.json()).then(data => {
            Object.keys(data).forEach(key => {
                var el = document.getElementById(key);
                if (el && data[key]) el.value = data[key];
            });
        }).catch(e => {});
    </script>
</body>
</html>
)rawliteral";

// ==================== משתנים גלובליים ====================

Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
String temp_old3;
bool a = false;
int temp_old1, temp_old2;

WiFiClientSecure streamClient;

WebServer server(WEB_SERVER_PORT);
Preferences preferences;

String deviceId;
String firebaseUrl;
String firebaseApiKey;
bool wifiConnected = false;
bool firebaseConfigured = false;

TaskHandle_t Task1Handle;
TaskHandle_t Task2Handle;
SemaphoreHandle_t dataMutex;

const int MPU_ADDR = 0x68; // I2C address of the MPU6050

#define SENSOR_SAMPLE_INTERVAL 100  // דגימה כל 100 מילי-שניות (10 פעמים בשנייה)
#define FIREBASE_BATCH_INTERVAL 5000 // שליחה כל 5 שניות

// נגדיל את ה-Buffer כי חבילה של 5 שניות מכילה הרבה נתונים
#define JSON_BATCH_BUFFER_SIZE 16384
unsigned long lastStreamActivity = 0;

struct SharedData {
    char jsonBuffer[JSON_BUFFER_SIZE];
    bool dataReady;
    int commandToAltera;
    bool commandReady;
} sharedData;



struct SensorData {
  float gx, gy, gz; // Gyroscope
  float ax, ay, az; // Accelerometer
};
// ==================== פונקציות ====================

void blink1();

void displaySetup() {
    display.clearDisplay();
    display.display();
    display.setTextColor(SH110X_WHITE, SH110X_BLACK);
    display.setTextSize(1);
    display.setCursor(1, 11);
    display.print("valA: ");
    display.setCursor(1, 21);
    display.print("valB: ");
    display.setCursor(1, 31);
    display.print("valC: ");
    display.setCursor(1, 51);
    display.print("TO ALTERA: ");
    display.display();
    pinMode(ledpin, OUTPUT);

}

void displayOnScreen(int x, int y, String data, int fontSize) {
    display.setTextSize(fontSize);
    display.setCursor(x, y);
    display.print(data); 
}

String getDeviceId() {
    uint64_t chipid = ESP.getEfuseMac();
    char id[32];
    snprintf(id, sizeof(id), "%s%04X%08X", DEVICE_ID_PREFIX, 
             (uint16_t)(chipid >> 32), (uint32_t)chipid);
    return String(id);
}

String getFirebaseHost() {
    String host = firebaseUrl;
    if (host.startsWith("https://")) {
        host = host.substring(8);
    } else if (host.startsWith("http://")) {
        host = host.substring(7);
    }
    if (host.endsWith("/")) {
        host = host.substring(0, host.length() - 1);
    }
    return host;
}

void loadPreferences() {
    preferences.begin(PREF_NAMESPACE, true);
    
    firebaseUrl = preferences.getString(PREF_FB_URL, "");
    firebaseApiKey = preferences.getString(PREF_FB_KEY, "");
    
    if (firebaseUrl.length() > 0) {
        firebaseConfigured = true;
        Serial.println("Firebase configuration loaded");
        Serial.printf("Firebase URL: %s\n", firebaseUrl.c_str());
        if (firebaseApiKey.length() > 0) {
            Serial.println("Auth: Database Secret configured");
        } else {
            Serial.println("Auth: Using public rules (no auth key)");
        }
    } else {
        Serial.println("No Firebase configuration found");
    }
    
    preferences.end();
}

bool tryConnectWiFi(String ssid, String password) {
    WiFi.begin(ssid.c_str(), password.c_str());
    
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < WIFI_CONNECT_TIMEOUT) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    
    return WiFi.status() == WL_CONNECTED;
}

void startAPMode() {
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
    WiFi.softAP(("ESP-AP-" + AP_SSID).c_str(), AP_PASSWORD);
    
    Serial.println("Access Point started");
    Serial.printf("SSID: ESP-AP-%s\n", AP_SSID.c_str());
    Serial.printf("Password: %s\n", AP_PASSWORD);
    Serial.printf("IP Address: %s\n", WiFi.softAPIP().toString().c_str());
    //displayOnScreen(1, 2, "A.P - IP :"+WiFi.softAPIP().toString(),1);
    display.display();
}

void connectToWiFi() {
    preferences.begin(PREF_NAMESPACE, true);
    
    String networks[MAX_WIFI_NETWORKS][2];
    networks[0][0] = preferences.getString(PREF_SSID_1, "");
    networks[0][1] = preferences.getString(PREF_PASS_1, "");
    networks[1][0] = preferences.getString(PREF_SSID_2, "");
    networks[1][1] = preferences.getString(PREF_PASS_2, "");
    networks[2][0] = preferences.getString(PREF_SSID_3, "");
    networks[2][1] = preferences.getString(PREF_PASS_3, "");
    
    preferences.end();
    
    bool connected = false;
    
    for (int i = 0; i < MAX_WIFI_NETWORKS; i++) {
        if (networks[i][0].length() > 0) {
            Serial.printf("Trying network %d: %s\n", i + 1, networks[i][0].c_str());
            if (tryConnectWiFi(networks[i][0], networks[i][1])) {
                connected = true;
                wifiConnected = true;
                Serial.printf("Connected to: %s\n", networks[i][0].c_str());
                Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
                //displayOnScreen(1, 2, "IP: "+WiFi.localIP().toString(),1);
                break;
            }
        }
    }
    
    if (!connected) {
        Serial.println("Starting Access Point mode...");
        startAPMode();
    }
}

void savePreferences(String ssid1, String pass1, String ssid2, String pass2, 
                     String ssid3, String pass3, String fbUrl, String fbKey) {
    preferences.begin(PREF_NAMESPACE, false);
    
    preferences.putString(PREF_SSID_1, ssid1);
    preferences.putString(PREF_PASS_1, pass1);
    preferences.putString(PREF_SSID_2, ssid2);
    preferences.putString(PREF_PASS_2, pass2);
    preferences.putString(PREF_SSID_3, ssid3);
    preferences.putString(PREF_PASS_3, pass3);
    preferences.putString(PREF_FB_URL, fbUrl);
    preferences.putString(PREF_FB_KEY, fbKey);
    
    preferences.end();
    
    Serial.println("Preferences saved to NVS");
}

void connectStream() {
    String host = getFirebaseHost();
    Serial.println("Stream connecting to: " + host);
    
    if (streamClient.connect(host.c_str(), 443)) {
        String path = "/toAltera.json";
        if (firebaseApiKey.length() > 0) {
            path += "?auth=" + firebaseApiKey;
        }
        
        streamClient.println("GET " + path + " HTTP/1.1");
        streamClient.println("Host: " + host);
        streamClient.println("Accept: text/event-stream");
        streamClient.println("Connection: keep-alive");
        streamClient.println();
        Serial.println("Stream connected!");
    } else {
        Serial.println("Stream connection failed!");
    }
}

void sendIPToFirebase() {
    if (!firebaseConfigured || !wifiConnected) return;
    
    HTTPClient http;
    String url = firebaseUrl + "/devices/" + deviceId + ".json";
    
    if (firebaseApiKey.length() > 0) {
        url += "?auth=" + firebaseApiKey;
    }
    
    StaticJsonDocument<256> doc;
    doc["ip"] = WiFi.localIP().toString();
    doc["last_update"] = millis() / 1000;
    
    String jsonStr;
    serializeJson(doc, jsonStr);
    
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    
    int httpCode = http.PATCH(jsonStr);
    
    if (httpCode == 200 || httpCode == 204) {
        Serial.printf("IP sent to Firebase: %d\n", httpCode);
    } else {
        Serial.printf("Firebase error: %d\n", httpCode);
    }
    
    http.end();
}

void handleRoot() {
    server.send_P(200, "text/html", HTML_PAGE);
}

void handleGetSettings() {
    preferences.begin(PREF_NAMESPACE, true);
    
    StaticJsonDocument<512> doc;
    doc["ssid1"] = preferences.getString(PREF_SSID_1, "");
    doc["ssid2"] = preferences.getString(PREF_SSID_2, "");
    doc["ssid3"] = preferences.getString(PREF_SSID_3, "");
    doc["fb_url"] = preferences.getString(PREF_FB_URL, "");
    doc["fb_key"] = preferences.getString(PREF_FB_KEY, "");
    
    preferences.end();
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void handleSaveSettings() {
    Serial.println(">>> handleSaveSettings called!");
    
    String ssid1 = server.arg("ssid1");
    String pass1 = server.arg("pass1");
    String ssid2 = server.arg("ssid2");
    String pass2 = server.arg("pass2");
    String ssid3 = server.arg("ssid3");
    String pass3 = server.arg("pass3");
    String fbUrl = server.arg("fb_url");
    String fbKey = server.arg("fb_key");
    
    Serial.println("Saving: SSID1=" + ssid1 + ", FB_URL=" + fbUrl);
    savePreferences(ssid1, pass1, ssid2, pass2, ssid3, pass3, fbUrl, fbKey);
    
    String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'></head><body style='text-align:center;font-family:Arial;padding:50px;direction:rtl;'>";
    html += "<h1>Settings Saved!</h1>";
    html += "<p>ESP32 is restarting...</p>";
    html += "</body></html>";
    server.send(200, "text/html", html);
    Serial.println("Settings saved! Restarting...");
    
    delay(1000);
    ESP.restart();
}

void handleNotFound() {
    Serial.print("404 Not Found: ");
    Serial.println(server.uri());
    server.send(404, "text/plain", "Not Found");
}

void setupWebServer() {
    server.on("/", HTTP_GET, handleRoot);
    server.on("/get", HTTP_GET, handleGetSettings);
    server.on("/save", HTTP_POST, handleSaveSettings);
    server.onNotFound(handleNotFound);
    
    server.begin();
    Serial.println("Web server started on port 80");
}

void sendToFirebase(const char* jsonData) {
    WiFiClientSecure secureClient; 
    secureClient.setInsecure(); // Required to bypass SSL verification for Firebase

    HTTPClient http;
    String url = firebaseUrl + "/fromAltera.json";
    if (firebaseApiKey.length() > 0) url += "?auth=" + firebaseApiKey;

    // Pass the secure client as the first argument
    http.begin(secureClient, url); 
    http.addHeader("Content-Type", "application/json");
    
    int httpCode = http.PUT(jsonData); 
    
    if (httpCode == 200 || httpCode == 204) {
        Serial.println("[Firebase] Batch Sent Successfully!");
    } else {
        Serial.printf("[Firebase] Error sending batch: %d\n", httpCode);
    }
    http.end();
}

int readFromFirebase() {
    // Check if disconnected OR if we haven't received ANY data (even keep-alives) in 40 seconds
    if (!streamClient.connected() || (millis() - lastStreamActivity > 40000)) {
        Serial.println("Stream disconnected or timed out... forcing reconnect");
        streamClient.stop(); // CRITICAL: Stop the old socket to prevent memory leaks
        delay(3000);
        connectStream();
        lastStreamActivity = millis(); // Reset timer after reconnecting
        return -1;
    }
    
    int payload = -1;
    while (streamClient.available()) {
        // Update the activity timer every time we read from the buffer
        lastStreamActivity = millis(); 
        
        String line = streamClient.readStringUntil('\n');
        Serial.println("FIREBASENIGGADATA: " + line);
        line.trim();
        
        if (line.startsWith("data: ")) {
            String dataStr = line.substring(6); // Renamed to avoid shadowing
            dataStr.trim();
            
            if (dataStr.length() > 0 && dataStr != "null") {
                if (dataStr.startsWith("{")) {
                    StaticJsonDocument<128> doc;
                    DeserializationError error = deserializeJson(doc, dataStr);
                    if (!error && doc.containsKey("data")) {
                        if (!doc["data"].isNull()) {
                            payload = doc["data"].as<int>();
                            Serial.println("Firebase value: " + String(payload));
                        }
                    }
                } else {
                    // Removed the redeclaration of the string here
                    if (isDigit(dataStr[0])) { 
                        payload = dataStr.toInt();
                        Serial.println("Firebase value: " + String(payload));
                    }
                }
            }
        }
    }
    return payload;
}

SensorData readIMU() {
  SensorData data;
  
  // 1. Point to the first Accelerometer register (0x3B)
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B); 
  Wire.endTransmission(false);
  
  // 2. Request 14 bytes (6 for Accel, 2 for Temp, 6 for Gyro)
  Wire.requestFrom(MPU_ADDR, 14, true);
  
  // 3. Read Accelerometer (Scale factor 16384.0 for +/- 2g range)
  data.ax = (int16_t)(Wire.read() << 8 | Wire.read()) / 16384.0;
  data.ay = (int16_t)(Wire.read() << 8 | Wire.read()) / 16384.0;
  data.az = (int16_t)(Wire.read() << 8 | Wire.read()) / 16384.0;
  
  // 4. Skip Temperature bytes (2 bytes)
  Wire.read(); Wire.read();
  
  // 5. Read Gyroscope (Scale factor 131.0 for +/- 250 deg/s)
  data.gx = (int16_t)(Wire.read() << 8 | Wire.read()) / 131.0;
  data.gy = (int16_t)(Wire.read() << 8 | Wire.read()) / 131.0;
  data.gz = (int16_t)(Wire.read() << 8 | Wire.read()) / 131.0;
  
  return data;
}

/**
 * Callable function to extract TF-Luna distance
 * Returns: Distance in cm, or -1 if no valid data available
 */
int getLidarDistance() {
  // 1. If the buffer is getting too full, clear the old data
  if (lidarSerial.available() > 50) {
     while(lidarSerial.available() > 9) lidarSerial.read(); 
  }

  if (lidarSerial.available() >= 9) {
    if (lidarSerial.read() == 0x59) {
      if (lidarSerial.read() == 0x59) {
        uint8_t buffer[7];
        lidarSerial.readBytes(buffer, 7);
        
        int dist = buffer[0] + (buffer[1] << 8);
        
        // Return 0 if the sensor reports an error code (often 0 or 65535)
        if (dist > 1200) return -1; 
        return dist;
      }
    }
  }
  return -1;
}

// void serialReadTask(void *parameter) {
//     Serial.println("[Task1] Serial Read Task started on Core 0");
//     String prevJsonData = "";
//     while (true) {
//         if (Serial2.available() >= SERIAL_BUFFER_SIZE) {
//             uint8_t buffer[SERIAL_BUFFER_SIZE];
            
//             for (int i = 0; i < SERIAL_BUFFER_SIZE; i++) {
//                 buffer[i] = Serial2.read();
//                 //Serial.printf("Data received -%s\n", String(buffer[i]));

//             }
           

//             String jsonData = "{\"A\":" + String(buffer[5]) + ",\"B\":" + String(buffer[11]) + ",\"C\":" + String(buffer[17]) + "}";
            
//             Serial.printf("[Task1] Data received - A:%d B:%d C:%d\n", buffer[5], buffer[11], buffer[17]);
            
//             if (xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
//                 strncpy(sharedData.jsonBuffer, jsonData.c_str(), JSON_BUFFER_SIZE - 1);
//                 sharedData.jsonBuffer[JSON_BUFFER_SIZE - 1] = '\0';
//                 sharedData.dataReady = true;
//                 xSemaphoreGive(dataMutex);
//             }
            
//             // displayOnScreen(65, 11, String(buffer[5]) + "  ",1);
//             // displayOnScreen(65, 21, String(buffer[11]) + "  ",1);
//             // displayOnScreen(65, 31, String(buffer[17]) + "  ",1);
//             // display.display();
//             // blink1();
         
        
//         }
//         if (xSemaphoreTake(dataMutex, 10) == pdTRUE) {
//             if (sharedData.commandReady) {
//                 Serial2.write(sharedData.commandToAltera);
//                 Serial.printf("[Task1] Sent to ALTERA: %d\n", sharedData.commandToAltera);
//                 // displayOnScreen(65, 51, String(sharedData.commandToAltera) + "  ",1);
//                 // display.display();
//                 sharedData.commandReady = false;
//             }
//             xSemaphoreGive(dataMutex);
//         }
        
//         vTaskDelay(10 / portTICK_PERIOD_MS);
//     }
// }
void firebaseCommunicationTask(void *parameter) {
    Serial.println("[Task2] Firebase Batch Task started");
    
    DynamicJsonDocument batchDoc(JSON_BATCH_BUFFER_SIZE);
    JsonArray readings = batchDoc.to<JsonArray>();
    
    unsigned long lastReadTime = 0;
    unsigned long lastSampleTime = 0;
    unsigned long lastBatchSendTime = 0;
    bool firebaseInitialized = false;
    int lastCommandState = 2; // Track state transitions

    // FORCE initial state to STOP (2)
    if (xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
        sharedData.commandToAltera = 2; 
        sharedData.commandReady = true; 
        xSemaphoreGive(dataMutex);
    }
    
    while (true) {
        if (wifiConnected && firebaseConfigured) {
            if (!firebaseInitialized) {
                streamClient.setInsecure();
                connectStream();
                sendIPToFirebase();
                firebaseInitialized = true;
            }

            unsigned long currentTime = millis();

            if (sharedData.commandToAltera == 0) { 
                
                // DETECT TRANSITION: If we just switched from STOP to START, reset timers
                if (lastCommandState != 0) {
                    lastSampleTime = currentTime;
                    lastBatchSendTime = currentTime;
                    lastCommandState = 0;
                }

                // 1: Sample Data (every 100ms)
                if (currentTime - lastSampleTime >= SENSOR_SAMPLE_INTERVAL) {
                    SensorData imu = readIMU();
                    int dist = getLidarDistance();

                    JsonObject obj = readings.createNestedObject();
                    obj["t"] = currentTime;
                    
                    JsonObject g = obj.createNestedObject("g");
                    g["x"] = imu.gx; g["y"] = imu.gy; g["z"] = imu.gz;
                    
                    JsonObject a = obj.createNestedObject("a");
                    a["x"] = imu.ax; a["y"] = imu.ay; a["z"] = imu.az;
                    
                    obj["d"] = dist;
                    lastSampleTime = currentTime;
                }

                // 2: Send Batch (every 5 seconds)
                if (currentTime - lastBatchSendTime >= FIREBASE_BATCH_INTERVAL) {
                    if (readings.size() > 0) {
                        String jsonOutput;
                        serializeJson(batchDoc, jsonOutput);
                        
                        Serial.printf("[Task2] Sending batch of %d readings...\n", readings.size());
                        sendToFirebase(jsonOutput.c_str());

                        batchDoc.clear();
                        readings = batchDoc.to<JsonArray>();
                    }
                    lastBatchSendTime = currentTime;
                }
            } else {
                lastCommandState = sharedData.commandToAltera;
                
                // Clear array if stopped so old data isn't sent later
                if (readings.size() > 0) {
                    batchDoc.clear();
                    readings = batchDoc.to<JsonArray>();
                }
            }

            // 3: Read Commands (Always runs)
            if (currentTime - lastReadTime >= FIREBASE_READ_INTERVAL) {
                int firebaseCommand = readFromFirebase(); 

                if (firebaseCommand == 0 || firebaseCommand == 2) {
                    if (xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
                        sharedData.commandToAltera = firebaseCommand;
                        sharedData.commandReady = true; 
                        xSemaphoreGive(dataMutex);
                        Serial.printf("[Firebase] Valid Command Received: %d\n", firebaseCommand);
                    }
                }
                lastReadTime = currentTime;
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}


void blink1() {
    a = !a;
    digitalWrite(ledpin, a ? HIGH : LOW);
}

void serialCommunicationTask(void *parameter) {
    Serial.println("[Task1] Serial Communication Task started on Core 0");
    while (true) {
        // בדיקה אם ליבה 1 סימנה שיש פקודה חדשה מה-Firebase
        if (sharedData.commandReady) {
            if (xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
                
                // שליחת הפקודה (0 או 2) ל-FPGA דרך UART2
                lidarSerial.write((uint8_t)sharedData.commandToAltera);
                
                // הדפסה לדיבאג ב-Serial Monitor
                Serial.printf("[Task1] Command Forwarded to FPGA: %d\n", sharedData.commandToAltera);
                
                // כיבוי הדגל כדי לא לשלוח שוב
                sharedData.commandReady = false;
                
                xSemaphoreGive(dataMutex);
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS); // השהייה קצרה למניעת עומס
    }
}
// ==================== Setup & Loop ====================


void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(1000);
    Serial.println("\n\n=== ESP32 IoT OS ===");
    
    Wire.begin(21, 22, 400000); 
    
    if (display.begin(i2c_Address, true)) {
        Serial.println("OLED initialized");
        displaySetup();
    } else {
        Serial.println("OLED not found - continuing without display");
    }
    
    //Serial2.begin(SERIAL2_BAUD, SERIAL_8N1, SERIAL2_RX, SERIAL2_TX);
    //Serial.println("Serial2 initialized (9600 baud)");
    
    dataMutex = xSemaphoreCreateMutex();
    sharedData.dataReady = false;
    sharedData.commandReady = false;
    
    deviceId = getDeviceId();
    Serial.printf("Device ID: %s\n", deviceId.c_str());
    
    loadPreferences();
    
    Serial.println("Attempting to connect to saved WiFi networks...");
    connectToWiFi();
    
    setupWebServer();
    
     // יצירת המשימה שתרוץ על ליבה 0 ותשלח פקודות ל-FPGA
    xTaskCreatePinnedToCore(
        serialCommunicationTask,
        "SerialCommTask",
        4096,
        NULL,
        1,
        &Task1Handle,
        0
    );
    
    delay(500);
    
    xTaskCreatePinnedToCore(
        firebaseCommunicationTask,
        "FirebaseTask",
        8192,
        NULL,
        1,
        &Task2Handle,
        1
    );
    
    Serial.println("=== Setup Complete ===\n");

  
  // Wake up the MPU6050
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B); 
  Wire.write(0);    
  Wire.endTransmission(true);

  Serial.println("MPU6050 Initialized on ESP32");

  lidarSerial.begin(115200, SERIAL_8N1, 16, 17);
  Serial.println("TF-Luna Functional Test Initialized...");
// Initialize I2C on ESP32 default pins
  //Wire.begin(21, 22, 100000); // TF-Luna prefers 100kHz standard mode
  

}

void loop() {
    server.handleClient();
    delay(10);
    
    // // 1. Send a test byte to yourself
    // lidarSerial.write(0x59); 
    // delay(10);

    // // 2. Check if it came back
    // if (lidarSerial.available() > 0) {
    //     Serial.printf("Hardware OK! Received: 0x%02X\n", lidarSerial.read());
    // } else {
    //     Serial.println("Hardware Error: Pins 16/17 not responding.");
    // }
    // delay(1000);
}
