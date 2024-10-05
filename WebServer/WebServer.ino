#include <WiFi.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <LiquidCrystal.h>

// Wi-Fi credentials
const char* ssid = "Logans-Phone";
const char* password = "Falcons1";

// Temperature sensor pin
#define ONE_WIRE_BUS 4

// Set up a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature sensor
DallasTemperature sensors(&oneWire);

float sensorData[2]; // Array to store temperature data from sensors

// Server IP address and port number for data transmission and command handling
const char* serverIP = "192.168.139.1";  // Replace with your laptop’s IP address
const int serverPort = 3000;             // Port to send data to JavaFX application
const int commandPort = 3001;            // Port to receive commands from JavaFX application

unsigned long previousMillis = 0;
const long interval = 1000;              // Interval to send data (1 second)

WiFiClient client;                       // WiFi client to manage TCP connection
WiFiServer commandServer(commandPort);   // Create server to listen for commands from JavaFX

// LCD and Button Configuration
#define buttonOne 18
#define buttonTwo 19
#define masterSwitch 21 // Master switch pin

const int rs = 13, en = 12, d4 = 14, d5 = 27, d6 = 26, d7 = 25;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

int onOne = 1;                           // Toggle flag for Sensor 1 display on LCD
int onTwo = 1;                           // Toggle flag for Sensor 2 display on LCD

int virtualOnOne = 1;                    // Separate flag for virtual control of Sensor 1
int virtualOnTwo = 1;                    // Separate flag for virtual control of Sensor 2

int masterSwitchState = LOW;             // State of the master switch
int lastMasterSwitchState = LOW;         // Last recorded state of the master switch

DeviceAddress tempDeviceAddress;
int numberOfDevices;

// Button debounce configuration
unsigned long lastDebounceTimeOne = 0;
unsigned long lastDebounceTimeTwo = 0;
int lastButtonStateOne = LOW;
int lastButtonStateTwo = LOW;
const unsigned long debounceDelay = 20;  // Debounce delay to avoid multiple presses

void setup() {
  Serial.begin(115200);

  // Initialize Wi-Fi connection
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
  Serial.println("ESP32 IP Address: ");
  Serial.println(WiFi.localIP());

  // Start the command server to listen for commands from JavaFX
  commandServer.begin();
  Serial.println("Command server started on port 3001");

  // Initialize temperature sensors
  sensors.begin();
  numberOfDevices = sensors.getDeviceCount();
  Serial.print("Found ");
  Serial.print(numberOfDevices, DEC);
  Serial.println(" temperature devices.");

  // Initialize LCD
  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("Sensor1:");
  lcd.setCursor(0, 1);
  lcd.print("Sensor2:");

  // Initialize button and master switch pins
  pinMode(buttonOne, INPUT);
  pinMode(buttonTwo, INPUT);
  pinMode(masterSwitch, INPUT); // Master switch pin
}

void loop() {
  // Read master switch state
  masterSwitchState = digitalRead(masterSwitch);

  // If master switch state changes, update the system
  if (masterSwitchState != lastMasterSwitchState) {
    if (masterSwitchState == LOW) {
      lcd.clear();
      lcd.print("System OFF");
      sendDataToServer("no_data");  // Send no_data message to JavaFX
    } else {
      lcd.clear();
      lcd.print("System ON");
    }
    lastMasterSwitchState = masterSwitchState;
  }

  // Only operate if master switch is ON
  if (masterSwitchState == HIGH) {
    // Get sensor readings
    sensors.requestTemperatures();
    sensorData[0] = sensors.getTempCByIndex(0);
    sensorData[1] = sensors.getTempCByIndex(1);

    // Display data on the LCD
    updateLCD();

    // Handle button presses for physical buttons
    handlePhysicalButtonPresses();

    // Handle incoming commands from JavaFX application
    handleIncomingCommands();

    // Send data to server at defined intervals
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      sendDataToServer();
    }
  }

  //delay(100);  // Short delay to avoid overloading the loop
}

void sendDataToServer() {
  if (WiFi.status() == WL_CONNECTED && masterSwitchState == HIGH) {
    if (client.connect(serverIP, serverPort)) {
      String jsonData = "{\"sensor1\":" + String(sensorData[0], 2) + ",\"sensor2\":" + String(sensorData[1], 2) + "}";
      client.println(jsonData);
      Serial.println("Data sent: " + jsonData);
      client.stop();
    } else {
      Serial.println("Connection to server failed.");
    }
  } else {
    Serial.println("WiFi not connected or Master Switch is OFF.");
  }
}

void sendDataToServer(String message) {
  if (WiFi.status() == WL_CONNECTED) {
    if (client.connect(serverIP, serverPort)) {
      client.println(message);
      Serial.println("Message sent: " + message);
      client.stop();
    } else {
      Serial.println("Connection to server failed.");
    }
  } else {
    Serial.println("WiFi not connected.");
  }
}

void updateLCD() {
  if (masterSwitchState == HIGH) {
    for (int i = 0; i < numberOfDevices; i++) {
      if (sensors.getAddress(tempDeviceAddress, i)) {
        float tempC = sensors.getTempC(tempDeviceAddress);
        if (tempC != -127.00) {
          if (i == 0 && onOne == 1) {
            lcd.setCursor(0, 0);
            lcd.print("Sensor1:");
            lcd.setCursor(8, 0);
            lcd.print(tempC);
          } else if (i == 0) {
            lcd.setCursor(8, 0);
            lcd.print(" OFF ");
          }
          if (i == 1 && onTwo == 1) {
            lcd.setCursor(0, 1);
            lcd.print("Sensor2:");
            lcd.setCursor(8, 1);
            lcd.print(tempC);
          } else if (i == 1) {
            lcd.setCursor(8, 1);
            lcd.print(" OFF ");
          }
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
    }
  } else {
    lcd.clear();
    lcd.print("No Data");
  }
}

void handlePhysicalButtonPresses() {
  // Read the state of each button
  int readingOne = digitalRead(buttonOne);
  int readingTwo = digitalRead(buttonTwo);

  // Debounce button 1
  if (readingOne != lastButtonStateOne) {
    lastDebounceTimeOne = millis();  // Reset debounce timer
  }
  if ((millis() - lastDebounceTimeOne) > debounceDelay && readingOne == HIGH) {
    onOne = !onOne; // Toggle Sensor 1 state
    virtualOnOne = onOne; // Synchronize with virtual state
    lcd.setCursor(8, 0);
    lcd.print(onOne ? " ON " : " OFF ");
  }
  lastButtonStateOne = readingOne;

  // Debounce button 2
  if (readingTwo != lastButtonStateTwo) {
    lastDebounceTimeTwo = millis();  // Reset debounce timer
  }
  if ((millis() - lastDebounceTimeTwo) > debounceDelay && readingTwo == HIGH) {
    onTwo = !onTwo; // Toggle Sensor 2 state
    virtualOnTwo = onTwo; // Synchronize with virtual state
    lcd.setCursor(8, 1);
    lcd.print(onTwo ? " ON " : " OFF ");
  }
  lastButtonStateTwo = readingTwo;
}

void handleIncomingCommands() {
  WiFiClient commandClient = commandServer.available(); // Check for incoming clients

  if (commandClient) {
    Serial.println("Command received from JavaFX application");
    String command = commandClient.readStringUntil('\n'); // Read the command
    command.trim();  // Remove any trailing whitespace

    // Handle the received command
    if (command == "toggleSensor1") {
      virtualOnOne = !virtualOnOne; // Toggle virtual state for Sensor 1
      onOne = virtualOnOne;         // Sync physical state with virtual state
      lcd.setCursor(8, 0);
      lcd.print(onOne ? " ON " : " OFF ");
      Serial.println("Sensor 1 display toggled");
    } else if (command == "toggleSensor2") {
      virtualOnTwo = !virtualOnTwo; // Toggle virtual state for Sensor 2
      onTwo = virtualOnTwo;         // Sync physical state with virtual state
      lcd.setCursor(8, 1);
      lcd.print(onTwo ? " ON " : " OFF ");
      Serial.println("Sensor 2 display toggled");
    }

    commandClient.stop(); // Close the connection
  }
}
