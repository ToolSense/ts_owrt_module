#include <libconfig.h>

struct mqttServerSetting
{
	char *host;
	int port;
	char *login;
	char *passwd;
	char *ssl_crt; //crt certificate file
};

int mqtt_init(config_t cfg);
int mqtt_send(char *msg, char *topic);