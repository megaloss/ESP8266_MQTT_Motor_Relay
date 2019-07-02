#include <EEPROM.h>

/*
  Basic ESP8266 MQTT example

  This sketch demonstrates the capabilities of the pubsub library in combination
  with the ESP8266 board/library.

  It connects to an MQTT server then:
  - publishes "hello world" to the topic "outTopic" every two seconds
  - subscribes to the topic "inTopic", printing out any messages
    it receives. NB - it assumes the received payloads are strings not binary
  - If the first character of the topic "inTopic" is an 1, switch ON the ESP Led,
    else switch it off

  It will reconnect to the server if the connection is lost using a blocking
  reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
  achieve the same result without blocking the main loop.

  To install the ESP8266 board, (using Arduino 1.6.4+):
  - Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
       http://arduino.esp8266.com/stable/package_esp8266com_index.json
  - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
  - Select your ESP8266 in "Tools -> Board"

*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#define MQTT_HOST   "192.168.0.15"
#define MQTT_PORT   1883
#define MQTT_TOPIC  "/homeassistant/staircase/window/motor/status"
#define MQTT_IN_TOPIC "/homeassistant/staircase/window/motor/set"
#define MQTT_USER "ha1"
#define MQTT_PASS "ha1"
#define esp_id "ESP-Motor-1"
#define SPR  300 //seconds per rotation, defines delay during opening and closing


// Update these with values suitable for your network.

const char* ssid = "****";
const char* password = "****";
const char* mqtt_server = "****";

WiFiClient espClient;
PubSubClient client(espClient);

long lastMsg = 0;
long lastStatus = 0;
char msg[50];
boolean motor_on = 0;
boolean opening = 0;
int status;
unsigned long motor_timer;
unsigned long motor_timeout;

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

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}
/*void window_open (int val)
  {
  if (val > 100) {
    val = 100;
  }
  digitalWrite(D3, LOW);   // DA LOW
  digitalWrite(D1, HIGH);   // PWM_A HIGH
  Serial.print("Opening...");
  //motor_on = 1;
  //  delay(val * SPR);
  //  digitalWrite(D1, LOW);   // PWM_A LOW
  //  Serial.println("...Done");

  //  status = status + val;
  }
*/
void window_close (int val)
{
  digitalWrite(D3, HIGH);   // DA HIGH
  digitalWrite(D1, LOW);   // PWM_A HIGH
  Serial.print("Closing...");

  delay(val * SPR);
  //  Serial.println("...Done");
  //motor_on = 1;
  digitalWrite(D1, LOW);   // PWM_A LOW
  //  status = status - val;

  // wait
}
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  int payload_value = atoi((const char *)payload);


  Serial.print (payload_value);



  if ((payload_value) >= status ) { // open more
    digitalWrite(D3, LOW);   // DA LOW
    digitalWrite(D1, HIGH);   // PWM_A HIGH
    Serial.print("Opening to...");
    motor_on = 1;
    opening = 1;
    motor_timeout = (payload_value - status) * SPR;
    Serial.println (payload_value - status);
    //window_open(payload_value - status);
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  }
  else if (payload_value == 0) {         // completely close
    digitalWrite(D3, HIGH);   // DA HIGH
    digitalWrite(D1, LOW);   // PWM_A HIGH
    Serial.print("Closing to...");
    motor_on = 1;
    opening = 0;
    motor_timeout = status  * SPR; // so it is closed for sure
    Serial.println (status - payload_value);

    //status = 0;
  }
  else {
    digitalWrite(D3, HIGH);   // DA HIGH
    digitalWrite(D1, LOW);   // PWM_A HIGH
    Serial.print("Closing...");
    motor_on = 1;
    opening = 0;
    motor_timeout = (status - payload_value) * SPR; //close a value
    //window_close(status - payload_value);
    Serial.println (status - payload_value );


  }

  motor_timer = millis();
  //snprintf(msg, sizeof(msg), "%d", status);
  Serial.print("motor_timer reset, start moving ");

  Serial.println(motor_timeout);
  //client.publish(MQTT_TOPIC, msg);



}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(esp_id, MQTT_USER, MQTT_PASS, MQTT_TOPIC, 0, 1, "offline")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(MQTT_TOPIC, "Online");
      // ... and resubscribe
      client.subscribe(MQTT_IN_TOPIC);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  window_close(10);
  Serial.begin(115200);
  setup_wifi();
  client.setServer(MQTT_HOST, MQTT_PORT);

  client.setCallback(callback);
  // Motor initialisation
  pinMode(D1, OUTPUT); // инициализируем Pin как выход
  pinMode(D3, OUTPUT); // инициализируем Pin как выход
  digitalWrite(D3, LOW);   // DA HIGH
  digitalWrite(D1, LOW);   // PWM_A HIGH

  status = 0;


}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long unsigned now = millis();
  if (now - lastMsg > 60000) {      //every 1 min send status
    lastMsg = now;
    //snprintf (msg, 50, "I am ok #%ld", value);
    snprintf(msg, sizeof(msg), "%d", status);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish(MQTT_TOPIC, msg);
  }
  if (motor_on) {
    if (now - motor_timer >= motor_timeout) {   //if time is out stop motor
      digitalWrite(D1, LOW);       //stop motor
      digitalWrite(D3, LOW); 
      
      motor_on = 0;
    }
    if (now - lastStatus >= SPR) {    //every 1 sec update status on moving motor
      lastStatus = now;

      if (opening == 0) {
        status--;
        if (status < 0) {
          status = 0;
        }
      }
      else {
        status++;
        if (status >100) {
          status = 100;
        }

      }
      snprintf(msg, sizeof(msg), "%d", status);
      Serial.print("Publish message: ");
      Serial.println(msg);
      client.publish(MQTT_TOPIC, msg);  // send status of moving motor
    }

  }

}
