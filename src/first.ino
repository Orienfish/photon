/*
 * Project: Establish MQTT connection with RPi and send local data
 * Author: Xiaofan Yu
 * Date: 10/01/2018
 */
#include "MQTT.h"

void callback(char* topic, byte* payload, unsigned int length);

/**
 * if want to use IP address,
 * byte server[] = { XXX,XXX,XXX,XXX };
 * MQTT client(server, 1883, callback);
 * want to use domain name,
 * exp) iot.eclipse.org is Eclipse Open MQTT Broker: https://iot.eclipse.org/getting-started
 * MQTT client("iot.eclipse.org", 1883, callback);
 **/
byte server[] = { 192,168,1,47 }; // specify the ip address of RPi
MQTT client(server, 1883, 60, callback); // ip, port, keepalive, callback, maxpacketsize=255

uint16_t sample_rate[] = {200, 220, 240, 260, 280, 300, 320, 340, 360, 380, 400};
uint16_t bw[] = {200, 700, 5000};
uint8_t batch_len = 36;
// should pay attention to the length of batch data string
char batch_data[] = "";//1.5663333 2.0231032 4.368568 -0.7739354 0.6374992 36494.797 11866.794 -0.57553095 1583.8704 -0.54552644 8.296114 1.6720939 0.16802186 1336.8407 46681.445 0.37339416 -1.5721303 -1.3606994 0.2258775 1.0056456 9.547066 0.020614887 0.49007025 1465.7668 0.35888034 0.09255244 0.64808124 10.55743 48605.066 0.58297986 0.40643385 1089.607 0.0567788 -1.4010533 0.13189445 9.742854";

/*
 * Receive Message - Not used here
 */
void callback(char* topic, byte* payload, unsigned int length) {
}


void setup() {
	Serial.begin(9600);
    // connect to the RPi
	// client_id, user, passwd, willTopic, willQoS, willRetain, willMessage, cleanSession, version?
    client.connect("henrykuo2", "admin", "IBMProject$", 0, client.QOS2, 0, 0, true, client.MQTT_V311);
	Serial.println("Connect!");

    // publish
    if (client.isConnected()) {
		// topic, payload, plength, retain, qos, dup, messageid
        client.publish("house.hand", (uint8_t *)batch_data, strlen(batch_data), false, client.QOS2, false, NULL);
		Serial.println("Publish successfully");
    }
}

void loop() {
    if (client.isConnected())
        client.loop();
}
