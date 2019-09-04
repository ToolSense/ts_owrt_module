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
	const char       *pIpBuff;
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

		config_setting_lookup_int(modbusClient, "id", &_modbusSettings.clients[i].id);
		config_setting_lookup_int(modbusClient, "port", &_modbusSettings.clients[i].port);
		config_setting_lookup_int(modbusClient, "offset", &_modbusSettings.clients[i].offset);
		config_setting_lookup_int(modbusClient, "bytesToRead", &_modbusSettings.clients[i].bytesToRead);
		config_setting_lookup_string(modbusClient, "ipAdress", &pIpBuff);

		snprintf(_modbusSettings.clients[i].ipAdress, IP_BUF_SIZE, "%s", pIpBuff);
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
		return MBE_INIT;
	}	

	if(_modbusSettings.clientsCnt == 0)
	{
		fprintf(stderr, "modbusInit: ERROR: No clients\n");
		return MBE_CLIENT;
	}
	else if(_modbusSettings.clientsCnt > MAX_CLIENT_NUM)
	{
		fprintf(stderr, "modbusInit: ERROR: Too much clients, max  %d\n", MAX_CLIENT_NUM);
		return MBE_CLIENT;
	}

	if(MODBUS_DEBUG)
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

			return MBE_CONTEXT;
		}
		
		// Connect
		if (modbus_connect(_modbusSettings.clients[clientNum].context) == -1) 
		{
			fprintf(stderr, "modbusInit: ERROR: Connection failed: %s, for ip: %s, port: %d\n", modbus_strerror(errno),
					_modbusSettings.clients[clientNum].ipAdress,
					_modbusSettings.clients[clientNum].port);

			mbStatus = MBE_FAIL;
			continue;
		}

		modbus_set_slave(_modbusSettings.clients[clientNum].context, _modbusSettings.clients[clientNum].id);

		_modbusSettings.clients[clientNum].connected = true;
		atLeastOne = true;

		if(MODBUS_DEBUG)
			printf("Connected to ip: %s, port: %d\n", 
					_modbusSettings.clients[clientNum].ipAdress,
					_modbusSettings.clients[clientNum].port);
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
			pDataList->clients[clientNum].clientId = _modbusSettings.clients[clientNum].id;

			rc = modbus_read_registers(_modbusSettings.clients[clientNum].context, 
									   _modbusSettings.clients[clientNum].offset, 
									   _modbusSettings.clients[clientNum].bytesToRead, 
									   pDataList->clients[clientNum].data);

			if (rc == -1) 
			{
				fprintf(stderr, "modbusReceiveData: ERROR: Recive data: %s, from ip: %s, port: %d\n", modbus_strerror(errno), 
					    _modbusSettings.clients[clientNum].ipAdress, 
					    _modbusSettings.clients[clientNum].port);

				_modbusSettings.clients[clientNum].connected = false;
				mbStatus = MBE_FAIL;
				continue;	
			}

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
		else
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
				// modbus_close(_modbusSettings.clients[clientNum].context); // svv Segmentation fault
				// modbus_free(_modbusSettings.clients[clientNum].context);  // svv Segmentation fault

				// Get context
				_modbusSettings.clients[clientNum].context = modbus_new_tcp(_modbusSettings.clients[clientNum].ipAdress, 
																			 _modbusSettings.clients[clientNum].port);	
				
				if (_modbusSettings.clients[clientNum].context == NULL) 
				{
					fprintf(stderr, "ERROR: Unable to allocate libmodbus context for ip: %s, port: %d\n", 
							_modbusSettings.clients[clientNum].ipAdress,
							_modbusSettings.clients[clientNum].port);

					return MBE_CONTEXT;
				}
 
				// Connect
				if (modbus_connect(_modbusSettings.clients[clientNum].context) == -1) 
				{
					fprintf(stderr, "ERROR: Connection failed: %s, for ip: %s, port: %d\n", modbus_strerror(errno),
							_modbusSettings.clients[clientNum].ipAdress,
							_modbusSettings.clients[clientNum].port);

					mbStatus = MBE_FAIL;
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

				return MBE_INIT;
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
		}
	}
}