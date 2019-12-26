
void wifi_setup() {
  connectWiFi();
}

//Wifi Routines
void connectWiFi() {
  // check for the WiFi module:
  digitalWrite(LED_BUILTIN, LOW);
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }
   
  WiFi.setHostname(HOSTNAME);
 
  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(SECRET_SSID, SECRET_PASS);
   // WiFi.setHostname(HOSTNAME);

    // wait 3 seconds for connection:
    delay(3000);

  client.setServer(mqtt_server, 1883);
  Serial.println("Connecting MQTT to callback");
  client.setCallback(callback);
  digitalWrite(LED_BUILTIN, HIGH);
  }

  // you're connected now, so print out the data:
  //Serial.print("You're connected to the network");
  digitalWrite(LED_BUILTIN, HIGH);
  printCurrentNet();
  printWifiData();
  
}



void printWifiData() {
  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  Serial.println(ip);

  // print your MAC address:
  byte mac[6];
  WiFi.macAddress(mac);
  Serial.print("MAC address: ");
  printMacAddress(mac);
}

void printCurrentNet() {
  String networkStatus = "";
  // print the SSID of the network you're attached to:
//  Serial.print("SSID: ");
//  networkStatus += "SSID: ";
//  networkStatus += WiFi.SSID();
//  networkStatus += "\n";
//  Serial.println(WiFi.SSID());

  // print the MAC address of the router you're attached to:
  byte bssid[6];
  WiFi.BSSID(bssid);
  //Serial.print("BSSID: ");
//  networkStatus += "BSSID: ";
 // networkStatus += String(bssid);
  //networkStatus += "\n\n";
  //printMacAddress(bssid);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  //Serial.print("signal strength (RSSI):");
  //Serial.println(rssi);
//  networkStatus += "Signal Strength (RSSI): ";
//  networkStatus += String(rssi);
//  networkStatus += "\n";
  // print the encryption type:
  byte encryption = WiFi.encryptionType();
//  Serial.print("Encryption Type:");
//  Serial.println(encryption, HEX);
//  Serial.println();

 // dBm to Quality:
 float dBm = rssi;
 int quality = 0;
    if(dBm <= -100)
        quality = 0;
    else if(dBm >= -50)
        quality = 100;
    else
        quality = 2 * (dBm + 100);

//    // Quality to dBm:
//    if(quality <= 0)
//        dBm = -100;
//    else if(quality >= 100)
//        dBm = -50;
//    else
//        dBm = (quality / 2) - 100;
   networkStatus += "WiFi Quality: ";
   networkStatus += quality;
   networkStatus += "%";

  networkStatus = "{\"wifiSignal\" : \"" + String(quality) + "\"}";
  String wifiSignal;
  wifiSignal = toJson("wifiSignal", String(quality), "TimeStamp", timeClient.getFormattedDate());
  //Serial.print("Publish: ");
  //Serial.println(networkStatus);
  client.publish((char*)(String("/" + deviceName + "/wifi").c_str()), (char*) wifiSignal.c_str());
  networkStatus = "";
}

void printMacAddress(byte mac[]) {
  for (int i = 5; i >= 0; i--) {
    if (mac[i] < 16) {
      Serial.print("0");
    }
    Serial.print(mac[i], HEX);
    if (i > 0) {
      Serial.print(":");  
    }
  }
  Serial.println();
}

void checkWiFiConnection() {
   status = WiFi.status();
  if (status != WL_CONNECTED) {
    Serial.println("OOPS! WiFi Disconnected!");
    digitalWrite(LED_BUILTIN, LOW);
    connectWiFi();
    timeClient.update();
    client.setCallback(callback);
  } else {
    //Serial.println("WiFi Seems connected???");
  }
}

void checkNetwork() {
  Serial.println("checkNetwork()");
  checkWiFiConnection();
     //Serial.print("LED_BUILTIN : "); Serial.println(LED_BUILTIN);
  while (!client.connected()) {
    Serial.println("OOPS! MQTT Disconnected!");
    digitalWrite(LED_BUILTIN, LOW);
    Serial.print("Attempting MQTT connection...");

    if (status != WL_CONNECTED) {
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
