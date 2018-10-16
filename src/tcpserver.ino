/*
* Project: Write certain files to SD card through TCP connection.
* Author: Xiaofan Yu
* Date: 10/15/2018
*/
#include "application.h"
#include "sd-card-library-photon-compat.h"

// SYSTEM_MODE(MANUAL);
TCPServer server = TCPServer(5000);
TCPClient client;

/* global variables for SD card */
File myFile;

// set up variables using the SD utility library functions:
Sd2Card card;
SdVolume volume;
SdFile root;

// SOFTWARE SPI pin configuration - modify as required
// The default pins are the same as HARDWARE SPI
const uint8_t chipSelect = A2;    // Also used for HARDWARE SPI setup
const uint8_t mosiPin = A5;
const uint8_t misoPin = A4;
const uint8_t clockPin = A3;

// specify the file to write
const char filename[] = "data.txt";

/*
* init_card - init SD card, used most often in read and write
*/
void init_card() {
	// Initialize HARDWARE SPI with user defined chipSelect
	if (!SD.begin(chipSelect)) {
		Serial.println("error");
		return;
	}

	Serial.println("done");
}

/*
* check the validation of read byte, to get rid of interfere
*/
bool check_validation(char byte) {
	if (byte >= '0' && byte <= '9')
		return 1;
	if (byte == '.')
		return 1;
	if (byte == '<')
		return 1;
	if (byte == '-')
		return 1;
	if (byte == '\n' || byte == '\r' || byte == ' ')
		return 1;
	return 0;
}

void setup()
{
	WiFi.connect();
	// Make sure your Serial Terminal app is closed before powering your device
	Serial.begin(9600);

	Serial.println(WiFi.localIP());
	Serial.println(WiFi.subnetMask());
	Serial.println(WiFi.gatewayIP());
	Serial.println(WiFi.SSID());

	// start listening for clients
	server.begin();

	// init sd card
	init_card();

	// open the file. note that only one file can be open at a time,
	// so you have to close this one before opening another.
	// change the mode to write from start each time!
	myFile = SD.open(filename, O_WRITE | O_CREAT | O_TRUNC);

	if (myFile) {
		// check until a new client to our server
		while (1) {
			if (client.connected())
			{
				// we got a new client
				// find where the client's remote address
				IPAddress clientIP = client.remoteIP();
				// print the address to Serial
				Serial.println(clientIP);
				// start data transmission
				bool connected = 1;
				while (1) {
					while (!client.available()) // wait until the available byte
					    // if the client disconnected during waiting, quit loops
						if (!client.connected()) {
							connected = 0;
							break;
						}
					if (!connected) // if not connected, quit loop
						break;
					char c = client.read();
					// write to terminal
					Serial.write(c);
					if (check_validation(c)) {
						// write to SD card
						myFile.write(c);
						// echo all available bytes back to the client
						delay(5); // don't be that fast
						server.write(c);
					}
				}
				// close the file:
				Serial.println("Finish writing.");
				myFile.close();
				break; // end up the whole process
			}
			else {
				// if no client is yet connected, check for a new connection
				client = server.available();
				// Serial.println("waiting for client...");
			}
		}
	}
	else {
		// if the file didn't open, print an error:
		Serial.println("error opening txt");
	}
}

void loop()
{
}