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
#include "modbus_connect.h"
#include "ts_data.h"
#include <syslog.h>

typedef struct DeviceInitStatus
{
	bool mqtt;
	bool modbus;
} init_status;



bool send_mqtt_1m = false;
bool send_mqtt_1s = false;
bool send_mqtt_100ms = false;

int timer_counter = 0;

void timer_handler_1m()
{
	send_mqtt_1m = true;
}

void timer_handler_100ms()
{
	send_mqtt_100ms = true;
	timer_counter++;

	if ((timer_counter % TIMER_1S) == 0) {
		send_mqtt_1s = true;
	}

	// if ((timer_counter % TIMER_5S) == 0) {
	// 	send_mqtt_1m = true;
	// }

	// Maximum period - TIMER_5S
	if (timer_counter == TIMER_5S) {
		send_mqtt_1m = true;
		timer_counter = 0;
	}
}

void timer_init(config_t cfg)
{
	struct sigaction psa;
	struct itimerval tv;
	int timer_intr = 0;

	if(!config_lookup_int(&cfg, "timer_intr", &timer_intr))
		timer_intr = DELAY_SEND_TIME;

	memset(&psa, 0, sizeof(psa));
	psa.sa_handler = &timer_handler_100ms;
	sigaction(SIGALRM, &psa, NULL);

	// tv.it_interval.tv_sec = timer_intr;
	// tv.it_interval.tv_usec = 0;
	// tv.it_value.tv_sec = timer_intr;
	// tv.it_value.tv_usec = 0;

	tv.it_interval.tv_sec = 0;
	tv.it_interval.tv_usec = timer_intr;
	tv.it_value.tv_sec = 0;
	tv.it_value.tv_usec = timer_intr;

	// struct itimerval old, old2;
	setitimer(ITIMER_REAL, &tv, NULL);
	// setitimer(ITIMER_REAL, &tv2, &old2);
}

int send_data(t_data data[], int count)
{
	char *buf = malloc(MAX_BUF);
	// sprintf(buf,"\nUnix time: %d",data.time);
	// int rc = mqtt_send(buf, "temp");

	//Temporary solution
	get_json(buf, data, count);
	printf("%s\n", buf);


	int rc = mqtt_send(buf, "temp");

	#ifdef SYSLOG
		syslog(LOG_INFO,"Send dada status %d", rc);
	#endif
		
	return rc;
}

int main(int argc, char *argv[])
{
	printf("\r\nMosquitto test SSL\r\n");

	int rc = 0;

	//Use log in /var/log/
	#ifdef SYSLOG
		openlog("ts_owrt_module",LOG_PID,LOG_USER);
	#endif

	char *cfg_file = "/etc/ts_module/ts_module.cfg";
	if (argc > 1) cfg_file = argv[1];

	//Test cl arguments
	// printf("argc=%d\n", argc);
	// printf("argv[0]=%s\n", argv[0]);	
	//------

	config_t cfg;
	config_init(&cfg);

	/* Read the file. If there is an error, report it and exit. */
	if(! config_read_file(&cfg, cfg_file))
	{
		fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg),
				config_error_line(&cfg), config_error_text(&cfg));
		config_destroy(&cfg);
		return(EXIT_FAILURE);
	}


	int count_data = 0;
	rc = init_count(&count_data, cfg);
	#ifdef SYSLOG
		if (!rc) syslog(LOG_ERR, "Init data count error %d", rc);
	#endif

	t_data data[count_data];
	rc = init_data(data, cfg);
	#ifdef SYSLOG
		if (!rc) syslog(LOG_ERR, "Init data error %d", rc);
	#endif

	timer_init(cfg);

	rc = mqtt_init(cfg);
	#ifdef SYSLOG
		if (!rc) syslog(LOG_ERR, "Init MQTT error %d", rc);
	#endif

	// modbus_connect begin
	ModbusError      status;
	ModbusClientData clientData;

	status = modbusInit(cfg);

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
	}
	// modbus_connect end

	while(1) {

		if (send_mqtt_100ms) {

			printf("Test Timer2\n");

			send_mqtt_100ms = false;

		}

		if (send_mqtt_1m) {

			// modbus_connect begin
			// Receive data
			status = modbusReceiveDataId(&clientData, 1);

			if(status == MBE_OK)
			{
				fprintf(stdout, "Modbus: Receive OK\n");


				printf("Modbus data: Id: %d, name: %s, unit: %s, data: %02X:%02X:%02X:%02X \n", 
					   clientData.id, clientData.name, clientData.unit, 
					   clientData.data[0], clientData.data[1], clientData.data[2], clientData.data[3]);

				if(clientData.dataType == MDT_INT)
					printf("Int data: %d\n", clientData.data_int);
				else if(clientData.dataType == MDT_BOOL)
					printf("Bool data: %s\n", clientData.data_bool ? "true":"false");
				else if(clientData.dataType == MDT_DWORD)
					printf("Dword data: %d\n", clientData.data_dword);
				else if(clientData.dataType == MDT_TIME)
					printf("Time data: %s\n", ctime (&clientData.data_time));
				else if(clientData.dataType == MDT_ENUM)
					printf("Enum (int) data: %d\n", clientData.data_enum);
				else
					printf("Wrong data type: %d\n", (int)clientData.dataType);

				// MQTT
				if(clientData.dataType == MDT_INT)
					data[TEMP].i_data = clientData.data_int;
				else
					data[TEMP].i_data = 25;
			}
			else
			{
				fprintf(stdout, "Modbus: Receive error\n");
				modbusReconnect();
			}
			// modbus_connect end

			time(&data[TIME].l_data);

			// mqtt_send("Test\n", "temp");
			send_data(data, count_data);
			send_mqtt_1m = false;
		}

		usleep(100);

	}

	#ifdef SYSLOG
		closelog();
	#endif

	fprintf(stdout, "ts_owrt_module stoped\n");

	// modbus_connect begin
	// modbusDeinit();
	// modbus_connect end
	
	// mosquitto_loop_forever(mosq, -1, 1);
	// mosquitto_destroy(mosq);
	// mosquitto_lib_cleanup();
	return 0;
}