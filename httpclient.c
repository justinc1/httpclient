// http://www.theinsanetechie.in/2014/02/a-simple-http-client-and-server-in-c.html
/**httpclient.c**/
#include"stdio.h"
#include"stdlib.h"
#include"sys/types.h"
#include"sys/socket.h"
#include"string.h"
#include"netinet/in.h"
#include"netdb.h"

#define BUF_SIZE 1024

int get_request(char * url, char * port);
int isValidIP(char * ip);
int parseHeader(char * header);
char * splitKeyValue(char * line, int index);
void openFile();

FILE * fileptr;
char keys[][25] = {"Date: ", "Hostname: ", "Location: ", "Content-Type: "};
char status[4] = {0, 0, 0, 0};
char contentFileType[100];
char path[1000];

int main(int argc, char**argv) {
    struct sockaddr_in addr, cl_addr;
    int sockfd, ret;
    struct hostent * server;
    char * url, * temp;
    int portNumber;
    char * fileName;
    char status_ok[] = "OK";
    char buffer[BUF_SIZE];
    char http_not_found[] = "HTTP/1.0 404 Not Found";
    char http_ok[] = "HTTP/1.0 200 OK";
    char location[] = "Location: ";
    char contentType[] = "Content-Type: ";
    int sPos, ePos;

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

    memset(&buffer, 0, sizeof(buffer));
    ret = recv(sockfd, buffer, BUF_SIZE, 0);
    if (ret < 0) {
        printf("Error receiving HTTP status!\n");
    } else {
        printf("%s\n", buffer);
        if ((temp = strstr(buffer, http_ok)) != NULL) {
            send(sockfd, status_ok, strlen(status_ok), 0);
        } else {
            close(sockfd);
            return 0;
        }
    }

    memset(&buffer, 0, sizeof(buffer));
    ret = recv(sockfd, buffer, BUF_SIZE, 0);
    if (ret < 0) {
        printf("Error receiving HTTP header!\n");
    } else {
        printf("%s\n", buffer);
        if (parseHeader(buffer) == 0) {
            send(sockfd, status_ok, strlen(status_ok), 0);
        } else {
            printf("Error in HTTP header!\n");
            close(sockfd);
            return 0;
        }
    }

//printf("file: [%s]\n", fileName);
    fileptr = fopen(path, "w");
    if (fileptr == NULL) {
        printf("Error opening file!\n");
        close(sockfd);
        return 0;
    }

    memset(&buffer, 0, sizeof(buffer));
    while (recv(sockfd, buffer, BUF_SIZE, 0) > 0) { //receives the file
        if ((strstr(contentFileType, "text/html")) != NULL) {
            fprintf(fileptr, "%s", buffer);
        } else {
            fwrite(&buffer, sizeof(buffer), 1, fileptr);
        }
        memset(&buffer, 0, sizeof(buffer));
    }

    fclose(fileptr);
    close(sockfd);

    openFile();

    return 0;
}


int get_request(char * url, char * port) {

    int sockfd, bindfd;
    char * ptr, * host;
    char getrequest[1024];
    struct sockaddr_in addr;

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
//printf("path=%s\n", path);
//fileptr = fopen(path, "w");
//strcpy(fileName, path);
//sprintf(fileName, "%s", path);

    //int optval = 1;
    //setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);

// writes the HTTP GET Request to the sockfd
    write(sockfd, getrequest, strlen(getrequest));

    return sockfd;
}


int isValidIP(char * ip) {
    struct sockaddr_in addr;
    int valid = inet_pton(AF_INET, ip, &(addr.sin_addr));
    return valid != 0;
}


int parseHeader(char * header) {
//"Date: %sHostname: %s:%d\nLocation: %s\nContent-Type: %s\n\n"
    char * line, * key, * value;
    char temp[100];
    int i = 0;
    line = strtok(header, "\n");
    while (line != NULL) {
        //printf("%s\n", line);
        strcpy(temp, line);
        value = splitKeyValue(line, i);
        if (i == 3) {
            strcpy(contentFileType, value);
        }
        //printf("value=%s\n", value);
        line = strtok(NULL, "\n");
        i++;
    }
    for (i = 0; i < 4; i++) {
        if (status[i] == 0) return 1;
        //printf("status[%d]=%d\n", i, status[i]);
    }
    return 0;
}

char * splitKeyValue(char * line, int index) {
    char * temp;
    if ((temp = strstr(line, keys[index])) != NULL) {
        temp = temp + strlen(keys[index]);
        status[index] = 1;
    }
    return temp;
}

void openFile() {
    char * temp;
    char command[100];
    char fileName[1000];
    strcpy(fileName, path);
//printf("File Name: %s\n", fileName);
//printf("Content Type: %s\n", contentFileType);
    if ((temp = strstr(contentFileType, "text/html")) != NULL) {
        if ((temp = strstr(fileName, ".txt")) != NULL) {
            sprintf(command, "gedit %s", fileName);
        } else {
            sprintf(command, "firefox %s", fileName);
        }
        system(command);
    } else if ((temp = strstr(contentFileType, "application/pdf")) != NULL) {
        sprintf(command, "acroread %s", fileName);
        system(command);
    } else {
        printf("The filetype %s is not supported. Failed to open %s!\n", contentFileType, fileName);
    }
}



