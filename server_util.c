#include "server_util.h"

// Global variables of the server
char hostname[HOSTNAME_LENGTH];
// Init flag
int intialized;

// TODO: Add a global entry for client info storage
// I am too lazy to write something like a search tree

// Initialize the server (initializing the hostname is enough)
int init(void)
{
    if(!intialized) intialized = 1;
    if(gethostname(hostname, HOSTNAME_LENGTH))
        return FAILED_HOSTNAME;
    return SUCCEED_EXITCODE;
}

// Server reply current time 
int server_action_rpl_time(
    int client_sock, const char *recv_buffer)
{
    char mesg_buffer[MESSAGE_LENGTH];
    int mesg_length = reply_time_msg(mesg_buffer);
    // Write to the socket
    write(client_sock, mesg_buffer, mesg_length);

    return SUCCEED_EXITCODE;
}

// Server reply hostname
int server_action_rpl_hostname(
    int client_sock, const char *recv_buffer)
{
    char mesg_buffer[MESSAGE_LENGTH];
    int mesg_length = reply_hostname_msg(mesg_buffer, hostname);
    write(client_sock, mesg_buffer, mesg_length);

    return SUCCEED_EXITCODE;
}

// Create a new socket on the referred port number
// The return value is either an error code (negative)
// or a socket descriptor.
int new_socket(uint16_t port)
{
    int init_status;
    if(!intialized) init_status = init();
    if(init_status != SUCCEED_EXITCODE) return init_status;
    // Create socket
    int socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_desc == -1)
    {
#ifdef SERVER_OUTPUT
        perror("Socket creation failed.");
#endif
        return FAILED_SOCKET_CREATION;
    }
#ifdef SERVER_OUTPUT
    puts("Socket created.");
#endif
    // sockaddr_in strcuture
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);

    // Binding
    if(bind(socket_desc, (struct sockaddr*)&server, sizeof(server)))
    {
#ifdef SERVER_OUTPUT
        perror("Binding failed.");
#endif
        return FAILED_BINDING;
    }
#ifdef SERVER_OUTPUT
    puts("Binded");
#endif

    // Listening
    listen(socket_desc, 3);
    return socket_desc;
}

// The argument is the socket descriptor
int accept_client(int sock_desc, int *client_sock)
{
#ifdef SERVER_OUTPUT
    puts("Waiting for connection.");
#endif
    struct sockaddr_in client;
    int addrlen = sizeof(struct sockaddr_in);

    // Accept a connection
    if((*client_sock = accept(sock_desc, (struct sockaddr*)&client, (socklen_t*)&addrlen)))
    {
#ifdef SERVER_OUTPUT
        puts("Connection accepted");
#endif
        // This is the client message buffer
        char recv_buffer[MESSAGE_LENGTH];
        int read_size;
        if((read_size = recv(*client_sock, recv_buffer, MESSAGE_LENGTH, 0)) <= 0)
            return SUCCEED_EXITCODE;
        if(!is_custom_protocol((unsigned char*)recv_buffer))
        {
#ifdef SERVER_OUTPUT
            puts("Unknown protocol.");
#endif
            return UNKNOWN_PROTOCOL;
        }
        // Receiving the client info
        int length = PROTOCOL_HEADER_LEN + get_body_length((unsigned char*)recv_buffer);
        char hostname[HOSTNAME_LENGTH];
        struct sockaddr_in client_socket_addr;
        msg2client_info((unsigned char*)hostname, &client_socket_addr, recv_buffer);
        add_client(sock_desc, &client_socket_addr);
        return SUCCEED_EXITCODE;
    }
    return FAILED_CONNECTING;
}

// And this is the server thread routine
void service(int client_sock)
{
    char recv_buffer[MESSAGE_LENGTH];
    int read_size;
    while((read_size = recv(client_sock, recv_buffer, MESSAGE_LENGTH, 0)) > 0)
    {
        if(!is_custom_protocol((unsigned char*)recv_buffer))
        {
#ifdef SERVER_OUTPUT
            puts("Unknown protocol.");
#endif
            return; // Exit thread
        }
        switch(get_msg_type((unsigned char*)recv_buffer))
        {
            case REQ_TIME:
                server_action_rpl_time(client_sock, recv_buffer); break;
            case REQ_HOSTNAME: 
                server_action_rpl_hostname(client_sock, recv_buffer); break;
            default: break;
        }
    }
}