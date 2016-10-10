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

#define MAX_PENDING 5
#define MAX_LINE 4096

int main(int argc, char* argv[])
{
    // check usage
    if(argc != 2)
    {
        fprintf(stderr,"usage: myftpd port\n");
        exit(1);
    }

    // initialize variables
    struct sockaddr_in sin;
    char buf[MAX_LINE];
    int len;
    int s, new_s;
    int opt = 1;

    //etup the sockaddr_in
    bzero((char *)&sin,sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(atoi(argv[1]));

    // open a socket, exit if there is an error
    if((s=socket(PF_INET,SOCK_STREAM,0)) < 0)
    {
        fprintf(stderr,"myftpd: error in socket creation\n");
        exit(1);
    }

    // allow socket to be reused after a closed connection, exit on error
    if((setsockopt(s,SOL_SOCKET,SO_REUSEADDR,(char *)&opt,sizeof(int)))<0)
    {
        fprintf(stderr,"myftpd: error in setsocketopt\n");
        close(s);
        exit(1);
    }

    // bind the socket to the port passed in on the command line
    if((bind(s,(struct sockaddr *)&sin,sizeof(sin)))<0)
    {
        fprintf(stderr,"myftpd: error in bind\n");
        close(s);
        exit(1);
    }   

    // begin listening for client connections on the socket
    if((listen(s,MAX_PENDING))<0)
    {
        fprintf(stderr,"myftpd: error in listen\n");
        close(s);
        exit(1);
    }

    // enter infinte loop waiting for connections
    while(1){
        // accept a client connection on a new socket (new_s), exit on error
        if((new_s=accept(s,(struct sockaddr *)&sin,(socklen_t *)&len))<-0)
        {
            fprintf(stderr,"myftpd: error in accept\n");
            close(s);
            exit(1);
        }
        // infinte loop while in connection with client
        while(1)
        {
            // receive the command from the client, exit on error
            if((len=recv(new_s,buf,sizeof(buf),0))==-1)
            {
                fprintf(stderr,"myftpd: server received error\n");
                close(new_s);
                close(s);
                exit(1);
            }
            // case: REQ
            if(strcmp(buf,"REQ") == 0)
            {
            }
            // case: UPL
            else if(strcmp(buf,"UPL") == 0)
            {
            }
            // case: LIS
            else if(strcmp(buf,"LIS") == 0)
            {
            }
            // case: MKD
            else if(strcmp(buf,"MKD") == 0)
            {
            }
            // case: RMD
            else if(strcmp(buf,"RMD") == 0)
            {
            }
            // case: CHD
            else if(strcmp(buf,"CHD") == 0)
            {
            }
            // case: DEL
            else if(strcmp(buf,"DEL") == 0)
            {
            }
            // case: XIT or termination of client
            if(len==0 || strcmp(buf,"XIT") == 0)
            {
                // enter accepting state
                break;
            }
        }
        close(new_s);
    }

    // close socket and end program
    close(s);
    return 0;
}
