#ifndef CLIENT_MANAGER_H
#define CLIENT_MANAGER_H

#include <stdbool.h>
#include <libconfig.h>
#include "ts_module_const.h"
#include "modbus_connect.h"
#include "opc_connect.h"

/**
 * Refresh rate
 */
typedef enum
{
	CP_MODBUS,
	CP_OPC
} ClientProtocol;

/**
 * Client info
 */
typedef struct
{
	const char     *name;  			// Name(Data Point, Variable)
	const char     *unit;			 // Unit (h, kg, cm, m/s ...)
	bool           connected;		 // Connected sign, changed automatically
	InnerIdx       innerIdx; 		// Inner index 0 - MAX_CLIENT_NUM
	RefreshRate    refreshRate;		// Time to refresh data
	ClientProtocol protocol;
	DataType       dataType;        // Type of receive data
	ModbusClient   modbus;
	OpcClient      opc;
} ClientSettings;

bool manager_init_config(config_t cfg);
bool manager_init_connections();
bool manager_reconnect(InnerIdx innerIdx);
bool manager_receive_data(InnerIdx innerIdx, ClientData *pClientData, char* pClientName, char* pUnit);
bool manager_receive_data_simple(InnerIdx innerIdx, ClientData *pClientData);
const ClientSettings* manager_get_client(InnerIdx innerIdx);
void manager_close_all_connections();

#endif //CLIENT_MANAGER_H
