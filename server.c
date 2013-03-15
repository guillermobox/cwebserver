#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <time.h>

void error(char * str){
    perror(str);
    exit(EXIT_FAILURE);
}

#define BUFFLEN 256
void copy(char * filepath, int fdout){
    char buffer[BUFFLEN], headers[128];
    struct stat st;
    int nchars, fd;
    time_t t;

    fd = open(filepath, O_RDONLY);
    if( fd<0 ){
        perror("opening file");
        exit(1);
    }
    
    stat(filepath, &st);

    t = time(NULL);

    bzero(headers, 128);
    strcpy(headers, "HTTP/1.1 200 OK\n");
    strcat(headers, "Content-Type: text/plain\n");
    sprintf(headers+strlen(headers), "Content-length: %ld\n", st.st_size);
    strcat(headers, "Date: ");
    strcat(headers, ctime(&t));
    strcat(headers, "\n");

    write(fdout, headers, strlen(headers));

    while(1){
        nchars = read(fd, buffer, BUFFLEN);
        if( nchars == 0 ){
            break;
        }else if( nchars < 0 ){
            perror("reading file");
            exit(1);
        }else{
            write(fdout, buffer, nchars);
        }
    }
}

char * geturl(char * header){
    char *ptstart, *ptend;
    char * ret;

    ptstart = index(header, ' ')+2;
    ptend = index(ptstart, ' ');

    ret = calloc(128,sizeof(char));
    strncpy(ret, ptstart, ptend-ptstart);
    ret[ strlen(ret) ] = '\0';
    return ret;
}

int handle(int newsockfd){
        char buffer[256], *path;
        int n;

        bzero(buffer,256);
        n = read(newsockfd,buffer,255);
        if (n < 0) error("ERROR reading from socket");

        path = geturl( buffer );
        puts(path);

        if( access(path, R_OK)!=0 )
                write(newsockfd, "HTTP/1.1 404 Not found\n", 23);
        else
                copy(path, newsockfd);

        free(path);
        close(newsockfd);
}

int main(int argc, char *argv[])
{
        int sockfd, newsockfd, portno;
        socklen_t clilen;
        char buffer[256];

        struct sockaddr_in serv_addr, cli_addr;
        int n;
        int err;

        portno = 12345;

        /* create socket */
        printf("Creating a socket...\n");
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) error("ERROR opening socket");

        /* bind socket to port 80 and own host */
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(portno);

        err = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
        if( err<0 ) error("error binding\n");

        printf("Listening...\n");
        listen(sockfd,1);
        clilen = sizeof(cli_addr);

        while(1){
                char * path;

                newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
                if (newsockfd < 0) error("ERROR on accept");
                handle( newsockfd );
        }

        close(sockfd);

        return 0; 
}
