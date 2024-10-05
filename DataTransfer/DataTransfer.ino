#include <WiFi.h>
#include <HTTPClient.h>
#include <FS.h>
#include <LittleFS.h>          // Include LittleFS library
#include "mbedtls/base64.h"
#include <AsyncTCP.h>          // Include AsyncTCP library
#include <ESPAsyncWebServer.h> // Include ESPAsyncWebServer library
#include <ArduinoJson.h>       // Include ArduinoJson library for JSON parsing
#include <DallasTemperature.h>
#include <OneWire.h>
#include <LiquidCrystal.h>

// Wi-Fi credentials
const char* ssid = "Logans-Phone";
const char* password = "Falcons1";

// MATLAB server URL
const char* matlabURL = "http://192.168.139.1:8080/receive_data";  // Update this with your MATLAB server IP

AsyncWebServer server(80);  // Create the web server on port 80

#define DS18B20_PIN 4          // DS18B20 Data Pin
#define buttonOne 18
#define buttonTwo 19

OneWire oneWire(DS18B20_PIN);
DallasTemperature sensors(&oneWire);

int onOne = 1;
int onTwo = 1;
int buttonStateOne = 0;
int buttonStateTwo = 0;
int numberOfDevices;

float sensorData[2];

unsigned long lastDebounceTimeOne = 0;
unsigned long lastDebounceTimeTwo = 0;
int lastButtonStateOne = 0;
int lastButtonStateTwo = 0;

unsigned long debounceDelay = 10;

const int rs = 13, en = 12, d4 = 14, d5 = 27, d6 = 26, d7 = 25;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

DeviceAddress tempDeviceAddress;
DeviceAddress deviceAddresses[2]; // Assuming we have 2 sensors

unsigned long previousMillis = 0;
const long interval = 1000; // Interval to send data to MATLAB (milliseconds)

void setup() {
  Serial.begin(115200);

  // Initialize LittleFS
  if (!LittleFS.begin(true)) {
    Serial.println("An Error occurred while mounting LittleFS");
    return;
  } else {
    Serial.println("LittleFS mounted successfully");
  }

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");

  // Get the ESP32's IP address
  Serial.print("ESP32 IP Address: ");
  Serial.println(WiFi.localIP());

  // Initialize sensors
  sensors.begin();
  numberOfDevices = sensors.getDeviceCount();

  Serial.print("Locating devices...");
  Serial.print("Found ");
  Serial.print(numberOfDevices, DEC);
  Serial.println(" devices.");

  // Initialize LCD
  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("Sensor1:");
  lcd.setCursor(0, 1);
  lcd.print("Sensor2:");

  // Initialize buttons
  pinMode(buttonOne, INPUT);
  pinMode(buttonTwo, INPUT);

  // For each sensor, get its address
  for (int i = 0; i < numberOfDevices; i++) {
    if (sensors.getAddress(tempDeviceAddress, i)) {
      Serial.print("Found device ");
      Serial.print(i, DEC);
      Serial.print(" with address: ");
      printAddress(tempDeviceAddress);
      Serial.println();
      // Store the address
      memcpy(deviceAddresses[i], tempDeviceAddress, sizeof(DeviceAddress));
    } else {
      Serial.print("Found ghost device at ");
      Serial.print(i, DEC);
      Serial.println(" but could not detect address. Check power and cabling");
    }
  }

  // Set up the web server routes
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = "<html><head><title>Graphs from MATLAB</title>";
    html += "<meta http-equiv='refresh' content='35'>"; // Refresh every 35 seconds
    html += "</head><body>";
    html += "<h1>Graphs from MATLAB</h1>";
    html += "<img src='/graph1.png?dummy=" + String(millis()) + "' alt='Graph 1 from MATLAB' />";
    html += "<img src='/graph2.png?dummy=" + String(millis()) + "' alt='Graph 2 from MATLAB' />";
    html += "</body></html>";
    request->send(200, "text/html", html);
  });

  // Serve the first graph image from LittleFS
  server.on("/graph1.png", HTTP_GET, [](AsyncWebServerRequest *request) {
  if (LittleFS.exists("/graph1.png")) {
    AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/graph1.png", "image/png");
    response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    response->addHeader("Pragma", "no-cache");
    response->addHeader("Expires", "0");
    request->send(response);
  } else {
    request->send(404, "text/plain", "Image not found");
  }
  });

  // Serve the second graph image from LittleFS
  server.on("/graph2.png", HTTP_GET, [](AsyncWebServerRequest *request) {
  if (LittleFS.exists("/graph2.png")) {
    AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/graph1.png", "image/png");
    response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    response->addHeader("Pragma", "no-cache");
    response->addHeader("Expires", "0");
    request->send(response);
  } else {
    request->send(404, "text/plain", "Image not found");
  }
  });


  // Start the server
  server.begin();
  Serial.println("Web server started. Access the graphs at http://" + WiFi.localIP().toString());
}

void loop() {
  unsigned long currentMillis = millis();

  // Read temperatures
  sensors.requestTemperatures();
  for (int i = 0; i < numberOfDevices; i++) {
    float tempC = sensors.getTempC(deviceAddresses[i]);
    sensorData[i] = tempC;
    float tempF = DallasTemperature::toFahrenheit(tempC);

    if (tempF != -196.00) {
      if (i == 0 && onOne == 1) {
        lcd.setCursor(8, 0);
        lcd.print(tempF);
      }

      if (i == 1 && onTwo == 1) {
        lcd.setCursor(8, 1);
        lcd.print(tempF);
      }

      Serial.print("Temp C: ");
      Serial.print(tempC);
      Serial.print(" Temp F: ");
      Serial.println(tempF); // Converts tempC to Fahrenheit
    } else {
      if (i == 0) {
        lcd.setCursor(8, 0);
        lcd.print("ERROR");
      } else {
        lcd.setCursor(8, 1);
        lcd.print("ERROR");
      }
    }
  }

  // Read buttons
  int readingOne = digitalRead(buttonOne);
  int readingTwo = digitalRead(buttonTwo);

  if (readingOne == HIGH) {
    Serial.println("Button 1 pressed");
    if (onOne == 1) {
      lcd.setCursor(8, 0);
      lcd.print(" OFF ");
      onOne = 0;
    } else {
      onOne = 1;
    }
    delay(200); // Simple debounce
  }

  if (readingTwo == HIGH) {
    Serial.println("Button 2 pressed");
    if (onTwo == 1) {
      lcd.setCursor(8, 1);
      lcd.print(" OFF ");
      onTwo = 0;
    } else {
      onTwo = 1;
    }
    delay(200); // Simple debounce
  }

  // Send data to MATLAB every interval
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    sendDataToMATLAB();
  }
}

void sendDataToMATLAB() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(matlabURL);

    // Prepare the data to send
    DynamicJsonDocument doc(1024);
    JsonArray data = doc.createNestedArray("data");
    for (int i = 0; i < numberOfDevices; i++) {
      data.add(sensorData[i]);
    }

    String jsonData;
    serializeJson(doc, jsonData);

    Serial.println("Sending data to MATLAB:");
    Serial.println(jsonData);

    // Specify content type
    http.addHeader("Content-Type", "application/json");

    // Send POST request
    int httpResponseCode = http.POST(jsonData);
    Serial.println("HTTP Response code: " + String(httpResponseCode));

    // Get the response headers and body
    String response = http.getString();
    Serial.println("Full HTTP Response:");
    Serial.println(response);

    if (httpResponseCode > 0) {
      // Parse the JSON response using ArduinoJson
      const size_t capacity = JSON_OBJECT_SIZE(2) + 1024; // Adjusted capacity
      DynamicJsonDocument doc(capacity);

      DeserializationError error = deserializeJson(doc, response);
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }

      // Rest of your code...
    } else {
      Serial.print("Error on sending POST: ");
      Serial.println(httpResponseCode);
    }

    http.end(); // Close connection
  } else {
    Serial.println("WiFi Disconnected");
  }
}

void decodeAndSaveImage(const char* base64Data, const char* fileName) {
  // Decode the base64 image using mbedTLS
  const char* input = base64Data;
  size_t inputLen = strlen(base64Data);

  // Calculate the maximum possible length of the decoded data
  size_t outputBufferSize = (inputLen * 3) / 4 + 4;  // Adjusted for padding

  // Allocate memory for the decoded data
  uint8_t* decodedData = (uint8_t*)malloc(outputBufferSize);
  if (decodedData == NULL) {
    Serial.println("Failed to allocate memory for decoded data");
    return;
  }

  // Perform the decoding
  size_t actualOutputLen = 0;
  int ret = mbedtls_base64_decode(decodedData, outputBufferSize, &actualOutputLen, (const unsigned char*)input, inputLen);
  if (ret != 0) {
    Serial.printf("Base64 decoding failed with error code %d\n", ret);
    if (ret == MBEDTLS_ERR_BASE64_INVALID_CHARACTER) {
      Serial.println("Invalid character in Base64 data");
    }
    free(decodedData);
    return;
  } else {
    Serial.println("Base64 decoding successful.");
    Serial.println("Length of decoded image data: " + String(actualOutputLen));
  }

  // Save decoded data as image
  File file = LittleFS.open(fileName, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    free(decodedData);
    return;
  }

  size_t bytesWritten = file.write(decodedData, actualOutputLen);
  Serial.println("Bytes written to " + String(fileName) + ": " + String(bytesWritten));
  file.close();
  Serial.println("Image saved to LittleFS as " + String(fileName));

  // Free allocated memory
  free(decodedData);
}

// Function to print a device address
void printAddress(DeviceAddress deviceAddress) {
  for (uint8_t i = 0; i < 8; i++) {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}
