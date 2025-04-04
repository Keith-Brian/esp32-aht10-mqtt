/*
MIT License

Copyright (c) 2025 Keith Brian

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights  
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell      
copies of the Software, and to permit persons to whom the Software is          
furnished to do so, subject to the following conditions:                       

The above copyright notice and this permission notice shall be included in all 
copies or substantial portions of the Software.                                

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR     
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,       
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE    
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER         
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,  
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE  
SOFTWARE.
*/

// adding mqtt headers
#include <WiFi.h>
#include <PubSubClient.h>

#include <AHT10.h>
#include <Wire.h>

#define WIFI_SSID "Schrodinger "
#define WIFI_PASS "@WaveEquation"

#define MQTT_SERVER "192.168.100.5"

WiFiClient espClient;
PubSubClient mqtt_device(espClient);

#define LED_PIN 4

float temperature = 0;
float humidity = 0;

long lastMsg = 0;

AHT10 myAHT10(AHT10_ADDRESS_0X38);

void wifi_connect();
void callback(char* topic, byte* payload, unsigned int length);
void reconnect();

void setup()
{
  Serial.begin(115200);
  Serial.println();

  pinMode(LED_PIN, OUTPUT);
  
  while (myAHT10.begin() != true)
  {
    Serial.println(F("AHT10 not connected or fail to load calibration coefficient"));
    delay(5000);
  }
  Serial.println(F("AHT10 OK"));

  wifi_connect();
  mqtt_device.setServer(MQTT_SERVER, 1883);
  mqtt_device.setCallback(callback);
}

void wifi_connect(){
  delay(10);

  Serial.print(F("Connecting to: "));
  Serial.println(F(WIFI_SSID));

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  
  Serial.println(F(""));
  Serial.println(F("WiFi connected!"));
}

void callback(char* topic, byte* payload, unsigned int length){
  Serial.print(F("Message arrived on ["));
  Serial.print(topic);
  Serial.print(F("] "));

  String message_buffer;

  for(int i=0; i < length; i++){
    Serial.print((char)payload[i]);
    message_buffer += (char)payload[i];
  }
  Serial.println(F(""));

  if(String(topic) == "actuators/lights"){
    Serial.println(F("Actuating the lights..."));

    if(message_buffer == "ON"){
      Serial.println(F("Switching lights ON"));
      digitalWrite(LED_PIN, HIGH);
    }else if(message_buffer == "OFF"){
      Serial.println(F("Switching lights OFF"));
      digitalWrite(LED_PIN, LOW);
    }
  }
}

void reconnect(){
  while(!mqtt_device.connected()){
    Serial.println("Attempting MQTT connection!");

    if(mqtt_device.connect("esp32-device")){
      Serial.println(F("MQTT Connected!"));
      mqtt_device.subscribe("actuators/lights");
    }else {
      Serial.print(F("failed, rc="));
      Serial.print(F(mqtt_device.state()));
      Serial.println(F(" trying again in 5s"));
      delay(5000);
    }
  }
}

void loop()
{
  if(!mqtt_device.connected()){
    reconnect();
  }

  mqtt_device.loop();

  long now = millis();

  if(now - lastMsg > 5000){
    lastMsg = now;

    temperature = myAHT10.readTemperature();
    humidity = myAHT10.readHumidity();

    char tempString[8];
    dtostrf(temperature,1,2,tempString);
    Serial.print(F("Temperature: "));
    Serial.println(F(tempString));
    mqtt_device.publish("sensors/temperature", tempString);

    char humiString[8];
    dtostrf(humidity,1,2,humiString);
    Serial.print(F("Humidity: "));
    Serial.println(F(humiString));
    mqtt_device.publish("sensors/humidity", humiString);
  }
}
