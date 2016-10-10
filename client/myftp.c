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
    if(argc != 3)
    {
        fprintf(stderr,"usage: myftp server_name port\n");
        exit(1);
    }
    
    struct hostent *hp;
    struct sockaddr_in sin;
    char *host;
    char buf[MAX_LINE];
    int s;
    int len;
    host = argv[1];
    hp = gethostbyname(host);
    if(!hp)
    {
        fprintf(stderr,"myftp: unknown host: %s\n",host);
        exit(1);
    }
    
    bzero((char *)&sin,sizeof(sin));
    sin.sin_family = AF_INET;
    bcopy(hp->h_addr,(char *)&sin.sin_addr,hp->h_length);
    sin.sin_port = htons(atoi(argv[2]));

    if((s=socket(PF_INET,SOCK_STREAM,0))<0)
    {
        fprintf(stderr,"myftp: could not open socket\n");
        exit(1);
    }   

    if(connect(s,(struct sockaddr *)&sin,sizeof(sin))<0)
    {
        fprintf(stderr,"myftp: error in connect call\n");
        close(s);
        exit(1);
    }

    while(fgets(buf,sizeof(buf),stdin))
    {
        buf[MAX_LINE-1] = '\0';
        if(!strncmp(buf,"Exit",4))
        {
            printf("Goodbye!\n");
            break;
        }
        len = strlen(buf)+1;
        if(send(s,buf,len,0)==-1)
        {
            fprintf(stderr,"myftp: error in send\n");
            close(s);
            exit(1);
        }
    }

    close(s);

    return 0;
}
