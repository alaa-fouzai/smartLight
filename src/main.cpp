#ifdef ESP8266
 #include <ESP8266WiFi.h>
 #else
 #include <WiFi.h>
#endif

#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFiClient.h>


#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <ESP8266mDNS.h>
/****** WiFi Connection Details *******/
String ssid = "";
String password = "";

/******* MQTT Broker Connection Details *******/
const char* mqtt_server = "192.168.1.16";
const char* mqtt_username = "xyz";
const char* mqtt_password = "xyz123";
const int mqtt_port =1884;
boolean connectedToConnecty = false;
boolean gotAccessPointData = false;
/**** Secure WiFi Connectivity Initialisation *****/
WiFiClient espClient;
ESP8266WebServer server(80);
/**** MQTT Client Initialisation Using WiFi Connection *****/
PubSubClient client(espClient);
//button declaration 
int LightSwitchPin = D6;
int RelayPin = D5;
boolean LightSwitchState = false;
boolean RelayState = false;
String UUID = "112233";
int cnxtimeout = 0;
void setup_wifi();



void reconnect() {
  delay(50);
  if (gotAccessPointData) {
    setup_wifi();
  }
  while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        String clientId = "ESP8266Client-";   // Create a random client ID
        clientId += String(random(0xffff), HEX);
        // Attempt to connect
        Serial.println(clientId);
        if (client.connect(clientId.c_str(), mqtt_username, mqtt_password)) {
          Serial.println("connected");

          client.subscribe("led_state");   // subscribe the topics here

        } else {
          Serial.print("failed, rc=");
          Serial.print(client.state());
          Serial.println(" try again in 5 seconds");   // Wait 5 seconds before retrying
        }
      }
}
/***** Call back Method for Receiving MQTT messages and Switching LED ****/

void callback(char* topic, byte* payload, unsigned int length) {
  String incommingMessage = "";
  for (unsigned int i = 0; i < length; i++) incommingMessage+=(char)payload[i];

  Serial.println("Message arrived ["+String(topic)+"]"+incommingMessage);

  //--- check the incomming message
  Serial.println(topic);
  Serial.println(UUID.c_str());
    if( strcmp(topic,UUID.c_str()) == 0){
     if (incommingMessage.equals("1")) {
      
     }
    Serial.println("user changed state from app");
    Serial.println("Button state changed");
    Serial.println(incommingMessage);

    // allocate the memory for the document
    const size_t CAPACITY = JSON_OBJECT_SIZE(1);
    StaticJsonDocument<CAPACITY> doc;
    deserializeJson(doc, incommingMessage);
    JsonObject object = doc.as<JsonObject>();
    const char* world = object["state"];
    Serial.println(world);
    if (world == "true") {
      RelayState = true;
    }else {
      RelayState = false;
      }
      
    digitalWrite(RelayPin,RelayState);
  } else {
    Serial.println("unrelated topic");
  }

}
/**** Method for Publishing MQTT Messages **********/
void publishMessage(const char* topic, String payload , boolean retained){
  if (client.publish(topic, payload.c_str(), true))
      Serial.println("Message publised ["+String(topic)+"]: "+payload);
}
void getAccesspointfromApp() {
  if (server.hasArg("plain")== false){ //Check if body received
 
            server.send(400, "text/json", "{\"status\": \"error\"}");
            return;
      }
 
      String message = "";
             message += server.arg("plain");
             message += "";
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, message);
      JsonObject obj = doc.as<JsonObject>();
      String appssid= obj["ssid"];
      String apppassword= obj["password"];
      String appUUID= obj["UUID"];
      Serial.println(appssid);
      Serial.println(apppassword);
      Serial.println(appUUID);
      ssid = appssid;
      password = apppassword;
      UUID = appUUID;
      gotAccessPointData = true;
  

    server.send(200, "text/json", "{\"status\": \"ok\"}");
    delay(3000);
    server.stop();
    delay(1000);
    WiFi.softAPdisconnect(true);
    delay(1000);
    setup_wifi();
}
void restServerRouting() {
    server.on("/", HTTP_GET, []() {
        server.send(200, F("text/html"),
            F("Welcome to the REST Web Server"));
    });
    server.on(F("/Accesspoint"), HTTP_POST, getAccesspointfromApp);
}
// Manage not found URL
void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}
void getApFromRestApi() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP("ConnectyLight", "ConnectyLight");
  // Set your Static IP address
  IPAddress local_IP(192, 168, 1, 200);
  // Set your Gateway IP address
  IPAddress gateway(192, 168, 1, 1);

  IPAddress subnet(255, 255, 0, 0);
// Activate mDNS this is used to be able to connect to the server
  // with local DNS hostmane esp8266.local
  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }
  if (!WiFi.softAPConfig(local_IP, gateway, subnet)) {
    Serial.println("WIFI_AP Failed to configure");
  }
  // Set server routing
  restServerRouting();
  // Set not found response
  server.onNotFound(handleNotFound);
  // Start server
  server.begin();
  Serial.println("HTTP server started");
}
void setup_wifi() {
  delay(10);
  
  Serial.println("disconnect");
  WiFi.disconnect();
  delay(1000);
  Serial.print("\nConnecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  int i = 0;
  while (WiFi.status() != WL_CONNECTED && i < 61 ) {
    delay(500);
    Serial.println(i);
    if (i == 60 ) {
      Serial.println("connection failed opening AP");
      gotAccessPointData=false;
      connectedToConnecty =false;
      getApFromRestApi();
      
    }
    i++;
  }
  delay(3000);
  if (WiFi.status() == WL_CONNECTED) {

    randomSeed(micros());
    Serial.println("\nWiFi connected\nIP address: ");
    Serial.println(WiFi.localIP());
    Serial.println(WiFi.SSID());
    connectedToConnecty =true;
  } else {
    Serial.println("WiFi not connected");
    Serial.println(WiFi.SSID());
    connectedToConnecty =false;
  }
  
}
void IRAM_ATTR isr() {
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  // If interrupts come faster than 200ms, assume it's a bounce and ignore
  if (interrupt_time - last_interrupt_time > 200) 
  {
  Serial.println("User pressed the button");
  Serial.println("Button state changed");
  LightSwitchState = ! LightSwitchState;
  RelayState = ! RelayState;
  digitalWrite(RelayPin,RelayState);
  // allocate the memory for the document
  const size_t CAPACITY = JSON_OBJECT_SIZE(4);
  StaticJsonDocument<CAPACITY> doc;
  // create an object
  JsonObject object = doc.to<JsonObject>();
  object["type"] = "LightSwitch";
  object["state"] = RelayState;
  object["UUID"] = UUID;
  object["login"] = "world";
  object["password"] = "world";

  // serialize the object and send the result to payload
  String payload="";
  serializeJson(doc, payload);
  publishMessage(UUID.c_str() ,payload,sizeof(payload));
  }
  last_interrupt_time = interrupt_time;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("hello world");
  while (!Serial) delay(1);
  //declare pins
  pinMode(LightSwitchPin, INPUT_PULLUP);
  pinMode(RelayPin, OUTPUT);
  digitalWrite(RelayPin, digitalRead(LightSwitchPin));
  LightSwitchState = digitalRead(LightSwitchPin);
  RelayState = digitalRead(LightSwitchPin);
  digitalWrite(RelayPin,RelayState);
  //declare interrupt for button
  attachInterrupt(LightSwitchPin, isr, CHANGE);
  delay(10);
  setup_wifi();
 

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}


void loop() {
  if(! connectedToConnecty)
  {
    server.handleClient();
  } else {
  // put your main code here, to run repeatedly:
    delay(500);
    if (!client.connected()) reconnect(); // check if client is connected
    client.loop();
  }
  attachInterrupt(LightSwitchPin, isr, CHANGE);
  
  
}