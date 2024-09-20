#include "WiFi.h"

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  Serial.println("Setup done");
}

void loop() {
  // put your main code here, to run repeatedly:
  //digitalWrite(13, HIGH);
  //delay(1000);
  //digitalWrite(13, LOW)

}
