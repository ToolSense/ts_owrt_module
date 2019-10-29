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
#include "client_manager.h"
#include "client_queue.h"
#include "ts_data.h"
#include "timer.h"
#include "logger.h"

#include "opc_connect.h"

#define INIT_CFG 0
#define INIT_SYS 1
#define PROCESS 2

typedef struct DeviceStatus
{
	int  state;		// set device process status
	bool timer;		// t - timer enable
	bool mqtt;		// t - MQTT init
	bool manager;	// t - Manager init
} device_status;

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

int init_sys(device_status *dev_status, config_t cfg) {

	int rc = 0;
	int i;
	const ClientSettings *pClientSettings;

	timer_stop();
	dev_status->timer = false;

	/* Init MQTT */
	if (!dev_status->mqtt) {
		rc = mqtt_init(cfg);
		if (rc == 0) dev_status->mqtt = true;
	}

	/* Init Clients Manager and Queue */
	if (!dev_status->manager) {

		if(!manager_init_config(cfg))
		{
			fprintf(stdout, "Manager ERROR: Wrong config\n");
			return -1;
		}

		if(!manager_init_connections())
			fprintf(stdout, "Manager ERROR: Connection fail\n");

		// Init queue
		queueCleen();
		for(i = 0; i < MAX_CLIENT_NUM; i++)
		{
			pClientSettings = manager_get_client(i);
			if(pClientSettings == NULL)
				break; // All clients added

			if(!queueAdd(pClientSettings->refreshRate, pClientSettings->innerIdx))
			{
				fprintf(stdout, "Queue ERROR: Can't add client: %s\n", pClientSettings->name);
				return -1;
			}
		}
	}

	/* Init Timer after all */
	if (!dev_status->timer && dev_status->manager && dev_status->mqtt) {
		timer_init();
		rc = timer_start(TIMER_100mc);
		if (rc == 0) dev_status->timer = true;
	}

	/* If all ok - change the status */
	if (dev_status->timer && dev_status->manager && dev_status->mqtt) {
		printf("Init finished\n");
		dev_status->state = PROCESS;
	/* else wait and repeat */
	} else {
		printf("Wait init\n");
		sleep(5);
	}

	return 0;
}

void modbus_to_mqtt(ClientData *pClientData, t_data data[])
{
	if(pClientData->dataType == MDT_INT)
		printf("Int data: %d\n", pClientData->data_int);
	else if(pClientData->dataType == MDT_BOOL)
		printf("Bool data: %s\n", pClientData->data_bool ? "true":"false");
	else if(pClientData->dataType == MDT_DWORD)
		printf("Dword data: %d\n", pClientData->data_dword);
	else if(pClientData->dataType == MDT_TIME)
		printf("Time data: %s\n", ctime (&pClientData->data_time));
	else if(pClientData->dataType == MDT_ENUM)
		printf("Enum (int) data: %d\n", pClientData->data_enum);
	else
		printf("Wrong data type: %d\n", (int)pClientData->dataType);


	if(pClientData->dataType == MDT_INT)
		data[TEMP].i_data = pClientData->data_int;
	else
		data[TEMP].i_data = 25;

	time(&data[TIME].l_data);
}

int main(int argc, char *argv[])
{
	printf("\r\nStart\r\n");

	//First, need to init devices and systems
	device_status dev_status;
	dev_status.state = INIT_SYS;
	dev_status.timer = false;
	dev_status.mqtt = false;
	dev_status.manager = false;

	int rc = 0;
	bool firstClient;
	ClientData clientData;
	InnerIdx tempClientIdx;
	char clientName[MAX_CLIENT_NAME_LEN];
	char unit[MAX_UNIT_NAME_LEN];

	//Use log in /tmp
	logger_init("/tmp/ts_owrt_module.txt");

	/* Init config file & data */
	char *cfg_file = "/etc/ts_module/ts_module.cfg";
	if (argc > 1) cfg_file = argv[1];

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
	if (rc != DATA_ERR_SUCCESS) {
		printf("Init data count err code: %d\n", rc);
		return (rc);
	}

	t_data data[count_data];
	rc = init_data(data, cfg);
	if (rc != DATA_ERR_SUCCESS) {
		printf("Init data err code: %d\n", rc);
		return (rc);
	}	
	/* End init config file & data */

	while(1) {

		switch (dev_status.state) {
			case INIT_SYS:
				/* Init system & devices */
				init_sys(&dev_status, cfg);
			break;

			case PROCESS: 

				if (t_period.period_100ms) {

					firstClient = true;

					while(true)
					{
						// Get client ID
						if(firstClient)
						{
							tempClientIdx = queueGetFirst(RR_100ms);
							firstClient = false;
						}
						else
							tempClientIdx = queueGetNext(RR_100ms);

						if(tempClientIdx == -1)
							break; // All clients were surveyed


						// Receive data
						if(manager_receive_data(tempClientIdx, &clientData, clientName, unit))
						{
							// Fill data
							modbus_to_mqtt(&clientData, data);
							fprintf(stdout, "DEBUG: Manager: Receive OK, client %s\n", clientName);
						}
						else
						{
							fprintf(stdout, "Manager: Receive error, client %s\n", clientName);
							continue;
						}

						// Send data
						fprintf(stdout, "Send data, period 100ms\n");
						if(send_data(data, count_data) != 0)
						{
							fprintf(stdout, "MQTT: Send error\n");
						}
					}

					t_period.period_100ms = false;
				}

				if (t_period.period_1m) {

					firstClient = true;

					while(true)
					{
						// Get client ID
						if(firstClient)
						{
							tempClientIdx = queueGetFirst(RR_1m);
							firstClient = false;
						}
						else
							tempClientIdx = queueGetNext(RR_1m);

						if(tempClientIdx == -1)
							break; // All clients were surveyed


						// Receive data
						if(manager_receive_data(tempClientIdx, &clientData, clientName, unit))
						{
							// Fill data
							modbus_to_mqtt(&clientData, data);
							fprintf(stdout, "DEBUG: Manager: Receive OK, client %s\n", clientName);
						}
						else
						{
							fprintf(stdout, "Manager: Receive error, client %s\n", clientName);
							continue;
						}

						// Send data
						fprintf(stdout, "Send data, period 1m\n");
						if(send_data(data, count_data) != 0)
						{
							fprintf(stdout, "MQTT: Send error\n");
						}
					}

					t_period.period_1m = false;
				}

				if (t_period.period_1s) {

					firstClient = true;

					while(true)
					{
						// Get client ID
						if(firstClient)
						{
							tempClientIdx = queueGetFirst(RR_1s);
							firstClient = false;
						}
						else
							tempClientIdx = queueGetNext(RR_1s);

						if(tempClientIdx == -1)
							break; // All clients were surveyed


						// Receive data
						if(manager_receive_data(tempClientIdx, &clientData, clientName, unit))
						{
							// Fill data
							modbus_to_mqtt(&clientData, data);
							fprintf(stdout, "DEBUG: Manager: Receive OK, client %s\n", clientName);
						}
						else
						{
							fprintf(stdout, "Manager: Receive error, client %s\n", clientName);
							continue;
						}

						// Send data
						fprintf(stdout, "Send data, period 1s\n");
						if(send_data(data, count_data) != 0)
						{
							fprintf(stdout, "MQTT: Send error\n");
						}
					}

					t_period.period_1s = false;
				}

			break;

			default: usleep(100);
		}

		// usleep(100);

	}

	manager_close_all_connections();

	fprintf(stdout, "ts_owrt_module stoped\n");

	// modbus_connect begin
	// modbusDeinit();
	// modbus_connect end
	
	// mosquitto_loop_forever(mosq, -1, 1);
	// mosquitto_destroy(mosq);
	// mosquitto_lib_cleanup();
	return 0;
}
