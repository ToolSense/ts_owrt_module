#include "client_queue.h"
#include "logger.h"

/**
 * Global queue struct
*/
ClientQueue _queue;

/**
 * Clear queue
*/
void queueCleen()
{
	int i;

	_queue.pos_100ms = 0;
	_queue.pos_1m    = 0;
	_queue.pos_1s    = 0;

	for(i = 0; i < MAX_CLIENT_NUM; i++)
	{
		_queue.rate_100ms[i] = -1;
		_queue.rate_1m[i]    = -1;
		_queue.rate_1s[i]    = -1;
	}
}

/**
 * Add client idx
 *
 * @param InnerIdx idx of client
 * @return true - ok, false - error
*/
bool queueAdd(RefreshRate rate, int clientId)
{
	int pos;

	switch(rate)
	{
		case RR_100ms:

			// Try to find free cell
			for(pos = 0; pos < MAX_CLIENT_NUM; pos++)
			{
				if(_queue.rate_100ms[pos] == -1)
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
				if(_queue.rate_1s[pos] == -1)
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
				if(_queue.rate_1m[pos] == -1)
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

/**
 * Get first client num
 *
 * @param RefreshRate Refresh Rate of client
 * @return client num - ok, -1 - error
*/
int queueGetFirst(RefreshRate rate)
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

/**
 * Get next client num
 *
 * @param RefreshRate Refresh Rate of client
 * @return client num - ok, -1 - error
*/
int queueGetNext(RefreshRate rate)
{
	switch(rate)
	{
		case RR_100ms:
			_queue.pos_100ms++;

			if(_queue.pos_100ms == MAX_CLIENT_NUM)
			{
				_queue.pos_100ms = 0;
				return -1;
			}

			return _queue.rate_100ms[_queue.pos_100ms];

		case RR_1s:
			_queue.pos_1s++;

			if(_queue.pos_1s == MAX_CLIENT_NUM)
			{
				_queue.pos_1s = 0;
				return -1;
			}

			return _queue.rate_1s[_queue.pos_1s];

		case RR_1m:
			_queue.pos_1m++;

			if(_queue.pos_1m == MAX_CLIENT_NUM)
			{
				_queue.pos_1m = 0;
				return -1;
			}

			return _queue.rate_1m[_queue.pos_1m];
	}

	LOG_E("Wrong rate");
	return 0;
}








