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
#include "timer.h"

#define INIT_CFG 0
#define INIT_SYS 1
#define PROCESS 2

typedef struct DeviceStatus
{
	int  state;		// set device process status
	bool timer;		// t - timer enable
	bool mqtt;		// t - MQTT init
	bool modbus;	// t - ModBus init
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

	timer_stop();
	dev_status->timer = false;

	/* Init MQTT */
	if (!dev_status->mqtt) {
		rc = mqtt_init(cfg);
		if (rc == 0) dev_status->mqtt = true;
	}

	/* Init ModBus */
	if (!dev_status->modbus) {

		// modbus_connect begin
		ModbusError      status;
		// ModbusClientData clientData;

		status = modbusInit(cfg);

		if(status == MBE_OK)
		{
			fprintf(stdout, "Modbus: Init OK\n");
			dev_status->modbus = true;
		}
		else if(status == MBE_NOT_ALL)
		{
			fprintf(stdout, "Modbus: Not all clients inited\n");
			dev_status->modbus = true;
		}
		else
		{
			fprintf(stdout, "Modbus: Init error\n");
		}
		// modbus_connect end
	}

	/* Init Timer after all */
	if (!dev_status->timer && dev_status->modbus && dev_status->mqtt) {
		timer_init();
		rc = timer_start(TIMER_100mc);
		if (rc == 0) dev_status->timer = true;
	}

	/* If all ok - change the status */
	if (dev_status->timer && dev_status->modbus && dev_status->mqtt) {
		printf("Init finished\n");
		dev_status->state = PROCESS;
	/* else wait and repeat */
	} else {
		printf("Wait init\n");
		sleep(5);
	}

	return 0;
}

int main(int argc, char *argv[])
{
	printf("\r\nMosquitto test SSL\r\n");

	//First, need to init devices and systems
	device_status dev_status;
	dev_status.state = INIT_SYS;
	dev_status.timer = false;
	dev_status.mqtt = false;
	dev_status.modbus = false;

	int rc = 0;

	/* Modbus status var*/
	ModbusError status;

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

					// printf("Test Timer2\n");

					t_period.period_100ms = false;

				}

				if (t_period.period_1m) {

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
						// modbusReconnect();
						dev_status.modbus = false;
						dev_status.state = INIT_SYS;
					}
					// modbus_connect end

					time(&data[TIME].l_data);

					// mqtt_send("Test\n", "temp");
					send_data(data, count_data);
					t_period.period_1m = false;
				}

			break;

			default: usleep(100);
		}

		// usleep(100);

	}

	fprintf(stdout, "ts_owrt_module stoped\n");

	// modbus_connect begin
	// modbusDeinit();
	// modbus_connect end
	
	// mosquitto_loop_forever(mosq, -1, 1);
	// mosquitto_destroy(mosq);
	// mosquitto_lib_cleanup();
	return 0;
}