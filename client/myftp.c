/* Name: Tim Chang (tchang2), Christopher Syers (csyers)
 * Date: October 3, 2016
 * CSE 30264: Computer Networks
 * Programming Assignment 3: Prototype FTP Connection
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_LINE 4096

int main(int argc, char* argv[])
{
    // check usage
    if(argc != 3)
    {
        fprintf(stderr,"usage: myftp server_name port\n");
        exit(1);
    }
    
    // initialize varaibles
    struct hostent *hp;
    struct sockaddr_in sin;
    char *host;
    char buf[MAX_LINE];
    char file[MAX_LINE];
    int s;
    int len;
    short int len_filename;

    // get host name from the first command line argument
    host = argv[1];
    hp = gethostbyname(host);

    // if hostname is not resolvable, print error and exit
    if(!hp)
    {
        fprintf(stderr,"myftp: unknown host: %s\n",host);
        exit(1);
    }
    
    // setup the sockaddr_in
    bzero((char *)&sin,sizeof(sin));
    sin.sin_family = AF_INET;
    bcopy(hp->h_addr,(char *)&sin.sin_addr,hp->h_length);
    sin.sin_port = htons(atoi(argv[2]));

    // open a socket, exit if there is an error
    if((s=socket(PF_INET,SOCK_STREAM,0))<0)
    {
        fprintf(stderr,"myftp: could not open socket\n");
        exit(1);
    }   

    // attempt to connect to server, exit if there is an error
    if(connect(s,(struct sockaddr *)&sin,sizeof(sin))<0)
    {
        fprintf(stderr,"myftp: error in connect call\n");
        close(s);
        exit(1);
    }

    // prompt user for command
    printf("Enter command (REQ, UPL, LIS, MKD, RMD, CHD, DEL, XIT): ");

    // while user is still inputting commands
    while(fgets(buf,sizeof(buf),stdin))
    {
        // set last char in buffer to null
        buf[MAX_LINE-1] = '\0';
        len = strlen(buf);

        // strip the newline character from the buffer
        if((len-1 > 0) && (buf[len-1] == '\n'))
        {
            buf[len-1] = '\0';
        }

        // case: REQ
        if(strcmp(buf,"REQ") == 0)
        {
	    int filesize;
	    FILE *fp;
	    int bytesReceived;

            // send REQ to server, print error and exit on failure
            if(send(s,buf,len+1,0)==-1)
            {
                fprintf(stderr,"myftp: error in send\n");
                close(s);
                exit(1);
            }

            // prompt user for filename
            printf("Enter file to retrieve: ");
            if(fgets(file,sizeof(file),stdin)<0)
            {
                fprintf(stderr,"myftp: error in fgets\n");
                close(s);
                exit(1);
            }
            file[MAX_LINE-1] = '\0';
            len_filename = strlen(file);

            // strip the newline character from the buffer
            if((len_filename-1 > 0) && (file[len_filename-1] == '\n'))
            {
                file[len_filename-1] = '\0';
                len_filename--;
            }

            // send size
	    len_filename = htons(len_filename);
            if(send(s, &len_filename,sizeof(len_filename),0)==-1)
            {
                fprintf(stderr,"myftp: error in send\n");
                close(s);
                exit(1);
            }
	    
            // send filename
            if(send(s, &file,sizeof(file),0)==-1)
            {
                fprintf(stderr,"myftp: error in send\n");
                close(s);
                exit(1);
            }

	    if((filesize = recv(s, &filesize, sizeof(int), 0)) == -1) {
                fprintf(stderr,"myftp: error in recv\n");
                close(s);
                exit(1);
	    }
	    filesize = ntohl(filesize);
	    printf("%d\n", filesize);
	    fflush(stdout);
	    
	    if(filesize == -1) {
		printf("file: %s does not exist on the server\n", file);
	    } else {
	        if(!(fp = fopen(file, "w"))) {	
                    fprintf(stderr,"myftp: error in recv\n");
                    close(s);
             	    exit(1);
	        }
	        while((bytesReceived = recv(s, buf, MAX_LINE, 0)) > 0) {
		    fwrite(buf, 1, bytesReceived, fp);
		}

		if(bytesReceived < 0) {
                    fprintf(stderr,"myftp: error in recv\n");
                    close(s);
                    exit(1);
		}
	    }

        // case: UPL
        }
        else if (strcmp(buf,"UPL") == 0)
        {

        // case: LIS
        }
        else if (strcmp(buf,"LIS") == 0)
        {

        // case: MKD
        }
        else if (strcmp(buf,"MKD") == 0)
        {

        // case: RMD
        }
        else if (strcmp(buf,"RMD") == 0)
        {

        // case: CHD
        }
        else if (strcmp(buf,"CHD") == 0)
        {

        // case: DEL
        }
        else if (strcmp(buf,"DEL") == 0)
        {

        // case: XIT
        }
        else if (strcmp(buf,"XIT") == 0)
        {
            // send the command to the server, print error send fails
            if(send(s,buf,len+1,0)==-1)
            {
                fprintf(stderr,"myftp: error in send\n");
                close(s);
                exit(1);
            }
            // end the loop, quit the client connection
            break;
        // case: unknown command
        }
        else 
        {
            // print error
            printf("myftp: invalid command\n");
        }
        // prompt user for next command
        printf("Enter command (REQ, UPL, LIS, MKD, RMD, CHD, DEL, XIT): ");
    }

    // close the socket and print exit message
    close(s);
    printf("\nmyftp: session has been closed\n");
    return 0;
}
