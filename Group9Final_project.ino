/*
 * Project AutomatedBlinds
 * Description: Control window blinds using a servo motor and a photoresistor, integrated with AWS IoT Core using MQTT.
 * Author:
 * Date:
 */

#include "application.h"
#include "MQTT-TLS.h"
#include "keys.h"

SerialLogHandler logHandler;

void callback(char *topic, byte *payload, unsigned int length);

const char amazonIoTRootCaPem[] = AMAZON_IOT_ROOT_CA_PEM;
const char clientKeyCrtPem[] = CELINT_KEY_CRT_PEM;
const char clientKeyPem[] = CELINT_KEY_PEM;


MQTT client("a16zeivztmr384-ats.iot.us-east-1.amazonaws.com", 8883, callback);

Servo myservo;
int servoPin = A3;
int pos = 0;
int photoresistorPin = A5;
int analogValue;
bool blindsOpen = false;
bool manualMode = false;  // Track whether the system is in manual mode

void callback(char *topic, byte *payload, unsigned int length) 
{
  Log.info("Received Data on topic %s", topic);
  char p[length + 1];
  memcpy(p, payload, length);
  p[length] = NULL;
  String message(p);

  if (message.equals("OPEN")) {
    manualMode = true;
    openBlinds();
  } else if (message.equals("CLOSE")) {
    manualMode = true;
    closeBlinds();
  } else if (message.equals("MODE")) {
    manualMode = !manualMode;
    client.publish("blindsStatus", manualMode ? "Manual Mode" : "Automatic Mode");
  }
}

void openBlinds() {
  if (!blindsOpen) {
    for (pos = 0; pos <= 180; pos += 1) {
      myservo.write(pos);
      delay(15);
    }
    blindsOpen = true;
    client.publish("blindsStatus", "Blinds Open");
  }
}

void closeBlinds() {
  if (blindsOpen) {
    for (pos = 180; pos >= 0; pos -= 1) {
      myservo.write(pos);
      delay(15);
    }
    blindsOpen = false;
    client.publish("blindsStatus", "Blinds Closed");
  }
}

void setup() {
  myservo.attach(servoPin);
  pinMode(photoresistorPin, INPUT);

  // Enable TLS and set Root CA pem, private key file.
  client.enableTls(amazonIoTRootCaPem, sizeof(amazonIoTRootCaPem),
                   clientKeyCrtPem, sizeof(clientKeyCrtPem),
                   clientKeyPem, sizeof(clientKeyPem));

  // Connect to the server
  bool status = client.connect("aws_" + String(Time.now()));
  if (status) {
    Log.info("Connection is completed");
    client.subscribe("blindsCommand");
  } else {
    Log.info("Connection is not completed");
  }
}

void loop() {
  if (client.isConnected()) {
    client.loop();

    if (!manualMode) {  // Only read photoresistor in automatic mode
      analogValue = analogRead(photoresistorPin);
      if (analogValue < 10) {
        openBlinds();
      } else {
        closeBlinds();
      }
    }
  } else {
    // Attempt to reconnect
    bool status = client.connect("aws_" + String(Time.now()));
    if (status) {
      client.subscribe("blindsCommand");
    }
  }
  delay(200);
}