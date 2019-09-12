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

#define IP_BUF_SIZE            16  // Ip adress buf "192.168.111.222"
#define MAX_CLIENT_NUM         10  // Max numbers of client, min 1 - max 128 
#define MAX_RCV_DATA_LEN       10  // Max numbers of byts to recive from client
#define MAX_SECTION_NAME_LEN   16  // Max len for ini section len

typedef unsigned int DWORD;

/**
 * Error code
 */
typedef enum
{
	MBE_OK,			// Ok
	MBE_NOT_ALL,	// Not all clients, but at least one
	MBE_CLIENT,		// Client error
	MBE_CONTEXT,	// TCP Context error
	MBE_CONNECT, 	// Connection error
	MBE_INIT,		// Init error, need reinit
	MBE_FAIL		// Unknown error
} ModbusError;

/**
 * Data type
 */
typedef enum
{
	MDT_BOOL,		// Boolean
	MDT_INT,		// Integer
	MDT_DWORD,		// Double word
	MDT_TIME,		// Time
	MDT_ENUM		// Enumerate
} ModbusDataType;

/**
 * Client info
 */
typedef struct
{
	int  id;								 // Client Id 1-128
	int  port;								 // TCP Port
	int  offset;							 // Data offset (see modbus protocol)
	int  registersToRead;				     // Number of registers to read from client
	int  refreshRateMs;						 // Time to refresh data		
	bool connected;							 // Connected sign, changed automatically
	const char *ipAdress;			 		 // TCP Adress
	const char *unit;			 		 	 // Unit (h, kg, cm, m/s ...)
	const char *name;			 		 	 // Name(Data Point, Variable)
	modbus_t *context;						 // Modbus handler
	ModbusDataType dataType;                 // Type of receive data
} ModbusClient;

/**
 * All clients settings
 */
typedef struct
{
	int          clientsCnt;				 // Number of clients
	ModbusClient clients[MAX_CLIENT_NUM];	 // Settings for all clients
} ModbusSettings;

/**
 * Data from one client
 */
typedef struct
{
	int id;							         // Client Id
	const char *name;			 		 	 // Name(Data Point, Variable)
	const char *unit;			 		 	 // Unit (h, kg, cm, m/s ...)
	ModbusDataType dataType;                 // Type of receive data
	uint16_t data[MAX_RCV_DATA_LEN];		 // Received data
	bool     data_bool;				     	 // Data in boolean format			
	int      data_int;				     	 // Data in integer format
	DWORD    data_dword;				 	 // Data in double word format
	time_t   data_time;					 	 // Data in time_t format
	int      data_enum;					 	 // Data in enumeration format
} ModbusClientData;

/**
 * Data from all clients
 */
typedef struct
{
	ModbusClientData clients[MAX_CLIENT_NUM]; // All received data
} ModbusClientsDataList;

ModbusError modbusInit(config_t cfg);
ModbusError modbusReceiveData(ModbusClientsDataList *pDataList);
ModbusError modbusReceiveDataId(ModbusClientData *pData, int id);
ModbusError modbusReconnect();
void        modbusDeinit();


#endif // MODBUS_CONNECT_H