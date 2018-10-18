// simple program for writing certain files to photon through tcp connection
#include <stdio.h> 
#include <stdlib.h> 
#include <errno.h> 
#include <string.h> 
#include <netdb.h> 
#include <sys/types.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <unistd.h>
#include <fcntl.h> 
#include <termios.h>

#define PORT 5000    /* the port client will be connecting to */
#define HOSTNAME "128.54.52.84"
#define MAXDATASIZE 100 /* max number of bytes we can get at once */
#define FILE_NAME "./extracted_subject1.txt"

int main(int argc, char *argv[])
{
    int sockfd, numbytes;  
    char buf[MAXDATASIZE];
    struct hostent *he;
    struct sockaddr_in their_addr; /* connector's address information */

    if ((he=gethostbyname(HOSTNAME)) == NULL) {  /* get the host info */
        herror("gethostbyname");
        exit(1);
    }

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    their_addr.sin_family = AF_INET;      /* host byte order */
    their_addr.sin_port = htons(PORT);    /* short, network byte order */
    their_addr.sin_addr = *((struct in_addr *)he->h_addr);
    bzero(&(their_addr.sin_zero), 8);     /* zero the rest of the struct */

    if (connect(sockfd, (struct sockaddr *)&their_addr, \
        sizeof(struct sockaddr)) == -1) {
        perror("connect");
        exit(1);
    }
	
	// open file
    int file, ret;
	char readByte, recvByte;
	file = open(FILE_NAME, O_RDONLY);
	if (file < 0)
	{
		fprintf(stderr, "open file error!");
		exit(1);
	}

	// read one byte at each time
	while (ret = read(file, &readByte, 1)) { // looping until the EOF
		if (ret < 0) // read error
		{
			fprintf(stderr, "read file error!");
			close(file);
			exit(1);
		}

		// printf("%c", readByte); // check reading
		// fflush(stdout); // flush out immediately

		if (send(sockfd, &readByte, 1, 0) < 0) // send byte
		{
			fprintf(stderr, "send error!");
			close(file);
			exit(1);
		}
		if (recv(sockfd, &recvByte, 1, 0) < 0) { // read echo back
	    	perror("recv");
	    	close(file);
	    	exit(1);
		}
		printf("%c", recvByte); // check reading
		fflush(stdout); // flush out immediately
	}
	
	close(sockfd);
	close(file);
	printf("\r\n");
	printf("read and write successfully!\r\n");


    return 0;
}