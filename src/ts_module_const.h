#define DELAY_SEND_TIME 5 //sec
#define MAX_BUF 128

// Data alias
#define TIME 0
#define TEMP 1

/**
 * Data type
 */

typedef enum
{
	TS_BOOL  = 1,		// Boolean
	TS_INT   = 2,		// Integer
	TS_DWORD = 3,		// Double word
	TS_LINT  = 4,		// Long integer
	TS_TIME  = 5,		// Time
	TS_ENUM  = 6		// Enumerate
} TSDataType;