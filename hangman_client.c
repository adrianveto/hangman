#include "networking.h"
#include <string.h>
#include <signal.h>

/*
  Arguments: the socket to the server
  Behavior: handles user input from the terminal, and then passes the commands along to the server
  Returns: none
*/
void clientInput(int server_socket){
  char input[WORD_SIZE];
  char buff[WORD_SIZE];
  // printf("enter a command: ");
  fgets(input, WORD_SIZE, stdin);
  input[strcspn(input, "\n")] = 0;

  // command logic
  // check which command
  if (strcasecmp(input, "help") == 0) {
    printf("To send a message, type 'chat'\n");
    printf("To view the current game status, type 'status'\n");
    printf("To make a letter guess, type 'guess'\n");
    printf("To make a word guess, type 'guess-word'\n");
    printf("To exit the game, type 'quit'\n");
  }
  // quit
  else if (strcasecmp(input, "quit") == 0) {
    write(server_socket, "quit", 5);
    exit(0);
  }
  else if (strcasecmp(input, "status") == 0) {
    write(server_socket, "status", 7);
    usleep(50);
    char buff[MESSAGE_SIZE];
    read(server_socket, buff, MESSAGE_SIZE);
    printf("%s", buff);
  }
  // guess
  else if (strcasecmp(input, "guess") == 0) {
    write(server_socket, "guess", 6);
    // printf("wrote to server\n");
    usleep(50);
    read(server_socket, buff, WORD_SIZE);
    // printf("read from server\n");
    if (strcasecmp(buff, "no") == 0) {
        printf("Wait for your turn!\n");
    }
    else if (strcasecmp(buff, "yes") == 0) {
        printf("guess a letter: ");
        fgets(input, WORD_SIZE, stdin);
        input[strcspn(input, "\n")] = 0;
        write(server_socket, input, WORD_SIZE);
    }
  }
  // guess-word
  else if (strcasecmp(input, "guess-word") == 0) {
    write(server_socket, "guess-word", 11);
    // printf("wrote to server\n");
    usleep(50);
    read(server_socket, buff, WORD_SIZE);
    // printf("read from server\n");
    if (strcasecmp(buff, "no") == 0) {
        printf("Wait for your turn!\n");
    }
    else if (strcasecmp(buff, "yes") == 0) {
        printf("guess a word: ");
        fgets(input, WORD_SIZE, stdin);
        input[strcspn(input, "\n")] = 0;
        write(server_socket, input, WORD_SIZE);
    }
  }
  else if (strcasecmp(input, "chat") == 0) {
    // chat server implementation -- implement later
    write(server_socket, "chat", 5);
    printf("chat message: ");
    usleep(50);
    char buff[MESSAGE_SIZE - 40];
    fgets(buff, MESSAGE_SIZE - 40, stdin);
    buff[strcspn(buff, "\n")] = 0;
    write(server_socket, buff, MESSAGE_SIZE - 40);
    printf("[me]: %s\n", buff);
  }
  else {
    printf("Invalid command. Type 'help' for a list of commands.\n");
  }
  
}

void displayServerMessage(int server_socket) {
  // printf("message from server received\n");
  char buff[MESSAGE_SIZE];
  int i = read(server_socket, buff, MESSAGE_SIZE);
  // printf("buff[0]: %c\n", buff[0]);
  // printf("i: %d, buff: %s\n", i, buff);
  if (i == -1) {
    error(-1, "reading from server failed");
  }
  else if (i == 0 || strcasecmp(buff, "quit") == 0) {
    printf("\n***Server disconnected\n");
    exit(0);
  }
  else if (strcasecmp(buff, "choose") == 0) {
    printf("\n***Choose starting word: ");
    char startWord[WORD_SIZE];
    fgets(startWord, WORD_SIZE, stdin);
    write(server_socket, startWord, WORD_SIZE);
  }
  else if (strcasecmp(buff, "guess") == 0) {
    printf("\n***It's your turn to guess!\n");
  }
  else if (buff[0] == '[') {
    printf("\n%s\n", buff);
    fflush(stdout);
  }
  else {
    printf("\n***%s", buff);
  }
} 

int main(int argc, char** argv) {
    char* IP = "127.0.0.1";
    if(argc>1){
        IP=argv[1];
    }
    int server_socket = client_tcp_handshake(IP);
    printf("client connected.\n");
    // gets username
    char username[WORD_SIZE];
    printf("Enter your username: ");
    fgets(username, 20, stdin);
    username[strcspn(username, "\n")] = 0;
    write(server_socket, username, 20);

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(STDIN_FILENO, &read_fds);
    FD_SET(server_socket, &read_fds);

    while (1) {
      printf("enter a command: ");
      fflush(stdout);
      FD_ZERO(&read_fds);
      FD_SET(STDIN_FILENO, &read_fds);
      FD_SET(server_socket, &read_fds);
      int i = select(server_socket + 1, &read_fds, NULL, NULL, NULL);
      if (FD_ISSET(STDIN_FILENO, &read_fds)) {
        // printf("stdin command\n");
        clientInput(server_socket);
      }
      else if (FD_ISSET(server_socket, &read_fds)) {
        // printf("server socket command\n");
        displayServerMessage(server_socket);
      }
      usleep(50);
    }
}
