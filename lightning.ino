

void checkLightning() {
  if(digitalRead(lightningInt) == HIGH){
    int intVal = lightning.readInterruptReg();
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

void lightning_setup() {
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
