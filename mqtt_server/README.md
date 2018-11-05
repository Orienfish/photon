# MQTT Server on a Linux System
This folder contains the python subscriber to receive data from photon and several test data folders under various conditions.

## Prerequisites
I use the Ubuntu 18.04LTS. To start with, you need to install mosquitto and paho-mqtt. Mosquitto is a popular MQTT server while paho-mqtt is a convenient Python library.

To install and configure, follow the steps [here](https://www.digitalocean.com/community/tutorials/how-to-install-and-secure-the-mosquitto-mqtt-messaging-broker-on-ubuntu-18-04). You need to configure several files under /etc/mosquitto/ for advanced settings like the log and the user's password.

To install paho-mqtt 1.4.0, see the tutorials [here](https://pypi.org/project/paho-mqtt/).

## Subscriber
This Python version program subscribes 4 topics: **"house.hand", "tmp", "parameter", "sync"**.
- **"house.hand"** represents the real data, namely the inference results sent from photon.
- **"tmp"** is the final file containing time test results. In each sampling iteration, the test results include the previous time before an iteration starts (prev), the current time after sending (cur), the sleep time (sleep), len of the sending data in bytes (len), computation time (c), reading from SD card time (r) and the total time of computation and reading (ready). All those data will be writen to /tmp_XX/local_log_$bandwidth$_$sample rate$.txt, where "XX" denotes the name of a special case. The complete log of this test is in /tmp_XX/total_log.txt, which is more intuitive for human to compare.
- **"parameter"** topic is for notifying bandwidth and sample rate settings in this iteration.
- **"sync"** is for time synchronization, simply sending the time before an iteration starts (prev).