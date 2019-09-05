#include <stdio.h>
#include <string.h>
#include <mosquitto.h>
#include "mqtt_connect.h"

struct mosquitto *mosq = NULL;

int mqtt_init(config_t cfg)
{
	int keepalive = 60;
	bool clean_session = true;
	struct mqttServerSetting mqttSetting;
	int rc = 0;

	config_setting_t *mqtt_conf;

	mqtt_conf = config_lookup(&cfg, "mqtt");
	if(mqtt_conf != NULL)
	{
		int count = config_setting_length(mqtt_conf);
		printf("mqtt_conf %d\n", count);
		config_setting_t *mqtt_cloud = config_setting_get_elem(mqtt_conf, 0);

		// mqttSetting.host = "m15.cloudmqtt.com";
		// mqttSetting.port = PORT_SSL;
		// mqttSetting.login = "thmcoslv";
		// mqttSetting.passwd = "odUivT2WEIsW";
		// mqttSetting.ssl_crt = "/etc/ssl/certs/ca-certificates.crt";

		config_setting_lookup_string(mqtt_cloud, "host", (const char**)&mqttSetting.host);
		config_setting_lookup_string(mqtt_cloud, "login", (const char**)&mqttSetting.login);
		config_setting_lookup_string(mqtt_cloud, "passwd", (const char**)&mqttSetting.passwd);
		config_setting_lookup_string(mqtt_cloud, "ssl_crt", (const char**)&mqttSetting.ssl_crt);
		config_setting_lookup_int(mqtt_cloud, "port", &mqttSetting.port);
	}

	mosquitto_lib_init();

	mosq = mosquitto_new(NULL, clean_session, NULL);

	if(!mosq){
		printf("Error: Out of memory.\n");
		return 1;
	}

	mosquitto_username_pw_set(mosq, mqttSetting.login, mqttSetting.passwd);

	rc = mosquitto_tls_set(mosq, mqttSetting.ssl_crt, NULL, NULL, NULL, NULL);
	if (rc) {
		printf ("set ssl failed %d\n", rc);
		return 1;
	}

	rc = mosquitto_tls_insecure_set(mosq, true);

	if (rc) {
		printf ("set insecure failed %d\n", rc);
		return 1;
	}	

	if(mosquitto_connect(mosq, mqttSetting.host, mqttSetting.port, keepalive)){
		printf("Unable to connect.\n");
		return 1;
	}

	int loop = mosquitto_loop_start(mosq);
	if(loop != MOSQ_ERR_SUCCESS){
		printf("Unable to start loop: %i\n", loop);
		return 1;
	}

	return 0;
}

int mqtt_send(char *msg, char *topic)
{
	int rc = mosquitto_publish(mosq, NULL, topic, strlen(msg), msg, 0, 0);
	printf("mqtt_send error=%i\n", rc);
	return rc;
}