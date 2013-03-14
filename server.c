#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

void error(char * str){
    perror(str);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
        int sockfd, newsockfd, portno;
        socklen_t clilen;
        char buffer[256];
        char answer[] = "HTTP/1.1 200 OK\n"
            "Content-Type: text/html\n"
            "Date: Fri, 31 Dec 2003 23:59:59 GMT\n"
            "Content-length: 12\n\n"
            "Hello world\n";
            
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

        /* accept the connection */
        printf("Accepting connection\n");
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        /* here I should span a new process with newsockfd */
        printf("Accepted!\n"); 

        if (newsockfd < 0) error("ERROR on accept");

        bzero(buffer,256);

        /* read, answer and close */
        n = read(newsockfd,buffer,255);

        if (n < 0) error("ERROR reading from socket");

        n = write(newsockfd, answer, strlen(answer));

        if (n < 0) error("ERROR writing to socket");

        close(newsockfd);

        close(sockfd);

        return 0; 
}
