all : bin/client bin/server

bin/server : src/server.c src/shifumi.c
	gcc -o bin/server src/server.c -pthread -w -std=c99 -D_GNU_SOURCE

bin/client : src/client.c src/shifumi.c
	gcc -o bin/client src/client.c -pthread -w -std=c99 -D_GNU_SOURCE
