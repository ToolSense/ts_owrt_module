all: 
	gcc -c -o main.o main.c -Wall
	gcc -c -o mqtt_connect.o mqtt_connect.c -Wall
	gcc -c -o mkjson.o mkjson.c -Wall
	gcc -o ts_owrt_module main.o mqtt_connect.o mkjson.o -lmosquitto