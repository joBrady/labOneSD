#include <DallasTemperature.h>
#include <OneWire.h>
#include <LiquidCrystal.h>


//Pins for Buttons and Such 18,19,21

#define DS18B20_PIN 4          // DS18B20 Data Pin

#define buttonOne 18
#define buttonTwo 19
#define switchThree 21

OneWire oneWire(DS18B20_PIN);
DallasTemperature sensors(&oneWire);

int onOne = 1;
int onTwo = 1;
int buttonStateOne = 0;
int buttonStateTwo = 0;
int numberOfDevices;
int tempDevices;
int error = 0;
int previousError = 1;
int chase = 1;
int fixMessage = 0;

unsigned long lastDebounceTimeOne = 0;
unsigned long lastDebounceTimeTwo = 0;
int lastButtonStateOne = 0;
int lastButtonStateTwo = 0;

unsigned long debounceDelay = 10;

const int rs = 13, en = 12, d4 = 14, d5 = 27, d6 = 26, d7 = 25;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

DeviceAddress tempDeviceAddress;

DeviceAddress deviceOne;
DeviceAddress deviceTwo;

void setup()
{
  Serial.begin(115200);

  pinMode(buttonOne,INPUT);
  pinMode(buttonTwo,INPUT);
  pinMode(switchThree,INPUT);
  
  lcd.begin(16, 2);
  // Clears the LCD display
  lcd.clear();
  lcd.print("Sensor1:");
  lcd.setCursor(0,1);
  lcd.print("Sensor2:");
  
  sensors.begin();
  numberOfDevices = sensors.getDeviceCount();
  
  Serial.print("Locating devices...");
  Serial.print("Found ");
  Serial.print(numberOfDevices, DEC);
  Serial.println(" devices.");
 
  if(numberOfDevices<2){
   lcd.clear();
   lcd.print("ERROR MISSING");
   lcd.setCursor(0,1);
   lcd.print("TEMP SENSOR");
  while(sensors.getDeviceCount()!=2){}
  }
  
/*
  for(int i=0;i<numberOfDevices; i++){
    // Search the wire for address
    if(sensors.getAddress(tempDeviceAddress, i)){
      Serial.print("Found device ");
      Serial.print(i, DEC);
      Serial.print(" with address: ");
      printAddress(tempDeviceAddress);
      Serial.println();
    }else {
      Serial.print("Found ghost device at ");
      Serial.print(i, DEC);
      Serial.print(" but could not detect address. Check power and cabling");
    }
  }
  */

}

void loop()
{   
    sensors.requestTemperatures();
    int readingOne = digitalRead(buttonOne);
    int readingTwo = digitalRead(buttonTwo);
    int readingThree = digitalRead(switchThree);

    
    if(readingThree == LOW){

      
      if(fixMessage == 1){
          lcd.print("Sensor1:");
          lcd.setCursor(0,1);
          lcd.print("Sensor2:");
          fixMessage = 0;
      }

      if(readingOne == HIGH){
      if(onOne == 1){
        lcd.setCursor(8,0);
        lcd.print(" OFF ");
        onOne = 0;
      }else{
        onOne = 1;
      }
    }

    if(readingTwo == HIGH){
      if(onTwo == 1){
        lcd.setCursor(8,1);
        lcd.print(" OFF ");
        onTwo = 0;
      }else{
        onTwo = 1;
      }
    }

    for(int i=0;i<numberOfDevices; i++){
    // Search the wire for address
    if(sensors.getAddress(tempDeviceAddress, i)){
      // Output the device ID
      

        if(previousError >= 2){
          error = error + 1;
          lcd.clear();
          lcd.print("ERROR MISSING");
          lcd.setCursor(0,1);
          lcd.print("TEMP SENSOR");
          if(error != (previousError -1)){
            error = 0;
            lcd.clear();
            lcd.print("Sensor1:");
            lcd.setCursor(0,1);
            lcd.print("Sensor2:");
            previousError = 1;
          }
        }else{

      Serial.print("Temperature for device: ");
      Serial.println(i,DEC);
      // Print the data
      float tempC = sensors.getTempC(tempDeviceAddress);
      float tempF = DallasTemperature::toFahrenheit(tempC);


      /*
      if(tempDeviceAddress == deviceOne){
        Serial.print("CountTwo");
        countTwo = countTwo + 1;
        countOne = 0;
      }
       if(tempDeviceAddress == deviceTwo){
        Serial.print("CountOne");
        countOne = countOne + 1;
        countTwo = 0;
      }
      if(countOne == 0 && errorOne == 1){
        lcd.setCursor(0,0);
        lcd.print("Sensor1:        ");
        errorOne = 0;
      }

      if(countTwo == 0 && errorTwo == 1){
        lcd.setCursor(0,1);
        lcd.print("Sensor2:        ");
        errorTwo = 0;
      }

      if(countOne > 5){
        tempF = -196.00;
        errorOne = 1;
      }
      if(countTwo > 5){
        tempF = -196.00;
        errorTwo = 1;
      }
      */

      if(tempF != -196.00){

       if(i==0 && onOne == 1 && error<4){
        lcd.setCursor(8,0);
        lcd.print(tempC);
       } else if(i == 1 && onTwo == 1){
        lcd.setCursor(8,1);
        lcd.print(tempC);
       }
        
        Serial.print("Temp C: ");
        Serial.print(tempC);
        Serial.print(" Temp F: ");
        Serial.println(tempF); // Converts tempC to Fahrenheit
        

      }else{
        if(i==1){
          lcd.setCursor(0,0);
          lcd.print("UNPLUGGED SENSOR");
        }else{
          lcd.setCursor(0,1);
          lcd.print("UNPLUGGED SENSOR");
        }
      }
        }
    }else{
      previousError = previousError + 1;
    }
    }
    }else{
      lcd.clear();
      fixMessage = 1;
    }
}


// function to print a device address
void printAddress(DeviceAddress deviceAddress){
  for (uint8_t i = 0; i < 8; i++){
    if (deviceAddress[i] < 16) Serial.print("0");{
      Serial.print(deviceAddress[i], HEX);
    }
  }
}

//OLD DOG CODE
/*
  // Debounce logic for button one
  if (readingOne != lastButtonStateOne) {
    lastDebounceTimeOne = millis();
  }
  if ((millis() - lastDebounceTimeOne) > debounceDelay) {
    if (readingOne != buttonStateOne) {
      buttonStateOne = readingOne;
      if (buttonStateOne == HIGH) {
        onOne = !onOne;  // Toggle on/off
        if (onOne == 0) {
          Serial.println("HELL1");
          lcd.setCursor(8, 0);
          lcd.print(" OFF ");
        }
      }
    }
  }
  */
/*
  // Debounce logic for button two
  if (readingTwo != lastButtonStateTwo) {
    lastDebounceTimeTwo = millis();
  }
  if ((millis() - lastDebounceTimeTwo) > debounceDelay) {
    if (readingTwo != buttonStateTwo) {
      buttonStateTwo = readingTwo;
      if (buttonStateTwo == HIGH) {
        onTwo = !onTwo;  // Toggle on/off
        if (onTwo == 0) {
          Serial.println("ROCKy");
          lcd.setCursor(8, 1);
          lcd.print(" OFF ");
        }
      }
    }
  }
  lastButtonStateTwo = readingTwo;
    /*
    int readOne = digitalRead(buttonOne);
    int readTwo = digitalRead(buttonTwo);


     if(readOne != lastOne){
      lastTimeOne = millis();
     }

    if((millis()-lastTimeOne)>debounceDelay){
      if(readOne!=buttonStateOne){
        buttonStateOne = readOne;
        Serial.println("Kill2");
        
        if(buttonStateOne == HIGH){
          lastOne = readOne;
          if(onOne = 1){
          lcd.setCursor(8,0);
          lcd.print(" OFF ");
          onOne = 0;
        }else{
        onOne = 1;
      }
      }
      }
    }
 */       
