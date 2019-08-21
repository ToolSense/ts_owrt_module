
struct mqttServerSetting
{
	char *host;
	int port;
	char *login;
	char *passwd;
	char *ssl_crt; //crt certificate file
};

int mqtt_init(struct mqttServerSetting mqttSetting);
int mqtt_send(char *msg, char *topic);