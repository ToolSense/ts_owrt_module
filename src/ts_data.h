#include <stdbool.h>
#include <libconfig.h>
#include "ts_module_const.h"

/* Error values */
#define	DATA_ERR_SUCCESS 0
#define	DATA_ERR_CONF_LOOKUP 1
#define	DATA_ERR_COUNT 2
#define	DATA_ERR_JSON 3
#define	DATA_ERR_TYPE 4

typedef struct DataStruct
{
	char *alias;
	TSDataType type;
	bool b_data;
	int i_data;
	long int l_data;
} t_data;

int init_count(int *count, config_t cfg);

int init_data(t_data data[], config_t cfg);

int get_json(char *buf, t_data data[], int count);