#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdio.h>
#include <sys/unistd.h>
#include <stdlib.h>
#include <string.h>

int main()
{

    /*<Setting up socket and connection>*/

    printf("\n");

    int client=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in cl_addr;
    cl_addr.sin_family=AF_INET;
    cl_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
    cl_addr.sin_port=htons(9000);

    if(connect(client, (struct sockaddr*)&cl_addr, sizeof(cl_addr)))
    {
        perror("Connect failed");
        printf("\n");
        return 1;
    }
    printf("Connected: %s:%d\n", inet_ntoa(cl_addr.sin_addr), ntohs(cl_addr.sin_port));

    unsigned int client_count=0;
    if(read(client, &client_count, 4)<=0)
    {
        perror("Exception occured");
        printf("\n");
        return 1;
    }
    printf("Welcome! There are %d connections.\n", client_count);

    /*<>*/


    /*<Communicating>*/
    fd_set fd_read_sample;
    FD_ZERO(&fd_read_sample);
    FD_SET(client, &fd_read_sample);
    FD_SET(STDIN_FILENO, &fd_read_sample);
    fd_set fd_read;

    while(1)
    {
        fd_read=fd_read_sample;

        if(select(FD_SETSIZE, &fd_read, NULL, NULL, NULL)==-1)
        {
            perror("Exception occured");
            printf("\n");
            break;
        }

        if(FD_ISSET(client, &fd_read))
        {
            int buf_count=0;
            if(read(client, &buf_count, 4)<=0)
            {
                printf("Server closed.\n");
                break;
            }

            printf("Received: ");
            for(int index=0; index<buf_count; index++)
            {
                char* buf=malloc(64);
                read(client, buf, 64);
                printf("%s", buf);
            }
            printf("\n");
        }

        if(FD_ISSET(STDIN_FILENO, &fd_read))
        {
            unsigned int buf_count=0;
            char **buf_storage=NULL;

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
            write(client, &buf_count, 4);
            for(int index=0; index<buf_count; index++)
            {
                write(client, buf_storage[index], 64);
                printf("%s", buf_storage[index]);
            }
            printf("\n");
        }
    }

    printf("\n");

    /*<>*/


    /*<Close connection>*/

    close(client);
    printf("Client closed.\n");
    return 1;

    /*<>*/

}