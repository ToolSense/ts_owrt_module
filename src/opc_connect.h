#ifndef OPC_CONNECT_H
#define OPC_CONNECT_H

#include "open62541.h"
#include "ts_module_const.h"

/**
 * Client info
 */
typedef struct
{
	int nameSpace;		// Data name space
	const char *node;   // Name of node
	DataType dataType;  // Type of data
} OpcClient;

/**
 * Client info
 */
typedef struct
{
	DataType   dataType;    // Type of data

	UA_Boolean data_bool;	// Data in boolean format
	UA_UInt32  data_int;	// Data in integer format
	UA_Double  data_double;	// Data in double  format
} OpcData;

bool opcConnect(const char *pServerName)
bool opcReceiveData(OpcClient *pClient, OpcData *pData);
void opcCloseConnection();

#endif //OPC_CONNECT_H
