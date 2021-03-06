/*
 * Project: Establish MQTT connection with RPi and send data after run xgboost
 * Suppose necessary files are alreay written on the SD card:
 *     data.txt (extracted_subject1.txt)
 *	   boost.txt
 * Author: Xiaofan Yu
 * Date: 10/23/2018
 */
#include "application.h"
#include "MQTT.h"
#include "bigint.h"
#include "sd-card-library-photon-compat.h"

// #define DEBUG_LOCAL_CONNECTOR // DEBUG mode
#ifdef DEBUG_LOCAL_CONNECTOR
 #define DEBUG_PRINTLN  Serial.println
 #define DEBUG_PRINT Serial.print
 #define DEBUG_PRINTF Serial.printf
#else
 #define DEBUG_PRINTLN
 #define DEBUG_PRINT
 #define DEBUG_PRINTF
#endif

// const variables in experiments
const int sample_rate[] = {200, 220, 240, 260, 280, 300, 320, 340, 360, 380, 400};
const int bw[] = {200, 700, 5000};
const uint8_t NUM_OF_SR = 11; // number of sameple rate choices
const uint8_t NUM_OF_BW = 3; // number of bandwidth choices
const uint8_t READ_CNT_PER_LINE = 12; // number of floats read from each line
const uint8_t MAX_COND_CNT = 100; // maximum number of conditions in each devices 
const uint16_t MAX_BATCH_LENGTH = 8002; // maximum length of sending batch
const uint8_t MAX_INFO_LENGTH = 100; // maximum length of sending info, e.g. bw and sample rate

// define all categories of topics
const char data_topic[] = "house.hand"; // for sending data
const char tmp_topic[] = "tmp"; // for sending tmp.txt
const char parameter_topic[] = "parameter"; // for sending bandwidth and sample rate
const char sync_topic[] = "sync"; // for sync, send current time

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

// specify the file storing data, extracted_subject1.txt
const char filename[] = "data.txt";
uint32_t position = 0; // used to remember the last reading position when open filename
// specify the file recording sleep time, tmp.txt
const char tmpname[] = "tmp.txt";
char write_buffer[20]; // to store the info sent to RPi
// specify the file containing xgboost parameters
const char boostname[] = "boost.txt";
float cond[READ_CNT_PER_LINE][MAX_COND_CNT]; // to store the conditions
uint16_t len_cond[READ_CNT_PER_LINE] = {0}; // length of conditions in each line

// string and float to hold float input
String inString = "";
float inFloat;
float inLine[READ_CNT_PER_LINE]; 
uint8_t batch_data[MAX_BATCH_LENGTH]; // send batch, can not use string cuz it is too long
uint32_t len_of_batch; // current length of batch_data
char send_info[MAX_INFO_LENGTH];
unsigned long prev_time, cur_time; // to store measured time
long sleep_time; // to store time, should be signed
BIGINT r_list[READ_CNT_PER_LINE]; // r_list

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

	Serial.println("SD card init done");
}


/*
 * read_line - read one line from myFile, each line read first READ_CNT_PER_LINE floats in 36 floats
 */
inline void read_line() {
	inString = ""; // clear string
	// read READ_CNT_PER_LINE float from one line
	for (uint8_t readcnt = 0; readcnt < READ_CNT_PER_LINE && myFile.available(); readcnt++) {
		// read until finish reading one float
		char byte = myFile.read();
		while (byte != ' ' && byte != '\n') {
			inString += byte;
			byte = myFile.read();
		}
		if (byte == '\n') // accidentally meet the end of line, return
			return;
		// finish reading one float, convert it
		float inFloat = inString.toFloat();
		DEBUG_PRINTF("%f ", inFloat);
		inLine[readcnt] = inFloat; // append to the sending batch
		inString = ""; // clear inString
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
 * search_sorted - convert the input float to an index and append it to batch_data
 * 				   input float is in inLine[12]
 */
inline void search_sorted() {
	BIGINT temp; // init to 0
	for (uint8_t i = 0; i < READ_CNT_PER_LINE; ++i) {
		uint8_t res_index;
		for (res_index = 0; res_index < len_cond[i]; ++res_index)
			if (inLine[i] <= cond[i][res_index])
				break;
		temp.add(r_list[i] * res_index);
	}
	// print temp for debugging
	/*temp.print_int();
	Serial.printf("\r\n");*/

	// append the BIGINT to send batch
	for (uint8_t i = 0; i < MAX_INT_BYTE; ++i)
		batch_data[len_of_batch++] = temp.get_byte(i);
}

/*
 * read_file - read sample_rate lines from myFile and append the data to sending batch
 * parameter - cur_sr current sample rate
 * Return - 0 not end
 *			1 meet end of file (EOF)
 */
uint8_t read_file(uint16_t cur_sr) {
	myFile = SD.open(filename); // open the file
	myFile.seek(position); // locate the last time of reading
	if (myFile) {
		for (int j = 0; j < cur_sr && myFile.available(); ++j) { // read sample_rate lines from myFile
			read_line();
			search_sorted();
			DEBUG_PRINTLN(j); // debug print the line number
		}
		
		// update position
		if (myFile.available()) {
			position = myFile.position(); // remember the position of last open time
			myFile.close();
			return 0;
		}
		// finish one sample rate!
		position = 0;
		myFile.close();
		return 1;
	}
	// open error!
	Serial.println("error opening file");
	return 1;
}

/*
 * write_sleep_time - write current time and sleep time to tmp.txt
 */
void write_sleep_time(char *buf) {
	// open the file. note that only one file can be open at a time,
	myFile = SD.open(tmpname, O_WRITE | O_APPEND);
	if (myFile) {
		myFile.write((const uint8_t *)buf, (size_t)strlen(buf));
		myFile.close();
	}
	else {
		Serial.println("error opening file");
	}
}

/*
 * send_sleep_time - send tmp.txt for this round of bandwidth and sample rate
 */
void send_sleep_time() {
	myFile = SD.open(tmpname); // open to read
	if (myFile) {
		len_of_batch = 0; // clear the sending batch
		while (myFile.available()) {
			char byte = myFile.read();
			// Serial.write(byte); // for reference
			batch_data[len_of_batch++] = (uint8_t)byte; // append lower byte to send batch

			// in case the length exceeds MAX_BATCH_LENGTH
			if (len_of_batch >= MAX_BATCH_LENGTH-2) {
				if (client.isConnected()) { // send one time
					client.publish(tmp_topic, batch_data, len_of_batch, false, client.QOS2, false, NULL);
					while (!client.loop_QoS2()); // block and wait for pub done
					Serial.println("publish one part of tmp.txt successfully"); // note success in publishing
				}
				len_of_batch = 0;
			}
		}
		// final pub
		if (client.isConnected()) {
			client.publish(tmp_topic, batch_data, len_of_batch, false, client.QOS2, false, NULL);
			while (!client.loop_QoS2()); // block and wait for pub done
			Serial.println("publish all parts of tmp.txt successfully"); // note success in publishing
		}
		myFile.close();
	}
	else {
		Serial.println("error opening file");
	}
}

/*
 * cond_insert - insert inFloat into cond array in order
 */
inline void cond_insert(uint8_t index, float inFloat) {
	uint8_t i, j;
	for (i = 0; i < len_cond[index]; ++i) {
		if (cond[index][i] == inFloat) // has already exist, directly return
			return; 
		if (cond[index][i] > inFloat) { // find the place to insert
			// move each element after i back for one position
			for (j = len_cond[index]; j > i; j--) 
				cond[index][j] = cond[index][j-1];
			break;
		}
	}
	cond[index][i] = inFloat; // insert herem, i is the right position
	len_cond[index]++;
	
	// print the array after insert
	DEBUG_PRINTF("%d %d %f\r\n", index, len_cond[index], inFloat);
	for (j = 0; j < len_cond[index]; ++j) {
		DEBUG_PRINTF("%f ", cond[index][j]);
	}
	DEBUG_PRINTF("\r\n\r\n");

	return;
}

/*
 * read_boost_cond - read decision boundaries from boost.txt
 */
void read_boost_cond() {
	myFile = SD.open(boostname);
	if (myFile) {
		inString = "";
		uint8_t flag = 1; // flag of whether reading a valid float or index
						  // 0 - invalid, 1 - index, 2 - float
		uint8_t index;

		while (myFile.available()) { // read until end
			char byte = myFile.read();
			if (byte == '\n') { // end of a line
				if (flag == 2) { // end of a valid float
					float inFloat = inString.toFloat();
					DEBUG_PRINTF("%d %f\r\n", index, inFloat);
					cond_insert(index, inFloat);
				}
				inString = ""; // clear
				flag = 1; // change to read index state
			}
			else if (byte == '<' && flag == 1) { // finish reading an index
				index = inString.toInt();
				// if in the first 12 devices, continue read; else, invalid
				flag = index < READ_CNT_PER_LINE ? 2 : 0; 
				inString = "";
			}
			else if (flag) // if valid reading
				inString += byte; // reading index or float to inString
		}
		myFile.close();

		// print what we read
		uint8_t i, j;
		Serial.println("read cond finish");
		for (i = 0; i < READ_CNT_PER_LINE; ++i) {
			Serial.println(i);
			Serial.println(len_cond[i]);
			/*for (j = 0; j < len_cond[i]; ++j)
				Serial.printf("%f ", cond[i][j]);
			Serial.printf("\r\n");*/
		}
	} 
	else {
		Serial.println("error opening file");
	}
}

/*
 * get_r_list - get the base number after each dimension
 *				must be called after read_boost_cond()
 */
void get_r_list() {
	BIGINT temp((uint8_t)1);
	Serial.println("r_list:");
	for (uint8_t i = 0; i < READ_CNT_PER_LINE; ++i) {
		r_list[i].change_value(temp);
		temp.multiply(len_cond[i]);
		r_list[i].print_int(); // print r_list
	}
}

/*
 * setup
 */
void setup() {
	Serial.begin(9600);
	init_card(); // init sd card
	read_boost_cond(); // read boost cond
	get_r_list();

	// connect to the RPi
	// client_id, user, passwd, willTopic, willQoS, willRetain, willMessage, cleanSession, version?
	client.connect("photon", "xiaofan", "0808", 0, client.QOS2, 0, 0, true, client.MQTT_V311);
	Serial.println("Connect!");

	for (uint8_t bw_index = 0; bw_index < NUM_OF_BW; bw_index++) {
		for (uint8_t sample_index = 0; sample_index < NUM_OF_SR; sample_index++) {
			// set bandwidth and sample rate
			sprintf(send_info, "%d %d", bw[bw_index], sample_rate[sample_index]);
			if (client.isConnected()) {
				client.publish(parameter_topic, (uint8_t *)send_info, strlen(send_info), false, client.QOS2, false, NULL);
				while (!client.loop_QoS2()); // block and wait for pub done
			}
			// sync time
			sprintf(send_info, "%ld", millis());
			if (client.isConnected()) {
				client.publish(sync_topic, (uint8_t *)send_info, strlen(send_info), false, client.QOS2, false, NULL);
				while (!client.loop_QoS2()); // block and wait for pub done
			}

			// clear tmp.txt
			myFile = SD.open(tmpname, O_WRITE | O_CREAT | O_TRUNC);
			if (myFile)
				myFile.close();
			else
				Serial.println("error opening file");

			// start reading!
			prev_time = millis();
			Serial.printf("prev_time %ld\n", prev_time);
			while (1) {
				len_of_batch = 0; // clear sending batch
				// read sample_rate lines, file is opened in this function
				uint8_t end = read_file(sample_rate[sample_index]);
					
				// publish
				if (client.isConnected()) {
					// topic, payload, plength, retain, qos, dup, messageid
					client.publish(data_topic, batch_data, len_of_batch, false, client.QOS2, false, NULL);
					while (!client.loop_QoS2()); // block and wait for pub done
					Serial.printf("%d %d %d publish successfully\r\n", bw[bw_index], sample_rate[sample_index], len_of_batch);
				}
				
				cur_time = millis();
				Serial.printf("cur_time %ld\r\n", cur_time);
				sleep_time = 1000 - (cur_time - prev_time);
				if (sleep_time > 0) {
					delay(sleep_time); // delay sleep_time milliseconds
					sprintf(write_buffer, "S\t%ld\t%ld\r\n", cur_time, sleep_time);
				}
				else {
					sprintf(write_buffer, "N\t%ld\t%ld\r\n", cur_time, sleep_time);
					Serial.println("N");
				}
				write_sleep_time(write_buffer);
				Serial.print(write_buffer);

				prev_time = millis(); // update prev_time
				Serial.printf("prev_time %ld\r\n", prev_time);
				
				if (end) // read until the end of file, ending this round of bandwidth and sample rate
					break;
			}
			send_sleep_time(); // send tmp.txt for this round
		}
	}

	client.disconnect();
}

/*
 * loop
 */
void loop() {
}
