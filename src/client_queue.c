#include "client_queue.h"
#include "logger.h"

/**
 * Global queue struct
 */
ClientQueue _queue;

bool initQueue(config_t cfg)
{
	int i;
	int clientsCnt;
	int tempId;
	const char *pTempRate;
	config_setting_t *modbusConf;
	config_setting_t *modbusClient;

	memset(&_queue, '\0', sizeof(ClientQueue));

	modbusConf = config_lookup(&cfg, "modbus");

	if(modbusConf == NULL)
	{
		LOG_E("No modbus settings");
		return false;
	}

	clientsCnt = config_setting_length(modbusConf);

	if(clientsCnt == 0)
	{
		LOG_E("No modbus clients in settings file");
		return false;
	}

	if(clientsCnt > MAX_CLIENT_NUM)
	{
		LOG_E("Too many clients, got %d, max %d", clientsCnt, MAX_CLIENT_NUM);
		return false;
	}

	// Init all clients
	for(i = 0; i < clientsCnt; i++)
	{
		modbusClient = config_setting_get_elem(modbusConf, i);

		config_setting_lookup_int(modbusClient,    "id",          &tempId);
		config_setting_lookup_string(modbusClient,    "refreshRate", &pTempRate);

		// Parse refresh rate
		if(strcmp(pTempRate, "100ms") == 0)
		{
			if(!addToQueue(RR_100ms, tempId))
			{
				LOG_E("Can't add to queue, client ID %s", tempId);
				return false;
			}
		}
		else if(strcmp(pTempRate, "1s") == 0)
		{
			if(!addToQueue(RR_1s, tempId))
			{
				LOG_E("Can't add to queue, client ID %s", tempId);
				return false;
			}
		}
		else if(strcmp(pTempRate, "1m") == 0)
		{
			if(!addToQueue(RR_1m, tempId))
			{
				LOG_E("Can't add to queue, client ID %s", tempId);
				return false;
			}
		}
		else
		{
			LOG_E("Wrong refresh rate: %s, client ID: %d",
					pTempRate, tempId);
			return false;
		}
	}

	return true;
}

bool addToQueue(ModbusRefreshRate rate, int clientId)
{
	int pos;

	switch(rate)
	{
		case RR_100ms:

			// Try to find free cell
			for(pos = 0; pos < MAX_CLIENT_NUM; pos++)
			{
				if(_queue.rate_100ms[pos] == '\0')
					break;

				if(pos == (MAX_CLIENT_NUM-1))
				{
					// Last cell not empty
					LOG_E("Attempt to add too many clients to queue, client id %d", clientId);
					return false;
				}
			}

			_queue.rate_100ms[pos] = clientId;
			break;


		case RR_1s:

			// Try to find free cell
			for(pos = 0; pos < MAX_CLIENT_NUM; pos++)
			{
				if(_queue.rate_1s[pos] == '\0')
					break;

				if(pos == (MAX_CLIENT_NUM-1))
				{
					// Last cell not empty
					LOG_E("Attempt to add too many clients to queue, client id %d", clientId);
					return false;
				}
			}

			_queue.rate_1s[pos] = clientId;
			break;

		case RR_1m:

			// Try to find free cell
			for(pos = 0; pos < MAX_CLIENT_NUM; pos++)
			{
				if(_queue.rate_1m[pos] == '\0')
					break;

				if(pos == (MAX_CLIENT_NUM-1))
				{
					// Last cell not empty
					LOG_E("Attempt to add too many clients to queue, client id %d", clientId);
					return false;
				}
			}

			_queue.rate_1m[pos] = clientId;
			break;
	}

	return true;
}

int getFirstClientId(ModbusRefreshRate rate)
{
	switch(rate)
	{
		case RR_100ms:
			_queue.pos_100ms = 0;
			return _queue.rate_100ms[_queue.pos_100ms];

		case RR_1s:
			_queue.pos_1s = 0;
			return _queue.rate_1s[_queue.pos_1s];

		case RR_1m:
			_queue.pos_1m = 0;
			return _queue.rate_1m[_queue.pos_1m];
	}

	LOG_E("Wrong rate");
	return 0;
}

int getNextClientId(ModbusRefreshRate rate)
{
	switch(rate)
	{
		case RR_100ms:
			_queue.pos_100ms++;

			if(_queue.pos_100ms == MAX_CLIENT_NUM)
			{
				_queue.pos_100ms = 0;
				return 0;
			}

			return _queue.rate_100ms[_queue.pos_100ms];

		case RR_1s:
			_queue.pos_1s++;

			if(_queue.pos_1s == MAX_CLIENT_NUM)
			{
				_queue.pos_1s = 0;
				return 0;
			}

			return _queue.rate_1s[_queue.pos_1s];

		case RR_1m:
			_queue.pos_1m++;

			if(_queue.pos_1m == MAX_CLIENT_NUM)
			{
				_queue.pos_1m = 0;
				return 0;
			}

			return _queue.rate_1m[_queue.pos_1m];
	}

	LOG_E("Wrong rate");
	return 0;
}















