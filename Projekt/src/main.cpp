#include <SPI.h>
#include <WiFiNINA.h>
#include <MQTT.h>
#include <Servo.h>

#include "arduino_secrets.h"
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID; // your network SSID (name)
char pass[] = SECRET_PASS; // your network password (use for WPA, or use as key for WEP)

Servo servo1;
bool ledOn = false;
int servoRotate = 0;

WiFiClient net;
MQTTClient client;

unsigned long lastMillis = 0;

void connect()
{
	Serial.print("checking wifi...");
	while (WiFi.status() != WL_CONNECTED)
	{
		Serial.print(".");
		delay(1000);
	}

	Serial.print("\nconnecting...");
	while (!client.connect("arduino", "eastfish922", "HUhQbfYVihz4btkB"))
	{
		Serial.print(".");
		delay(1000);
	}

	Serial.println("\nconnected!");

	client.subscribe("hello");
	// client.unsubscribe("/hello");
}

void messageReceived(String &topic, String &payload)
{
	Serial.println("incoming: " + topic + " - " + payload);
	if (payload == "led")
	{
		if (ledOn)
		{
			ledOn = false;
			digitalWrite(5, LOW);
		}
		else
		{
			ledOn = true;
			digitalWrite(5, HIGH);
		}
	}
	if (payload == "servo")
	{
		servoRotate += 10;
		if (servoRotate == 180) {
			servoRotate = 0;
		}
		servo1.write(servoRotate);
	}

	// Note: Do not use the client in the callback to publish, subscribe or
	// unsubscribe as it may cause deadlocks when other things arrive while
	// sending and receiving acknowledgments. Instead, change a global variable,
	// or push to a queue and handle it in the loop after calling `client.loop()`.
}

void setup()
{
	Serial.begin(9600);
	WiFi.begin(ssid, pass);

	// Note: Local domain names (e.g. "Computer.local" on OSX) are not supported
	// by Arduino. You need to set the IP address directly.
	client.begin("eastfish922.cloud.shiftr.io", net);
	client.onMessage(messageReceived);

	pinMode(5, OUTPUT);
	servo1.attach(4);

	connect();
}

void loop()
{
	client.loop();

	if (!client.connected())
	{
		connect();
	}
}