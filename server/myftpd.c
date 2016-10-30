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
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <mhash.h>

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
    int len = sizeof(sin);
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
        if((new_s=accept(s,(struct sockaddr *)&sin,(socklen_t *)&len))<0)
        {
            fprintf(stderr,"myftpd: error in accept\n");
            close(s);
            exit(1);
        }
        // infinte loop while in connection with client
        while(1)
        {
            //printf("Waiting for user command\n");
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
                FILE *fp;
                struct stat st;
                int filesize;
                int bytes;
                short int len_filename;
                MHASH td;
                char hash[16];

                //printf("REQ received\n");
                // receive len of filename
                if((len=recv(new_s, &len_filename, sizeof(short int),0))==-1)
                {
                    fprintf(stderr,"myftpd: server received error\n");
                    close(new_s);
                    close(s);
                    exit(1);
                }

                len_filename = ntohs(len_filename);
        
                // receive file name
                bzero(buf, sizeof(buf));
                if((len=recv(new_s,buf,len_filename,0))==-1)
                {
                    fprintf(stderr,"myftpd: server received error\n");
                    close(new_s);
                    close(s);
                    exit(1);
                }
                buf[len] = '\0';

                // print name of file
                /*printf("%s\n", buf);*/
                if (access(buf, R_OK) != -1) {
                    //printf("file exist\n");
                    //return size of file
                    stat(buf, &st);
                    filesize = st.st_size;
                    filesize = htonl(filesize);
                    if(send(new_s, &filesize, sizeof(int), 0)==-1) 
                    {
                        fprintf(stderr, "myftpd: server send error\n");
                        close(new_s);
                        close(s);
                        exit(1);
                    }

                    // send the file
                    if (!(fp = fopen(buf, "r"))) {
                        fprintf(stderr,"myftpd: file is unreadable\n");
                        close(new_s);
                        close(s);
                        exit(1);
                    }

                    // send MD5 Hash
                    td = mhash_init(MHASH_MD5);
                    if (td == MHASH_FAILED) {
                        fprintf(stderr, "myftpd: md5 hash failed\n");
                        close(new_s);
                        close(s);
                        exit(1);
                    }
		            bzero(buf, sizeof(buf));
                    bytes=fread(buf,1,MAX_LINE,fp);
                    while(bytes>0)
                    {
                       // printf("%s",buf);
                        mhash(td,&buf,sizeof(buf));
		                bzero(buf, sizeof(buf));
                        bytes=fread(buf,1,MAX_LINE,fp);
                    }
            
                    rewind(fp);

                    mhash_deinit(td, hash);
                    if(send(new_s, &hash, sizeof(hash), 0)==-1) // NULL terminator?
                    {
                        fprintf(stderr, "myftpd: server send error\n");
                        close(new_s);
                        close(s);
                        exit(1);
                    }

                    //printf("sending the file\n");
                    while(1) {
                        bzero((char *)buf, sizeof(buf));
                        int nred = fread(buf, 1, MAX_LINE, fp);

                        if (nred > 0) {
                            if(send(new_s, &buf, nred, 0)==-1) // NULL terminator?
                            {
                                fprintf(stderr, "myftpd: server send error\n");
                                close(new_s);
                                close(s);
                                exit(1);
                            }
                        }

                        if (nred < MAX_LINE) {
                            if(ferror(fp)) {
                                fprintf(stderr, "myftpd: server file read error\n");
                                close(new_s);
                                close(s);
                                exit(1);
                            }
                            break;
                        }
                    }
                } else {
                    int temp = -1;
                    temp = htonl(temp);
                    if(send(new_s, &temp, sizeof(int), 0)==-1) 
                    {
                        fprintf(stderr, "myftpd: server send error\n");
                        close(new_s);
                        close(s);
                        exit(1);
                    }
                }   
            }
            // case: UPL
            else if(strcmp(buf,"UPL") == 0)
            {
                FILE *fp;
                char file[MAX_LINE];
                int filesize, bytesReceived, bytes;
                short int len_filename = 0;
                MHASH td;
                char hash[16];
                char client_hash[16];

                struct timeval tv_start, tv_end;

                // receive len of filename
                if((len=recv(new_s, &len_filename, sizeof(short int),0))==-1)
                {
                    fprintf(stderr,"myftpd: server received error\n");
                    close(new_s);
                    close(s);
                    exit(1);
                }

                len_filename = ntohs(len_filename);
                
                // receive file name
                bzero(buf, sizeof(buf));
                if((len=recv(new_s,file,len_filename,0))==-1)
                {
                    fprintf(stderr,"myftpd: server received error\n");
                    close(new_s);
                    close(s);
                    exit(1);
                }
                file[len] = '\0';


                // ack the send
                int temp = -1;
                temp = htonl(temp);
                if(send(new_s, &temp, sizeof(int), 0)==-1) 
                {
                    fprintf(stderr, "myftpd: server send error\n");
                    close(new_s);
                    close(s);
                    exit(1);
                }

                // receive filesize
                filesize = 0;
                len = 0;
                if((len=recv(new_s, &filesize, sizeof(int), 0))==-1) {
                    fprintf(stderr,"myftp: error in recv\n");
                    close(new_s);
                    close(s);
                    exit(1);
                }
                filesize = ntohl(filesize);

                // open file for receiving
                if(!(fp = fopen(file, "w+"))) {
                    fprintf(stderr, "myftp: error in opening file\n");
                    close(new_s);
                    close(s);
                    exit(1);
                }

                gettimeofday(&tv_start,0);
                // receive file
                int remaining_filesize = filesize;
                while (remaining_filesize > 0) {
                    bytesReceived = recv(new_s, buf, MAX_LINE > remaining_filesize ? remaining_filesize: MAX_LINE, 0);
                    if(bytesReceived < 0) {
                        fprintf(stderr, "myftpd: error in recv\n");
                        close(new_s);
                        close(s);
                        exit(1);
                    }
                    remaining_filesize -= bytesReceived;
                    if(fwrite(buf, sizeof(char), bytesReceived, fp) < bytesReceived) {
                        fprintf(stderr, "myftpd: error in write\n");
                        close(new_s);
                        close(s);
                        exit(1);
                    }
                }

                gettimeofday(&tv_end,0);
                //fclose(fp);

                // receive MD5 hash
                if (recv(new_s, client_hash, sizeof(hash), 0) == -1) {
                    fprintf(stderr, "myftpd: error in recv\n");
                    close(new_s);
                    close(s);
                    exit(1);
                }
                rewind(fp);
                td = mhash_init(MHASH_MD5);
                if (td == MHASH_FAILED) {
                    fprintf(stderr, "myftp: hashing failed\n");
                    close(s);
                    exit(1);
                }
                bzero(buf,sizeof(buf));
                bytes = fread(buf, 1, MAX_LINE, fp);
                while(bytes > 0 )
                {
                    mhash(td, &buf, sizeof(buf));
                    bzero(buf,sizeof(buf));
                    bytes = fread(buf,1, MAX_LINE, fp);
                }
                mhash_deinit(td,hash);
                fclose(fp);

                if(strncmp(hash,client_hash,16) == 0){
                    // same
                    int time_diff = (tv_end.tv_sec - tv_start.tv_sec)*1000000L +tv_end.tv_usec - tv_start.tv_usec;
                    // send throughput
                    time_diff = htonl(time_diff);
                    if(send(new_s, &time_diff, sizeof(int), 0)==-1) 
                    {
                        fprintf(stderr, "myftpd: server send error\n");
                        close(new_s);
                        close(s);
                        exit(1);
                    }
                } else {
                    int flag = -1;
                    flag = htonl(flag);
                    if(send(new_s, &flag, sizeof(int), 0)==-1) 
                    {
                        fprintf(stderr, "myftpd: server send error\n");
                        close(new_s);
                        close(s);
                        exit(1);
                    }
                }

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
                char file[MAX_LINE];
                short int len_filename;

                // receive len of filename
                if((len=recv(new_s, &len_filename, sizeof(short int),0))==-1)
                {
                    fprintf(stderr,"myftpd: server received error\n");
                    close(new_s);
                    close(s);
                    exit(1);
                }

                len_filename = ntohs(len_filename);
        
                // receive file name
                bzero(file, sizeof(file));
                if((len=recv(new_s,file,len_filename,0))==-1)
                {
                    fprintf(stderr,"myftpd: server received error\n");
                    close(new_s);
                    close(s);
                    exit(1);
                }
                file[len] = '\0';

                // check if file exists
                int flag;
                if (access(file, F_OK) != -1) {
                    flag = 1;
                } else {
                    flag = -1;
                }
                flag = htonl(flag);
                if(send(new_s, &flag, sizeof(int), 0)==-1) 
                {
                    fprintf(stderr, "myftpd: server send error\n");
                    close(new_s);
                    close(s);
                    exit(1);
                }

                // receive resp from client
                if((len=recv(new_s,&flag, sizeof(int),0))==-1)
                {
                    fprintf(stderr,"myftpd: server received error\n");
                    close(new_s);
                    close(s);
                    exit(1);
                }
                flag = ntohl(flag);

                if (flag == 1) {
                    flag = remove(file);
                    flag = htonl(flag);
                    if(send(new_s, &flag, sizeof(int), 0)==-1) 
                    {
                        fprintf(stderr, "myftpd: server send error\n");
                        close(new_s);
                        close(s);
                        exit(1);
                    }
                } else if (flag == -1) {
                    // do nothing 
                } else {
                    fprintf(stderr, "myftpd: invalid response from client\n");
                    close(new_s);
                    close(s);
                    exit(1);
                }
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
