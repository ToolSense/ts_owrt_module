#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>

#include <libconfig.h>

#include "mqtt_connect.h"

// modbus_connect begin
#include "modbus_connect.h"
// modbus_connect end

#define DELAY_SEND_TIME 5 //sec
// #define PORT 10333
// #define PORT_SSL 20333

bool send_mqtt_1m = false;

int timer_intr = DELAY_SEND_TIME;

typedef struct DataStruct
{
	int temp;
	time_t time;
} t_data;

t_data data;

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


	tv.it_interval.tv_sec = timer_intr;
	tv.it_interval.tv_usec = 0;
	tv.it_value.tv_sec = timer_intr;
	tv.it_value.tv_usec = 0;
	setitimer(ITIMER_REAL, &tv, NULL);
}

int send_data(t_data data)
{
	char *buf = malloc(64);
	// sprintf(buf,"\nUnix time: %d",data.time);
	// int rc = mqtt_send(buf, "temp");

	//Temporary solution
	sprintf (buf, "{\"temp\": %d, \"time\": %d}", data.temp, data.time);
	printf("%s\n", buf);


	int rc = mqtt_send(buf, "temp");
		
	return rc;
}

int main(int argc, char *argv[])
{
	
	config_t cfg;
	config_setting_t *mqtt_conf;

	config_init(&cfg);

	/* Read the file. If there is an error, report it and exit. */
	if(! config_read_file(&cfg, "/tmp/ts_module.cfg"))
	{
	fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg),
	        config_error_line(&cfg), config_error_text(&cfg));
	config_destroy(&cfg);
	return(EXIT_FAILURE);
	}

	if(config_lookup_int(&cfg, "timer_intr", &timer_intr))
		timer_intr = DELAY_SEND_TIME;


	mqtt_conf = config_lookup(&cfg, "mqtt");
	if(mqtt_conf != NULL)
	{
		int count = config_setting_length(mqtt_conf);
		printf("mqtt_conf %d\n", count);
		config_setting_t *mqtt_cloud = config_setting_get_elem(mqtt_conf, 0);

		config_setting_lookup_string(mqtt_cloud, "host", &mqttSetting.host);
		config_setting_lookup_string(mqtt_cloud, "login", &mqttSetting.login);
		config_setting_lookup_string(mqtt_cloud, "passwd", &mqttSetting.passwd);
		config_setting_lookup_string(mqtt_cloud, "ssl_crt", &mqttSetting.ssl_crt);
		config_setting_lookup_int(mqtt_cloud, "port", &mqttSetting.port);
	}




	// modbus_connect begin
	ModbusError           status;
	ModbusClientsList     clientsList;
	ModbusClientsDataList clientsDataList;
	// modbus_connect end

	printf("\r\nMosquitto test SSL\r\n");

	//Test cl arguments
	// printf("argc=%d\n", argc);
	// printf("argv[0]=%s\n", argv[0]);
	//------

	timer_init();

	// mqttSetting.host = "m15.cloudmqtt.com";
	// mqttSetting.port = PORT_SSL;
	// mqttSetting.login = "thmcoslv";
	// mqttSetting.passwd = "odUivT2WEIsW";
	// mqttSetting.ssl_crt = "/etc/ssl/certs/ca-certificates.crt";

	// modbus_connect begin
	clientsList.clientsCnt = 1;               // One client
	clientsList.clients[0].id = 1;            // Client id
	clientsList.clients[0].port = 502;        // Port
	clientsList.clients[0].offset = 0;        // Bytes offset
	clientsList.clients[0].numOfBytes = 1;    // Bytes num
	snprintf(clientsList.clients[0].ipAdress, IP_BUF_SIZE, "%s", "192.168.1.9"); // Ip

	status = modbusInit(&clientsList);

	if(status == MBE_OK)
	{
		fprintf(stdout, "Modbus: Init OK\n");
	}
	else if(status == MBE_NOT_ALL)
	{
		fprintf(stdout, "Modbus: Not all clients inited\n");
	}
	else
	{
		fprintf(stdout, "Modbus: Init error\n");
		modbusDeinit();
	}
	// modbus_connect end

	mqtt_init(mqttSetting);

	while(1) {

		if (send_mqtt_1m) {

			// modbus_connect begin
			// Receive data
			status = modbusReceiveData(&clientsDataList);

			if(status == MBE_OK)
			{
				fprintf(stdout, "Modbus: Receive OK\n");
			}
			else if(status == MBE_NOT_ALL)
			{
				fprintf(stdout, "Modbus: Not all clients send data\n");
				modbusReconnect();
			}
			else
			{
				fprintf(stdout, "Modbus: Receive error\n");
				modbusReconnect();
			}
			

			printf("Id:%2d temp=%6d \t (0x%X)\n", clientsDataList.clients[0].clientId, 
											  clientsDataList.clients[0].data[0], 
											  clientsDataList.clients[0].data[0]);

			data.temp = clientsDataList.clients[0].data[0];
			// modbus_connect end

			time(&data.time);

			// mqtt_send("Test\n", "temp");
			send_data(data);
			send_mqtt_1m = false;
		}

		sleep(1);

	}

	// modbus_connect begin
	// modbusDeinit();
	// modbus_connect end
	
	// mosquitto_loop_forever(mosq, -1, 1);
	// mosquitto_destroy(mosq);
	// mosquitto_lib_cleanup();
	return 0;
}