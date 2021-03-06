#include "server_util.h"

int server_should_stop = 0;

void* start_serve(void* client_desc) 
{
    service(*(int*)client_desc);
    pthread_exit(0);
}

void* start_accept(void* server_desc) 
{
    while (!server_should_stop)
    {
        int client_desc;
        int accept_exitcode = accept_client(*(int*)server_desc, &client_desc);
        if(accept_exitcode != SUCCEED_EXITCODE)
        {
            printf("Accept Fault: %d\n", accept_exitcode);
            pthread_exit(0);
        }

        pthread_t  pid;
        pthread_create(&pid, NULL, start_serve, &client_desc);
        pthread_detach(pid);

        //service(client_desc);
    }
    pthread_exit(0);
}

int getPort(const char* str) 
{
    int length = strlen(str);
    int num=0;
    for(int i = 0; i<length; i++)
    {
        num = num * 10 +str[i] - '0';
    }
    return num;
}

int main(int argc, char **argv)
{
    int port;
    if(argc>1)
    {
        port = getPort(argv[1]);
        if(port<=1024 || port >=65535) 
        {
            printf("port number invalid, using default port 4531\n");
            port = 4531;
        }
    }
    else {
        port = 4531;
    }
    int server_desc = new_socket(port);
    if(server_desc < 0) return server_desc;
    pthread_t pid;
    pthread_create(&pid, NULL, start_accept, &server_desc);

    char input[256];
    scanf("%s", input);
    if(strcmp((char*)input, (char*)"exit")==0) 
    {
        server_should_stop = 1;
        close(server_desc);
    }

    return 0;
}