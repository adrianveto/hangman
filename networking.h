#include <sys/socket.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/select.h>
#define MAX_CLIENTS 8
#define WORD_SIZE 128
#define COMMAND_SIZE 128
#define MESSAGE_SIZE 512

#ifndef NETWORKING_H
#define NETWORKING_H
void error(int number, char* message);
int server_setup();
int client_tcp_handshake(char * server_address);
int server_tcp_handshake(int listen_socket);
#endif