#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdio.h>
#include <sys/unistd.h>
#include <stdlib.h>
#include <string.h>

int main()
{

    /*<Setting up socket>*/

    printf("\n");

    int server=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in sv_addr;
    sv_addr.sin_family=AF_INET;
    sv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    sv_addr.sin_port=htons(9000);

    if(bind(server, (struct sockaddr*)&sv_addr, sizeof(sv_addr)))
    {
        perror("Bind address to socket failed");
        printf("\n");
        return 1;
    }

    /*<>*/


    /*<Waiting and accepting connection>*/

    if(listen(server, 5))
    {
        perror("Connect failed");
        printf("\n");
        return 1;
    }

    /*<>*/


    /*<Communicating>*/

    fd_set fd_read;
    FD_ZERO(&fd_read);

    typedef struct client
    {
        int client;
        struct sockaddr_in cl_addr;
        struct client *next;
    }
    client;
    client** clients=malloc(sizeof(client*));
    *clients=NULL;
    unsigned int client_count=0;

    while(1)
    {
        FD_SET(server, &fd_read);
        client* temp=*clients;
        while(temp!=NULL)
        {
            FD_SET(temp->client, &fd_read);
            temp=temp->next;
        }

        if(select(FD_SETSIZE, &fd_read, NULL, NULL, NULL)==-1)
        {
            perror("Exception occured");
            printf("\n");
            break;
        }

        temp=*clients;
        client* prev=NULL;
        while(temp!=NULL)
        {
            if(FD_ISSET(temp->client, &fd_read))
            {
                unsigned int buf_count=0;
                char **buf_storage=NULL;

                if(read(temp->client, &buf_count, 4)<=0)
                {
                    printf("%s:%d disconnected.\n", inet_ntoa((temp->cl_addr).sin_addr), ntohs((temp->cl_addr).sin_port));
                    client* delete=temp;
                    temp=temp->next;
                    free(delete);
                    client_count-=1;
                    if(prev!=NULL) {prev->next=temp;}
                    else {*clients=temp;}
                }

                else
                {
                    buf_storage=(char**)malloc(8*buf_count);
                    printf("Received from %s:%d: ", inet_ntoa((temp->cl_addr).sin_addr), ntohs((temp->cl_addr).sin_port));
                    for(int index=0; index<buf_count; index++)
                    {
                        char* buf=malloc(64);
                        read(temp->client, buf, 64);
                        buf_storage[index]=buf;
                        printf("%s", buf);
                    }
                    printf("\n");

                    unsigned int start_index=0;
                    unsigned int end_index=buf_count*64-1;
                    char break_flag=0;

                    for(int buf_index=0; buf_index<buf_count; buf_index++)
                    {
                        for(int char_index=0; char_index<64; char_index++)
                        {
                            char temp_char=buf_storage[buf_index][char_index];
                            if(temp_char!=' '&&temp_char!='\n'&&temp_char!='\t'&&temp_char!='\0')
                            {
                                break_flag=1;
                                break;
                            }
                            start_index+=1;
                        }
                        if(break_flag==1) {break;}
                    }
                    if(break_flag==0) {start_index-=1;}
                    break_flag=0;
                    for(int buf_index=buf_count-1; buf_index>-1; buf_index--)
                    {
                        for(int char_index=63; char_index>-1; char_index--)
                        {
                            char temp_char=buf_storage[buf_index][char_index];
                            if(temp_char!=' '&&temp_char!='\n'&&temp_char!='\t'&&temp_char!='\0')
                            {
                                break_flag=1;
                                break;
                            }
                            end_index-=1;
                        }
                        if(break_flag==1) {break;}
                    }
                    if(break_flag==0) {end_index+=1;}

                    if(start_index>end_index)
                    {
                        unsigned int sendbuf_count=1;
                        write(temp->client, &sendbuf_count, 4);
                        char* sendbuf=malloc(64);
                        memcpy(sendbuf, "Cannot process text.", 20);
                        write(temp->client, sendbuf, 64);
                    }
                    else
                    {
                        unsigned int sendbuf_count=0;
                        char **sendbuf_storage=NULL;

                        char last_char=255;

                        unsigned int index=start_index;
                        while(index<=end_index)
                        {
                            char* buf=(char*)malloc(64);
                            int buf_bytecount=0;
                            while(buf_bytecount<=64&&index<=end_index)
                            {
                                char temp_char=buf_storage[index/64][index-64*(index/64)];
                                if((temp_char==' '||temp_char=='\n'||temp_char=='\t'||temp_char=='\0')&&(last_char==' '||last_char=='\n'||last_char=='\t'||last_char=='\0')) {}
                                else
                                {
                                    memcpy(buf+buf_bytecount, &temp_char, 1);
                                    buf_bytecount++;
                                }
                                last_char=temp_char;
                                index+=1;
                            }

                            if(buf_bytecount!=0)
                            {
                                sendbuf_storage=(char**)realloc(sendbuf_storage, 8*(sendbuf_count+1));
                                sendbuf_storage[sendbuf_count]=buf;
                                sendbuf_count+=1;
                            }
                        }

                        write(temp->client, &sendbuf_count, 4);
                        if(sendbuf_count==1&&!strcmp(sendbuf_storage[0], "exit"))
                        {
                            char* buf=malloc(64);
                            memcpy(buf, "Goodbye!", 8);
                            write(temp->client, buf, 64);
                        }
                        for(int index=0; index<sendbuf_count; index++) {write(temp->client, sendbuf_storage[index], 64);}
                    }

                    prev=temp;
                    temp=temp->next;
                }
            }
        }

        if(FD_ISSET(server, &fd_read))
        {
            client* new_client=malloc(sizeof(client));
            int cl_addr_length=sizeof(new_client->cl_addr);
            new_client->client=accept(server, (struct sockaddr*)&(new_client->cl_addr), &cl_addr_length);
            new_client->next=*clients;
            *clients=new_client;
            client_count+=1;
            printf("Accepted connection: %s:%d\n", inet_ntoa((new_client->cl_addr).sin_addr), ntohs((new_client->cl_addr).sin_port));
            write(new_client->client, &client_count, 4);
        }

        FD_ZERO(&fd_read);
    }

    printf("\n");

    /*<>*/


    /*<Close connection>*/

    close(server);
    printf("Server closed.\n");
    printf("\n");

    return 1;

    /*<>*/

}