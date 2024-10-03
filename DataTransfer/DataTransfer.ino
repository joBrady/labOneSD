#include <WiFi.h>
#include <HTTPClient.h>
#include <Base64.h> // Include the Base64 library
#include <SPIFFS.h>
#include "mbedtls/base64.h"

const char* ssid = "Logans-Phone";
const char* password = "Falcons1";
const char* matlabURL = "http:// 192.168.139.1:8080/receive_data";  // Update this with your MATLAB server IP

void setup() {
    Serial.begin(115200);

    // Initialize SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");
    Serial.println(WiFi.localIP());
}

void loop() {
    // Example data to send
    int data[5] = {10, 20, 30, 40, 50};
    
    // Send data to MATLAB
    sendDataToMATLAB(data, 5);
    
    delay(10000); // Send data every 10 seconds
}

void sendDataToMATLAB(int* data, int len) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(matlabURL);

        // Create JSON data
        String jsonData = "{\"data\": [";
        for (int i = 0; i < len; i++) {
            jsonData += String(data[i]);
            if (i < len - 1) jsonData += ",";
        }
        jsonData += "]}";

        // Specify content type
        http.addHeader("Content-Type", "application/json");

        // Send POST request
        int httpResponseCode = http.POST(jsonData);
        if (httpResponseCode > 0) {
            String response = http.getString();
            Serial.println("Response from MATLAB: " + response);

            // Extract the base64 image from the response
            int startIndex = response.indexOf("\"image\":\"") + 9; // Move index past `"image":"`
            int endIndex = response.indexOf("\"", startIndex);
            String base64Graph = response.substring(startIndex, endIndex);

            // Replace any escaped characters (if present)
            base64Graph.replace("\\/", "/");

            // Decode the base64 image using mbedTLS
            const char* input = base64Graph.c_str();
            size_t inputLen = base64Graph.length();

            // Calculate the maximum possible length of the decoded data
            size_t outputBufferSize = (inputLen * 3) / 4;  // Base64 decoding reduces size by approximately 25%

            // Allocate memory for the decoded data
            uint8_t* decodedGraph = (uint8_t*)malloc(outputBufferSize);
            if (decodedGraph == NULL) {
                Serial.println("Failed to allocate memory for decoded graph");
                return;
            }

            // Perform the decoding
            size_t actualOutputLen = 0;
            int ret = mbedtls_base64_decode(decodedGraph, outputBufferSize, &actualOutputLen, (const unsigned char*)input, inputLen);
            if (ret != 0) {
                Serial.printf("Base64 decoding failed with error code %d\n", ret);
                free(decodedGraph);
                return;
            }

            // Save decoded graph as image (requires SPIFFS or SD card)
            File file = SPIFFS.open("/graph.png", FILE_WRITE);
            if (!file) {
                Serial.println("Failed to open file for writing");
                free(decodedGraph);
                return;
            }
            file.write(decodedGraph, actualOutputLen);
            file.close();
            Serial.println("Graph saved to SPIFFS as /graph.png");

            // Free allocated memory
            free(decodedGraph);
        } else {
            Serial.print("Error on sending POST: ");
            Serial.println(httpResponseCode);
        }

        http.end(); // Close connection
    }
}
