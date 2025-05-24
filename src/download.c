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
    
    // Convert the first address in h_addr_list to a string
    struct in_addr addr;
    memcpy(&addr, h->h_addr_list[0], sizeof(struct in_addr));
    strcpy(ip, inet_ntoa(addr));
    
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

    return 0;
}