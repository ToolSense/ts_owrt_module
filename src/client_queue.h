#ifndef CLIENT_QUEUE_H
#define CLIENT_QUEUE_H

#include <stdio.h>
#include "modbus_connect.h"

/**
 * Client queue according to refresh rate
 */
typedef struct
{
	int  pos_100ms;
	int  pos_1s;
	int  pos_1m;
	char rate_100ms[MAX_CLIENT_NUM];
	char rate_1s   [MAX_CLIENT_NUM];
	char rate_1m   [MAX_CLIENT_NUM];
}ClientQueue;

bool initQueue(config_t cfg);
bool addToQueue(ModbusRefreshRate rate, int clientId);
int getFirstClientId(ModbusRefreshRate rate);
int getNextClientId(ModbusRefreshRate rate);

#endif // CLIENT_QUEUE_H
