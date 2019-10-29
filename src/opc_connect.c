#include "opc_connect.h"
#include "logger.h"

UA_Client *_handler;


int opc_test()
{
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));

    // Connect to a server
    /* anonymous connect would be: retval = UA_Client_connect(client, "opc.tcp://localhost:4840"); */
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://Vlad_Note:53530/OPCUA/SimulationServer");
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return EXIT_FAILURE;
    }

    /* Read attribute */
    UA_Double value = 0;

    UA_Variant *val = UA_Variant_new();
    retval = UA_Client_readValueAttribute(client, UA_NODEID_STRING(3, "test1"), val);

    if(retval == UA_STATUSCODE_GOOD)
    {
    	value = *(UA_Double*)val->data;
    	printf("OPC OK, value: %lf\n", value);
    }
    else
    {
    	printf("OPC Fail\n");
    }

    UA_Variant_delete(val);

    return 1;
}

bool opcConnect(const char *pServerName)
{
	_handler = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(_handler));

    // Connect to a server, example "opc.tcp://Vlad_Note:53530/OPCUA/SimulationServer"
    UA_StatusCode retval = UA_Client_connect(_handler, pServerName);
    if(retval != UA_STATUSCODE_GOOD)
    {
        UA_Client_delete(_handler);
        LOG_E("Can't connect to server %s", pServerName);
        return false;
    }

    return true;
}

bool opcReceiveData(OpcClient *pClient, ClientData *pData)
{
    UA_Variant *val = UA_Variant_new();
    UA_StatusCode retval;

    char nodeName[MAX_OPC_NODE_LEN + 1] = {'\0'};

    if(strlen(pClient->node) > MAX_OPC_NODE_LEN)
    {
    	LOG_E("Too big node name: %s, max %d chars", pClient->node, MAX_OPC_NODE_LEN);
    	return false;
    }

    retval = UA_Client_readValueAttribute(_handler, UA_NODEID_STRING(pClient->nameSpace, nodeName), val);

    if(retval == UA_STATUSCODE_GOOD)
    {
    	pData->dataType = pClient->dataType;

    	switch(pData->dataType)
    	{
    		case MDT_BOOL:
    			pData->data_bool = *(UA_Boolean*)val->data;
    			break;

    		case MDT_INT:
    		    pData->data_int = *(UA_UInt32*)val->data;
    		    break;


       		case MDT_DWORD:
        		 pData->data_dword = *(UA_Double*)val->data;
        		 break;

       		case MDT_ENUM:
       		     pData->data_enum = *(UA_UInt32*)val->data; // svv check it
       		     break;

       		default:
       			LOG_E("Unsupported data type %d, node: %s\n", (int)pData->dataType, pClient->node);
       			retval = UA_STATUSCODE_BADDATATYPEIDUNKNOWN;
    	}
    }
    else
    {
    	LOG_E("Fail to read attribute %d; %s\n", pClient->nameSpace, pClient->node);
    }

    UA_Variant_delete(val);

    return (retval == UA_STATUSCODE_GOOD);
}

void opcCloseConnection()
{
	if(_handler != NULL)
	{
		UA_Client_delete(_handler);
		_handler = NULL;
	}
}
