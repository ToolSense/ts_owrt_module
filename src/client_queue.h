#ifndef CLIENT_QUEUE_H
#define CLIENT_QUEUE_H

#include <stdio.h>
#include <stdbool.h>
#include "ts_module_const.h"

/**
 * Client queue according to refresh rate
 */
typedef struct
{
	int pos_100ms;
	int pos_1s;
	int pos_1m;
	int rate_100ms[MAX_CLIENT_NUM];
	int rate_1s   [MAX_CLIENT_NUM];
	int rate_1m   [MAX_CLIENT_NUM];
}ClientQueue;

void queueCleen();
bool queueAdd(RefreshRate rate, int clientId);
int  queueGetFirst(RefreshRate rate);
int  queueGetNext(RefreshRate rate);

#endif // CLIENT_QUEUE_H
