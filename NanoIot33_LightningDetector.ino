#include "arduino_secrets.h"
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
// Use arduino_secrets.h.example to create one for the first time.

#include "SimpleTimer.h"
#include "WiFiNINA.h"
#include "PubSubClient.h"
#include "NTPClient.h"
#include "WiFiUdp.h"

//SimpleTimer
SimpleTimer timer;

//WIFI
WiFiClient wifiClient;
char ssid[] = SECRET_SSID;        
char pass[] = SECRET_PASS;    
int status = WL_IDLE_STATUS;    

//MQTT
PubSubClient client(wifiClient);
const char* mqtt_server = MQTT_SERVER;
String deviceName = MQTT_DEVICE_NAME;

//NTP
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

//Temperature and Humidity
int lastTemp = 0;
int lastHumidity = 0;

//Lightning Detector
#include <SPI.h>
#include <Wire.h>
#include "SparkFun_AS3935.h"

#define INDOOR 0x12 
#define OUTDOOR 0xE
#define LIGHTNING_INT 0x08
#define DISTURBER_INT 0x04
#define NOISE_INT 0x01

// SPI
SparkFun_AS3935 lightning;

// Interrupt pin for lightning detection 
const int lightningInt = 2; 
// Chip select pin 
int spiCS = 3; 

// Values for modifying the IC's settings. All of these values are set to their
// default values. 
byte noiseFloor = 2;
byte location = OUTDOOR; //linked to Noise Floor = INDOOR (28 - 146) or OUTDOOR (390 - 2000)
byte spike = 2;  //Higher = less disturburs but less sensitivity
byte watchDogVal = 1;  // 0 - 15 - higher = less sensitivity and less distance, lower = higher sensitivity
byte lightningThresh = 1; 
int tuneCap = 120; // 8pF: 8, 16, 24, 32 etc. up to 120pF  //Don't change, used oscilloscope to determine
boolean maskDisturbers = true;
int divRatio = 16;  //Don't change, used oscilloscope to determine

// This variable holds the number representing the lightning or non-lightning
// event issued by the lightning detector. 
byte intVal = 0; 

//Timer
unsigned long startTime = millis ();
unsigned long interval = 60000;
bool firstRun = true;

//Temp and Humidity
#include <SimpleDHT.h>
int pinDHT11 = 7;
SimpleDHT11 dht11(pinDHT11);
 

void setup() {
  SPI.begin(); // For SPI
  delay(500);
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);

  //Lightning
  setupLightningDetector();
    
  //NTP
  timeClient.begin();
  timeClient.setTimeOffset(7200); //7200 = GMT+2

  //SimpleTimer
  timer.setInterval(30000, checkNetwork, true);  //Every 30 Seconds, check network connection
  //checkNetwork();
  timer.setInterval(30000, updateNTP, true);    //Every 5 Minutes, update RTC via NTP
  //updateNTP();
  timer.setInterval(60000, getTemperatureAndHumidity, true); //Every 60 Seconds, get Temperature and Humidity
}

void loop() {
  checkLightning();
  timer.run();
  printCurrentNet();
  client.loop();
}

void getTemperatureAndHumidity() {
  byte temperature = 0;
    byte humidity = 0;
    byte pdata[40];
    int err = SimpleDHTErrSuccess;
    
    if ((err = dht11.read(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
      Serial.print("DHT11 read failed "); Serial.print(err); 
    } else {
      if ((int)temperature != lastTemp) {
        lastTemp = (int)temperature;
        String sTemp;
        sTemp = toJson("Temperature", String(temperature), "TimeStamp", timeClient.getFormattedDate());
        //Serial.println(sTemp);
        client.publish((char*)(String("/" + deviceName + "/temperature").c_str()), (char*) sTemp.c_str()); 
      }
      if ((int)humidity != lastHumidity) {
        lastHumidity = (int)humidity;
        String sHumidity;
        sHumidity = toJson("Humidity", String(humidity), "TimeStamp", timeClient.getFormattedDate());
        //Serial.println(sHumidity);
        client.publish((char*)(String("/" + deviceName + "/humidity").c_str()), (char*) sHumidity.c_str()); 
      }
    }
}

void checkLightning() {
  if(digitalRead(lightningInt) == HIGH){
    intVal = lightning.readInterruptReg();
    if(intVal == NOISE_INT){
      Serial.print("N"); 
    }
    else if(intVal == DISTURBER_INT){
      Serial.print("D"); 
    } 
    if(intVal == LIGHTNING_INT){
      Serial.println();
      Serial.println("Lightning Strike Detected!"); 
      byte distance = lightning.distanceToStorm(); 
      Serial.print("Approximately: "); 
      Serial.print(distance); 
      Serial.println("km away!"); 

      long lightEnergy = lightning.lightningEnergy(); 
      Serial.print("Lightning Energy: "); 
      Serial.println(lightEnergy); 
      //lightning.clearStatistics(true);

      String lightningEvent;
      lightningEvent = toJson("Km",String(distance),"TimeStamp",String(timeClient.getFormattedDate()), "Energy", String(lightEnergy));
      client.publish((char*)(String("/" + deviceName + "/detector").c_str()), (char*) lightningEvent.c_str()); 
    }
  }
}
void reset() {
  //https://community.st.com/s/question/0D50X00009XkamfSAB/nvicsystemreset-in-interrupt
  asm(
    "ldr     r1, =0xE000ED0C \n" //; NVIC Application Interrupt and Controller"
    "ldr     r0, =0x05FA0004 \n" //; Magic"
    "str     r0, [r1, #0] \n" //   ; Reset"
    "bx      lr\n"
    );
}

void updateNTP() {
   timeClient.update();
   client.publish((char*)(String("/" + deviceName + "/deviceCmd").c_str()), (char*)String("Set Time").c_str()); 
}

void setupLightningDetector() {
  if( !lightning.beginSPI(spiCS, 2000000) ) { 
    Serial.println ("Lightning Detector did not start up, freezing!"); 
    while(1); 
  }
  else
    Serial.println("Lightning Detector Ready!\n");

  lightning.maskDisturber(maskDisturbers); 
  
  int maskVal = lightning.readMaskDisturber();
  Serial.print("Are disturbers being masked: "); 
  if (maskVal == 1)
    Serial.println("YES"); 
  else if (maskVal == 0)
    Serial.println("NO"); 

  lightning.setIndoorOutdoor(location); 

  int enviVal = lightning.readIndoorOutdoor();
  Serial.print("Are we set for indoor or outdoor: ");  
  if( enviVal == INDOOR )
    Serial.println("Indoor.");  
  else if( enviVal == OUTDOOR )
    Serial.println("Outdoor.");  
  else 
    Serial.println(enviVal, BIN); 

  lightning.setNoiseLevel(noiseFloor);  

  int noiseVal = lightning.readNoiseLevel();
  Serial.print("Noise Level is set at: ");
  Serial.println(noiseVal);

  lightning.watchdogThreshold(watchDogVal); 

  int watchVal = lightning.readWatchdogThreshold();
  Serial.print("Watchdog Threshold is set to: ");
  Serial.println(watchVal);

  lightning.spikeRejection(spike); 

  int spikeVal = lightning.readSpikeRejection();
  Serial.print("Spike Rejection is set to: ");
  Serial.println(spikeVal);

  lightning.lightningThreshold(lightningThresh); 

  uint8_t lightVal = lightning.readLightningThreshold();
  Serial.print("The number of strikes before interrupt is triggerd: "); 
  Serial.println(lightVal); 

  lightning.tuneCap(tuneCap); 

  // When reading the internal capcitor value, it will return the value in pF.
  int tuneVal = lightning.readTuneCap();
  Serial.print("Internal Capacitor is set to: "); 
  Serial.println(tuneVal);

  lightning.changeDivRatio(divRatio);

  // Read the division ratio - 16 is default.  
  byte divVal = lightning.readDivRatio(); 
  Serial.print("Division Ratio is set to: "); 
  Serial.println(divVal); 
  
  lightning.clearStatistics(true);
}

void callback(char* topic, byte* payload, unsigned int length) {
  checkNetwork();
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  //payload[lenght];
  String message = String((char*)payload).substring(0,length);
  message.trim();
  
  Serial.println();

  if (String(topic) == "/" + deviceName + "/deviceCmd") {
    Serial.println("");
    Serial.println("");
    Serial.print("message: "); Serial.print(message); Serial.print("  Length: "); Serial.println(message.length());
    Serial.println("/deviceCmd");

    if (message == "reboot") {
      Serial.println("Rebooting!!!!!");
      client.publish((char*)(String("/" + deviceName + "/deviceCmd").c_str()), (char*)String("Rebooting").c_str()); 
      
      Serial.end();
      delay(100);
      reset();
    }
    if (message == "time") {
      Serial.println("Sending time...");
      client.publish((char*)(String("/" + deviceName + "/deviceCmd").c_str()), (char*)timeClient.getFormattedDate().c_str()); 
    }
    if (message == "settime") {
      Serial.println("Setting time...");
      timeClient.update();
      delay(500);
      client.publish((char*)(String("/" + deviceName + "/deviceCmd").c_str()), (char*)timeClient.getFormattedDate().c_str()); 
    }
    Serial.println("");
    Serial.println("");
   }

}
void checkNetwork() {
  checkWiFiConnection();
     
  while (!client.connected()) {
    Serial.println("OOPS! MQTT Disconnected!");
    digitalWrite(LED_BUILTIN, LOW);
    Serial.print("Attempting MQTT connection...");

    if (status != WL_CONNECTED) {
      digitalWrite(LED_BUILTIN, LOW);
      break;
      connectWiFi();
      digitalWrite(LED_BUILTIN, HIGH);
      Serial.println(timeClient.getFormattedDate());
    }
  
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe((char*)String("/" + deviceName + "/deviceCmd").c_str());
      digitalWrite(LED_BUILTIN, HIGH);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
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


//
//https://github.com/patrickmoffitt/Atmel-SAMD21-Monitor
//float get_battery_vdc() {
//    // Read the battery level from the Feather M0 analog in pin.
//    const size_t readings_len{30};
//    std::array<int, readings_len> readings;
//    std::fill_n(begin(readings), readings_len, 0);
//    analogReadResolution(10);
//    analogReference(AR_INTERNAL1V0);
//    for (int i=0; i < (int) readings_len; i++) {
//        readings[i] = analogRead(ADC_PIN);
//        delay(33);
//    }
//    int sum = std::accumulate(begin(readings), end(readings), 0, std::plus<int>());
//    int level = (sum / readings_len);
//    DEBUG_PRINT("Raw ADC value: ");
//    DEBUG_PRINTLN(level);
//    return ((float) level * ADC_10_BIT_CORRECTION) / VDC_DIVISOR;
//}
//
//int get_battery_percent() {
//    float vdc = get_battery_vdc();
//    DEBUG_PRINT("Battery: "); DEBUG_PRINT(vdc); DEBUG_PRINTLN(" VDC");
//    int level;
//    // Convert battery level to percent.
//    level = map(vdc, MIN_BATTERY_VDC, MAX_BATTERY_VDC, 0.0, 100.0);
//    DEBUG_PRINT("Battery level: ");
//    DEBUG_PRINT(level);
//    DEBUG_PRINTLN("%");
//    return level;
//}
//
//int map(float x, float in_min, float in_max, float out_min, float out_max) {
//    float divisor = (in_max - in_min);
//    if(divisor == 0){
//        return -1; //AVR returns -1, SAM returns 0
//    }
//    return (x - in_min) * (out_max - out_min) / divisor + out_min;
//}
