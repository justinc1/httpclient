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
#include <errno.h>
#include <assert.h>
#include <time.h>

#define BUF_SIZE 1024

int main_loop(char * url, char * port);
int isValidIP(char * ip);


int sockfd = -1;
char getrequest[1024];
int num_requests;
char path[1000];

struct timespec ts_start, ts_stop;

void timer_start()
{
    clock_gettime(CLOCK_REALTIME, &ts_start);
}

double timer_get_us()
{
    clock_gettime(CLOCK_REALTIME, &ts_stop);
    double result = (ts_stop.tv_sec - ts_start.tv_sec) * 1e6 + (ts_stop.tv_nsec - ts_start.tv_nsec) / 1e3;
    return result;
}

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
    int ret;
    struct hostent * server;
    char * url, * temp;
    int portNumber;

    if (argc < 3) {
        printf("Usage: [URL] [port number] [num_requests]\n");
        printf("Example: %s http://192.168.122.90/index_html 80 5\n");
        exit(1);
    }

    num_requests = 10000;
    url = argv[1];
    portNumber = atoi(argv[2]);
    switch (argc) {
        case 4:
            num_requests = atoi(argv[3]);
        default:
            break;
    }

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

    ret = main_loop(url, argv[2]);

    return 0;
}

int clnt_connect(char * url, char * port) {
    char * ptr, * host;
    struct sockaddr_in addr;
    int ret;

    if (isValidIP(url)) { //when an IP address is given
        sprintf(getrequest, "GET / HTTP/1.1\nHOST: %s\n\n", url);
    } else { //when a host name is given
        if ((ptr = strstr(url, "/")) == NULL) {
            //when hostname does not contain a slash
            sprintf(getrequest, "GET / HTTP/1.1\nHOST: %s\n\n", url);
        } else {
            //when hostname contains a slash, it is a path to file
            strcpy(path, ptr);
            host = strtok(url, "/");
            sprintf(getrequest, "GET %s HTTP/1.1\nHOST: %s\n\n", path, url);
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
    //debug(" INFO sent %d of %d bytes\n", ret, strlen(getrequest));
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
    char http_header_term[] = "\r\n\r\n";
    char http_content_length[] = "\r\nContent-Length: ";
    int buf_pos = 0;
    bool response_done = false;
    int header_length = 0;
    int content_length = 0;

    memset(&buffer, 0, sizeof(buffer));
    while(response_done == false) {
        ret = recv(sockfd, buffer, BUF_SIZE, 0);
        //debug(" INFO recv ret=%d, errno=%d\n", ret, errno); sleep(1);
        if (ret < 0) {
            printf("Error receiving HTTP status!\n");
        }
        else if (ret == 0) {
            if (errno == 0) {
                debug(" INFO recv ret=%d, errno=%d, server closed connection?\n", ret, errno);
                break;
            }
        }
        else {
            //printf("%s\n", buffer);
            // reply should start with "HTTP OK".
            if (buf_pos == 0 &&
                (temp = strstr(buffer, http_ok)) == NULL) {
                debug(" ERROR reply is not HTTP 200 OK\n");
                close(sockfd);
                sockfd = -1;
                return 1;
            }
            buf_pos += ret;
            if (header_length == 0) {
                if ((temp = strstr(buffer, http_header_term)) == NULL) {
                    debug(" ERROR reply is missing http_header_term\n");
                    close(sockfd);
                    sockfd = -1;
                    return 1;
                }
                header_length += temp + strlen(http_header_term) - buffer;
                //debug(" INFO header_length=%d\n", header_length);
            }
            if (content_length == 0) {
                if ((temp = strstr(buffer, http_content_length)) == NULL) {
                    debug(" ERROR reply is missing Content-Length\n");
                    close(sockfd);
                    sockfd = -1;
                    return 1;
                }
                temp += strlen(http_content_length);
                content_length = atoi(temp);
                //debug(" INFO content_length=%d\n", content_length);
            }
            if (content_length > 0) {
                assert(header_length > 0);
                //debug(" INFO  header_length=%d content_length=%d, buf_pos=%d\n", header_length, content_length, buf_pos);
                if(buf_pos == header_length+content_length) {
                    response_done = true;
                }
            }
        }
    }

    return 0;
}


int main_loop(char * url, char * port) {
    int ret;
    double duration;

    timer_start();

    ret = clnt_connect(url, port);
    if (ret) {
        printf("ERR clnt_connect %d\n", ret);
        return ret;
    }

// writes the HTTP GET Request to the sockfd
    int ii;
    for (ii=0; ii<num_requests; ii++) {
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
    }

    duration = timer_get_us();
    printf(" RATE % 6.0f req/s,  DURATION % 4.3f s, NUM_REQUEST % 7d\n", num_requests*1e6/duration, duration/1e6, num_requests);

    close(sockfd);
    sockfd = -1;
}


int isValidIP(char * ip) {
    struct sockaddr_in addr;
    int valid = inet_pton(AF_INET, ip, &(addr.sin_addr));
    return valid != 0;
}

