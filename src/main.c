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
#include "ts_module_const.h"
#include "logger.h"
#include <syslog.h>
#include <unistd.h>

bool send_mqtt_1m = false;

void timer_handler_1m()
{
	send_mqtt_1m = true;
}

void timer_init(config_t cfg)
{
	struct sigaction psa;
	struct itimerval tv;
	int timer_intr = DELAY_SEND_TIME;

	if(config_lookup_int(&cfg, "timer_intr", &timer_intr))
		timer_intr = DELAY_SEND_TIME;

	memset(&psa, 0, sizeof(psa));
	psa.sa_handler = &timer_handler_1m;
	sigaction(SIGALRM, &psa, NULL);


	tv.it_interval.tv_sec = timer_intr;
	tv.it_interval.tv_usec = 0;
	tv.it_value.tv_sec = timer_intr;
	tv.it_value.tv_usec = 0;
	setitimer(ITIMER_REAL, &tv, NULL);
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
		
	return rc;
}

int main(int argc, char *argv[])
{
	printf("\r\nMosquitto test SSL\r\n");

	char *cfg_file = "/etc/ts_module/ts_module.cfg";
	if (argc > 1) cfg_file = argv[1];

	//Test cl arguments
	// printf("argc=%d\n", argc);
	// printf("argv[0]=%s\n", argv[0]);	
	//------
	
	config_t cfg;
	config_init(&cfg);

	logger_init("/tmp/log.txt");

	/* Read the file. If there is an error, report it and exit. */
	if(! config_read_file(&cfg, cfg_file))
	{
		fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg),
				config_error_line(&cfg), config_error_text(&cfg));
		config_destroy(&cfg);
		return(EXIT_FAILURE);
	}

	int count_data = 0;
	init_count(&count_data, cfg);
	t_data data[count_data];
	init_data(data, cfg);

	timer_init(cfg);

	mqtt_init(cfg);

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
		fprintf(stdout, "ERROR: Modbus: Not all clients inited\n");
	}
	else
	{
		fprintf(stdout, "ERROR: Modbus: Init error\n");
	}
	// modbus_connect end

	while(1) {

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
				fprintf(stdout, "ERROR: Modbus: Receive error\n");
				modbusReconnect();
			}
			// modbus_connect end

			time(&data[TIME].l_data);

			// mqtt_send("Test\n", "temp");
			send_data(data, count_data);
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
