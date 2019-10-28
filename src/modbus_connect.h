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
#include <libconfig.h>
#include "ts_module_const.h"


//#define IP_BUF_SIZE            16  // Ip adress buf "192.168.111.222"
#define MAX_RCV_DATA_LEN       10  // Max numbers of byts to recive from client

/**
 * Data from one client
 */
typedef struct
{
	int len;							// Data len
	uint16_t data[MAX_RCV_DATA_LEN];	// Received data
} ModbusData;

/**
 * Protocol type
 */
typedef enum
{
	MPT_TCP,		// TCP Protocol
	MPT_RTU			// RTU Protocol
} ModbusProtocolType;

/**
 * Client info
 */
typedef struct
{
	int  id;								 // Client Id 1-128
	int  port;								 // TCP Port
	int  offset;							 // Data offset (see modbus protocol)
	int  registersToRead;				     // Number of registers to read from client
	int  baudRate;			 		 		 // Baud rate (115200)
	int  dataBit;							 // Data bit (8)
	int  stopBit;							 // Stop bit (1)
	const char *ipAdress;			 		 // TCP Adress
	const char *device;			 		 	 // Device (ttyUSB0)
	const char *parity;			 		 	 // Parity(N)
	modbus_t   *context;					 // Modbus handler
	ModbusProtocolType protocolType;		 // Protocol type (tcp, rtu)
} ModbusClient;

bool modbusConnect(ModbusClient *pClient);
bool modbusReceiveData(ModbusClient *pClient, ModbusData *pData);
bool modbusReconnect(ModbusClient *pClient);
void modbusCloseConnection(ModbusClient *pClient);

#endif // MODBUS_CONNECT_H
