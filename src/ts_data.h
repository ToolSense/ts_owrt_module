#include <stdbool.h>
#include <libconfig.h>

#define TS_BOOL 1
#define TS_INT 2
#define TS_LINT 3


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