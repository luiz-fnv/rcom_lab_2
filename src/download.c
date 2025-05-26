#include "download.h"

int parse(char *url, char *user, char *password, char *host, char *path, char *filename) {
    // Parses and checks if the URL is ftp://[<user>:<password>@]<host>/<url-path>
    
    // Verify URL starts with "ftp://"
    if (strncmp(url, "ftp://", 6) != 0) {
        printf("Error: URL must start with 'ftp://'\n");
        return -1;
    }
    
    strcpy(user, "anonymous"); // Default user
    strcpy(password, "password"); // Default password

    // Skip "ftp://"
    char *ptr = url + 6;
    
    // Check if authentication info is present
    char *at_sign = strchr(ptr, '@');
    if (at_sign != NULL) {
        // Extract authentication info
        char auth[256];
        memset(auth, 0, sizeof(auth));
        strncpy(auth, ptr, at_sign - ptr);
        
        // Look for username:password separator
        char *colon = strchr(auth, ':');
        if (colon != NULL) {
            // Extract username
            strncpy(user, auth, colon - auth);
            user[colon - auth] = '\0';
            
            // Extract password
            strcpy(password, colon + 1);
        } else {
            // Only username provided
            strcpy(user, auth);
        }
        
        // Move ptr past the authentication part
        ptr = at_sign + 1;
    }
    
    // Extract hostname
    char *slash = strchr(ptr, '/');
    if (slash == NULL) {
        printf("Error: No path specified\n");
        return -1;
    }
    
    strncpy(host, ptr, slash - ptr);
    host[slash - ptr] = '\0';
    
    // Extract path
    strcpy(path, slash);
    
    // Extract filename from path
    char *last_slash = strrchr(path, '/');
    if (last_slash != NULL) {
        strcpy(filename, last_slash + 1);
    } else {
        strcpy(filename, path);
    }
    
    return 0;
}

int getip(char *host, char *ip) {
    // Gets the IP address from the hostname
    struct hostent *h;
    
    h = gethostbyname(host);
    if (h == NULL) {
        herror("gethostbyname");
        return -1;
    }
    
    struct in_addr addr;
    memcpy(&addr, h->h_addr_list[0], sizeof(struct in_addr));
    strcpy(ip, inet_ntoa(addr));
    
    return 0;
}

int connect_to_server(char *ip, int port) {
    int sockfd;
    struct sockaddr_in server_addr;
    
    /*server address handling*/
    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);    /*32 bit Internet address network byte ordered*/
    server_addr.sin_port = htons(port);             /*server TCP port must be network byte ordered */
    
    /*open a TCP socket*/
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        return -1;
    }
    
    /*connect to the server*/
    if (connect(sockfd, 
                (struct sockaddr *) &server_addr,
                sizeof(server_addr)) < 0) {
        perror("connect()");
        close(sockfd);
        return -1;
    }
    
    return sockfd;
}

int ftp_read_response(int sockfd, char *response, size_t response_size) {
    FILE *f = fdopen(sockfd, "r+");
    char *line = NULL;
    size_t linelen = 0;
    ssize_t nread;
    size_t used = 0;
    char code_str[4] = {0};
    char sep;

    nread = getline(&line, &linelen, f);

    memcpy(code_str, line, 3);
    int code = atoi(code_str);

    sep = line[3];  

    /* Copy first line into response buffer */
    if ((size_t)nread >= response_size) nread = response_size - 1;
    memcpy(response, line, nread);
    used = nread;
    response[used] = '\0';

    /* --- If multi-line, keep reading until you see "NNN " --- */
    if (sep == '-') {
        while (1) {
            nread = getline(&line, &linelen, f);

            /* Append this line (truncated if needed) */
            size_t to_copy = (used + nread < response_size)
                               ? nread
                               : response_size - 1 - used;
            memcpy(response + used, line, to_copy);
            used += to_copy;
            response[used] = '\0';

            /* Check for terminator: starts with the same "NNN " */
            if (nread >= 4 &&
                strncmp(line, code_str, 3) == 0 &&
                line[3] == ' ')
            {
                break;
            }
        }
    }

    free(line);
    return code;
}

int ftp_login(int sockfd, const char *user, const char *password) {
    printf("\n----- Starting FTP Login Process -----\n");
    char response[1024];
    int code;
    
    // Read welcome message
    printf("Waiting for welcome message...\n");
    code = ftp_read_response(sockfd, response, sizeof(response));
    if (code < 0) {
        return -1;
    }
    printf("Server response: %s\n", response);
    printf("Welcome code: %d\n", code);
    
    // Send USER command
    printf("\n----- Sending username -----\n");
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "USER %s\r\n", user);
    
    if (send(sockfd, cmd, strlen(cmd), 0) < 0) {
        perror("send() USER command");
        return -1;
    }
    printf("USER command sent\n");
    
    code = ftp_read_response(sockfd, response, sizeof(response));
    if (code != 331 && code != 230) {
        printf("Error: Username not accepted (code %d)\n", code);
        return -1;
    }
    printf("Username accepted with response code: %d\n", code);
    
    // Send PASS command if needed (code 331)
    if (code == 331) {
        printf("\n----- Sending password -----\n");
        snprintf(cmd, sizeof(cmd), "PASS %s\r\n", password);
        
        if (send(sockfd, cmd, strlen(cmd), 0) < 0) {
            perror("send() PASS command");
            return -1;
        }
        printf("PASS command sent\n");
        
        code = ftp_read_response(sockfd, response, sizeof(response));
        if (code != 230) {
            printf("Error: Password not accepted (code %d)\n", code);
            return -1;
        }
        printf("Password accepted with response code: %d\n", code);
    } else {
        printf("No password required (already logged in with response code: %d)\n", code);
    }
    
    printf("----- Login process completed successfully -----\n\n");
    return 0;
}

int ftp_passive_mode(int control_sockfd, char *ip, int *port) {
    char response[1024];
    int code;
    
    printf("\n----- Entering Passive Mode -----\n");
    
    // Send PASV command
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "PASV\r\n");
    
    if (send(control_sockfd, cmd, strlen(cmd), 0) < 0) {
        perror("send() PASV command");
        return -1;
    }
    printf("PASV command sent\n");
    
    // Read response
    code = ftp_read_response(control_sockfd, response, sizeof(response));
    if (code != 227) {
        printf("Error: Failed to enter passive mode (code %d)\n", code);
        return -1;
    }
    printf("Server response: %s\n", response);
    
    // Parse the response to get IP and port
    // Format: 227 Entering Passive Mode (h1,h2,h3,h4,p1,p2)
    char *start = strchr(response, '(');
    char *end = strchr(response, ')');
    if (start == NULL || end == NULL) {
        printf("Error: Invalid PASV response format\n");
        return -1;
    }
    
    start++; // Skip '('
    char pasv_data[64];
    memset(pasv_data, 0, sizeof(pasv_data));
    strncpy(pasv_data, start, end - start);
    
    // Parse the comma-separated values
    int ip1, ip2, ip3, ip4, port1, port2;
    if (sscanf(pasv_data, "%d,%d,%d,%d,%d,%d", &ip1, &ip2, &ip3, &ip4, &port1, &port2) != 6) {
        printf("Error: Failed to parse PASV response\n");
        return -1;
    }
    
    // Calculate the port number
    *port = port1 * 256 + port2;
    
    // Construct the IP address
    sprintf(ip, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);
    
    printf("Passive mode activated: IP=%s, Port=%d\n", ip, *port);
    printf("----- Passive Mode Setup Complete -----\n\n");
    
    return 0;
}

int main(int argc, char *argv[]) {

    if(argc < 2) {
        printf("Usage: %s <url>\n", argv[0]);
        return 1;
    }

    char user[256], password[256], host[256], path[256], filename[256], ip[16];

    int result = parse(argv[1], user, password, host, path, filename);
    if (result == 0) {
        printf("Parsed successfully:\n");
        printf("  User: %s\n", user);
        printf("  Password: %s\n", password);
        printf("  Host: %s\n", host);
        printf("  Path: %s\n", path);
        printf("  Filename: %s\n", filename);
    } else {
        printf("Failed to parse URL\n");
    }

    result = getip(host, ip);
    if (result == 0) {
        printf("IP Address of %s: %s\n", host, ip);
    } else {
        printf("Failed to get IP address for host %s\n", host);
    }

    int sockfd = connect_to_server(ip, 21); // FTP default port is 21
    if (sockfd < 0) {
        printf("Failed to connect to server %s on port 21\n", ip);
    } else {
        printf("Connected to server %s on port 21\n", ip);
    }

    if (ftp_login(sockfd, user, password) < 0) {
        printf("Failed to login to FTP server\n");
        close(sockfd);
        return 1;
    }
    printf("Logged in successfully\n");

    char passive_ip[16];
    int data_port;
    if (ftp_passive_mode(sockfd, passive_ip, &data_port) < 0) {
        printf("Failed to enter passive mode\n");
        close(sockfd);
        return 1;
    }
    printf("Passive mode established: IP=%s, Port=%d\n", passive_ip, data_port);

    close(sockfd);
    close(data_port);
    return 0;
}