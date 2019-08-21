all: 
	gcc -c -o main.o main.c -Wall
	gcc -c -o mqtt_connect.o mqtt_connect.c -Wall
	gcc -o ts_owrt_module main.o mqtt_connect.o -lmosquitto