#include <DallasTemperature.h>
#include <OneWire.h>


#define BUTTON_PIN 0

#define DS18B20_PIN 4          // DS18B20 Data Pin
#define DS18B20_POWER_PIN 12    // DS18B20 Vcc pin 

#define DEEP_SLEEP_TIME 1000 * 60 * 2 //Deep sleep in 2 minutes

OneWire oneWire(DS18B20_PIN);
DallasTemperature sensors(&oneWire);

int numDev

Device Address tempDeviceAddress;
void setup()
{
  Serial.begin(115200)
  pinMode(DS18B20_POWER_PIN, OUTPUT);
  digitalWrite(DS18B20_POWER_PIN, HIGH);
  
  sensors.begin();
  numDev = sensors.getDeviceCount();
  
  Serial.print("Locating devices...");
  Serial.print("Found ");
  Serial.print(numberOfDevices, DEC);
  Serial.println(" devices.");

  for(int i=0;i<numberOfDevices; i++){
    // Search the wire for address
    if(sensors.getAddress(tempDeviceAddress, i)){
      Serial.print("Found device ");
      Serial.print(i, DEC);
      Serial.print(" with address: ");
      printAddress(tempDeviceAddress);
      Serial.println();
    } else {
      Serial.print("Found ghost device at ");
      Serial.print(i, DEC);
      Serial.print(" but could not detect address. Check power and cabling");
    }
  }

}

void loop()
{   
    sensors.requestTemperatures();
    float temperatureC = sensors.getTempCByIndex(0);

     for(int i=0;i<numberOfDevices; i++){
    // Search the wire for address
    if(sensors.getAddress(tempDeviceAddress, i)){
      // Output the device ID
      Serial.print("Temperature for device: ");
      Serial.println(i,DEC);
      // Print the data
      float tempC = sensors.getTempC(tempDeviceAddress);
      Serial.print("Temp C: ");
      Serial.print(tempC);
      Serial.print(" Temp F: ");
      Serial.println(DallasTemperature::toFahrenheit(tempC)); // Converts tempC to Fahrenheit
    }
  }
  delay(5000);
}

// function to print a device address
void printAddress(DeviceAddress deviceAddress) {
  for (uint8_t i = 0; i < 8; i++){
    if (deviceAddress[i] < 16) Serial.print("0");
      Serial.print(deviceAddress[i], HEX);
  }
}

    delay(100);

}