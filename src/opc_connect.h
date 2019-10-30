#ifndef OPC_CONNECT_H
#define OPC_CONNECT_H

#include "open62541.h"
#include "ts_module_const.h"

/**
 * Client info
 */
typedef struct
{
	int nameSpace;			// Data name space
	const char *node;   	// Name of node
	const char *serverName; // Name of server
	DataType dataType;  	// Type of data
} OpcClient;

bool opcConnect(const char *pServerName);
bool opcReceiveData(OpcClient *pClient, ClientData *pData);
void opcCloseConnection();

#endif //OPC_CONNECT_H
