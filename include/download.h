#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <ctype.h>

int parse(char *url, char *user, char *password, char *host, char *path, char *filename);
int getip(char *host, char *ip);
int connect_to_server(char *ip, int port);

int ftp_read_response(int sockfd, char *response, size_t response_size);
int ftp_login(int sockfd, const char *user, const char *password);
int ftp_passive_mode(int sockfd, char *ip, int *port);
int ftp_retrieve_file(int control_sockfd, int data_sockfd, const char *path, const char *filename);
int ftp_quit(int sockfd);