/*


*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoJson.h>
#include <Time.h>
#include <ArduinoOTA.h>
// IR library
#include <IRremoteESP8266.h>
IRsend irsend(5);

// just added my own array for the raw ir signal
unsigned int powerOn[200] = {4500,4250,700,1500,650,450,650,1550,650,1550,650,450,650,450,650,1500,700,400,650,450,650,1550,650,450,650,450,650,1550,650,1500,700,400,700,1500,650,1550,650,450,650,1550,650,1550,600,1550,700,1500,650,1550,650,1550,600,500,650,1500,700,400,650,450,650,450,700,400,700,400,700,400,650,450,650,450,650,1550,650,450,650,450,600,500,600,450,650,450,650,1550,700,1500,650,450,650,1550,650,1550,600,1550,650,1550,650,1550,650,5100,4500,4300,650,1550,650,450,650,1500,700,1500,700,400,700,400,700,1500,650,450,650,450,650,1550,650,450,650,450,600,1550,700,1500,650,450,650,1550,650,1550,650,450,600,1550,700,1500,650,1550,650,1550,650,1500,700,1500,650,450,650,1550,650,450,650,450,650,450,650,450,650,450,650,450,600,500,600,500,600,1550,700,400,700,400,650,450,650,450,650,450,600,1600,650,1550,600,500,600,1550,650,1550,650,1550,650,1550,650,1500,650};
unsigned int powerOff[200] =  {4550,4250,700,1500,650,450,650,1550,650,1550,650,450,650,450,600,1550,650,450,650,450,650,1550,650,450,650,450,650,1550,650,1500,650,450,700,1500,650,450,700,1500,650,1550,650,1550,650,1500,650,450,650,1550,650,1550,650,1550,600,500,600,500,650,450,600,450,650,1550,650,450,650,450,650,1550,650,1550,650,1500,650,450,650,450,700,400,700,400,650,450,650,450,650,450,650,450,650,1550,650,1550,650,1500,650,1550,650,1550,650,5150,4450,4300,650,1550,650,450,650,1550,650,1500,700,400,650,450,650,1550,650,450,650,450,650,1550,650,450,650,450,650,1500,650,1550,650,450,650,1550,600,500,650,1550,650,1500,650,1550,650,1550,650,450,650,1500,700,1550,600,1550,650,450,650,450,650,450,650,450,650,1550,650,450,650,450,650,1550,600,1550,650,1550,650,450,650,450,650,450,650,450,650,450,600,500,650,450,650,450,650,1500,700,1500,650,1550,650,1550,650,1550,600};

//For Json output
StaticJsonBuffer<200> jsonBuffer;
JsonObject& root = jsonBuffer.createObject();

const char* ssid = "NETGEAR83";
const char* password = "........";
const char* mqtt_server = "192.168.1.79";
const char* mqtt_user = "emonpi";
const char* mqtt_password = "emonpimqtt2016";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;
bool acon = false;

#define ONE_WIRE_BUS 13  // DS18B20 pin
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);
float temp;

// Built in led on Nodemcu
//#define LED 14 
// Built in led on Esp12
#define LED 2

void setup() {
  pinMode(LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  digitalWrite(LED, LOW);   // turn the LED on.
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  if (!client.connected()) {
    reconnect();
  }
  ota();  // Over the air programming
  
  delay(1000);
  DS18B20.requestTemperatures(); 
  temp = DS18B20.getTempCByIndex(0);
  Serial.print("Temperature: ");
  Serial.println(temp);

  irsend.begin();
  // Testing hardware by sending on and off code 
  Serial.println("Testing hardware by sending on and off code");
  Serial.println("On");
  irsend.sendRaw(powerOn,200,38);
  client.publish("accontrollerPower", "Ac on");

  delay(5000);
  Serial.println("Off");
  irsend.sendRaw(powerOff,200,38);
  digitalWrite(LED, HIGH);
  client.publish("accontrollerPower", "Ac off");

}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

}

// When a Mqtt message has arrived
// 
void callback(char* topic, byte* payload, unsigned int length) {
  char message[14] ="";
  //time_t pctime = 0;
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  // Extract payload
  String stringPayload = "";
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    stringPayload += (char)payload[i];
  }
  Serial.println();
    
  if (String(topic)=="time") {
    Serial.println("Topic=time");
    String servertime = stringPayload;
    Serial.println();
    //Serial.println( servertime );
    // Servertime is in millis, remove last chars
    servertime = servertime.substring(0,10);
    //Serial.println( servertime );
    uint32_t iclienttime;
    iclienttime = servertime.toInt();
    //Serial.print( "iclienttime: " );
    //Serial.println( iclienttime );
  
    setTime(iclienttime);  
    Serial.print("Got time from server: ");
    int realhour = hour()+2; // Timezone = DST+2
    Serial.println(realhour); 
    
    if (realhour >= 12 && realhour <= 18) {
      Serial.println("Ok to run AC");
      // It's ok to start AC
      if (!acon) {
        acon = true;
        digitalWrite(LED, LOW);          // turn the LED on.
        client.publish("accontrollerPower", "Ac on");
        Serial.println("Ac on");
        irsend.sendRaw(powerOn,200,38);
      }
      
    }
    if (realhour >= 19 || realhour <= 11) {
        Serial.println("NOT ok to run AC");
        if (acon) {
          digitalWrite(LED, HIGH);          // turn the LED off.
          acon = false;
          client.publish("accontrollerPower", "Ac off");
          Serial.println("Ac off");
          irsend.sendRaw(powerOff,200,38);
        }
    }
  }
  else if (topic="accontrol") {
    Serial.print("Got command: ");
    String command = stringPayload;
    Serial.println(stringPayload);
    if (stringPayload=="on") {
      digitalWrite(LED, HIGH);          // turn the LED on.
      client.publish("accontrollerPower", "Ac on");
      irsend.sendRaw(powerOn,200,38);
    }
    else if (stringPayload=="off"){
      digitalWrite(LED, LOW);          // turn the LED off.
      acon = false;
      client.publish("accontrollerPower", "Ac off");
      irsend.sendRaw(powerOff,200,38);
    }
    else  {
      Serial.println("Wrong command");
      client.publish("accontroller", "Command error");
    }
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("Accontroller", mqtt_user, mqtt_password)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("accontroller", "Hello from accontroller");
      // ... and resubscribe
      client.subscribe("time"); // Time is published on the network, we use it for time keeping
      client.subscribe("accontrol");    // Used to control AC manually via Mqtt
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void loop() {
  ArduinoOTA.handle();

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 120000) {
  //if (now - lastMsg > 20000) {
    lastMsg = now;
    DS18B20.requestTemperatures(); 
    temp = DS18B20.getTempCByIndex(0);  // temp is a float

    // Is time set?
    String tset = String(timeStatus());
    String tmess = String("Time set = ") += tset;
    Serial.println(tmess);
    
    // If time not set then tset=0, else it's 2
    // Dont do anything until time is set
    if (tset=="2") {
      root["temp"] = temp;
      String timeHM = String(hour()+2);
      timeHM += ":";
      timeHM += minute();
      root["time"] = timeHM;
      
      char buffer[256];
      root.printTo(buffer, sizeof(buffer));
      char* servermess=buffer;
  
      dtostrf(temp, 3, 1, msg); // Convert float to char
      String mess = String(temp, 2);
      mess += "-";
      mess += timeHM;
      Serial.print("Publish message: ");
      Serial.println(mess);   
      
      client.publish("accontroller", servermess);  // Wants a char
   }
  }
}

void ota(){
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}

