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

// Mqtt topics
char tempTopic[] = "accontroller/temp";
char statusTopic[] = "accontroller/status";
char controlTopic[] = "accontroller/control";

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
  client.publish(statusTopic, "Ac on");

  delay(5000);
  Serial.println("Off");
  irsend.sendRaw(powerOff,200,38);
  digitalWrite(LED, HIGH);
  client.publish(statusTopic, "Ac off");

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

  if (String(topic)==controlTopic) {
     client.publish(statusTopic, "Got ctrl command");  // Wants a char
     if (stringPayload=="On") {
        client.publish(statusTopic, "Ac on");  // Wants a char
        digitalWrite(LED, HIGH);          // turn the LED on.
        irsend.sendRaw(powerOn,200,38);
     }
     else if (stringPayload=="Off"){
      digitalWrite(LED, LOW);          // turn the LED off.
      acon = false;
      client.publish(statusTopic, "Ac off");
      irsend.sendRaw(powerOff,200,38);
    }
    else  {
      Serial.println("Wrong command");
      client.publish(statusTopic, "Command error");
    }
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("AccontrollerEsp", mqtt_user, mqtt_password)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(statusTopic, "Hello from accontroller");
      // ... and resubscribe
      client.subscribe("time"); // Time is published on the network, we use it for time keeping
      client.subscribe(controlTopic);    // Used to control AC manually via Mqtt
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
    root["temp"] = temp;
    // Convert json object to char
    root.printTo((char*)msg, root.measureLength() + 1);
    client.publish(tempTopic, msg);  // Wants a char
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

