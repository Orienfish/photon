/*
 * Project: Establish MQTT connection with RPi and send local data
 * Suppose necessary files are alreay written on the SD card:
 *     extracted_subject1.txt
 * Author: Xiaofan Yu
 * Date: 10/13/2018
 */
#include "application.h"
#include "MQTT.h"
#include "sd-card-library-photon-compat.h"

// #define DEBUG_LOCAL_CONNECTOR // DEBUG mode
#ifdef DEBUG_LOCAL_CONNECTOR
 #define DEBUG_PRINTLN(x)  Serial.println(x)
 #define DEBUG_PRINT(x) Serial.print(x)
#else
 #define DEBUG_PRINTLN(x)
 #define DEBUG_PRINT(x)
#endif

const uint16_t sample_rate[] = {200, 220, 240, 260, 280, 300, 320, 340, 360, 380, 400};
const uint16_t bw[] = {200, 700, 5000};
const uint8_t READ_CNT_PER_LINE = 12; // number of floats read from each line
const uint16_t MAX_BATCH_LENGTH = 9602; // maximum length of sending batch

/* global variables for MQTT */
void callback(char* topic, byte* payload, unsigned int length);

byte server[] = { 137,110,160,230 }; // specify the ip address of RPi
MQTT client(server, 1883, 60, callback, MAX_BATCH_LENGTH+100); // ip, port, keepalive, callback, maxpacketsize=

/* glabal variables for SD card */
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

// specify the file storing data
const char filename[] = "data.txt";

// string and float to hold float input
String inString = "";
float inFloat;

// should pay attention to the length of batch data string
uint8_t batch_data[MAX_BATCH_LENGTH]; // can not use string cuz it is too long
uint32_t len_of_batch;

/*
 * Receive Message - Not used here
 */
void callback(char* topic, byte* payload, unsigned int length) {
}

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
 * batch_append - append a new float data to the sending batch
 * Return - 0 fail, exceed maximum length
 *			1 success
 */
uint8_t batch_append(float data) {
	if (len_of_batch >= MAX_BATCH_LENGTH-2) // no placy to append, return failure
		return 0;
	uint8_t *data_byte = (uint8_t *)&data; // convert the pointer
	batch_data[len_of_batch++] = *(data_byte); // append lower byte to send batch
	batch_data[len_of_batch++] = *(data_byte + 1); // append higher byte to send batch
	return 1;
}

/*
 * read_line - read one line from myFile, each line read first READ_CNT_PER_LINE floats in 36 floats
 */
void read_line() {
	inString = ""; // clear string
	for (int readcnt = 0; readcnt < READ_CNT_PER_LINE;) { // read READ_CNT_PER_LINE float from one line
		char byte = myFile.read();
		if (byte == ' ') {
			// a float is finished, export it
			float inFloat = inString.toFloat();

			DEBUG_PRINT(inFloat);
			DEBUG_PRINT(" ");

			batch_append(inFloat); // append to the sending batch
			readcnt++; // add one on the reading cnt
			inString = ""; // clear inString
		}
		else if (byte == '\n') // accidentally meet the end of line, return
		 	return;
		inString += byte; // read one byte
	}
	// if read until end of line, move on
	while (myFile.available()) {
		char byte = myFile.read();
		if (byte == '\n')
			break;
	}
	return;
}

/*
 * read_float_data - read sample_rate lines from myFile and append the data to sending batch
 * parameter - cur_sr current sample rate
 */
void read_float_data(uint16_t cur_sr) {
	myFile = SD.open(filename); // open the file and read from start
	if (myFile) {
		while (myFile.available()) { // read until EOF
			for (int j = 0; j < cur_sr; ++j) { // read sample_rate lines from myFile
				read_line();
				DEBUG_PRINTLN(j);
			}
		}
		myFile.close(); // close the file after one sampling round
	}
	else {
		// if the file didn't open, print an error:
		Serial.println("error opening file");
	}
	return;
}

/*
 * setup
 */
void setup() {
	Serial.begin(115200);
	init_card(); // init sd card
	// connect to the RPi
	// client_id, user, passwd, willTopic, willQoS, willRetain, willMessage, cleanSession, version?
    client.connect("photon", "xiaofan", "0808", 0, client.QOS2, 0, 0, true, client.MQTT_V311);
	Serial.println("Connect!");

	//for (int sample_index = 0; sample_index < 11; ++sample_index) {
		len_of_batch = 0; // clear sending batch
		read_float_data(sample_rate[0]); // read one line by one line
		// publish
		if (client.isConnected()) {
			// topic, payload, plength, retain, qos, dup, messageid
			client.publish("house.hand", batch_data, len_of_batch, false, client.QOS2, false, NULL);
			while (!client.loop_QoS2()); // block and wait for pub done

			Serial.print(len_of_batch);
			Serial.print(" ");
			Serial.println("Publish successfully");
		}
	//}
}

/*
 * loop
 */
void loop() {
}
