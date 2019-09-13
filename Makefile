CC = g++

all : main.c threadObject http
	cc -o Server.out main.c Thread.o HttpServer.o -pthread

http : HttpServer.c HttpServer.h
	cc -c HttpServer.c

threadObject : Thread.h Thread.c
	cc -c Thread.c