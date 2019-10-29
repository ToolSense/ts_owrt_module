#ifndef TS_MODULE_CONST_H
#define TS_MODULE_CONST_H

#include <time.h>

#define MAX_BUF 128

#define SYSLOG

// Data alias
#define TIME 0
#define TEMP 1

/**
 * Data type
 */

#define	TS_BOOL  1		// Boolean
#define	TS_INT   2		// Integer
#define	TS_DWORD 3		// Double word
#define	TS_LINT  4		// Long integer
#define	TS_TIME  5		// Time
#define	TS_ENUM  6		// Enumerate

#define MAX_CLIENT_NUM      10  // Max numbers of client, min 0 - max 128
#define MAX_CLIENT_NAME_LEN 80  // Max client name len
#define MAX_UNIT_NAME_LEN   20  // Max unit name
#define MAX_OPC_NODE_LEN    80  // Max opc ua node name len

typedef unsigned int DWORD;
typedef int InnerIdx;

/**
 * Refresh rate
 */
typedef enum
{
	RR_100ms,
	RR_1s,
	RR_1m
} RefreshRate;

/**
 * Data type
 */
typedef enum
{
	MDT_BOOL,		// Boolean
	MDT_INT,		// Integer
	MDT_DWORD,		// Double word
	MDT_TIME,		// Time
	MDT_ENUM		// Enumerate
} DataType;

/**
 * Client data
 */
typedef struct
{
	DataType dataType;		// Type of data

	bool     data_bool;		// Data in boolean format
	int      data_int;		// Data in integer format
	DWORD    data_dword;	// Data in double word format
	time_t   data_time;		// Data in time_t format
	int      data_enum;		// Data in enumeration format
} ClientData;

// typedef enum
// {
// 	TS_BOOL  = 1,		// Boolean
// 	TS_INT   = 2,		// Integer
// 	TS_DWORD = 3,		// Double word
// 	TS_LINT  = 4,		// Long integer
// 	TS_TIME  = 5,		// Time
// 	TS_ENUM  = 6		// Enumerate
// } TSDataType;

#endif // TS_MODULE_CONST_H
