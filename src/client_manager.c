#include "client_manager.h"
#include "logger.h"

#if __BYTE_ORDER == __BIG_ENDIAN
	#define IS_BIG_ENDIAN 1
#elif __BYTE_ORDER == __LITTLE_ENDIAN
	#define IS_BIG_ENDIAN 0
#else
     #error "Wrong endian type! file: modbus_connect.c"
#endif

#define SIZEOF_UINT16_T 2

int _clientsCnt;
ClientSettings _clientSettings[MAX_CLIENT_NUM+1];

/**
 * Fill type specific fields (data_int, data_dword ...) in ModbusClientData
 *
 * @param  pData pointer no client data
 * @return true - ok, false - wrong type
 */
bool modbusToClientData(ModbusData *pData, DataType dataType, ClientData *pClientData)
{
	int numToCopy, servicePos, dataPos;
	uint16_t serviceData[MAX_RCV_DATA_LEN];


	// Fill data according to type
	switch(dataType)
	{
		case MDT_BOOL:
			pClientData->data_bool = pData->data[0] != 0;
			break;

		case MDT_INT:

			numToCopy = sizeof(int) / SIZEOF_UINT16_T;

			if(IS_BIG_ENDIAN)
				for(servicePos = 0, dataPos = numToCopy - 1; servicePos < numToCopy; servicePos++, dataPos--)
					serviceData[servicePos] = pData->data[dataPos];
			else
				memcpy(&serviceData, &(pData->data), numToCopy * SIZEOF_UINT16_T);

			memcpy(&(pClientData->data_int), &serviceData, sizeof(int));
			break;

		case MDT_DWORD:

			numToCopy = sizeof(DWORD) / SIZEOF_UINT16_T;

			if(IS_BIG_ENDIAN)
				for(servicePos = 0, dataPos = numToCopy - 1; servicePos < numToCopy; servicePos++, dataPos--)
					serviceData[servicePos] = pData->data[dataPos];
			else
				memcpy(&serviceData, &(pData->data), numToCopy * SIZEOF_UINT16_T);

			memcpy(&(pClientData->data_dword), &serviceData, sizeof(DWORD));
			break;

		case MDT_TIME:

			numToCopy = sizeof(time_t) / SIZEOF_UINT16_T;

			if(IS_BIG_ENDIAN)
				for(servicePos = 0, dataPos = numToCopy - 1; servicePos < numToCopy; servicePos++, dataPos--)
					serviceData[servicePos] = pData->data[dataPos];
			else
				memcpy(&serviceData, &(pData->data), numToCopy * SIZEOF_UINT16_T);

			memcpy(&(pClientData->data_time), &serviceData, sizeof(time_t));

			break;

		case MDT_ENUM:

			numToCopy = sizeof(int) / SIZEOF_UINT16_T;

			if(IS_BIG_ENDIAN)
				for(servicePos = 0, dataPos = numToCopy - 1; servicePos < numToCopy; servicePos++, dataPos--)
					serviceData[servicePos] = pData->data[dataPos];
			else
				memcpy(&serviceData, &(pData->data), numToCopy * SIZEOF_UINT16_T);

			memcpy(&(pClientData->data_enum), &serviceData, sizeof(int));

			break;

		default:
			LOG_E("Wrong data type");
			return false;
	}

	return true;
}

/**
 * Init data from config file
 *
 * @param  config_t config file
 * @return true - ok, false - error
 */
bool manager_init_config(config_t cfg)
{
	int i;
	const char       *pDataType;
	const char       *pModbusType;
	const char       *pRefreshRate;
	const char       *pProtocol;
	config_setting_t *clientConf;
	config_setting_t *client;

	clientConf = config_lookup(&cfg, "clients");

	if(clientConf == NULL)
	{
		LOG_E("No client settings");
		return false;
	}

	_clientsCnt = config_setting_length(clientConf);

	if(_clientsCnt == 0)
	{
		LOG_E("No clients in settings file");
		return false;
	}

	if(_clientsCnt > MAX_CLIENT_NUM)
	{
		LOG_E("Too many clients, got %d, max %d", _clientsCnt, MAX_CLIENT_NUM);

		return false;
	}

	// Init all clients
	for(i = 0; i < _clientsCnt; i++)
	{
		client = config_setting_get_elem(clientConf, i);

		// Common settings
		_clientSettings[i].innerIdx = i;
		if(config_setting_lookup_string(client, "name",         &_clientSettings[i].name) != CONFIG_TRUE){
			LOG_E("Wrong parameter name, client counter %d", i);
			return false;
		}

		if(config_setting_lookup_string(client, "protocol",     &pProtocol) != CONFIG_TRUE){
			LOG_E("Wrong parameter protocol, client: %s", _clientSettings[i].name);
			return false;
		}

		if(config_setting_lookup_string(client, "refresh_rate ", &pRefreshRate) != CONFIG_TRUE){
			LOG_E("Wrong parameter refresh_rate, client: %s", _clientSettings[i].name);
			return false;
		}

		if(config_setting_lookup_string(client, "data_type",     &pDataType) != CONFIG_TRUE){
			LOG_E("Wrong parameter data_type, client: %s", _clientSettings[i].name);
			return false;
		}

		if(config_setting_lookup_string(client, "unit",          &_clientSettings[i].unit) != CONFIG_TRUE){
			LOG_E("Wrong parameter unit, client: %s", _clientSettings[i].name);
			return false;
		}

		if(strlen(_clientSettings[i].name) > MAX_CLIENT_NAME_LEN)
		{
			LOG_E("Client name too big, max %d", MAX_CLIENT_NAME_LEN);
				return false;
		}

		if(strlen(_clientSettings[i].unit) > MAX_UNIT_NAME_LEN)
		{
			LOG_E("Unit name too big, max %d", MAX_UNIT_NAME_LEN);
				return false;
		}

		// Parse refresh rate
		if(strcmp(pRefreshRate, "100ms") == 0)
			_clientSettings[i].refreshRate = RR_100ms;
		else if(strcmp(pRefreshRate, "1s") == 0)
			_clientSettings[i].refreshRate = RR_1s;
		else if(strcmp(pRefreshRate, "1m") == 0)
			_clientSettings[i].refreshRate = RR_1m;
		else
		{
			LOG_E("Wrong refresh rate: %s, client: %s",
					pRefreshRate, _clientSettings[i].name);
			return false;
		}

		// Parse data type
		if (strcmp(pDataType, "int") == 0){
			_clientSettings[i].dataType = MDT_INT;
		}
		else if (strcmp(pDataType, "dword") == 0) {
			_clientSettings[i].dataType = MDT_DWORD;
		}
		else if (strcmp(pDataType, "bool") == 0) {
			_clientSettings[i].dataType = MDT_BOOL;
		}
		else if (strcmp(pDataType, "time") == 0) {
			_clientSettings[i].dataType = MDT_TIME;
		}
		else if (strcmp(pDataType, "enum") == 0) {
			_clientSettings[i].dataType = MDT_ENUM;
		}
		else
		{
			LOG_E("Wrong data type: %s, client: %s",
				  pDataType, _clientSettings[i].name);
			return false;
		}

		// Parse protocol
		if(strcmp(pProtocol, "modbus") == 0)
		{
			_clientSettings[i].protocol = CP_MODBUS;
		}
		else if(strcmp(pModbusType, "opc") == 0)
		{
			_clientSettings[i].protocol = CP_OPC;
		}
		else
		{
			LOG_E("Wrong protocol type: %s, client: %s",
					pProtocol, _clientSettings[i].name);
			return false;
		}

		// Protocol specific
		if(_clientSettings[i].protocol == CP_MODBUS)
		{
			// Modbus settings
			if(config_setting_lookup_int(client,    "mb_id",         &_clientSettings[i].modbus.id) != CONFIG_TRUE){
				LOG_E("Wrong parameter mb_id, client: %s", _clientSettings[i].name);
				return false;
			}

			if(config_setting_lookup_string(client, "mb_type",       &pModbusType) != CONFIG_TRUE){
				LOG_E("Wrong parameter mb_type, client: %s", _clientSettings[i].name);
				return false;
			}


			// Parse modbus type
			if(strcmp(pModbusType, "TCP") == 0)
			{
				_clientSettings[i].modbus.protocolType = MPT_TCP;
				if(config_setting_lookup_int(client,    "mb_port", &_clientSettings[i].modbus.port) != CONFIG_TRUE){
					LOG_E("Wrong parameter mb_port, client: %s", _clientSettings[i].name);
					return false;
				}

				if(config_setting_lookup_string(client, "mb_ip",   &_clientSettings[i].modbus.ipAdress) != CONFIG_TRUE){
					LOG_E("Wrong parameter mb_ip, client: %s", _clientSettings[i].name);
					return false;
				}
			}
			else if(strcmp(pModbusType, "RTU") == 0)
			{
				_clientSettings[i].modbus.protocolType = MPT_RTU;
				if(config_setting_lookup_string(client, "mb_device",    &_clientSettings[i].modbus.device) != CONFIG_TRUE){
					LOG_E("Wrong parameter mb_device, client: %s", _clientSettings[i].name);
					return false;
				}

				if(config_setting_lookup_string(client, "mb_parity",    &_clientSettings[i].modbus.parity) != CONFIG_TRUE){
					LOG_E("Wrong parameter mb_parity, client: %s", _clientSettings[i].name);
					return false;
				}

				if(config_setting_lookup_int(client,    "mb_baud_rate", &_clientSettings[i].modbus.baudRate) != CONFIG_TRUE){
					LOG_E("Wrong parameter mb_baud_rate, client: %s", _clientSettings[i].name);
					return false;
				}

				if(config_setting_lookup_int(client,    "mb_data_bit",  &_clientSettings[i].modbus.dataBit) != CONFIG_TRUE){
					LOG_E("Wrong parameter mb_data_bit, client: %s", _clientSettings[i].name);
					return false;
				}

				if(config_setting_lookup_int(client,    "mb_stop_bit",  &_clientSettings[i].modbus.stopBit) != CONFIG_TRUE){
					LOG_E("Wrong parameter mb_stop_bit, client: %s", _clientSettings[i].name);
					return false;
				}
			}

			// Parse data type
			if (_clientSettings[i].dataType == MDT_INT){
				_clientSettings[i].modbus.registersToRead = 2;
			}
			else if (_clientSettings[i].dataType == MDT_DWORD) {
				_clientSettings[i].modbus.registersToRead = 4;
			}
			else if (_clientSettings[i].dataType == MDT_BOOL) {
				_clientSettings[i].modbus.registersToRead = 1;
			}
			else if (_clientSettings[i].dataType == MDT_TIME) {
				_clientSettings[i].modbus.registersToRead = 4;
			}
			else if (_clientSettings[i].dataType == MDT_ENUM) {
				_clientSettings[i].modbus.registersToRead = 2;
			}
			else
			{
				LOG_E("Wrong data type for modbus, client: %s",
					  pDataType, _clientSettings[i].name);
				return false;
			}

			// Offset
			_clientSettings[i].modbus.offset = 0; // Default for all clients
		}
		else if(_clientSettings[i].protocol == CP_OPC)
		{
			if(config_setting_lookup_int(client,    "opc_ns",     &_clientSettings[i].opc.nameSpace) != CONFIG_TRUE){
				LOG_E("Wrong parameter opc_ns, client: %s", _clientSettings[i].name);
				return false;
			}

			if(config_setting_lookup_string(client, "opc_node",   &_clientSettings[i].opc.node) != CONFIG_TRUE){
				LOG_E("Wrong parameter opc_node, client: %s", _clientSettings[i].name);
				return false;
			}

			if(config_setting_lookup_string(client, "opc_server", &_clientSettings[i].opc.serverName) != CONFIG_TRUE){
				LOG_E("Wrong parameter opc_server, client: %s", _clientSettings[i].name);
				return false;
			}

			_clientSettings[i].opc.dataType = _clientSettings[i].dataType;
		}
	}

	return true;
}

/**
 * Connect to all clients
 *
 * @return true - ok, false - error
 */
bool manager_init_connections()
{
	int i;
	bool allClientsConnected = true;

	// For all clients
	for(i = 0; i < _clientsCnt; i++)
	{
		if(_clientSettings[i].protocol == CP_MODBUS)
		{
			// Connect
			if(modbusConnect(&(_clientSettings[i].modbus)))
			{
				_clientSettings[i].connected = true;
				LOG("Client: %s connected OK",_clientSettings[i].name);
			}
			else
			{
				_clientSettings[i].connected = false;
				allClientsConnected = false;
				LOG("Client: %s connection FAIL", _clientSettings[i].name);
				continue;
			}
		}
		else if(_clientSettings[i].protocol == CP_OPC)
		{
			if(opcConnect(_clientSettings[i].opc.serverName))
			{
				_clientSettings[i].connected = true;
				LOG("Client: %s connected OK",_clientSettings[i].name);
			}
			else
			{
				_clientSettings[i].connected = false;
				allClientsConnected = false;
				LOG("Client: %s connection FAIL", _clientSettings[i].name);
				continue;
			}
		}
		else
		{
			LOG_E("Wrong protocol, client: %s", _clientSettings[i].name);
				return false;
		}
	}

	if(!allClientsConnected)
		return false;

	return true;
}

/**
 * Get client ptr
 *
 * @param InnerIdx idx of client
 * @return ptr - ok, NULL - error
 */
const ClientSettings* manager_get_client(InnerIdx innerIdx)
{
	if((innerIdx < 0) || (innerIdx > _clientsCnt-1))
		return NULL;

	return &(_clientSettings[innerIdx]);
}

/**
 * Receive data from client
 *
 * @param InnerIdx idx of client
 * @param ClientData receiving data
 * @return true - ok, false - error
 */
bool manager_receive_data_simple(InnerIdx innerIdx, ClientData *pClientData)
{
	if((innerIdx < 0) || (innerIdx > _clientsCnt-1))
	{
		LOG_E("Wrong innerIdx: %d, min 0, max %d, client: %s",
				innerIdx, _clientsCnt-1, _clientSettings[innerIdx].name);
			return false;
	}

	if(!_clientSettings[innerIdx].connected)
	{
		LOG_E("Client: %s not connected, need reconnect", _clientSettings[innerIdx].name);
	}

	if(_clientSettings[innerIdx].protocol == CP_MODBUS)
	{
		ModbusData data;

		// Receive data
		if(modbusReceiveData(&(_clientSettings[innerIdx].modbus), &data))
		{
			// Convert data
			if(!modbusToClientData(&data, _clientSettings[innerIdx].dataType, pClientData))
			{
				LOG_E("Can't convert data, client: %s", _clientSettings[innerIdx].name);
				return false;
			}
		}
		else
		{
			LOG_E("Can't receive data, client: %s", _clientSettings[innerIdx].name);
			_clientSettings[innerIdx].connected = false;
			return false;
		}
	}
	else if(_clientSettings[innerIdx].protocol == CP_OPC)
	{
		if(!opcReceiveData(&(_clientSettings[innerIdx].opc), pClientData))
		{
			LOG_E("Can't receive data, client: %s", _clientSettings[innerIdx].name);
			_clientSettings[innerIdx].connected = false;
			return false;
		}
	}
	else
	{
		LOG_E("Wrong protocol");
		return false;
	}

	return true;
}

/**
 * Reconnect to client
 *
 * @param InnerIdx idx of client
 * @return true - ok, false - error
 */
bool manager_reconnect(InnerIdx innerIdx)
{
	if((innerIdx < 0) || (innerIdx > _clientsCnt-1))
	{
		LOG_E("Wrong innerIdx: %d, min 0, max %d, client: %s",
				innerIdx, _clientsCnt-1, _clientSettings[innerIdx].name);
			return false;
	}

	// Reconnect
	if(_clientSettings[innerIdx].protocol == CP_MODBUS)
	{
		if( modbusReconnect(&(_clientSettings[innerIdx].modbus)) )
		{
			LOG("client: %s, reconnect OK", _clientSettings[innerIdx].name);
			_clientSettings[innerIdx].connected = true;
		}
		else
		{
			LOG_E("Can't reconnect, client: %s", _clientSettings[innerIdx].name);
			_clientSettings[innerIdx].connected = false;
			return false;
		}

	}
	else if(_clientSettings[innerIdx].protocol == CP_OPC)
	{
		opcCloseConnection();
		if(opcConnect(_clientSettings[innerIdx].opc.serverName))
		{
			LOG("client: %s, reconnect OK", _clientSettings[innerIdx].name);
			_clientSettings[innerIdx].connected = true;
		}
		else
		{
			LOG_E("Can't reconnect, client: %s", _clientSettings[innerIdx].name);
			_clientSettings[innerIdx].connected = false;
			return false;
		}
	}
	else
	{
		LOG_E("Wrong protocol");
		return false;
	}

	return true;
}

/**
 * Receive data from client, reconnect if error
 *
 * @param InnerIdx idx of client
 * @param ClientData receiving data
 * @return true - ok, false - error
 */
bool manager_receive_data(InnerIdx innerIdx, ClientData *pClientData, char* pClientName, char* pUnit)
{
	if((innerIdx < 0) || (innerIdx > _clientsCnt-1))
	{
		LOG_E("Wrong innerIdx: %d, min 0, max %d, client: %s",
				innerIdx, _clientsCnt-1, _clientSettings[innerIdx].name);
			return false;
	}

	// Check client connection state
	if(!_clientSettings[innerIdx].connected)
	{
		LOG_E("Client: %s not connected, try to reconnect", _clientSettings[innerIdx].name);

		// Reconnect
		if(!manager_reconnect(innerIdx))
		{
			LOG_E("Reconnection error, client: %s", _clientSettings[innerIdx].name);
			return false;
		}
	}

	// Receive data
	if(!manager_receive_data_simple(innerIdx, pClientData))
	{
		LOG_E("Error receiving data, client: %s, try to reconnect", _clientSettings[innerIdx].name);

		_clientSettings[innerIdx].connected = false;

		// Reconnect
		if(!manager_reconnect(innerIdx))
		{
			LOG_E("Reconnection error, client: %s", _clientSettings[innerIdx].name);
			return false;
		}

		// Receive data
		if(!manager_receive_data_simple(innerIdx, pClientData))
		{
			LOG_E("Error receiving data, client: %s", _clientSettings[innerIdx].name);
			return false;
		}

		_clientSettings[innerIdx].connected = true;
	}

	if(pClientName)
		memcpy(pClientName, _clientSettings[innerIdx].name, strlen(_clientSettings[innerIdx].name));

	if(pUnit)
		memcpy(pUnit, _clientSettings[innerIdx].unit, strlen(_clientSettings[innerIdx].unit));

	return true;
}

/**
 * Close all connections
 *
 */
void manager_close_all_connections()
{
	int i;
	bool opcClose = false;

	// For all clients
	for(i = 0; i < _clientsCnt; i++)
	{
		if(_clientSettings[i].protocol == CP_MODBUS)
		{
			modbusCloseConnection(&(_clientSettings[i].modbus));
		}
		else if(_clientSettings[i].protocol == CP_OPC)
		{
			if(!opcClose)
			{
				opcCloseConnection();
				opcClose = true;
			}
		}
		else
		{
			LOG_E("Wrong protocol");
		}

		_clientSettings[i].connected = false;
	}
}
