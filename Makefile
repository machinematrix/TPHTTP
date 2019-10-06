CC = g++

all : HttpServer.o Vector.o main.c
	cc -o Server.out main.c Thread.o HttpServer.o Vector.o -pthread

#Usar select en vez de threads
HttpServer.o : Thread.o Vector.o HttpServer.c HttpServer.h
	cc -c HttpServer.c -D USE_SELECT

Thread.o : Thread.h Thread.c
	cc -c Thread.c

Vector.o : Vector.h Vector.c
	cc -c Vector.c