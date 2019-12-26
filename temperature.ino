


// which analog pin to connect
#define THERMISTORPIN A7       
// resistance at 25 degrees C
#define THERMISTORNOMINAL 10000      
// temp. for nominal resistance (almost always 25 C)
#define TEMPERATURENOMINAL 25   
// how many samples to take and average, more takes longer
// but is more 'smooth'
#define NUMSAMPLES 10
#define SAMPLEDELAY 20
// The beta coefficient of the thermistor (usually 3000-4000)
#define BCOEFFICIENT 3950
// the value of the 'other' resistor
#define SERIESRESISTOR 10000    

int samples[NUMSAMPLES];
float maxTemp = 0;
float minTemp = 0;

void setMinTemp(float temp) {
  if (minTemp == 0)
    minTemp = temp;
    
   if (temp < minTemp)
    minTemp = temp;
}

void setMaxTemp(float temp) {
  if (maxTemp == 0)
    maxTemp = temp;

  if (maxTemp < temp)
    maxTemp = temp;
}

float getTemp() {
  uint8_t i;
  float average;

  // take N samples in a row, with a slight delay
  for (i=0; i< NUMSAMPLES; i++) {
   samples[i] = analogRead(THERMISTORPIN);
   delay(SAMPLEDELAY);
  }
  
  // average all the samples out
  average = 0;
  for (i=0; i< NUMSAMPLES; i++) {
     average += samples[i];
  }
  average /= NUMSAMPLES;

  Serial.print("Average analog reading "); 
  Serial.println(average);
  
  // convert the value to resistance
  average = 1023 / average - 1;
  average = SERIESRESISTOR / average;
  Serial.print("Thermistor resistance "); 
  Serial.println(average);
  
  float steinhart;
  steinhart = average / THERMISTORNOMINAL;     // (R/Ro)
  steinhart = log(steinhart);                  // ln(R/Ro)
  steinhart /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
  steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
  steinhart = 1.0 / steinhart;                 // Invert
  steinhart -= 273.15;                         // convert to C
  
  Serial.print("Temperature "); 
  Serial.print(steinhart);
  Serial.println(" *C");

  setMinTemp(steinhart);
  setMaxTemp(steinhart);
  return steinhart;
}

void getTemperatureAndHumidity() {
  Serial.println("getTemperatureAndHumidity()");
  byte temperature = 0;
    byte humidity = 0;
    byte pdata[40];
    int err = SimpleDHTErrSuccess;
    
    if ((err = dht11.read(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
      Serial.print("DHT11 read failed "); Serial.println(err); 
    } else {
      if ((int)temperature != lastTemp) {
        lastTemp = (int)temperature;
        lastTemp = 0; //disable last value tracking
        String sTemp;
        sTemp = toJson("Temperature", String(temperature), "TimeStamp", timeClient.getFormattedDate());
        //Serial.println(sTemp);
        client.publish((char*)(String("/" + deviceName + "/temperature").c_str()), (char*) sTemp.c_str()); 
      }
      if ((int)humidity != lastHumidity) {
        lastHumidity = (int)humidity;
        lastHumidity = 0; //disable last value tracking;
        String sHumidity;
        sHumidity = toJson("Humidity", String(humidity), "TimeStamp", timeClient.getFormattedDate());
        //Serial.println(sHumidity);
        client.publish((char*)(String("/" + deviceName + "/humidity").c_str()), (char*) sHumidity.c_str()); 
      }
    }
}
