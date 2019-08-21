#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>

#include "mqtt_connect.h"

#define DELAY_SEND_TIME 5 //sec
// #define PORT 10333
#define PORT_SSL 20333

bool send_mqtt_1m = false;

struct DataStruct
{
	int temp;
	time_t t;
} data;

struct mqttServerSetting mqttSetting; 

void timer_handler_1m()
{
	send_mqtt_1m = true;
}

void timer_init()
{
	struct sigaction psa;
	struct itimerval tv;

	memset(&psa, 0, sizeof(psa));
	psa.sa_handler = &timer_handler_1m;
	sigaction(SIGALRM, &psa, NULL);


	tv.it_interval.tv_sec = DELAY_SEND_TIME;
	tv.it_interval.tv_usec = 0;
	tv.it_value.tv_sec = DELAY_SEND_TIME;
	tv.it_value.tv_usec = 0;
	setitimer(ITIMER_REAL, &tv, NULL);
}

int send_data(struct DataStruct data)
{
	char *buf = malloc(64);

	sprintf(buf,"\nCurrent time is: %d",data.t);
	int rc = mqtt_send(buf, "temp");
	return rc;
}

int main(int argc, char *argv[])
{
	printf("\r\nMosquitto test SSL\r\n");

	mqttSetting.host = "m15.cloudmqtt.com";
	mqttSetting.port = PORT_SSL;
	mqttSetting.login = "thmcoslv";
	mqttSetting.passwd = "odUivT2WEIsW";
	mqttSetting.ssl_crt = "/etc/ssl/certs/ca-certificates.crt";

	mqtt_init(mqttSetting);
	timer_init();

	while(1) {

		if (send_mqtt_1m) {

			data.temp = 24;
			time(&data.t);

			// mqtt_send("Test\n", "temp");
			send_data(data);
			send_mqtt_1m = false;
		}

		sleep(1);

	}

	// mosquitto_loop_forever(mosq, -1, 1);
	// mosquitto_destroy(mosq);
	// mosquitto_lib_cleanup();
	return 0;
}