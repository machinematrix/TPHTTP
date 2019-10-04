CC = g++

all : http vectorObject main.c
	cc -o Server.out main.c Thread.o HttpServer.o Vector.o -pthread

select : httpSelect vectorObject main.c
	cc -o Server.out main.c Thread.o HttpServer.o Vector.o -pthread

http : threadObject vectorObject HttpServer.c HttpServer.h
	cc -c HttpServer.c

#Usar select en vez de threads
httpSelect : threadObject vectorObject HttpServer.c HttpServer.h
	cc -c HttpServer.c -D USE_SELECT

threadObject : Thread.h Thread.c
	cc -c Thread.c

vectorObject : Vector.h Vector.c
	cc -c Vector.c