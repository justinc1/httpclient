// http://www.theinsanetechie.in/2014/02/a-simple-http-client-and-server-in-c.html
/**httpclient.c**/
#include "stdio.h"
#include "stdlib.h"
#include "sys/types.h"
#include "sys/socket.h"
#include "string.h"
#include "netinet/in.h"
#include "netdb.h"

#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUF_SIZE 1024

int get_request(char * url, char * port);
int isValidIP(char * ip);

char path[1000];


/*extern "C"*/ pid_t gettid() __attribute((weak));
pid_t my_gettid() {
    if (gettid)
        return gettid();
    else
        return getpid();
}

#define debug(fmt, ...) if(1) { fprintf(stderr, "%s:%d tid=% 5d %s: " fmt, __FILE__, __LINE__, my_gettid(), __FUNCTION__, ##__VA_ARGS__); }


int main(int argc, char**argv) {
    struct sockaddr_in addr, cl_addr;
    int sockfd, ret;
    struct hostent * server;
    char * url, * temp;
    int portNumber;

    if (argc < 3) {
        printf("usage: [URL] [port number]\n");
        exit(1);
    }

    url = argv[1];
    portNumber = atoi(argv[2]);

//checking the protocol specified
    if ((temp = strstr(url, "http://")) != NULL) {
        url = url + 7;
    } else if ((temp = strstr(url, "https://")) != NULL) {
        url = url + 8;
    }

//checking the port number
    if (portNumber > 65536 || portNumber < 0) {
        printf("Invalid Port Number!");
        exit(1);
    }

    sockfd = get_request(url, argv[2]);

    return 0;
}

int sockfd = -1;
char getrequest[1024];

int clnt_connect(char * url, char * port) {
    char * ptr, * host;
    struct sockaddr_in addr;
    int ret;

    if (isValidIP(url)) { //when an IP address is given
        sprintf(getrequest, "GET / HTTP/1.0\nHOST: %s\n\n", url);
    } else { //when a host name is given
        if ((ptr = strstr(url, "/")) == NULL) {
            //when hostname does not contain a slash
            sprintf(getrequest, "GET / HTTP/1.0\nHOST: %s\n\n", url);
        } else {
            //when hostname contains a slash, it is a path to file
            strcpy(path, ptr);
            host = strtok(url, "/");
            sprintf(getrequest, "GET %s HTTP/1.0\nHOST: %s\n\n", path, url);
        }
    }

// creates a socket to the host
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("Error creating socket!\n");
        exit(1);
    }
    printf("Socket created...\n");

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(url);
    addr.sin_port = htons(atoi(port));

    if (connect(sockfd, (struct sockaddr *) &addr, sizeof(addr)) < 0 ) {
        printf("Connection Error!\n");
        exit(1);
    }
    printf("Connection successful...\n\n\n");
    ptr = strtok(path, "/");
    strcpy(path, ptr);

    return 0;
}

int clnt_send_request() {
    int ret;
    ret = write(sockfd, getrequest, strlen(getrequest));
    debug(" sent %d of %d bytes\n", ret, strlen(getrequest));
    if (ret < strlen(getrequest)) {
        printf("ERR clnt_send_request %d\n", ret);
        return -1;
    }
    return 0;
}

int clnt_recv_respose() {
    int ret;
    char * url, * temp;
    char buffer[BUF_SIZE];
    char http_ok[] = "HTTP/1.1 200 OK\r\n";
    int buf_pos = 0;

    memset(&buffer, 0, sizeof(buffer));
    ret = recv(sockfd, buffer, BUF_SIZE, 0);
    if (ret < 0) {
        printf("Error receiving HTTP status!\n");
    } else {
        buf_pos += ret;
        printf("%s\n", buffer);
        if ((temp = strstr(buffer, http_ok)) == NULL) {
            debug(" recv-1\n");
            close(sockfd);
            sockfd = -1;
            return 1;
        }
    }

    memset(&buffer, 0, sizeof(buffer));
    ret = recv(sockfd, buffer, BUF_SIZE, 0);
    if (ret < 0) {
        printf("Error receiving HTTP header!\n");
    } else {
        buf_pos += ret;
        printf("%s\n", buffer);
        if (0 /*parseHeader(buffer) != 0*/) {
            debug(" recv-2\n");
            close(sockfd);
            sockfd = -1;
            return 1;
        }
    }
    return 0;
}


int get_request(char * url, char * port) {
    int ret;
    ret = clnt_connect(url, port);
    if (ret) {
        printf("ERR clnt_connect %d\n", ret);
        return ret;
    }

// writes the HTTP GET Request to the sockfd
    ret = clnt_send_request();
    if (ret) {
        printf("ERR clnt_send_request %d\n", ret);
        return ret;
    }

    ret = clnt_recv_respose();
    if (ret) {
        printf("ERR clnt_recv_respose %d\n", ret);
        return ret;
    }

    return sockfd;
}


int isValidIP(char * ip) {
    struct sockaddr_in addr;
    int valid = inet_pton(AF_INET, ip, &(addr.sin_addr));
    return valid != 0;
}

