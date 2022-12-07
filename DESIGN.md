# Motivation

The motivation behind this project came from a desire to automate blinds that was hindered by a relatively costly barrier to entry. Many commercial products on the market ranged anywhere from $75 to $750 per set of blinds. While DIY smart blinds are not a novel concept, many tutorials and blog posts focused on cord tilt blinds while I envisioned a simple motor to turn a tilt wand to change the shade level.

A CS50 final project was the perfect excuse to get started with  Arduino as well as diving deeper into Home Assistant which has presented a steep learning curve up until now.

# Decisions

## Arduino

Using an Arduino-based microcontroller seemed like a logical choice as it was significantly cheaper compared to RaspberryPis which was the leading alternative. During the research phase of this project, I also learned that there are countless other projects using ESP8266 boards specifically which allowed to narrow my focus. 

### mqtt_stepper.ino

```cpp
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
```

## MQTT

MQTT (Message Queuing Telemetry Transport) is a lightweight communication protocol that is used for IoT (Internet of Things) communication. It is designed to be simple and efficient, making it ideal for low-power and low-bandwidth devices, such as sensors and actuators. MQTT is a publish/subscribe protocol, which means that devices can both publish data to a broker (server) and subscribe to data from the broker. This allows for efficient communication between devices, as they only receive the data they are interested in.

## Home Assistant

Home Assistant is an open-source home automation platform that allows users to control and automate various devices in their home. Home Assistant has a large and active community of users who contribute to the platform by creating custom integrations and add-ons for a wide range of devices and services. While I’ve been using Home Assistant prior to this project, its a logical choice to tie in a MQTT broker a other IoT devices. 

## YAML

YAML (Yet Another Markup Language) is a human-readable data serialization language that is commonly used for configuration files, but can also be used in data storage and communication. It is designed to be easy to read and write, and is therefore often used as an alternative to JSON (JavaScript Object Notation) or XML (eXtensible Markup Language). I didn’t  have any choice but to use YAML to add custom MQTT devices. Some users in the Home Assistant community begrudge the YAML editing aspect.  

### configuration.yaml

```yaml
# Configure a default setup of Home Assistant (frontend, api, etc)
default_config:

# Text to speech
tts:
  - platform: google_translate

automation: !include automations.yaml
script: !include scripts.yaml
scene: !include scenes.yaml

# start custom configuration 
mqtt:
  sensor:
    - unique_id: "blinds01_state" # this must be unique in order to enable it in the dashboard
      name: "Blinds01 State"
      state_topic: "blinds_node/position"
  switch:
    - unique_id: "blinds01_switch" # this must be unique in order to enable it in the dashboard
      name: "Blinds01 Switch"
      state_topic: "blinds_node/position"
      command_topic: "blinds_node/input"
      payload_on: "0" # this paylod opens the blinds 
      payload_off: "1" # this payload closes the blinds 
      state_on: "OPENED" # looks at topic 'blinds_node/position' and if it is 'OPENED', set state to on
      state_off: "CLOSED" # looks at topic 'blinds_node/position' and if it is 'CLOSED', set state to off
      qos: 1 # keeps sending MQTT payload until it acknolwedged 
      retain: true # enables home assistant to know that the blinds are still open/closed 
```

## Stepper Motors vs DC Motors

2 common Arduino project motors are DC and stepper motors. Stepper motors move in discrete steps, allowing for precise control of the output shaft's position and speed. On the other hand, DC motors use direct current (DC) to generate rotational motion and are typically less precise than stepper motors. Stepper motors are commonly used in applications where precise positioning is required, such as 3D printers and CNC machines. DC motors are commonly used in applications where speed and cost are more important than precise positioning, such as electric vehicles and fans. Since precision was more key for this project, stepper motors were an obvious choice. 

## 3D Printing

While this project would most likely still be possible without the use of a 3D printer, it has definitely made it significantly easier. Being able to start with initial CAD design and then iterating on it over the course of a week is invaluable. 

# Bill of Materials

| Item | Price | Quantity  | Price / Per | URL |
| --- | --- | --- | --- | --- |
| Kuman Stepper Motor 28BYJ-48 ULN2003 5V Stepper Motor + ULN2003 Driver Board | 13.99 | 5 | 2.798 | https://smile.amazon.com/dp/B01IP7IOGQ?psc=1 |
| HiLetgo 2pcs ESP8266 NodeMCU | 12.69 | 2 | 6.345 | https://smile.amazon.com/dp/B010N1SPRK?psc=1 |
| Female Barrel Power Jack to 2 Pin/Way | 7.39 | 20 | 0.3695 | https://www.amazon.com/dp/B07LFRDSB7?psc=1&ref=ppx_yo2ov_dt_b_product_details |
| Qunqi Buck Converter | 8.59 | 5 | 1.718 | https://www.amazon.com/dp/B014Y3OT6Y?psc=1&ref=ppx_yo2ov_dt_b_product_details |
| Songhe Buck Converter | 12.88 | 5 | 2.576 | https://www.amazon.com/LM2596-Converter-4-0-40V-1-25-37V-Voltmeter（2pcs）/dp/B085WC5G8N/ref=sr_1_3?crid=2F85HQL6MKYPP&keywords=buck+converter&qid=1670350081&sprefix=buck+converter%2Caps%2C86&sr=8-3 |

As configured, the total price per controller is $12.38. While this is significantly cheaper than any commercial product, there are many costs not considered including the many hours development as well as assembly and setup time. Also not included are many of the items I already owned including a 3D printer, filament, Home Assistant VM, and miscellaneous other tools. 

The inclusion of buck converters is due the fact that my soldering iron failed on me midway through the project so I was not able to use the cheaper Qunqi one. On one hand, it made this project easier as it doesn’t include any soldering. However, I was forced to use a larger buck converted that I already had which caused the case to be larger than initially anticipated. 

# Limitations and Next Steps

As with all projects, the deadline for this drew faster than expected so I wasn’t able to achieve all my goals. In no particular order, here is a list of features I have not yet implemented:

- open blinds to a certain percentage
- make controller box battery powered
- add a solar panel to charge the battery
- miniaturize the case
- design a better closing mechanism for the case to prevent warping
- remove LEDs / mitigate light leakage
- miniaturize case so that it can fit in the OEM blinds housing
- make it easier to adopter multiple controllers
- enable support for Google Assistant / Alexa
- optimize for power efficiency

I look forward to making this project available to public to see what improvements can be made and how it can help others automate their homes. 

# Acknowledgments

The following resources were invaluable in researching and studying for this project: 

[https://github.com/thehookup/Motorized_MQTT_Blinds](https://github.com/thehookup/Motorized_MQTT_Blinds)

 [https://hackaday.io/project/183279-accelstepper-the-missing-manual](https://hackaday.io/project/183279-accelstepper-the-missing-manual)

[https://randomnerdtutorials.com/esp8266-nodemcu-stepper-motor-28byj-48-uln2003/](https://randomnerdtutorials.com/esp8266-nodemcu-stepper-motor-28byj-48-uln2003/)

[https://youtu.be/dqTn-Gk4Qeo](https://youtu.be/dqTn-Gk4Qeo)
