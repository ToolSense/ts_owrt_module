#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <endian.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include <netdb.h>


#include "logger.h"
#include "modbus_connect.h"

bool checkServer(const char *pIpAdress, int port)
{
    int sockfd = 0;
    struct sockaddr_in serv_addr;

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        LOG_E("Could not create socket");
        return false;
    }

    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if(inet_pton(AF_INET, pIpAdress, &serv_addr.sin_addr)<=0)
    {
        LOG_E("inet_pton error occured");
        return false;
    }

    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0 )
    {
       LOG_E("Socket connection fail");
       return false;
    }

    shutdown(sockfd, 2);

    return true;
}

/**
 * Init modbus connection
 *
 * @param  ModbusClient client pointer
 * @return true - ok, false - error
 */
bool modbusConnect(ModbusClient *pClient)
{

	pClient->context = NULL;

	// Different connection according to protocol
	if(pClient->protocolType == MPT_TCP)
	{
		// Get context
		pClient->context = modbus_new_tcp(pClient->ipAdress, pClient->port);

		if (pClient->context == NULL)
		{
			LOG_E("Unable to allocate libmodbus context for ip: %s, port: %d",
					pClient->ipAdress,
					pClient->port);

			return false;
		}

		// Check connection
		if(!checkServer(pClient->ipAdress, pClient->port))
		{
			LOG_E("No connection to ip: %s, port: %d", pClient->ipAdress, pClient->port);
			return false;
		}

		// Connect
		if (modbus_connect(pClient->context) != 0)
		{
			LOG_E("Modbus protocol connection error: %s, for ip: %s, port: %d", modbus_strerror(errno),
					pClient->ipAdress,
					pClient->port);
		}

		modbus_set_slave(pClient->context, pClient->id);
	}
	else // Modbus RTU protocol
	{
		// Get context
		pClient->context = modbus_new_rtu(  pClient->device,
											pClient->baudRate,
											pClient->parity[0],
											pClient->dataBit,
											pClient->stopBit);

		if (pClient->context == NULL)
		{
			LOG_E("Unable to allocate libmodbus context for device: %s, baud: %d",
					pClient->device,
					pClient->baudRate);

			return false;
		}

		// Connect
		if (modbus_connect(pClient->context) != 0)
		{
			LOG_E("Connection failed error: %s, for mb client id: %s, device: %s, baud: %d",
					modbus_strerror(errno),
					pClient->id,
					pClient->device,
					pClient->baudRate);
		}

		modbus_set_slave(pClient->context, pClient->id);
	}

	return true;
}

/**
 * Receive data from one client
 *
 * @param  ModbusClient     client
 * @param  ModbusClientData struct for result
 * @return true - ok, false - error
 */
bool modbusReceiveData(ModbusClient *pClient, ModbusData *pData)
{
	int  rc;

	memset(pData, '\0', sizeof(ModbusData));

	rc = modbus_read_registers(pClient->context,
							   pClient->offset,
							   pClient->registersToRead,
							   pData->data);

	if (rc < 0)
	{
		if(pClient->protocolType == MPT_TCP)
		{
			LOG_E("Recive data error: %s, from ip: %s, port: %d", modbus_strerror(errno),
					pClient->ipAdress,
					pClient->port);
		}
		else // Modbus RTU
		{
			LOG_E("Recive data error: %s, mb client id: %s, device: %s, baud: %d",
				  modbus_strerror(errno),
				  pClient->id,
				  pClient->device,
				  pClient->baudRate);
		}

		return false;
	}

	pData->len = pClient->registersToRead;

	return true;
}

/**
 * Close and open all connections
 *
 * @param  ModbusClient client
 * @return true - ok, false - error
 */
bool modbusReconnect(ModbusClient *pClient)
{

	if(pClient->context != NULL)
	{
		// Check connection
		if(!checkServer(pClient->ipAdress, pClient->port))
		{
			LOG_E("No connection to ip: %s, port: %d", pClient->ipAdress, pClient->port);
			return false;
		}

		// Connect
		if (modbus_connect(pClient->context) != 0)
		{
			if(pClient->protocolType == MPT_TCP)
			{
				LOG_E("Modbus protocol connection failed: %s, for ip: %s, port: %d", modbus_strerror(errno),
						pClient->ipAdress,
						pClient->port);
			}
			else
			{
				LOG_E("Connection failed: %s, for mb client id %d, device: %s, baud: %d",
				modbus_strerror(errno),
				pClient->id,
				pClient->device,
				pClient->baudRate);
			}

			return false;
		}

		modbus_set_slave(pClient->context, pClient->id);
	}
	else
	{
		if(pClient->protocolType == MPT_TCP)
		{
			LOG_E("No context, need reinit, client ip: %s, port: %d",
					pClient->ipAdress,
					pClient->port);
		}
		else
		{
			LOG_E("No context, need reinit mb client id %d, device: %s, baud: %d",
					pClient->id,
					pClient->device,
					pClient->baudRate);
		}

		return false;
	}

	return true;
}

/**
 * Close connection and deinit client
 *
 * @param  ModbusClient client
 */
void modbusCloseConnection(ModbusClient *pClient)
{
	if(pClient->context != NULL)
	{
		modbus_close(pClient->context);
		modbus_free(pClient->context);
		pClient->context = NULL;
	}
}
