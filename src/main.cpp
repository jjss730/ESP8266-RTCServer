#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <RTClib.h>
#include <Wire.h>

#define MAX_MSG_LEN (128)
#define LED (2)

// WiFi Configuration
const char* ssid = "Searching...";
const char* password = "l0nbr1g@9@c@";


//MQTT Configuration
const char* mqttServer = "10.0.0.49";
const int mqttPort = 1883;
const char* mqttUser = "";
const char* mqttPassword = "";
// const char *topic = "testTopic";
const char *topic = "sprinkler";

char t[32];
char t2[32];


WiFiClient espClient;
PubSubClient client(espClient);
RTC_DS3231 rtc;

DateTime now;
long ThisTime;
long LastTime;
 
// Assigning relays to digital outputs 

// int Tx_Pin = GPIO1;
// int Rx_Pin = GPIO3;

int relay8Pin = D8;
long Delta_T = 0;
 

void setRelay8State(boolean state){

  digitalWrite (relay8Pin, !state);
  if (state) {
    client.publish("sprinkler", "r8_is_on");
    
  } else {
    client.publish("sprinkler", "r8_is_off");  
  }
}

void callback(char *msgTopic, byte *msgPayload, unsigned int msgLength) {
  // copy payload to a static string
  static char message[MAX_MSG_LEN+1];
  if (msgLength > MAX_MSG_LEN) {
    msgLength = MAX_MSG_LEN;
  }
  strncpy(message, (char *)msgPayload, msgLength);
  message[msgLength] = '\0';
  
  Serial.printf("topic %s, message received: %s\n", msgTopic, message);

  // decode message

  DateTime now = rtc.now();
  sprintf(t, "%02d:%02d:%02d", now.hour(),now.minute(), now.second());
  Serial.println(t);
  client.publish("timehour", t);


  if (strcmp(message, "r8_off") == 0) {
    setRelay8State(false);
  } else if (strcmp(message, "r8_on") == 0) {
    setRelay8State(true);
  }

}

void connectMQTT() {
  // Wait until we're connected
  while (!client.connected()) {
    // Create a random client ID
    String clientId = "ESP8266-";
    clientId += String(random(0xffff), HEX);
    Serial.printf("MQTT connecting as client %s...\n", clientId.c_str());
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("MQTT connected");
      // Once connected, publish an announcement...
      client.publish(topic, "hello from ESP8266");
      // ... and resubscribe
      client.subscribe(topic);
    } else {
      Serial.printf("MQTT failed, state %s, retrying...\n", client.state());
      // Wait before retrying
      delay(2500);
    }
  }
}

void setup() {
 
  pinMode(relay8Pin, OUTPUT);

  digitalWrite(relay8Pin, HIGH);
 
  // Connect to WiFi network
  Serial.begin(9600);
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  Wire.begin();
  rtc.begin();
  rtc.adjust(DateTime(F(__DATE__),F(__TIME__)));
  //rtc.adjust(DateTime(2021, 7,20, 5, 15,0));


  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
 
  // Print the IP address
  Serial.print("Use this URL : ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
 
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
  
  while (!client.connected()) {
    Serial.println("Connecting to MQTT..."); 
    if (client.connect("ESP8266Client", mqttUser, mqttPassword )) {
  
      Serial.println("connected");  
  
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }
 
  client.publish("sprinkler", "Hello from ESP8266");
  client.subscribe("sprinkler");
}




 
void loop() {
  if (!client.connected()){
    connectMQTT();
  }
  client.loop();

  now = rtc.now();
  ThisTime = now.hour()*60*60 + now.minute()*60 + now.second();
  Delta_T = ThisTime - LastTime;
  if ((Delta_T >= 10)&(Delta_T>=0)){
    sprintf(t, "%02d:%02d:%02d", now.hour(),now.minute(), now.second());
    sprintf(t2, "%05d", now.hour()*60*60 + now.minute()*60 + now.second());
    Serial.print(t);
    Serial.print("  ");
    Serial.print(t2);
    Serial.println("  ");

    client.publish("timehour", t);
    client.publish("timeref", t2);
    LastTime = ThisTime;

  } else{
    Serial.println(String(Delta_T));
  }
  delay(500);
  
}