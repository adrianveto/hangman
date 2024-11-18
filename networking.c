#include "networking.h"

void error(int number, char* message) {
    if (number == -1) {
        printf("%s\n", message);
        printf("%s\n", strerror(errno));
        exit(1);
    }
}

/*
    creates the server socket for clients to connect to
*/
int server_setup() {
    struct addrinfo * hints, * results;
    hints = calloc(1,sizeof(struct addrinfo));
    char* PORT = "1738";

    hints->ai_family = AF_INET;
    hints->ai_socktype = SOCK_STREAM; //TCP socket
    hints->ai_flags = AI_PASSIVE; //only needed on server
    error(getaddrinfo(NULL, PORT , hints, &results), "getaddrinfo failed");  //NULL is localhost or 127.0.0.1

    //create socket
    int listen_socket = socket(results->ai_family, results->ai_socktype, results->ai_protocol);\

    //this code allows the port to be freed after program exit.  (otherwise wait a few minutes)
    int yes = 1;
    error(( setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1 ), "setsockopt failed");

    error(bind(listen_socket, results->ai_addr, results->ai_addrlen), "bind failed");
    listen(listen_socket, 3);//3 clients can wait to be processed
    printf("Listening on port %s\n",PORT);

    free(hints);
    freeaddrinfo(results);
    return listen_socket;
}

/*Accept a connection from a client
 *return the to_client socket descriptor
 *blocks until connection is made.
 */
int server_tcp_handshake(int listen_socket){
    int client_socket;
    socklen_t sock_size;
    struct sockaddr_storage client_address;
    sock_size = sizeof(client_address);

    //accept the client connection
    client_socket = accept(listen_socket,(struct sockaddr *)&client_address, &sock_size);
  
    return client_socket;
}

/*Connect to the server
 *return the to_server socket descriptor
 *blocks until connection is made.*/
int client_tcp_handshake(char * server_address) {

  //getaddrinfo
  struct addrinfo *hints, *results;
  hints = calloc(1, sizeof(struct addrinfo));
  hints->ai_family = AF_INET;
  hints->ai_socktype = SOCK_STREAM; //TCP socket
  hints->ai_flags = AI_PASSIVE; //only needed on server
  error(getaddrinfo(server_address, "1738", hints, &results), "getaddrinfo failed");
  
  //create the socket
  int sd = socket(results->ai_family, results->ai_socktype, results->ai_protocol);
  
  //connect to the server
  error(connect(sd, results->ai_addr, results->ai_addrlen), "connect to server failed");
  
  free(hints);
  freeaddrinfo(results);

  return sd;
}

