/*
 * Project: Establish MQTT connection with RPi and send data after running
 *     basic neural network.
 * Suppose necessary files are alreay written on the SD card:
 *     data.txt (extracted_subject1.txt)
 * Author: Xiaofan Yu
 * Date: 11/7/2018
 */
// put it here to avoid "was not declared error"
#include "application.h"
#include "MQTT.h"
#include "sd-card-library-photon-compat.h"

#define NUM_OF_SR 11 // number of sameple rate choices
#define NUM_OF_BW 3 // number of bandwidth choices
#define MAX_INFO_LENGTH 100 // maximum length of sending info, e.g. bw and sample rate
#define MAX_RAND_NUM 1000000 // maximum number in random()
#define MAX_SAMPLE_RATE 400 // maximum sample rate
#define SEND_INTERVAL 1000 // the interval in ms of each iteration
#define FEATURE_NUM 12 // number of features, linear regression input
#define CLASSES_NUM 18 // number of classes, linear regression output
// size of neural network
#define N_IN FEATURE_NUM // nodes as input
#define N_OUT CLASSES_NUM // nodes as output
#define N_1 60 // nodes in the first layer
#define N_2 60 // nodes in the second layer
#define BATCH_LEN 2 // the number of input samples for one time

// const variables in experiments
const int sample_rate[] = {200, 220, 240, 260, 280, 300, 320, 340, 360, 380, 400};
const int bw[] = {200, 700, 5000};

// define all categories of topics
const char data_topic[] = "house.hand"; // for sending data
const char tmp_topic[] = "tmp"; // for sending tmp.txt
const char parameter_topic[] = "parameter"; // for sending bandwidth and sample rate
const char sync_topic[] = "sync"; // for sync, send current time

/* global variables for MQTT */
byte server[] = { 137,110,160,230 }; // specify the ip address of RPi
MQTT client(server, 1883, 60, callback, MAX_SAMPLE_RATE+10); // ip, port, keepalive, callback, maxpacketsize=

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
unsigned long position = 0; // used to remember the last reading position when open filename
// specify the file recording sleep time, tmp.txt
const char tmpname[] = "tmp.txt";
char write_buffer[MAX_INFO_LENGTH]; // to store the info sent to RPi
// specify the file containing neural network parameters
// const char boostname[] = "nn.txt";

// nodes for neural networks
float nn_in[BATCH_LEN][N_IN], nn_out[BATCH_LEN][N_OUT]; // input and output nodes
float nn_l1[BATCH_LEN][N_1], nn_l2[BATCH_LEN][N_2]; // middle layer nodes in nn
float nn_w1[N_IN][N_1], nn_w2[N_1][N_2], nn_w3[N_2][N_OUT]; // weights between layers
uint8_t batch_data[MAX_SAMPLE_RATE+10]; // send batch
unsigned int len_of_batch = 0; // len of send batch
// global array to store assistant info
char send_info[MAX_INFO_LENGTH];
unsigned long prev_time, cur_time; // to store measured time
unsigned long comp_start, comp_time; // to record the start and time of computation
unsigned long read_start, read_time; // to record the start and time of reading
unsigned long ready_time; // total time of computation and reading
unsigned long total_comp_t, total_read_t; // to record the total time for calculating avg
unsigned int send_num = 0; // the number of times it sends
long sleep_time; // to store sleep time, should be signed

/*************************************************************************
 * NN functions
 ************************************************************************/
/*
 * init_weights
 */
void init_nn_weights() {
	long randNum;
	for (int i = 0; i < N_IN; ++i)
		for (int j = 0; j < N_1; ++j) {
			randNum = random(0, MAX_RAND_NUM);
			nn_w1[i][j] = ((float)randNum) / MAX_RAND_NUM;
		}
	for (int i = 0; i < N_1; ++i)
		for (int j = 0; j < N_2; ++j) {
			randNum = random(0, MAX_RAND_NUM);
			nn_w2[i][j] = ((float)randNum) / MAX_RAND_NUM;
		}
	for (int i = 0; i < N_2; ++i)
		for (int j = 0; j < N_OUT; ++j) {
			randNum = random(0, MAX_RAND_NUM);
			nn_w3[i][j] = ((float)randNum) / MAX_RAND_NUM;
		}
}

/*
 * ReLU activation. Read continuously!
 */
inline void ReLU(float *x, int row, int col) {
	for (int i = 0; i < row; ++i) {
		float *line = x + i * col; // record to save energy
		for (int j = 0; j < col; ++j)
			*(line + j) > 0? 1: *(line + j) = 0; // do nothing or make it zero
	}
}

/*
 * matrix_multiply: m1*m2=res.
 */
void matrix_multiply(float *m1, int row_m1, int col_m1, 
	float *m2, int row_m2, int col_m2, 
	float *res, int row_res, int col_res) {
	// check whether the number of rows and cols meet the requirement
	if (col_m1 != row_m2 || row_m1 != row_res || col_m2 != col_res)
		return;
	for (int i = 0; i < row_res; ++i) {
		// record the start pointer of i_th line in m1 and res
		float *line_m1 = m1 + i * col_m1;
		float *line_res = res + i * col_res;
		for (int j = 0; j < col_res; ++j) {
			float tmp = 0;
			// record the start pointer of j_th column in m2, save energy
			float *column_m2 = m2 + j;
			for (int k = 0; k < col_m1; ++k) {
				// tmp += *(line_m1 + k) * *(column_m2 + k * col_m2);
				tmp += *(line_m1++) * *column_m2;
				column_m2 += col_m2;
			}
			*(line_res++) = tmp; // set the value
		}
	}
}

/*
 * nn: in -> nn -> out
 * "in" is feature_num * batch_num, "out" is classnum*batch_num
 */
void nn(float *in, int in_row, int in_col,
	    float *out, int out_row, int out_col) {
	// check whether the rows of input and output equal
	if (in_row != out_row || in_col != N_IN || out_col != N_OUT)
		return;
	matrix_multiply(in, in_row, in_col,
					(float *)nn_w1, N_IN, N_1,
					(float *)nn_l1, in_row, N_1);
	ReLU((float *)nn_l1, in_row, N_1);

	matrix_multiply((float *)nn_l1, in_row, N_1,
					(float *)nn_w2, N_1, N_2,
					(float *)nn_l2, in_row, N_2);
	ReLU((float *)nn_l2, in_row, N_2);

	matrix_multiply((float *)nn_l2, in_row, N_2,
					(float *)nn_w3, N_2, N_OUT,
					out, out_row, out_col);
	ReLU(out, out_row, out_col);
}

/*
 * pack: pack out to batch_data. 
 * out_col should equal to N_OUT as we read continuously!
 */
void pack(float *out, int out_row, int out_col) {
	for (int i = 0; i < out_row; ++i) {
		float max = 0;
		uint8_t max_index = 0;
		float *line = out + i * out_col; // record to save energy
		// go through the out array and find out argmax-index
		for (int j = 0; j < out_col; ++j) {
			if (*line > max) {
				max = *(line++);
				max_index = j;
			}
		}
		batch_data[len_of_batch++] = max_index;
		// Serial.printf("max index is %d (%d)\r\n", max_index, len_of_batch);
	}
}

/*
 * print_matrix
 */
/*inline void print_matrix(float *m, int row, int col) {
	for (int i = 0; i < row; ++i) {
		for (int j = 0; j < col; ++j)
			Serial.printf("%f ", *(m+i*col+j));
		Serial.printf("\r\n");
	}
	Serial.printf("\r\n");
}*/

/*************************************************************************
 * Other settings for local connector
 ************************************************************************/
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
 * read_matrix
 * Parameter - in: the matrix to store read values, size is in_row*in_col
 */
void read_matrix(float *in, int in_row, int in_col) {
	String readString = ""; // clear string
	for (int i = 0; i < in_row && myFile.available(); ++i) {
		// record the start pointer of i_th line, save energy
		float *line = in + i * in_col;
		// read FEATURE_NUM float from one line
		for (int j = 0; j < in_col && myFile.available(); ++j) {
			// read until finish reading one float
			char byte = myFile.read();
			while (byte != ' ' && byte != '\n') {
				readString += byte;
				byte = myFile.read();
			}
			if (byte == '\n') // accidentally meet the end of line, return
				return;
			// finish reading one float, convert it
			float readFloat = readString.toFloat();
			*(line++) = readFloat; // add the float to readLine
			readString = ""; // clear inString
		}
		// read until end of this line, move on
		while (myFile.available()) {
			char byte = myFile.read();
			if (byte == '\n')
				break;
		}
	}
	return;
}

/*
 * read_data_file - read sample_rate lines from myFile and append the data to sending batch
 * parameter - readFile: the 2-D array to store reading floats
 *			   cur_sr： current sample rate, number of lines to read
 *			   readcnt: number of features to read in one line
 * Return - 0 not end
 *			1 meet end of file (EOF)
 * Set sample_num! A global variable
 */
uint8_t read_data_file(int cur_sample_rate) {
	myFile = SD.open(filename); // open the file
	myFile.seek(position); // locate the last time of reading
	if (myFile) {
		// read sample_rate lines from myFile
		for (int i = 0; i < cur_sample_rate && myFile.available(); i+=BATCH_LEN) {
			// read and process BATCH_LEN lines each time 
			read_start = millis();
			// calculate the len of this batch, less than BATCH_LEN if exceeds cur_sample_rate
			int len = min(cur_sample_rate - i, BATCH_LEN);
			read_matrix((float *)nn_in, len, N_IN);

			comp_start = millis();
			read_time += comp_start - read_start; // cumulative add
			// compute argmax-index data for this line
			nn((float *)nn_in, len, N_IN, (float *)nn_out, len, N_OUT);
			// pack the result into send batch
			pack((float *)nn_out, len, N_OUT);
			comp_time += millis() - comp_start; // cumulative add
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
		}
		// publish tmp.txt
		if (client.isConnected()) {
			client.publish(tmp_topic, batch_data, len_of_batch, false, client.QOS2, false, NULL);
			while (!client.loop_QoS2()); // block and wait for pub done
			// Serial.println("publish all parts of tmp.txt successfully"); // note success in publishing
		}
		myFile.close();
	}
	else {
		Serial.println("error opening file");
	}
}

/*
 * setup
 */
void setup() {
	Serial.begin(115200);
	init_card(); // init sd card
	init_nn_weights(); // init nn weights

	// connect to the RPi
	// client_id, user, passwd, willTopic, willQoS, willRetain, willMessage, cleanSession, version?
	client.connect("photon", "xiaofan", "0808", 0, client.QOS2, 0, 0, true, client.MQTT_V311);
	Serial.println("Connect!");

	for (int bw_index = 0; bw_index < NUM_OF_BW; bw_index++) {
		for (int sample_index = 0; sample_index < NUM_OF_SR; sample_index++) {
			// set bandwidth and sample rate
			sprintf(send_info, "%d %d", bw[bw_index], sample_rate[sample_index]);
			if (client.isConnected()) {
				client.publish(parameter_topic, (uint8_t *)send_info, strlen(send_info), 
					false, client.QOS2, false, NULL);
				while (!client.loop_QoS2()); // block and wait for pub done
			}
			// sync time
			sprintf(send_info, "%ld", millis());
			if (client.isConnected()) {
				client.publish(sync_topic, (uint8_t *)send_info, strlen(send_info), 
					false, client.QOS2, false, NULL);
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
			send_num = total_read_t = total_comp_t = 0;
			while (1) {
				len_of_batch = 0;
				read_time = comp_time = 0; // clear and ready for cumulative add
				// read sample_rate lines and do linear regression after reading each line
				uint8_t end = read_data_file(sample_rate[sample_index]);
				ready_time = millis() - prev_time; // total time in read and comp
					
				// publish
				if (client.isConnected()) {
					// topic, payload, plength, retain, qos, dup, messageid
					client.publish(data_topic, batch_data, len_of_batch, 
						false, client.QOS2, false, NULL);
					while (!client.loop_QoS2()); // block and wait for pub done
					Serial.printf("%d %d %d publish successfully\r\n", bw[bw_index], 
						sample_rate[sample_index], len_of_batch);
				}
				
				cur_time = millis();
				// Serial.printf("cur_time %ld\r\n", cur_time);
				sleep_time = SEND_INTERVAL - (cur_time - prev_time);
				if (sleep_time > 0) {
					delay(sleep_time); // delay sleep_time milliseconds
					sprintf(write_buffer, "S\tprev:%ld\tcur:%ld\tsleep:%ld\tlen:%d\tc:%ld\tr:%ld\tready:%ld\r\n", 
						prev_time, cur_time, sleep_time, len_of_batch, comp_time, read_time, ready_time);
				}
				else {
					sprintf(write_buffer, "N\tprev:%ld\tcur:%ld\tsleep:%ld\tlen:%d\tc:%ld\tr:%ld\tready:%ld\r\n", 
						prev_time, cur_time, sleep_time, len_of_batch, comp_time, read_time, ready_time);
					Serial.println("N");
				}
				write_sleep_time(write_buffer);
				Serial.print(write_buffer);

				prev_time = millis(); // update prev_time
				
				// update total time
				total_read_t += read_time;
				total_comp_t += comp_time;
				send_num++;
				
				if (end) // read until the end of file, ending this round of bandwidth and sample rate
					break;
			}
			send_sleep_time(); // send tmp.txt for this round
		}
	}
	// compute the avg time consumption and send it
	float read_avg = (float)total_read_t / send_num;
	float comp_avg = (float)total_comp_t / send_num;
	sprintf(write_buffer, "read avg:%f\tcomp avg:%f\r\n", read_avg, comp_avg);
	if (client.isConnected()) {
		client.publish(tmp_topic, (uint8_t *)write_buffer, strlen(write_buffer), 
			false, client.QOS2, false, NULL);
		while (!client.loop_QoS2()); // block and wait for pub done
	}
	client.disconnect();
}

/*
 * loop
 */
void loop() {
}
