#include <DallasTemperature.h>
#include <OneWire.h>
#include "WiFi.h"

#include "SSD1306.h"

#define BUTTON_PIN 0

//#define OLED_RESET_PIN 16       // OLED Reset Pin - required only for selected ESP32 boards and OLED screens
//#define OLED_SDA_PIN 4          // ESP32 I2C SDA - OLED SDA
//#define OLED_SCL_PIN 15         // ESP32 I2C SCL - OLED SCL
//#define OLED_I2C_ADDR 0x3c      // OLED I2C Address
#define DS18B20_PIN 13          // DS18B20 Data Pin
#define DS18B20_POWER_PIN 12    // DS18B20 Vcc pin 

#define DEEP_SLEEP_TIME 1000 * 60 * 2 //Deep sleep in 2 minutes

OneWire oneWire(DS18B20_PIN);
DallasTemperature sensors(&oneWire);
//SSD1306 display(OLED_I2C_ADDR, OLED_SDA_PIN, OLED_SCL_PIN);

void setup()
{
    /*
     * Setup OLED display
     */
    pinMode(OLED_RESET_PIN, OUTPUT);
    digitalWrite(OLED_RESET_PIN, LOW); // set GPIO16 low to reset OLED
    delay(50);
    digitalWrite(OLED_RESET_PIN, HIGH); // while OLED is running, must set GPIO16 to high
    Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);

  // display.init();
  //display.flipScreenVertically();
    
    /*
     * Initialize DS18B20 sensor
     */
    pinMode(DS18B20_POWER_PIN, OUTPUT);
    digitalWrite(DS18B20_POWER_PIN, HIGH);
    sensors.begin();

    WiFi.mode(WIFI_OFF);
    btStop();

    delay(500);
}

void loop()
{   
    sensors.requestTemperatures();
    float temperatureC = sensors.getTempCByIndex(0);

    //display.clear();
    //display.setFont(ArialMT_Plain_24);
    //display.drawString(10, 24, String(temperatureC, 1) + "ºC");
    //display.display();

    delay(100);

    if (millis() > DEEP_SLEEP_TIME) {
        //Disable OLED 
        //digitalWrite(OLED_RESET_PIN, LOW);

        esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
        esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
        esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
        digitalWrite(DS18B20_POWER_PIN, LOW);
        //Enter deep sleep
        esp_deep_sleep_start();
    }

}
/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 by Daniel Eichhorn
 * Copyright (c) 2016 by Fabrice Weinberg
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */
