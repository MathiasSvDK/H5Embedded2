#pragma region Includes
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFiNINA.h>
#include <MQTT.h>
#include <Servo.h>
#include <Keypad.h>
#include "arduino_secrets.h"
#include "ThingSpeak.h"
#include "DHT.h"
#include <MFRC522.h>
#pragma endregion

#pragma region Keypad
const int ROW_NUM = 4;	  //four rows
const int COLUMN_NUM = 3; //three columns
char keys[ROW_NUM][COLUMN_NUM] = {
	{'1', '2', '3'},
	{'4', '5', '6'},
	{'7', '8', '9'},
	{'*', '0', '#'}};
String menu[] = {
	"1. Se temperatur",
	"2. Drej motor"};
byte pin_rows[ROW_NUM] = {9, 8, 7, 6};	 //connect to the row pinouts of the keypad
byte pin_column[COLUMN_NUM] = {5, 4, 3}; //connect to the column pinouts of the keypad
Keypad keypad = Keypad(makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM);
#pragma endregion

#pragma region Servo
Servo myservo;
#pragma endregion

#pragma region Variables
//Pin setups
int servoPin = 10;
#define DHTPIN 14 // Digital pin connected to the DHT sensor

//Is the device locked?
bool locked = true;

//Objects
WiFiClient net;
MQTTClient client;

unsigned long myChannelNumber = 1556887;
const char *myReadAPIKey = "1F25IZ7UM265V5ES";

///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID; // your network SSID (name)
char pass[] = SECRET_PASS; // your network password (use for WPA, or use as key for WEP)

#define DHTTYPE DHT11 // DHT 22  (AM2302), AM2321
DHT dht(DHTPIN, DHTTYPE);

//Screen
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET 4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//Dims
#define RST_PIN   5     // Configurable, see typical pin layout above
#define SS_PIN    53   // Configurable, see typical pin layout above


MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance

/* Set your new UID here! */
#define NEW_UID                \
	{                          \
		0xDE, 0xAD, 0xBE, 0xEF \
	}

MFRC522::MIFARE_Key key;

#pragma endregion

void Menu()
{
	//Locked, man skal bruge RFID
	for (String x : menu)
	{
		Serial.println(x);
	}
}

void messageReceived(String &topic, String &payload)
{
	Serial.println("incoming: " + topic + " - " + payload);
	if (payload == "temp")
	{
	}
}

void connect()
{
	Serial.print("checking wifi...");
	while (WiFi.status() != WL_CONNECTED)
	{
		Serial.print(".");
		delay(1000);
	}

	Serial.print("\nconnecting...");
	while (!client.connect("DCQJMTYeNCMACzYDAiQ9NRM", "DCQJMTYeNCMACzYDAiQ9NRM", "P1JevNhGWtcSktA2xMWVibQN"))
	{
		Serial.print(".");
		delay(1000);
	}

	Serial.println("\nconnected!");

	// client.unsubscribe("/hello");
}

void setup()
{
	Serial.begin(9600);
	WiFi.begin(ssid, pass);
	client.begin("mqtt.thingspeak.com", net);
	client.onMessage(messageReceived);
	dht.begin();
	SPI.begin();
	mfrc522.PCD_Init(); // Init MFRC522 card

	myservo.attach(servoPin); // attaches the servo on pin 9 to the servo object

	connect();
	ThingSpeak.begin(net); // Initialize ThingSpeak

	// SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
	if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
	{ // Address for 128x64
		Serial.println(F("SSD1306 allocation failed"));
		for (;;)
			; // Don't proceed, loop forever
	}
}

void loop()
{

	client.loop();

	if (locked)
	{
#pragma region Locked
		while (true)
		{
			// Look for new cards, and select one if present
			if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial())
			{
				delay(50);
			}
			else
			{
				break;
			}
		}

		// Now a card is selected. The UID and SAK is in mfrc522.uid.

		// Dump UID
		Serial.print(F("Card UID:"));
		for (byte i = 0; i < mfrc522.uid.size; i++)
		{
			Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
			Serial.print(mfrc522.uid.uidByte[i], HEX);
		}
		Serial.println();

		// Dump PICC type
		//  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
		//  Serial.print(F("PICC type: "));
		//  Serial.print(mfrc522.PICC_GetTypeName(piccType));
		//  Serial.print(F(" (SAK "));
		//  Serial.print(mfrc522.uid.sak);
		//  Serial.print(")\r\n");
		//  if (  piccType != MFRC522::PICC_TYPE_MIFARE_MINI
		//    &&  piccType != MFRC522::PICC_TYPE_MIFARE_1K
		//    &&  piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
		//    Serial.println(F("This sample only works with MIFARE Classic cards."));
		//    return;
		//  }

		// Set new UID
		byte newUid[] = NEW_UID;
		if (mfrc522.MIFARE_SetUid(newUid, (byte)4, true))
		{
			Serial.println(F("Wrote new UID to card."));
		}

		// Halt PICC and re-select it so DumpToSerial doesn't get confused
		mfrc522.PICC_HaltA();
		if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial())
		{
			return;
		}

		// Dump the new memory contents
		Serial.println(F("New UID and contents:"));
		mfrc522.PICC_DumpToSerial(&(mfrc522.uid));

		delay(2000);
#pragma endregion
	}
	else
	{
		char key = keypad.getKey();
		Serial.println(key);
		if (key)
		{
#pragma region Unlocked
			if (key == '1')
			{
				Serial.println("Clicked menu 1");

				while (key == '1')
				{
					int sensorValue = analogRead(A6);
					int val = sensorValue / 5.6895;

					myservo.write(val);
					delay(1);
				}
			}
			if (key == '2')
			{

				// Reading temperature or humidity takes about 250 milliseconds!
				// Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
				float h = dht.readHumidity();
				// Read temperature as Celsius (the default)
				float t = dht.readTemperature();
				// Check if any reads failed and exit early (to try again).
				if (isnan(h) || isnan(t))
				{
					Serial.println(F("Failed to read from DHT sensor!"));
					return;
				}

				client.publish("channels/15568871/publish/fields/field1/8CLQKNC8PYG7FND9", String(t));
				float temperatureInF = ThingSpeak.readFloatField(myChannelNumber, 1, myReadAPIKey);
				Serial.println(temperatureInF);

				display.clearDisplay();

				display.setTextSize(1);		 // Normal 1:1 pixel scale
				display.setTextColor(WHITE); // Draw white text
				display.setCursor(0, 0);	 // Start at top-left corner
				display.print(F("Stue temp: "));
				display.print(t);
				display.println(F(""));
				display.print(F("Soveroom temp: "));
				display.print(temperatureInF);

				display.display();
			}
		}
#pragma endregion
	}
}