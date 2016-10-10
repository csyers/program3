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
    if(argc != 2)
    {
        fprintf(stderr,"usage: myftpd port\n");
        exit(1);
    }
    struct sockaddr_in sin;
    char buf[MAX_LINE];
    int len;
    int s, new_s;
    int opt = 1;

    bzero((char *)&sin,sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(atoi(argv[1]));

    if((s=socket(PF_INET,SOCK_STREAM,0)) < 0)
    {
        fprintf(stderr,"myftpd: error in socket creation\n");
        exit(1);
    }

    if((setsockopt(s,SOL_SOCKET,SO_REUSEADDR,(char *)&opt,sizeof(int)))<0)
    {
        fprintf(stderr,"myftpd: error in setsocketopt\n");
        close(s);
        exit(1);
    }

    if((bind(s,(struct sockaddr *)&sin,sizeof(sin)))<0)
    {
        fprintf(stderr,"myftpd: error in bind\n");
        close(s);
        exit(1);
    }   

    if((listen(s,MAX_PENDING))<0)
    {
        fprintf(stderr,"myftpd: error in listen\n");
        close(s);
        exit(1);
    }

    printf("Welcome to the first TCP server!\n");

    while(1){
        if((new_s=accept(s,(struct sockaddr *)&sin,(socklen_t *)&len))<-0)
        {
            fprintf(stderr,"myftpd: error in accept\n");
            close(s);
            exit(1);
        }
        
        while(1)
        {
            if((len=recv(new_s,buf,sizeof(buf),0))==-1)
            {
                fprintf(stderr,"myftpd: server received error\n");
                close(s);
                exit(1);
            }
            if(len==0)
            {
                break;
            }
            printf("%s",buf);

        }
        close(new_s);
    }

    close(s);
    return 0;
}
