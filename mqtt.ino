


void callback(char* topic, byte* payload, unsigned int length) {
  //checkNetwork();
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  //payload[lenght];
  String message = String((char*)payload).substring(0,length);
  message.trim();
  
  //Serial.println();

  if (String(topic) == "/" + deviceName + "/deviceCmd") {
//    Serial.println("");
//    Serial.println("");
//    Serial.print("message: "); Serial.print(message); Serial.print("  Length: "); Serial.println(message.length());
//    Serial.println("/deviceCmd");

    if (message == "reboot") {
      Serial.println("Rebooting!!!!!");
      client.publish((char*)(String("/" + deviceName + "/deviceStatus").c_str()), (char*)String("Rebooting").c_str()); 
      
      Serial.end();
      delay(100);
      reset();
    }
    if (message == "time") {
      Serial.println("Sending time...");
      client.publish((char*)(String("/" + deviceName + "/deviceStatus").c_str()), (char*)timeClient.getFormattedDate().c_str()); 
    }
    if (message == "settime") {
      Serial.println("Setting time...");
      timeClient.update();
      delay(500);
      client.publish((char*)(String("/" + deviceName + "/deviceStatus").c_str()), (char*)timeClient.getFormattedDate().c_str()); 
    }
    //Serial.println("");
    //Serial.println("");
   }
}

void mqttPublish(String topic, String payload) {
  client.publish((char*)(String("/" + deviceName + "/" + topic).c_str()), (char*)payload.c_str()); 
}
