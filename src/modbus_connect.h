#ifndef MODBUS_CONNECT_H
#define MODBUS_CONNECT_H

#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include <modbus.h>

#define IP_BUF_SIZE    16
#define MAX_CLIENT_NUM 10
#define MAX_RCV_DATA_LEN 10

typedef enum
{
	MBE_OK,
	MBE_NOT_ALL,
	MBE_CLIENT,
	MBE_CONTEXT,
	MBE_CONNECT,
	MBE_REINIT,
	MBE_FAIL
} ModbusError;

typedef struct
{
	int  id;
	int  port;
	int  offset;
	int  numOfBytes;
	bool connected;
	char ipAdress[IP_BUF_SIZE+1];
	modbus_t *context;
} ModbusClient;

typedef struct
{
	int clientsCnt;
	ModbusClient clients[MAX_CLIENT_NUM];
} ModbusClientsList;

typedef struct
{
	int clientId;
	uint16_t data[MAX_RCV_DATA_LEN];
} ModbusClientData;

typedef struct
{
	ModbusClientData clients[MAX_CLIENT_NUM];
} ModbusClientsDataList;

ModbusError modbusInit(ModbusClientsList *pClientsList);
ModbusError modbusReceiveData(ModbusClientsDataList *pDataList);
ModbusError modbusReconnect();
void        modbusDeinit();


#endif // MODBUS_CONNECT_H