#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <stdbool.h>
#include "modbus_connect.h"

#define MODBUS_DEBUG 1

/*
#define LOG_MSG(msg) log_msg(msg, __FUNCTION__, __LINE__)
#define ERR_MSG(msg) err_msg(msg, __FUNCTION__, __LINE__)
 
void log_msg(const char *msg, const char *func, const int line)
{
	if(MODBUS_DEBUG)
    	printf("MSG: %s Fun: %s line: %d\n", msg, func, line);
}

void err_msg(const char *msg, const char *func, const int line)
{
	if(MODBUS_DEBUG)
    	printf(stderr, "ERROR: %s Fun: %s line: %d\n", msg, func, line);
    else
    	printf(stderr, "ERROR: %s\n", msg);
}
*/

ModbusClientsList _clientsList;

ModbusError modbusInit(ModbusClientsList *pClientsList)
{
	int  clientNum;
	bool atLeastOne = false;
	ModbusError mbStatus = MBE_OK;

	if(pClientsList->clientsCnt == 0)
	{
		fprintf(stderr, "ERROR: No clients");
		return MBE_CLIENT;
	}
	else if(pClientsList->clientsCnt > MAX_CLIENT_NUM)
	{
		fprintf(stderr, "ERROR: Too much clients, max  %d\n", MAX_CLIENT_NUM);
		return MBE_CLIENT;
	}

	if(MODBUS_DEBUG)
		printf("Find %d clients\n", pClientsList->clientsCnt);

	// Save clients data in _clientsList
	memcpy(&_clientsList, pClientsList, sizeof(ModbusClientsList));

	// Use _clientsList for connection
	for(clientNum = 0; clientNum < _clientsList.clientsCnt; clientNum++)
	{
		// Default value
		_clientsList.clients[clientNum].context   = NULL;
		_clientsList.clients[clientNum].connected = false;

		// Get context
		_clientsList.clients[clientNum].context = modbus_new_tcp(_clientsList.clients[clientNum].ipAdress, 
																 _clientsList.clients[clientNum].port);	
		
		if (_clientsList.clients[clientNum].context == NULL) 
		{
			fprintf(stderr, "ERROR: Unable to allocate libmodbus context for ip: %s, port: %d\n", 
					_clientsList.clients[clientNum].ipAdress,
					_clientsList.clients[clientNum].port);

			return MBE_CONTEXT;
		}
		
		// Connect
		if (modbus_connect(_clientsList.clients[clientNum].context) == -1) 
		{
			fprintf(stderr, "ERROR: Connection failed: %s, for ip: %s, port: %d\n", modbus_strerror(errno),
					_clientsList.clients[clientNum].ipAdress,
					_clientsList.clients[clientNum].port);

			mbStatus = MBE_FAIL;
			continue;
		}

		modbus_set_slave(_clientsList.clients[clientNum].context, _clientsList.clients[clientNum].id);

		_clientsList.clients[clientNum].connected = true;
		atLeastOne = true;

		if(MODBUS_DEBUG)
			printf("Connected to ip: %s, port: %d\n", 
					_clientsList.clients[clientNum].ipAdress,
					_clientsList.clients[clientNum].port);
	}

	// Check status
	if(mbStatus != MBE_OK)
	{
		if(atLeastOne)
			return MBE_NOT_ALL;
		else
			return MBE_FAIL;
	}

	return MBE_OK;
}

ModbusError modbusReceiveData(ModbusClientsDataList *pDataList)
{
	int rc;
	int  clientNum;
	bool atLeastOne = false;
	ModbusError mbStatus = MBE_OK;

	memset(pDataList, '\0', sizeof(ModbusClientsDataList));

	for(clientNum = 0; clientNum < _clientsList.clientsCnt; clientNum++)
	{
		if(_clientsList.clients[clientNum].connected)
		{
			pDataList->clients[clientNum].clientId = _clientsList.clients[clientNum].id;

			rc = modbus_read_registers(_clientsList.clients[clientNum].context, 
									   _clientsList.clients[clientNum].offset, 
									   _clientsList.clients[clientNum].numOfBytes, 
									   pDataList->clients[clientNum].data);

			if (rc == -1) 
			{
				fprintf(stderr, "ERROR: Recive data: %s, from ip: %s, port: %d\n", modbus_strerror(errno), 
					    _clientsList.clients[clientNum].ipAdress, 
					    _clientsList.clients[clientNum].port);

				_clientsList.clients[clientNum].connected = false;
				mbStatus = MBE_FAIL;
				continue;	
			}

			atLeastOne = true;
		}
		else
		{
			fprintf(stderr, "ERROR: Not connected client ip: %s, port: %d, need reconnect\n", 
					_clientsList.clients[clientNum].ipAdress, 
					_clientsList.clients[clientNum].port);

				
			mbStatus = MBE_FAIL;			
		}
	}

	// Check status
	if(mbStatus != MBE_OK)
	{
		if(atLeastOne)
			return MBE_NOT_ALL;
		else
			return MBE_FAIL;
	}

	return MBE_OK;
}

ModbusError modbusReconnect()
{
	int  clientNum;
	bool atLeastOne = false;
	ModbusError mbStatus = MBE_OK;
	int rc;
	uint16_t data[MAX_RCV_DATA_LEN];

	for(clientNum = 0; clientNum < _clientsList.clientsCnt; clientNum++)
	{
		if(_clientsList.clients[clientNum].connected == false)
		{	
			// Client not connected
			if(_clientsList.clients[clientNum].context != NULL)	
			{
				// modbus_close(_clientsList.clients[clientNum].context); Segmentation fault
				// modbus_free(_clientsList.clients[clientNum].context);  Segmentation fault

				// Get context
				_clientsList.clients[clientNum].context = modbus_new_tcp(_clientsList.clients[clientNum].ipAdress, 
																			 _clientsList.clients[clientNum].port);	
				
				if (_clientsList.clients[clientNum].context == NULL) 
				{
					fprintf(stderr, "ERROR: Unable to allocate libmodbus context for ip: %s, port: %d\n", 
							_clientsList.clients[clientNum].ipAdress,
							_clientsList.clients[clientNum].port);

					return MBE_CONTEXT;
				}
 
				// Connect
				if (modbus_connect(_clientsList.clients[clientNum].context) == -1) 
				{
					fprintf(stderr, "ERROR: Connection failed: %s, for ip: %s, port: %d\n", modbus_strerror(errno),
							_clientsList.clients[clientNum].ipAdress,
							_clientsList.clients[clientNum].port);

					mbStatus = MBE_FAIL;
					continue;
				}

				modbus_set_slave(_clientsList.clients[clientNum].context, _clientsList.clients[clientNum].id);	

				_clientsList.clients[clientNum].connected = true;
			}
			else
			{
				fprintf(stderr, "ERROR: No context, need reinit, client ip: %s, port: %d\n", 
					    _clientsList.clients[clientNum].ipAdress, 
					    _clientsList.clients[clientNum].port);

				return MBE_REINIT;
			}

		}
	}

	// Check status
	if(mbStatus != MBE_OK)
	{
		if(atLeastOne)
			return MBE_NOT_ALL;
		else
			return MBE_FAIL;
	}

	return MBE_OK;	
}

void modbusDeinit()
{
	int  clientNum;

	for(clientNum = 0; clientNum < _clientsList.clientsCnt; clientNum++)
	{
		if(_clientsList.clients[clientNum].context != NULL)	
		{
			modbus_close(_clientsList.clients[clientNum].context);	
			modbus_free(_clientsList.clients[clientNum].context);
		}
	}
}
