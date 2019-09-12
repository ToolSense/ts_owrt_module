#include <stdbool.h>
#include <libconfig.h>
#include "ts_module_const.h"


typedef struct DataStruct
{
	char *alias;
	int type;
	bool b_data;
	int i_data;
	long int l_data;
} t_data;

int init_count(int *count, config_t cfg);

int init_data(t_data data[], config_t cfg);

int get_json(char *buf, t_data data[], int count);