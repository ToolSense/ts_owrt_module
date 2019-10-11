#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include "logger.h"
#include "modbus_connect.h"

ModbusSettings _modbusSettings;

/**
 * Read settings from file to _modbusSettings
 *
 * @param  pSettingsFileName .ini file name "/tmp/settings.ini"
 * @return true - init ok, false - init fail
 */
bool modbusInitSettings(config_t cfg)
{
	int i;
	const char       *pDataType;
	const char       *pProtocolType;
	config_setting_t *modbusConf;
	config_setting_t *modbusClient;

	modbusConf = config_lookup(&cfg, "modbus");

	if(modbusConf == NULL)
	{
		LOG_E("No modbus settings");
		return false;
	}

	_modbusSettings.clientsCnt = config_setting_length(modbusConf);

	if(_modbusSettings.clientsCnt == 0)
	{
		LOG_E("No modbus clients in settings file");
		return false;
	}

	if(_modbusSettings.clientsCnt > MAX_CLIENT_NUM)
	{
		LOG_E("Too many clients, got %d, max %d", _modbusSettings.clientsCnt, MAX_CLIENT_NUM);

		return false;
	}

	// Init all clients
	for(i = 0; i < _modbusSettings.clientsCnt; i++)
	{
		modbusClient = config_setting_get_elem(modbusConf, i);

		config_setting_lookup_int(modbusClient,    "id",            &_modbusSettings.clients[i].id);
		config_setting_lookup_int(modbusClient,    "refreshRateMs", &_modbusSettings.clients[i].refreshRateMs);
		config_setting_lookup_string(modbusClient, "unit",          &_modbusSettings.clients[i].unit);
		config_setting_lookup_string(modbusClient, "name",          &_modbusSettings.clients[i].name);
		config_setting_lookup_string(modbusClient, "protocol",      &pProtocolType);
		config_setting_lookup_string(modbusClient, "dataType",      &pDataType);

		if(strcmp(pProtocolType, "TCP") == 0)
		{
			_modbusSettings.clients[i].protocolType = MPT_TCP;
			config_setting_lookup_int(modbusClient,    "port",          &_modbusSettings.clients[i].port);
			config_setting_lookup_string(modbusClient, "ipAdress",      &_modbusSettings.clients[i].ipAdress);
		}
		else if(strcmp(pProtocolType, "RTU") == 0)
		{
			_modbusSettings.clients[i].protocolType = MPT_RTU;
			config_setting_lookup_string(modbusClient, "device",         &_modbusSettings.clients[i].device);
			config_setting_lookup_string(modbusClient, "parity",         &_modbusSettings.clients[i].parity);
			config_setting_lookup_int(modbusClient,    "baudRate",       &_modbusSettings.clients[i].baudRate);
			config_setting_lookup_int(modbusClient,    "dataBit",        &_modbusSettings.clients[i].dataBit);
			config_setting_lookup_int(modbusClient,    "stopBit",        &_modbusSettings.clients[i].stopBit);
		}

		// Offset
		_modbusSettings.clients[i].offset = 0; // Default for all clients

		// Parse data type
		if (strcmp(pDataType, "int") == 0){
			_modbusSettings.clients[i].dataType = MDT_INT;
			_modbusSettings.clients[i].registersToRead = 2;
		}
		else if (strcmp(pDataType, "dword") == 0) {
			_modbusSettings.clients[i].dataType = MDT_DWORD;
			_modbusSettings.clients[i].registersToRead = 4;
		}
		else if (strcmp(pDataType, "bool") == 0) {
			_modbusSettings.clients[i].dataType = MDT_BOOL;
			_modbusSettings.clients[i].registersToRead = 1;	
		}
		else if (strcmp(pDataType, "time") == 0) {
			_modbusSettings.clients[i].dataType = MDT_TIME;
			_modbusSettings.clients[i].registersToRead = 4;	
		}
		else if (strcmp(pDataType, "enum") == 0) { 
			_modbusSettings.clients[i].dataType = MDT_ENUM;
			_modbusSettings.clients[i].registersToRead = 2;
		}
		else
		{
			LOG_E("Wrong data type: %s in client with ip: %s",
				  pDataType, _modbusSettings.clients[i].ipAdress);
			return false;	
		}				
	}

	return true;
}

/**
 * Fill type specific fields (data_int, data_dword ...) in ModbusClientData
 *
 * @param  pData pointer no client data
 * @return true - ok, false - wrong type
 */
bool modbusFillDataType(ModbusClientData *pData)
{
		// Fill data according to type
		switch(pData->dataType)
		{
			case MDT_BOOL:
				pData->data_bool = pData->data[0] != 0;
				break;

			case MDT_INT:
				memcpy(&(pData->data_int), pData->data, sizeof(int));
				break;

			case MDT_DWORD:
				memcpy(&(pData->data_dword), pData->data, sizeof(DWORD));
				break;

			case MDT_TIME:
				memcpy(&(pData->data_time), pData->data, sizeof(time_t));
				break;

			case MDT_ENUM:
				memcpy(&(pData->data_enum), pData->data, sizeof(int));
				break;

			default:
				LOG_E("Wrong data type");
				return false;
		}

		return true;
}

/**
 * Init modbus connection
 *
 * @param  pSettingsFileName .ini file name "/tmp/settings.ini"
 * @return MBE_OK - init ok, else - error
 */
ModbusError modbusInit(config_t cfg)
{
	int  clientNum;
	bool atLeastOne = false;
	ModbusError mbStatus = MBE_OK;


	memset(&_modbusSettings, '\0', sizeof(ModbusSettings));

	//modbus_new_rtu("/dev", 115200, 'N', 8, 1);
	// Get settings from file to _modbusSettings
	if(!modbusInitSettings(cfg))
	{
		LOG_E("Can't init");
		return MBE_FAIL;
	}	

	if(_modbusSettings.clientsCnt == 0)
	{
		LOG_E("No clients");
		return MBE_FAIL;
	}
	else if(_modbusSettings.clientsCnt > MAX_CLIENT_NUM)
	{
		LOG_E("Too much clients, max  %d", MAX_CLIENT_NUM);
		return MBE_FAIL;
	}

	LOG("Find %d clients", _modbusSettings.clientsCnt);

	// Use _modbusSettings for connection
	for(clientNum = 0; clientNum < _modbusSettings.clientsCnt; clientNum++)
	{
		// Default value
		_modbusSettings.clients[clientNum].context   = NULL;
		_modbusSettings.clients[clientNum].connected = false;

		// Different connection according to protocol
		if(_modbusSettings.clients[clientNum].protocolType == MPT_TCP)
		{
			// Get context
			_modbusSettings.clients[clientNum].context = modbus_new_tcp(_modbusSettings.clients[clientNum].ipAdress,
																    	_modbusSettings.clients[clientNum].port);

			if (_modbusSettings.clients[clientNum].context == NULL)
			{
				LOG_E("Unable to allocate libmodbus context for ip: %s, port: %d",
						_modbusSettings.clients[clientNum].ipAdress,
						_modbusSettings.clients[clientNum].port);

				return MBE_FAIL;
			}

			// Connect
			if (modbus_connect(_modbusSettings.clients[clientNum].context) == -1)
			{
				LOG_E("Connection failed: %s, for ip: %s, port: %d", modbus_strerror(errno),
						_modbusSettings.clients[clientNum].ipAdress,
						_modbusSettings.clients[clientNum].port);

				mbStatus = MBE_CONNECT;
				continue;
			}

			modbus_set_slave(_modbusSettings.clients[clientNum].context, _modbusSettings.clients[clientNum].id);

			_modbusSettings.clients[clientNum].connected = true;

			LOG("Connected to ip: %s, port: %d",
				_modbusSettings.clients[clientNum].ipAdress,
				_modbusSettings.clients[clientNum].port);

		}
		else // Modbus RTU protocol
		{
			// Get context
			_modbusSettings.clients[clientNum].context = modbus_new_rtu(_modbusSettings.clients[clientNum].device,
																		_modbusSettings.clients[clientNum].baudRate,
																		_modbusSettings.clients[clientNum].parity[0],
																		_modbusSettings.clients[clientNum].dataBit,
																		_modbusSettings.clients[clientNum].stopBit);

			if (_modbusSettings.clients[clientNum].context == NULL)
			{
				LOG_E("Unable to allocate libmodbus context for device: %s, baud: %d",
						_modbusSettings.clients[clientNum].device,
						_modbusSettings.clients[clientNum].baudRate);

				return MBE_FAIL;
			}

			// Connect
			if (modbus_connect(_modbusSettings.clients[clientNum].context) == -1)
			{
				LOG_E("Connection failed error: %s, for client: %s, device: %s, baud: %d",
						modbus_strerror(errno),
						_modbusSettings.clients[clientNum].name,
						_modbusSettings.clients[clientNum].device,
						_modbusSettings.clients[clientNum].baudRate);

				mbStatus = MBE_CONNECT;
				continue;
			}

			modbus_set_slave(_modbusSettings.clients[clientNum].context, _modbusSettings.clients[clientNum].id);

			_modbusSettings.clients[clientNum].connected = true;

			LOG("Connected to client: %s, device: %s, baud: %d",
				_modbusSettings.clients[clientNum].name,
				_modbusSettings.clients[clientNum].device,
				_modbusSettings.clients[clientNum].baudRate);
		}
		
		atLeastOne = true;
	}

	// Check status
	if(mbStatus != MBE_OK)
	{
		if(atLeastOne)
			return MBE_NOT_ALL;
		else if(mbStatus == MBE_CONNECT)
			return MBE_CONNECT;
		else
			return MBE_FAIL;
	}

	return MBE_OK;
}

/**
 * Receive data from all clients
 *
 * @param  pDataList struct for result
 * @return MBE_OK - init ok, else - error
 */
ModbusError modbusReceiveData(ModbusClientsDataList *pDataList)
{
	int rc;
	int  clientNum;
	bool atLeastOne = false;
	ModbusError mbStatus = MBE_OK;

	memset(pDataList, '\0', sizeof(ModbusClientsDataList));

	for(clientNum = 0; clientNum < _modbusSettings.clientsCnt; clientNum++)
	{
		if(_modbusSettings.clients[clientNum].connected)
		{
			pDataList->clients[clientNum].id = _modbusSettings.clients[clientNum].id;

			// Copy all info
			pDataList->clients[clientNum].id       = _modbusSettings.clients[clientNum].id;
			pDataList->clients[clientNum].name     = _modbusSettings.clients[clientNum].name;
			pDataList->clients[clientNum].unit     = _modbusSettings.clients[clientNum].unit;
			pDataList->clients[clientNum].dataType = _modbusSettings.clients[clientNum].dataType;


			rc = modbus_read_registers(_modbusSettings.clients[clientNum].context, 
									   _modbusSettings.clients[clientNum].offset, 
									   _modbusSettings.clients[clientNum].registersToRead, 
									   pDataList->clients[clientNum].data);

			if (rc == -1) 
			{
				if(_modbusSettings.clients[clientNum].protocolType == MPT_TCP)
				{
					LOG_E("Recive data error: %s, from ip: %s, port: %d", modbus_strerror(errno),
							_modbusSettings.clients[clientNum].ipAdress,
							_modbusSettings.clients[clientNum].port);
				}
				else // Modbus RTU
				{
					LOG_E("Recive data error: %s, client: %s, device: %s, baud: %d",
						  modbus_strerror(errno),
						  _modbusSettings.clients[clientNum].name,
						  _modbusSettings.clients[clientNum].device,
						  _modbusSettings.clients[clientNum].baudRate);
				}

				_modbusSettings.clients[clientNum].connected = false;
				mbStatus = MBE_CONNECT;
				continue;	
			}

			// Fill data according to type
			if( !modbusFillDataType(&(pDataList->clients[clientNum])) )
			{
				LOG_E("Convert error");
				return MBE_FAIL;
			}

			pDataList->clients[clientNum].received = true;
			atLeastOne = true;
		}
		else
		{
			if(_modbusSettings.clients[clientNum].protocolType == MPT_TCP)
			{
				LOG_E("Not connected client ip: %s, port: %d, need reconnect",
						_modbusSettings.clients[clientNum].ipAdress,
						_modbusSettings.clients[clientNum].port);
			}
			else
			{
				LOG_E("Not connected client %s, device: %s, baud: %d",
				_modbusSettings.clients[clientNum].name,
				_modbusSettings.clients[clientNum].device,
				_modbusSettings.clients[clientNum].baudRate);
			}
				
			mbStatus = MBE_FAIL;			
		}
	}

	// Check status
	if(mbStatus != MBE_OK)
	{
		if(atLeastOne)
			return MBE_NOT_ALL;
		else if(mbStatus == MBE_CONNECT)
			return MBE_CONNECT;
		else
			return MBE_FAIL;
	}

	return MBE_OK;
}

/**
 * Receive data from one client
 *
 * @param  pDataList struct for result
 * @return MBE_OK - init ok, else - error
 */
ModbusError modbusReceiveDataId(ModbusClientData *pData, int id)
{
	int  rc;
	int  clientNum;
	bool found = false;

	memset(pData, '\0', sizeof(ModbusClientData));

	// Search for client
	for(clientNum = 0; clientNum < _modbusSettings.clientsCnt; clientNum++)
	{
		if(_modbusSettings.clients[clientNum].id == id)
		{
			found = true;
			break;
		}
	}

	if(!found)
	{
		LOG_E("No client with id: %d", id);
		return MBE_FAIL;
	}

	if(_modbusSettings.clients[clientNum].connected)
	{
		// Copy all info
		pData->id       = _modbusSettings.clients[clientNum].id;
		pData->name     = _modbusSettings.clients[clientNum].name;
		pData->unit     = _modbusSettings.clients[clientNum].unit;
		pData->dataType = _modbusSettings.clients[clientNum].dataType;

		rc = modbus_read_registers(_modbusSettings.clients[clientNum].context, 
								   _modbusSettings.clients[clientNum].offset, 
								   _modbusSettings.clients[clientNum].registersToRead, 
								   pData->data);

		if (rc == -1) 
		{
			if(_modbusSettings.clients[clientNum].protocolType == MPT_TCP)
			{
				LOG_E("Recive data error: %s, from ip: %s, port: %d", modbus_strerror(errno),
						_modbusSettings.clients[clientNum].ipAdress,
						_modbusSettings.clients[clientNum].port);
			}
			else // Modbus RTU
			{
				LOG_E("Recive data error: %s, client: %s, device: %s, baud: %d",
					  modbus_strerror(errno),
					  _modbusSettings.clients[clientNum].name,
					  _modbusSettings.clients[clientNum].device,
					  _modbusSettings.clients[clientNum].baudRate);
			}

			_modbusSettings.clients[clientNum].connected = false;
			return MBE_CONNECT;	
		}

		// Fill data according to type
		if(!modbusFillDataType(pData))
		{
			LOG_E("Convert error");
			return MBE_FAIL;
		}

		pData->received = true;
	}
	else
	{
		if(_modbusSettings.clients[clientNum].protocolType == MPT_TCP)
		{
			LOG_E("Not connected client ip: %s, port: %d, need reconnect",
					_modbusSettings.clients[clientNum].ipAdress,
					_modbusSettings.clients[clientNum].port);
		}
		else
		{
			LOG_E("Not connected client %s, device: %s, baud: %d",
			_modbusSettings.clients[clientNum].name,
			_modbusSettings.clients[clientNum].device,
			_modbusSettings.clients[clientNum].baudRate);
		}

		return MBE_FAIL;			
	}

	return MBE_OK;
}

/**
 * Close and open all connections
 *
 * @return MBE_OK - init ok, else - error
 */

ModbusError modbusReconnect()
{
	int  clientNum;
	bool atLeastOne = false;
	ModbusError mbStatus = MBE_OK;

	for(clientNum = 0; clientNum < _modbusSettings.clientsCnt; clientNum++)
	{
		if(_modbusSettings.clients[clientNum].connected == false)
		{	
			// Client not connected
			if(_modbusSettings.clients[clientNum].context != NULL)	
			{
				// Connect
				if (modbus_connect(_modbusSettings.clients[clientNum].context) == -1) 
				{
					if(_modbusSettings.clients[clientNum].protocolType == MPT_TCP)
					{
						LOG_E("Connection failed: %s, for ip: %s, port: %d", modbus_strerror(errno),
								_modbusSettings.clients[clientNum].ipAdress,
								_modbusSettings.clients[clientNum].port);
					}
					else
					{
						LOG_E("Connection failed: %s, for client %s, device: %s, baud: %d",
						modbus_strerror(errno),
						_modbusSettings.clients[clientNum].name,
						_modbusSettings.clients[clientNum].device,
						_modbusSettings.clients[clientNum].baudRate);
					}

					mbStatus = MBE_CONNECT;
					continue;
				}

				modbus_set_slave(_modbusSettings.clients[clientNum].context, _modbusSettings.clients[clientNum].id);	
				_modbusSettings.clients[clientNum].connected = true;

				LOG("Client %s reconnected", _modbusSettings.clients[clientNum].name);
			}
			else
			{
				if(_modbusSettings.clients[clientNum].protocolType == MPT_TCP)
				{
					LOG_E("No context, need reinit, client ip: %s, port: %d",
							_modbusSettings.clients[clientNum].ipAdress,
							_modbusSettings.clients[clientNum].port);
				}
				else
				{
					LOG_E("No context, need reinit client %s, device: %s, baud: %d",
					_modbusSettings.clients[clientNum].name,
					_modbusSettings.clients[clientNum].device,
					_modbusSettings.clients[clientNum].baudRate);
				}

				return MBE_FAIL;
			}
		}
	}

	// Check status
	if(mbStatus != MBE_OK)
	{
		if(atLeastOne)
			return MBE_NOT_ALL;
		else if(mbStatus == MBE_CONNECT)
			return MBE_CONNECT;
		else
			return MBE_FAIL;
	}

	return MBE_OK;	
}

/**
 * Close all connections and deinit all clients 
 *
 */
void modbusDeinit()
{
	int  clientNum;

	for(clientNum = 0; clientNum < _modbusSettings.clientsCnt; clientNum++)
	{
		if(_modbusSettings.clients[clientNum].context != NULL)	
		{
			modbus_close(_modbusSettings.clients[clientNum].context);	
			modbus_free(_modbusSettings.clients[clientNum].context);
			_modbusSettings.clients[clientNum].context = NULL;
		}
	}
}
