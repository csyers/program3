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
    struct timeval tv_start, tv_end;

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
            int bytes;
            char server_hash[16];
            char client_hash[16];
            MHASH td;

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
            if(send(s, &file,strlen(file),0)==-1)
            {
                fprintf(stderr,"myftp: error in send\n");
                close(s);
                exit(1);
            }

            if((recv(s, &filesize, sizeof(int), 0)) == -1) {
                fprintf(stderr,"myftp: error in recv\n");
                close(s);
                exit(1);
            }

            filesize = ntohl(filesize);
            fflush(stdout);
        
            if(filesize == -1) {
                printf("file: %s does not exist on the server\n", file);
            } else {
                    // receive server_hash
                        if((recv(s, server_hash, sizeof(server_hash), 0)) == -1) {
                            fprintf(stderr,"myftp: error in recv\n");
                            close(s);
                            exit(1);
                    }
                    // receive file
                    if(!(fp = fopen(file, "w+"))) {    
                        fprintf(stderr,"myftp: error in recv\n");
                        close(s);
                        exit(1);
                    }
                    
                    gettimeofday(&tv_start,0);

                    int remaining_filesize = filesize;
                    while(remaining_filesize > 0) {
                        bytesReceived = recv(s, buf, MAX_LINE, 0);
                        if(bytesReceived < 0) {
                            fprintf(stderr,"myftp: error in recv\n");
                            close(s);
                            exit(1);
                        }
                        remaining_filesize -= bytesReceived;
                        if(fwrite(buf, sizeof(char), bytesReceived, fp) < bytesReceived) {
                            fprintf(stderr, "myftp: error in write\n");
                            close(s);
                            exit(1);
                        }
                    }

                    gettimeofday(&tv_end,0);

                    td = mhash_init(MHASH_MD5);
                    if (td == MHASH_FAILED) {
                        fprintf(stderr, "myftpd: md5 hash failed\n");
                        close(s);
                           exit(1);
                    }
                    rewind(fp);
                    bzero(buf, sizeof(buf));
                    bytes=fread(buf,1,MAX_LINE,fp);
                    while(bytes>0)
                    {
                        mhash(td,&buf,sizeof(buf));	
                        bzero(buf, sizeof(buf));
                        bytes=fread(buf,1,MAX_LINE,fp);
                    }
                  
                    mhash_deinit(td, client_hash);
                    fclose(fp);
                    if(strncmp(client_hash,server_hash, 16) == 0)
                    {
                        // files are the same   
                        // calculate time of file transfer
                        long int time_diff = (tv_end.tv_sec - tv_start.tv_sec)*1000000L +tv_end.tv_usec - tv_start.tv_usec;
                        double throughput = filesize/(double)time_diff;
                        printf("Transfer was successful\n");
                        printf("Throughput: %lf MBps\n",throughput);
                    } else
                    {
                        // files are different 
                        printf("Files are different. Transfer was unsuccessful\n");
                        remove(file);
                    }
                }
        }
        // case: UPL
        else if (strcmp(buf,"UPL") == 0)
        {
        int ack, bytes, filesize;
        int time_diff;
        double throughput;
        FILE *fp;
        struct stat st;
        char client_hash[16];
        MHASH td;

            // send UPL to server, print error and exit on failure
            if(send(s,buf,len+1,0)==-1)
            {
                fprintf(stderr,"myftp: error in send\n");
                close(s);
                exit(1);
            }

            // prompt user for filename
            printf("Enter file to upload: ");
            bzero(file, sizeof(file));
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

            int sent=0;

            // send len_filename
            len_filename = htons(len_filename);
            if((sent=send(s, &len_filename,sizeof(len_filename),0))==-1)
            {
                fprintf(stderr,"myftp: error in send\n");
                close(s);
                exit(1);
            }
        
            // send filename
            if((sent=send(s, &file,strlen(file),0))==-1)
            {
                fprintf(stderr,"myftp: error in send\n");
                close(s);
                exit(1);
            }

        // receive ack
            if((recv(s, &ack, sizeof(int), 0)) == -1) {
                fprintf(stderr,"myftp: error in recv\n");
                close(s);
                exit(1);
            }
            ack = ntohl(ack);

            if (ack != -1) {
                fprintf(stderr, "myftpd: upl was not acknowledged\n");
                close(s);
                exit(1);
            }

        // send filesize
            stat(file, &st);
            filesize = st.st_size;
            filesize = htonl(filesize);
            if(send(s, &filesize, sizeof(int),0)==-1) {
                fprintf(stderr, "myftpd: client send error\n");
                close(s);
                exit(1);
            }
        // send file
            if (!(fp = fopen(file, "r"))) {
                fprintf(stderr, "myftpd: file doesn't exist\n");
                close(s);
                exit(1);
            }

            while(1) {
                bzero((char *)buf, sizeof(buf));
                int nred = fread(buf, sizeof(char), MAX_LINE, fp);
                if (nred > 0) {
                    if (send(s, &buf, nred, 0)==-1) {
                        fprintf(stderr, "myftpd: error reading file\n");
                        close(s);
                        exit(1);
                    }
                }

                if (nred < MAX_LINE) {
                    if(ferror(fp)) {
                        fprintf(stderr, "myftpd: error reading file\n");
                        close(s);
                        exit(1);
                    }
                    break;
                }
            }

            // calculate MD5 hash
            fseek(fp, 0, SEEK_SET);
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
            mhash_deinit(td,client_hash);

            // send MD5 hash
            if(send(s, &client_hash, sizeof(client_hash), 0) == -1)
            {
                fprintf(stderr, "myftp: hashing failed\n");
                close(s);
                exit(1);
            }

            // receive throughput
            if((recv(s, &time_diff, sizeof(int), 0)) == -1) {
                fprintf(stderr,"myftp: error in recv\n");
                close(s);
                exit(1);
            }
            time_diff = ntohl(time_diff);

            if (time_diff == -1) {
                printf("upload unsuccessful\n");
            } else {
                filesize = ntohl(filesize);
                throughput = filesize/(double)time_diff;
                printf("Transfer was successful\n");
                printf("Throughput: %lf MBps\n",throughput);
            }


        // case: LIS
        }
        else if (strcmp(buf,"LIS") == 0)
        {
            char listing[MAX_LINE];
            int len_listing;

            // send LIS to server, print error and exit on failure
            if(send(s,buf,len+1,0)==-1)
            {
                fprintf(stderr,"myftp: error in send\n");
                close(s);
                exit(1);
            }

            // receive listing size
            if((recv(s, &len_listing, sizeof(int), 0)) == -1) {
                fprintf(stderr,"myftp: error in recv\n");
                close(s);
                exit(1);
            }
            len_listing = ntohl(len_listing);

            // receive listing
            bzero(listing, sizeof(listing));
            if((recv(s, listing, len_listing, 0)) == -1) {
                fprintf(stderr,"myftp: error in recv\n");
                close(s);
                exit(1);
            }

            printf("%s", listing);

        // case: MKD
        }
        else if (strcmp(buf,"MKD") == 0)
        {
            int resp;

            // send MKD to server, print error and exit on failure
            if(send(s,buf,len+1,0)==-1)
            {
                fprintf(stderr,"myftp: error in send\n");
                close(s);
                exit(1);
            }

            // prompt user for filename
            printf("Enter directory to create: ");
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
            if(send(s, &file,strlen(file),0)==-1)
            {
                fprintf(stderr,"myftp: error in send\n");
                close(s);
                exit(1);
            }

            // receive resp
            if((recv(s, &resp, sizeof(int), 0)) == -1) {
                fprintf(stderr,"myftp: error in recv\n");
                close(s);
                exit(1);
            }
            resp = ntohl(resp);

            if (resp == -2) {
                printf("The directory already exists on the server\n");
            } else if (resp == -1) {
                printf("Error making directory\n");
            } else if (resp == 1) {
                printf("Directory successfully made\n");
            } else {
                fprintf(stderr, "myftp: error in server response\n");
            }

        // case: RMD
        }
        else if (strcmp(buf,"RMD") == 0)
        {
            int resp, flag, return_val;
            short int len_dirname;
            int bytesSent;
            int bytesReceived;
            char dir[MAX_LINE];

            // send RMD to server, print error and exit on failure
            if(send(s,buf,len+1,0)==-1)
            {
                fprintf(stderr,"myftp: error in send\n");
                close(s);
                exit(1);
            }

            // prompt user for dirname
            printf("Enter directory to delete: ");
            if(fgets(dir,sizeof(dir),stdin)<0)
            {
                fprintf(stderr,"myftp: error in fgets\n");
                close(s);
                exit(1);
            }
            dir[MAX_LINE-1] = '\0';
            len_dirname = strlen(dir);

            // strip the newline character from the buffer
            if((len_dirname-1 > 0) && (dir[len_dirname-1] == '\n'))
            {
                dir[len_dirname-1] = '\0';
                len_dirname--;
            }

            // send size
            len_dirname = htons(len_dirname);
            if((bytesSent=send(s, &len_dirname,sizeof(len_dirname),0))==-1)
            {
                fprintf(stderr,"myftp: error in send\n");
                close(s);
                exit(1);
            }
        
            // send dirname
            if((bytesSent=send(s, &dir,strlen(dir),0))==-1)
            {
                fprintf(stderr,"myftp: error in send\n");
                close(s);
                exit(1);
            }

            // receive resp from server
            if((bytesReceived=recv(s, &resp, sizeof(int), 0)) == -1) {
                fprintf(stderr,"myftp: error in recv\n");
                close(s);
                exit(1);
            }
            resp = ntohl(resp  );

            // confirm delete
            if (resp == 1) {
                bzero(buf, sizeof(buf));
                while (strcmp(buf, "Yes") != 0 && strcmp(buf, "No") != 0) {
                    printf("are you sure you want to delete %s? Please answer Yes or No: ", dir);
                    if(fgets(buf,sizeof(buf),stdin)<0)
                    {
                        fprintf(stderr,"myftp: error in fgets\n");
                        close(s);
                        exit(1);
                    }
                    buf[MAX_LINE-1] = '\0';
                    len = strlen(buf);

                    // strip the newline character from the buffer
                    if((len-1 > 0) && (buf[len-1] == '\n'))
                    {
                        buf[len-1] = '\0';
                        len--;
                    }
                }
                if (strcmp(buf, "Yes") == 0) {
                    flag = 1;
                    flag = htonl(flag);
                    if(send(s, &flag ,sizeof(int),0)==-1)
                    {
                        fprintf(stderr,"myftp: error in send\n");
                        close(s);
                        exit(1);
                    }
                    // receive server delete resp
                    if((recv(s, &return_val, sizeof(int), 0)) == -1) {
                    fprintf(stderr,"myftp: error in recv\n");
                    close(s);
                    exit(1);
                    }
                    return_val = ntohl(return_val);

                    if (return_val == 0) {
                        printf("Directory deleted\n");
                    } else {
                        printf("Failed to delete directory\n");
                    }
                } else if (strcmp(buf, "No") == 0) {
                    printf("Delete abandoned by the user!\n");
                    flag = -1;
                    flag = htonl(flag);
                    if(send(s, &flag ,sizeof(int),0)==-1)
                    {
                        fprintf(stderr,"myftp: error in send\n");
                        close(s);
                        exit(1);
                    }
                }
            } else if (resp == -1) {
                printf("The directory does not exist on server\n");
            } else {
                fprintf(stderr, "myftp: invalid response from server: %d\n", resp);
            }

        // case: CHD
        }
        else if (strcmp(buf,"CHD") == 0)
        {

        // case: DEL
        }
        else if (strcmp(buf,"DEL") == 0)
        {
            int resp, flag, return_val;

            // send DEL to server, print error and exit on failure
            if(send(s,buf,len+1,0)==-1)
            {
                fprintf(stderr,"myftp: error in send\n");
                close(s);
                exit(1);
            }

            // prompt user for filename
            printf("Enter file to delete: ");
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
            if(send(s, &file, strlen(file),0)==-1)
            {
                fprintf(stderr,"myftp: error in send\n");
                close(s);
                exit(1);
            }

            // receive resp from server
            if((recv(s, &resp, sizeof(int), 0)) == -1) {
                fprintf(stderr,"myftp: error in recv\n");
                close(s);
                exit(1);
            }
            resp = ntohl(resp);

            // confirm delete
            if (resp == 1) {
                bzero(buf, sizeof(buf));
                while (strcmp(buf, "Yes") != 0 && strcmp(buf, "No") != 0) {
                    printf("are you sure you want to delete %s? Please answer Yes or No: ", file);
                    if(fgets(buf,sizeof(buf),stdin)<0)
                    {
                        fprintf(stderr,"myftp: error in fgets\n");
                        close(s);
                        exit(1);
                    }
                    buf[MAX_LINE-1] = '\0';
                    len = strlen(buf);

                    // strip the newline character from the buffer
                    if((len-1 > 0) && (buf[len-1] == '\n'))
                    {
                        buf[len-1] = '\0';
                        len--;
                    }
                }
                if (strcmp(buf, "Yes") == 0) {
                    flag = 1;
                    flag = htonl(flag);
                    if(send(s, &flag ,sizeof(int),0)==-1)
                    {
                        fprintf(stderr,"myftp: error in send\n");
                        close(s);
                        exit(1);
                    }
                    // receive server delete resp
                    if((recv(s, &return_val, sizeof(int), 0)) == -1) {
                    fprintf(stderr,"myftp: error in recv\n");
                    close(s);
                    exit(1);
                }
                return_val = ntohl(return_val);

                if (return_val == 0) {
                    printf("Deletion successful\n");
                } else {
                    printf("Deletion unsuccessful\n");
                }
                } else if (strcmp(buf, "No") == 0) {
                    printf("Delete abandoned by the user!\n");
                    flag = -1;
                    flag = htonl(flag);
                    if(send(s, &flag ,sizeof(int),0)==-1)
                    {
                        fprintf(stderr,"myftp: error in send\n");
                        close(s);
                        exit(1);
                    }
                }
            } else if (resp == -1) {
                printf("myftp: file does not exist on server\n");
            } else {
                fprintf(stderr, "myftp: invalid response from server: %d\n", resp);
            }
            
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
