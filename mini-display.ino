#include <SimpleTimer.h> //https://github.com/jfturcot/SimpleTimer
#include <PubSubClient.h> //https://github.com/knolleary/pubsubclient
#include <ESP8266WiFi.h> //https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WiFi
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h> //https://github.com/esp8266/Arduino/tree/master/libraries/ArduinoOTA
#include <Adafruit_GFX.h> //https://github.com/adafruit/Adafruit-GFX-Library
#include <Adafruit_SSD1306.h> //https://github.com/adafruit/Adafruit_SSD1306

#define OLED_RESET 0  // GPIO0
Adafruit_SSD1306 OLED(OLED_RESET);

//USER CONFIGURED SECTION START//
const char* hostName = "MiniDisplay";
const char* ssid = "xxxxx";
const char* password = "xxxxx";
const char* mqtt_server = "192.168.xx.xx";
const int mqtt_port = 1883;
const char *mqtt_user = "hass";
const char *mqtt_pass = "xxxxxx";
const char *mqtt_client_name = "MessageDisplay"; // Client connections can't have the same connection name
const char *mqtt_message_topic = "display/message";
const char *mqtt_size_topic = "display/size";
const char *mqtt_state_topic = "state/display";
const char *mqtt_state_payload = "Message Displayed";
const char *mqtt_status_topic = "checkIn/display";
//USER CONFIGURED SECTION END//

WiFiClient espClient;
PubSubClient client(espClient);
SimpleTimer timer;
String currentMessage;

bool boot = true;

//Functions

void setup_wifi() {
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

void displayMessage(String message){
  OLED.begin();
  OLED.clearDisplay();
  OLED.display();
 
  //Add stuff into the 'display buffer'
  //OLED.setTextWrap(false);
  //OLED.setTextSize(1);
  OLED.setTextColor(WHITE);
  OLED.setCursor(1,1);
  OLED.print(message);
 
  OLED.display(); //output 'display buffer' to screen  
}

void reconnect() 
{
  // Loop until we're reconnected
  int retries = 0;
  while (!client.connected()) {
    if(retries < 15)
    {
        Serial.print("Attempting MQTT connection...");
      // Attempt to connect
      if (client.connect(mqtt_client_name, mqtt_user, mqtt_pass)) 
      {
        Serial.println("connected");
        // Once connected, publish an announcement...
        if(boot == true)
        {
          client.publish(mqtt_status_topic, "Rebooted");
          boot = false;
        }
        if(boot == false)
        {
          client.publish(mqtt_status_topic, "Reconnected");
          client.publish(mqtt_status_topic, "Online");
        }
        // ... and resubscribe
        client.subscribe(mqtt_message_topic);
        client.subscribe(mqtt_size_topic);
      } 
      else 
      {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
        retries++;
        // Wait 5 seconds before retrying
        delay(5000);
      }
    }
    if(retries > 14)
    {
    ESP.restart();
    }
  }
}

void onMessageCallback(char* topic, byte* payload, unsigned int length) 
{
  Serial.print("Message arrived [");
  String newTopic = topic;
  Serial.print(topic);
  Serial.print("] ");
  payload[length] = '\0';
  String newPayload = String((char *)payload);
  Serial.println(newPayload);
  Serial.println();
  if (newTopic == mqtt_message_topic) 
  {
      currentMessage = newPayload;
      displayMessage(currentMessage);
  }
  if(newTopic == mqtt_size_topic)
  {
    OLED.display();
    OLED.setTextSize(newPayload.toInt());
    displayMessage(currentMessage);
  }
}



void checkIn()
{
  client.publish(mqtt_status_topic, "Online");
}

//Run once setup
void setup() {
  Serial.begin(115200);

  
  setup_wifi();
  displayMessage("Test Message");
  
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(onMessageCallback);
  
  ArduinoOTA.setHostname(hostName);
  ArduinoOTA.begin(); 
  
  timer.setInterval(120000, checkIn);
}

void loop() 
{
  if (!client.connected()) 
  {
    reconnect();
  }
  client.loop();
  ArduinoOTA.handle();
  timer.run();
}
