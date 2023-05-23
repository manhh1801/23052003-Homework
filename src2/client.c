#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdio.h>
#include <sys/unistd.h>
#include <stdlib.h>
#include <string.h>

void main(int argc, char *argv[])
{

    /*<Setting up socket and connection>*/

    printf("\n");

    //Socket for sending

    int send_socket=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    struct sockaddr_in send_addr;
    send_addr.sin_family=AF_INET;
    send_addr.sin_addr.s_addr=inet_addr(argv[1]);
    send_addr.sin_port=htons(atoi(argv[2]));

    //Socket for receiving

    int recv_socket=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    struct sockaddr_in recv_addr;
    recv_addr.sin_family=AF_INET;
    recv_addr.sin_addr.s_addr=inet_addr(argv[1]);
//    recv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    recv_addr.sin_port=htons(atoi(argv[3]));

    if(bind(recv_socket, (struct sockaddr*)&recv_addr, sizeof(recv_addr)))
    {
        perror("Bind address to receiving socket failed");
        printf("\n");
        return;
    }

    printf("Connected: 127.0.0.1:%d-%d\n", ntohs(send_addr.sin_port), ntohs(recv_addr.sin_port));

    /*<>*/


    /*<Communicating>*/

    fd_set fd_read_sample, fd_read;
    FD_ZERO(&fd_read_sample);
    FD_SET(recv_socket, &fd_read_sample);
    FD_SET(STDIN_FILENO, &fd_read_sample);

    while(1)
    {
        fd_read=fd_read_sample;

        if(select(FD_SETSIZE, &fd_read, NULL, NULL, NULL)==-1)
        {
            perror("Exception occured");
            printf("\n");
            break;
        }

        if(FD_ISSET(recv_socket, &fd_read))
        {
            int buf_count=0;

            if(recvfrom(recv_socket, &buf_count, 4, 0, NULL, NULL)<=0)
            {
                printf("Connection closed.\n");
                break;
            }
            else
            {
                printf("Received: ");
                for(int index=0; index<buf_count; index++)
                {
                    char* buf=malloc(64);
                    recvfrom(recv_socket, buf, 64, 0, NULL, NULL);
                    printf("%s", buf);
                }
                printf("\n");
            }
        }

        if(FD_ISSET(STDIN_FILENO, &fd_read))
        {
            unsigned int buf_count=0;
            char** buf_storage=NULL;

            while(1)
            {
                char* input_recv=malloc(65);
                fgets(input_recv, 65, stdin);
                int buf_length=strlen(input_recv);
                if(buf_length==64)
                {
                    char* buf=malloc(64);
                    memcpy(buf, input_recv, 64);
                    buf_storage=(char**)realloc(buf_storage, 8*(buf_count+1));
                    buf_storage[buf_count++]=buf;
                    if(buf[63]==10)
                    {
                        buf[63]='\0';
                        break;
                    }
                }
                else
                {
                    if(buf_length!=1)
                    {
                        char* buf=malloc(64);
                        memcpy(buf, input_recv, 64);
                        buf_storage=(char**)realloc(buf_storage, 8*(buf_count+1));
                        buf_storage[buf_count++]=buf;
                        for(int index=buf_length-1; index<64; index++) {buf[index]='\0';}
                    }
                    break;
                }
            }

            printf("Sent: ");
            sendto(send_socket, &buf_count, 4, 0, (struct sockaddr*)&send_addr, sizeof(send_addr));
            for(int index=0; index<buf_count; index++)
            {
                sendto(send_socket, buf_storage[index], 64, 0, (struct sockaddr*)&send_addr, sizeof(send_addr));
                printf("%s", buf_storage[index]);
            }
            printf("\n");
        }
    }

    /*<>*/


    /*<Close connection>*/

    close(send_socket);
    return;

    /*<>*/

}