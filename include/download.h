#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

int parse(char *url, char *user, char *password, char *host, char *path, char *filename);
int getip(char *host, char *ip);
int connect_to_server(char *ip, int port);