#include <SPI.h>
#include <WiFiNINA.h>
#include <MQTT.h>
#include <Servo.h>
#include "DHT.h"

#define DHTPIN 7	  // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11 // DHT 22  (AM2302), AM2321
DHT dht(DHTPIN, DHTTYPE);

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
	while (!client.connect("Mh0qMzAcIg0WKAQNCRUSFQY", "Mh0qMzAcIg0WKAQNCRUSFQY", "RHw+ZHeqWAJIHBjG0B0l5Xfq"))
	{
		Serial.print(".");
		delay(1000);
	}

	Serial.println("\nconnected!");

	//client.subscribe("hello");
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
		if (servoRotate == 180)
		{
			servoRotate = 0;
		}
		servo1.write(servoRotate);
	}
	if (payload == "weather")
	{
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
	client.begin("mqtt.thingspeak.com", net);
	client.onMessage(messageReceived);

	pinMode(5, OUTPUT);
	servo1.attach(4);

	dht.begin();

	connect();
}

void loop()
{
	client.loop();

	if (!client.connected())
	{
		connect();
	}

	// Reading temperature or humidity takes about 250 milliseconds!
	// Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
	float h = dht.readHumidity();
	// Read temperature as Celsius (the default)
	float t = dht.readTemperature();
	// Read temperature as Fahrenheit (isFahrenheit = true)
	float f = dht.readTemperature(true);

	// Check if any reads failed and exit early (to try again).
	if (isnan(h) || isnan(t) || isnan(f))
	{
		Serial.println(F("Failed to read from DHT sensor!"));
		return;
	}

	// Compute heat index in Fahrenheit (the default)
	float hif = dht.computeHeatIndex(f, h);
	// Compute heat index in Celsius (isFahreheit = false)
	float hic = dht.computeHeatIndex(t, h, false);

	delay(5000);
	client.publish("channels/15568871/publish/fields/field1/8CLQKNC8PYG7FND9", String(t));
	client.publish("channels/15568871/publish/fields/field2/8CLQKNC8PYG7FND9", String(h));
}