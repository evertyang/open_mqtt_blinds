#include <ESP8266WiFi.h> // enables wifi connections https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/readme.html
#include <PubSubClient.h> // enables MQTT protocol https://pubsubclient.knolleary.net
#include <AccelStepper.h> // library for better than stock stepper control http://www.airspayce.com/mikem/arduino/AccelStepper/

// ####### START USER CONFIG ######

#define WIFI_SSID "your_wifi_name"
#define WIFI_PASS "your_wifi_password"
#define USER_MQTT_URL "xxx.xxx.xxx.xxx"
#define USER_MQTT_PORT 1883
#define USER_MQTT_USERNAME "your_mqtt_username"
#define USER_MQTT_PASSWORD "your_mqtt_password"

// defines which pins on ESP will be used to connect with pins on stepper driver ULN2003
#define STEPPER_IN1 D5
#define STEPPER_IN2 D6
#define STEPPER_IN3 D7
#define STEPPER_IN4 D8

#define MSG_BUFFER_SIZE (50)

#define STEPS_PER_REV 2048 // specific to stepper motor 28BYJ-48 ULN2003 
#define REVS_TO_CLOSE 8    // this needs to be tuned by the user

// ###### END USER CONFIG ######

// declares all users defined variables 
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASS;
const char* mqtt_server = USER_MQTT_URL;
const int   mqtt_port = USER_MQTT_PORT;
const char* mqtt_user = USER_MQTT_USERNAME;
const char* mqtt_pass = USER_MQTT_PASSWORD;

const int IN1 = STEPPER_IN1;
const int IN2 = STEPPER_IN2;
const int IN3 = STEPPER_IN3;
const int IN4 = STEPPER_IN4;

int stepsPerRev = STEPS_PER_REV;
int revsToClose = REVS_TO_CLOSE;
const int stepsToPhase = stepsPerRev * stepsToClose; // how many steps it will take to close blinds 

AccelStepper myStepper(AccelStepper::FULL4WIRE, IN1, IN3, IN2, IN4); // defining stepper 
WiFiClient espClient; // defining wificlient object
PubSubClient client(espClient); // mqtt client object 
unsigned long lastMsg = 0;

char msg[MSG_BUFFER_SIZE];
int value = 0;

// connects board to wifi 
void setup_wifi() 
{
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print("."); // waiting...
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP()); // output local IP address to serial 
}

// waits for MQTT payload 
void callback(char* topic, byte* payload, unsigned int length) 
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) 
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // check payload
  if ((char)payload[0] == '1') 
  {
    digitalWrite(BUILTIN_LED, LOW);  // turn the LED on and close blinds if message is '1'
    myStepper.moveTo(-stepsToPhase);
    client.publish("blinds_node/position", "CLOSED");
  } 
  else 
  {
    digitalWrite(BUILTIN_LED, HIGH);  // turn the LED and open blinds if message is '0'
    myStepper.moveTo(0);
    client.publish("blinds_node/position", "OPENED");
  }
}

// conncets to MQTT broker and subscribes to topic 
void reconnect() 
{
  // loop until reconnected
  while (!client.connected()) 
  {
    Serial.print("Attempting MQTT connection...");
    // create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // attempt to connect
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) 
    {
      Serial.println("connected");
      // once connected, publish an announcement...
      client.publish("blinds_node/log", "hello world");
      // ... and resubscribe
      client.subscribe("blinds_node/input");
    } 
    else 
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() 
{
  myStepper.setMaxSpeed(600.0); // defines stepper max speed; it is capable of a speed of 750 it tends to skip 
  myStepper.setAcceleration(50.0); // defines max acceleration 
  pinMode(BUILTIN_LED, OUTPUT);  // initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() 
{
  if (!client.connected()) // tries to reconnect if not connected 
  { 
    reconnect();
  }
  client.loop();
  myStepper.run();
}
