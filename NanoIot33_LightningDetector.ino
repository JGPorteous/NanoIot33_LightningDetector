
#include "arduino_secrets.h"
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
// Use arduino_secrets.h.example to create one for the first time.



#include "SimpleTimer.h"
#include "NTPClient.h"
//WIFI
#include "WiFiNINA.h"
#include "WiFiUdp.h"

//Temp and Humidity
#include <SimpleDHT.h>

//MQTT
#include "PubSubClient.h"

// Lightning Detector - SPI
#include <SPI.h>
#include <Wire.h>
#include "SparkFun_AS3935.h"

#define INDOOR 0x12 
#define OUTDOOR 0xE
#define LIGHTNING_INT 0x08
#define DISTURBER_INT 0x04
#define NOISE_INT 0x01

SparkFun_AS3935 lightning;
const int lightningInt = 2;  // Interrupt pin for lightning detection 
int spiCS = 3;  // Chip select pin 

byte noiseFloor = 3;
byte location = INDOOR; //linked to Noise Floor = INDOOR (28 - 146) or OUTDOOR (390 - 2000)
byte spike = 2;  //Higher = less disturburs but less sensitivity
byte watchDogVal = 1;  // 0 - 15 - higher = less sensitivity and less distance, lower = higher sensitivity
byte lightningThresh = 1; 
int tuneCap = 120; // 8pF: 8, 16, 24, 32 etc. up to 120pF  //Don't change, used oscilloscope to determine
boolean maskDisturbers = true;
int divRatio = 16;  //Don't change, used oscilloscope to determine

//WIFI
WiFiClient wifiClient;
char ssid[] = SECRET_SSID;        
char pass[] = SECRET_PASS;    
int status = WL_IDLE_STATUS;   

//MQTT
PubSubClient client(wifiClient);
const char* mqtt_server = MQTT_SERVER;
String deviceName = MQTT_DEVICE_NAME;

//Temp
int pinDHT11 = 7;
SimpleDHT11 dht11(pinDHT11);
//Temperature and Humidity
int lastTemp = 0;
int lastHumidity = 0;

//SimpleTimer
SimpleTimer timer;

//NTP
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

void setup() {
  SPI.begin(); // For SPI
  delay(500);
  pinMode(LED_BUILTIN, OUTPUT);
  
  Serial.begin(115200);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);

  //Lightning
  lightning_setup();
    
  //NTP
  timeClient.begin();
  timeClient.setTimeOffset(7200); //7200 = GMT+2

  //SimpleTimer
  // This is a custom implementation that allows running timers set on Intervals on the first run regardless of time
  timer.setInterval(30000, checkNetwork, true);  //Every 30 Seconds, check network connection
  timer.setInterval(300000, updateNTP, true);    //Every 5 Minutes (300000 milliseconds), update RTC via NTP
  timer.setInterval(60000, getTemperatureAndHumidity, true); //Every 60 Seconds, get Temperature and Humidity
  
  //MQTT
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  //WIFI
  wifi_setup();
}

void loop() {
  checkLightning();
  timer.run();
  //printCurrentNet();
  client.loop();
}

void reset() {
  //https://community.st.com/s/question/0D50X00009XkamfSAB/nvicsystemreset-in-interrupt
  // NVIC_SystemReset () is not working, used below assembler code to get it to work
  asm(
    "ldr     r1, =0xE000ED0C \n" //; NVIC Application Interrupt and Controller"
    "ldr     r0, =0x05FA0004 \n" //; Magic"
    "str     r0, [r1, #0] \n" //   ; Reset"
    "bx      lr\n"
    );
}

void updateNTP() {
  Serial.println("updateNTP()");
   timeClient.update();
   client.publish((char*)(String("/" + deviceName + "/deviceStatus").c_str()), (char*)String("Set Time").c_str()); 
}

String toJson(String item, String value) {
  return "{\"" + item + "\" : \"" + value + "\"}";
}

String toJson(String item1, String value1, String item2, String value2) {
  return "{\"" + item1 + "\" : \"" + value1 + "\", \"" + item2 + "\" : \"" + value2 + "\"}";
}

String toJson(String item1, String value1, String item2, String value2, String item3, String value3) {
  return "{\"" + item1 + "\" : \"" + value1 + "\", \"" + item2 + "\" : \"" + value2 + "\", \"" + item3 + "\" : \"" + value3 + "\"}";
}
