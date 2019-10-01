#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
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
	config_setting_t *modbusConf;
	config_setting_t *modbusClient;

	modbusConf = config_lookup(&cfg, "modbus");

	if(modbusConf == NULL)
	{
		fprintf(stdout, "modbusInitSettings: ERROR: No modbus settings\n");
		return false;
	}

	_modbusSettings.clientsCnt = config_setting_length(modbusConf);

	if(_modbusSettings.clientsCnt == 0)
	{
		fprintf(stdout, "modbusInitSettings: ERROR: No modbus clients in settings file\n");
		return false;
	}

	if(_modbusSettings.clientsCnt > MAX_CLIENT_NUM)
	{
		fprintf(stdout, "modbusInitSettings: ERROR: too many clients, got %d, max %d\n",
			    _modbusSettings.clientsCnt, 
			    MAX_CLIENT_NUM);

		return false;
	}

	// Init all clients
	for(i = 0; i < _modbusSettings.clientsCnt; i++)
	{
		modbusClient = config_setting_get_elem(modbusConf, i);

		config_setting_lookup_int(modbusClient,    "id",            &_modbusSettings.clients[i].id);
		config_setting_lookup_int(modbusClient,    "port",          &_modbusSettings.clients[i].port);
		config_setting_lookup_int(modbusClient,    "refreshRateMs", &_modbusSettings.clients[i].refreshRateMs);
		config_setting_lookup_string(modbusClient, "unit",          &_modbusSettings.clients[i].unit);
		config_setting_lookup_string(modbusClient, "ipAdress",      &_modbusSettings.clients[i].ipAdress);
		config_setting_lookup_string(modbusClient, "name",          &_modbusSettings.clients[i].name);
		config_setting_lookup_string(modbusClient, "dataType",      &pDataType);

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
			fprintf(stderr, "modbusInitSettings: ERROR: Wrong data type: %s in client with ip: %s \n", 
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
				fprintf(stderr, "modbusFillDataType: ERROR: Wrong data type\n");
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

	// Get settings from file to _modbusSettings
	if(!modbusInitSettings(cfg))
	{
		fprintf(stderr, "modbusInit: ERROR: Can't init\n");
		return MBE_FAIL;
	}	

	if(_modbusSettings.clientsCnt == 0)
	{
		fprintf(stderr, "modbusInit: ERROR: No clients\n");
		return MBE_FAIL;
	}
	else if(_modbusSettings.clientsCnt > MAX_CLIENT_NUM)
	{
		fprintf(stderr, "modbusInit: ERROR: Too much clients, max  %d\n", MAX_CLIENT_NUM);
		return MBE_FAIL;
	}

	printf("Find %d clients\n", _modbusSettings.clientsCnt);

	// Use _modbusSettings for connection
	for(clientNum = 0; clientNum < _modbusSettings.clientsCnt; clientNum++)
	{
		// Default value
		_modbusSettings.clients[clientNum].context   = NULL;
		_modbusSettings.clients[clientNum].connected = false;

		// Get context
		_modbusSettings.clients[clientNum].context = modbus_new_tcp(_modbusSettings.clients[clientNum].ipAdress, 
																    _modbusSettings.clients[clientNum].port);	
		if (_modbusSettings.clients[clientNum].context == NULL) 
		{
			fprintf(stderr, "modbusInit: ERROR: Unable to allocate libmodbus context for ip: %s, port: %d\n", 
					_modbusSettings.clients[clientNum].ipAdress,
					_modbusSettings.clients[clientNum].port);

			return MBE_FAIL;
		}
		
		// Connect
		if (modbus_connect(_modbusSettings.clients[clientNum].context) == -1) 
		{
			fprintf(stderr, "modbusInit: ERROR: Connection failed: %s, for ip: %s, port: %d\n", modbus_strerror(errno),
					_modbusSettings.clients[clientNum].ipAdress,
					_modbusSettings.clients[clientNum].port);

			mbStatus = MBE_CONNECT;
			continue;
		}

		modbus_set_slave(_modbusSettings.clients[clientNum].context, _modbusSettings.clients[clientNum].id);

		_modbusSettings.clients[clientNum].connected = true;
		atLeastOne = true;

		printf("Connected to ip: %s, port: %d\n",
				_modbusSettings.clients[clientNum].ipAdress,
				_modbusSettings.clients[clientNum].port);
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
				fprintf(stderr, "modbusReceiveData: ERROR: Recive data: %s, from ip: %s, port: %d\n", modbus_strerror(errno), 
					    _modbusSettings.clients[clientNum].ipAdress, 
					    _modbusSettings.clients[clientNum].port);

				_modbusSettings.clients[clientNum].connected = false;
				mbStatus = MBE_CONNECT;
				continue;	
			}

			// Fill data according to type
			if( !modbusFillDataType(&(pDataList->clients[clientNum])) )
			{
				fprintf(stderr, "ERROR: modbusReceiveData: Convert error\n");
				return MBE_FAIL;
			}

			pDataList->clients[clientNum].received = true;
			atLeastOne = true;
		}
		else
		{
			fprintf(stderr, "ERROR: Not connected client ip: %s, port: %d, need reconnect\n", 
					_modbusSettings.clients[clientNum].ipAdress, 
					_modbusSettings.clients[clientNum].port);

				
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
		fprintf(stderr, "modbusReceiveDataId: ERROR: No client with id: %d\n", id);
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
			fprintf(stderr, "modbusReceiveDataId: ERROR: Recive data: %s, from ip: %s, port: %d\n", modbus_strerror(errno), 
				    _modbusSettings.clients[clientNum].ipAdress, 
				    _modbusSettings.clients[clientNum].port);

			_modbusSettings.clients[clientNum].connected = false;
			return MBE_CONNECT;	
		}

		// Fill data according to type
		if(!modbusFillDataType(pData))
		{
			fprintf(stderr, "ERROR: modbusReceiveDataId: Convert error\n");
			return MBE_FAIL;
		}

		pData->received = true;
	}
	else
	{
		fprintf(stderr, "ERROR: Not connected client ip: %s, port: %d, need reconnect\n", 
				_modbusSettings.clients[clientNum].ipAdress, 
				_modbusSettings.clients[clientNum].port);

			
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
					fprintf(stderr, "ERROR: Connection failed: %s, for ip: %s, port: %d\n", modbus_strerror(errno),
							_modbusSettings.clients[clientNum].ipAdress,
							_modbusSettings.clients[clientNum].port);

					mbStatus = MBE_CONNECT;
					continue;
				}

				modbus_set_slave(_modbusSettings.clients[clientNum].context, _modbusSettings.clients[clientNum].id);	

				_modbusSettings.clients[clientNum].connected = true;
			}
			else
			{
				fprintf(stderr, "ERROR: No context, need reinit, client ip: %s, port: %d\n", 
					    _modbusSettings.clients[clientNum].ipAdress, 
					    _modbusSettings.clients[clientNum].port);

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
