#include "hangman_server.h"
#include "hangman.h"
#include "networking.h"

/*
    Arguments: the starting game info struct
    Behavior: prompts the user for a word for the game
    Returns: the new game info struct
*/
struct game_info* user_start_word(struct game_info* game) {
    game->real_word = malloc(WORD_SIZE);
    write(game->client_sockets[game->chooser], "choose", 7);
    usleep(50);
    read(game->client_sockets[game->chooser], game->real_word, WORD_SIZE);
    game->real_word[strcspn(game->real_word, "\n")] = 0;

    return game;
}

/*
    Arguments: the game info struct, the message to send to the clients
    Behavior: Sends the message to every client in the client list
    Returns: None
*/
void message_blast(struct game_info *game, char *message, int exclude_index) {
    if (exclude_index == -1) {
        for (int i = 0; i < game->num_clients; i++) {
            // printf("attempting to write to client %s\n", game->usernames[i]);
            error(write(game->client_sockets[i], message, MESSAGE_SIZE), "Writing to client failed");
            // printf("%d bytes written\n", i);
        }
    }
    else {
        for (int i = 0; i < game->num_clients; i++) {
            // printf("attempting to write to client %s\n", game->usernames[i]);
            if (i != exclude_index) {
                error(write(game->client_sockets[i], message, MESSAGE_SIZE), "Writing to client failed");
                // printf("%d bytes written\n", i);
            }
        } 
    }
}

/* 
    Arguments: the game information, the index of the client
    Behavior: checks if the character is included in the word
    Returns: none
*/
void client_guess(int index, struct game_info* game) {
    char* buff = malloc(WORD_SIZE);
    // printf("in client_guess. index %d guesser %d\n", index, game->guesser);
    // if the user isn't the guesser
    if (index != game->guesser) {
        write(game->client_sockets[index], "no", 3);
        // printf("wrote no\n");
        return;
    }
    // printf("client socket[0] (1): %d\n", game->client_sockets[0]); // passed
    write(game->client_sockets[index], "yes", 4);
    // printf("wrote yes\n");
    usleep(50);
    // printf("client socket[0] (2): %d\n", game->client_sockets[0]); // passed
    error(read(game->client_sockets[index], buff, WORD_SIZE), "read failed");
    // printf("client socket[0] (3): %d\n", game->client_sockets[0]); // passed
    char guess = buff[0];
    // printf("1\n"); passed 1

    char message[MESSAGE_SIZE];

    sprintf(message, "%s guessed the letter %c. To view game status, type 'status'.\n", game->usernames[index], guess);

    message_blast(game, message, -1);

    game = checkLetterGuess(game, guess);

    free(buff);
}

/* 
    Arguments: the game information, the index of the client
    Behavior: checks if the word is correct
    Returns: none
*/
void client_guess_word(int index, struct game_info* game) {
    // printf("in client guess word\n");
    char buff[WORD_SIZE];
    if (index != game->guesser) {
        write(game->client_sockets[index], "no", 3);
        // printf("wrote no\n");
        return;
    }
    write(game->client_sockets[index], "yes", 4);
    // printf("wrote yes\n");
    usleep(50);
    error(read(game->client_sockets[index], buff, WORD_SIZE), "read failed");
    buff[strcspn(buff, "\n")] = 0;

    char message[MESSAGE_SIZE];
    // printf("username: %s\n", game->usernames[index]);
    // printf("buff: %s\n", buff);
    sprintf(message, "%s guessed the word %s. To view game status, type 'status'.\n", game->usernames[index], buff);
    // printf("message: %s\n", message);
    message_blast(game, message, -1);

    game = checkWordGuess(game, buff);
}

/* 
    Arguments: the index for the client socket, the game info struct
    Behavior: gets the game status, writes it to a string, and sends the string to the client
    Returns: none
*/
void client_status(int index, struct game_info* game) {
    char* buff = malloc(MESSAGE_SIZE);
    if (game->guessing_order == NULL) {
        sprintf(buff, "Game hasn't started yet!\n");
    }
    else if (index == game->guessing_order[game->guesser_index]) {
        sprintf(buff, "-----------------\nThe word is %s.\nThe incorrect guesses are %s\nThere are %d guesses remaining.\nIt's your turn to guess!\n-----------------\n", game->current_word, game->failed_guesses, game->num_guesses);
    }
    else if (index == game->chooser && game->gamemode == USER_CHOOSING) {
        sprintf(buff, "-----------------\nThe word is %s.\nThe incorrect guesses are %s\nThere are %d guesses remaining.\nYou're the word chooser!\n-----------------\n", game->current_word, game->failed_guesses, game->num_guesses);
    }
    else {
        sprintf(buff, "-----------------\nThe word is %s.\nThe incorrect guesses are %s\nThere are %d guesses remaining.\nIt's not your turn to guess.\n-----------------\n", game->current_word, game->failed_guesses, game->num_guesses);
    }
    write(game->client_sockets[index], buff, MESSAGE_SIZE);
    free(buff);
}

/*
    Arguments: the index of the user submitting the chat, the struct with the game info
    Behavior: prompts the user for a chat, broadcasts the chat to the whole server
    Returns: none
*/
void client_chat(int index, struct game_info* game) {
    char buff[MESSAGE_SIZE - 40];
    buff[strcspn(buff, "\n")] = 0;
    read(game->client_sockets[index], buff, MESSAGE_SIZE - 40);
    char message[MESSAGE_SIZE];
    sprintf(message, "[%s]: %s", game->usernames[index], buff);
    message_blast(game, message, index);
}

/*
    Arguments: the index of the socket with the command, a struct with the game info
    Behavior: Handles the command (if it is a command) or produces an error message
    Returns: none
*/
void client_command(int index, struct game_info* game) {
    // printf("recieved command from client\n");
    char buff[WORD_SIZE];
    int i = read(game->client_sockets[index], buff, WORD_SIZE);
    if (i == 0 || strcasecmp(buff, "quit") == 0) {
        game->client_sockets[index] = -1;
        game->usernames[index] = "";
        game->num_clients--;
        printf("Client disconnected. %d clients connected.\n", game->num_clients);
        printf("server command: ");
        fflush(stdout);
    }
    else if (strcasecmp(buff, "status") == 0) {
        client_status(index, game);
    }
    else if (strcasecmp(buff, "guess") == 0) {
        // printf("received guess\n");
        if (game->guessing_order != NULL) {
            client_guess(index, game);
        }
        else {
            write(game->client_sockets[index], "Game hasn't started!\n", 22);
        }
    }
    else if (strcasecmp(buff, "guess-word") == 0) {
        // printf("guess-word received\n");
        if (game->guessing_order != NULL) {
            client_guess_word(index, game);
        }
        else {
            write(game->client_sockets[index], "Game hasn't started!\n", 22);
        }
    }
    else if (strcasecmp(buff, "chat") == 0) {
        // TEST MESSAGE BLAST
        // message_blast(game, "MESSAGE BLAST TEST");
        // printf("chat received\n");
        client_chat(index, game);
    }
}

/*
    Arguments: the struct containing the game info of the current game
    Behavior: prompts the user for input, then changes the gamemode accordingly
    Returns: the struct containing the game info after changing gamemode
*/
struct game_info* change_gamemode(struct game_info* game) {
    // printf("old gamemode: %d\n", game->gamemode);
    printf("New gamemode ('computer' or 'user'): ");
    char buff[20];
    fgets(buff, 19, stdin);
    buff[strcspn(buff, "\n")] = 0;
    if (strcasecmp(buff, "computer") == 0) {
        game->gamemode = COMPUTER_CHOOSING;
    }
    else if (strcasecmp(buff, "user") == 0) {
        game->gamemode = USER_CHOOSING;
    }
    else {
        printf("invalid gamemode, changing gamemode failed\n");
    }
    // printf("new gamemode: %d\n", game->gamemode);
    return game;
}

/*
    Arguments: the current game info struct
    Behavior: Prompts the user for input for a new chooser, then changes the game info
    Returns: The new game info struct
*/
struct game_info* change_chooser(struct game_info* game) {
    // printf("old chooser: %d\n", game->chooser);
    printf("Enter the index of the new chooser: ");
    char buff[3];
    fgets(buff, 3, stdin);
    buff[strcspn(buff, "\n")] = 0;
    int newChooser = atoi(buff);
    if (newChooser < game->num_clients && newChooser >= 0) {
        game->chooser = newChooser;
    }
    else {
        printf("new chooser doesn't exist, command failed\n");
    }
    // printf("new chooser: %d\n", game->chooser);
    return game;
}

/*
    Arguments: the current game info struct
    Behavior: prints the information from the struct in human-readable format
    Returns: none
*/
void print_status(struct game_info* game) {
    printf("--------------Current Game Status--------------\n");
    if (game->gamemode == COMPUTER_CHOOSING) {
        printf("Gamemode: computer choosing\n");
    }
    else {
        printf("Gamemode: user choosing\n");
    }
    if (game->guessing_order != NULL && game->gamemode == COMPUTER_CHOOSING) {
        printf("Guessing order:\n");
        for (int i = 0; i < game->num_clients; i++) {
            printf("[%d]: %s", i, game->usernames[game->guessing_order[i]]);
            if (game->guessing_order[i] == game->guesser) {
                printf(" (guesser)\n");
            }
            else {
                printf("\n");
            }
        }
        printf("Correct word: %s\n", game->real_word);
        printf("Current user word: %s\n", game->current_word);
        printf("Num guesses remaining: %d\n", game->num_guesses);
    }
    else if (game->guessing_order != NULL && game->gamemode == USER_CHOOSING) {
        printf("Guessing order:\n");
        for (int i = 0; i < game->num_clients - 1; i++) {
            printf("[%d]: %s", i, game->usernames[game->guessing_order[i]]);
            if (game->guessing_order[i] == game->guesser) {
                printf(" (guesser)\n");
            }
            else {
                printf("\n");
            }
        }
        printf("Chooser: %s\n", game->usernames[game->chooser]);
        printf("Correct word: %s\n", game->real_word);
        printf("Current user word: %s\n", game->current_word);
        printf("Num guesses remaining: %d\n", game->num_guesses);
    }
    else {
        for (int i = 0; i < game->num_clients; i++) {
            printf("[%d]: %s", i, game->usernames[i]);
            if (i == game->chooser && game->gamemode == USER_CHOOSING) {
                printf(" (chooser)\n");
            }
            else {
                printf("\n");
            }
        }
    }
    printf("-----------------------------------------------\n");
}

/*
    Arguments: the current game info struct
    Behavior: prompts the server for the number of guesses they want the game to have, changes the number of guesses
    Returns: the updated game info struct
*/
struct game_info* change_num_guesses(struct game_info* game) {
    printf("Enter the number of guesses you want: ");
    char buff[4];
    fgets(buff, 4, stdin);
    buff[strcspn(buff, "\n")] = 0;
    int newGuesses = atoi(buff);

    game->num_guesses = newGuesses;

    return game;
}

/*
    Arguments: the struct with the current game info
    Behavior: handles commands from the server given through stdin
    Returns: the struct with the new game info
*/
struct game_info* server_command(struct game_info* game) {
    char command[COMMAND_SIZE] = "";
    read(fileno(stdin), command, COMMAND_SIZE);
    command[strcspn(command, "\n")] = 0;

    //handling commands
    if (strcasecmp(command, "help") == 0) {
        printf("To start the game, type 'start'\n");
        printf("To change gamemode, type 'gamemode'\n");
        printf("To change the number of guesses, type 'num_guesses'\n");
        printf("To change the word chooser, type 'chooser'\n");
        printf("To get current game info, type 'status'\n");
        printf("To end the current round, type 'stop'\n");
        printf("To end the game, type 'quit'\n");
    }
    else if (strcasecmp(command, "start") == 0) {
        game = startGame(game);
        printf("Game started\n");
    }
    else if (strcasecmp(command, "gamemode") == 0) {
        game = change_gamemode(game);
    }
    else if (strcasecmp(command, "num_guesses") == 0) {
        game = change_num_guesses(game);
    }
    else if (strcasecmp(command, "chooser") == 0) {
        game = change_chooser(game);
    }
    else if (strcasecmp(command, "status") == 0) {
        print_status(game);
    }
    else if (strcasecmp(command, "stop") == 0) {
        game = endGame(game);
        message_blast(game, "ROUND ENDED BY SERVER\n", -1);
    }
    else if (strcasecmp(command, "quit") == 0) {
        message_blast(game, "quit", -1);
        free(game->client_sockets);
        exit(0);
    }
    else {
        printf("Invalid command. For instructions, type 'help'.\n");
    }
    return game;
}

int main(){
    // opens the socket for clients to connect to
    int listen_socket = server_setup();
    fflush(stdout);
    fd_set read_fds;
    struct game_info* game = malloc(sizeof(struct game_info));
    // printf("addr gamemode: %p\n", &game->gamemode);
    // printf("addr client_sockets: %p\n", &game->client_sockets);
    game->num_clients = 0;
    game->gamemode = COMPUTER_CHOOSING;
    game->client_sockets = (int*) malloc(sizeof(int) * MAX_CLIENTS);
    game->usernames = malloc(sizeof(char*) * MAX_CLIENTS);
    // printf("addr *client_sockets: %p\n", game->client_sockets);
    game->guesser = 0;
    game->guessing_order = NULL;
    // first user to join is automatically the chooser
    game->chooser = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        game->usernames[i] = malloc(20);
    }
    // initializes arrays of usernames and of sockets to clients
    for (int i = 0; i < MAX_CLIENTS; i++) {
        game->client_sockets[i] = -1;
        // printf("client sockets[%d]: %d\n", i, game->client_sockets[i]);
    }
    // printf("addr gamemode: %p\n", &game->gamemode);
    // printf("addr client_sockets[1]: %p\n", &game->client_sockets[1]);
    // printf("8 gamemode: %d\n", game->gamemode);

    printf("server command: ");
    fflush(stdout);

    while(1){
        // printf("server command: ");
        // fflush(stdout);
        // add every socket to the select statement
        FD_ZERO(&read_fds);
        // printf("1\n");
        int max_socket = listen_socket;
        if (game->num_clients > 0) {
            max_socket = game->client_sockets[game->num_clients - 1];
        }
        // for (int i = 0; i < MAX_CLIENTS; i++) {
        //     printf("client: %d\n", game->client_sockets[i]);
        // }
        for (int i = 0; i < game->num_clients; i++) {
            if (game->client_sockets[i] != -1) {
                FD_SET(game->client_sockets[i], &read_fds);
                // printf("client at index %d: %d\n", i, game->client_sockets[i]);
            }
        }
        FD_SET(listen_socket,&read_fds);
        // printf("listen_socket %d\n", listen_socket);
        FD_SET(STDIN_FILENO, &read_fds);
        // printf("13\n");
        // printf("max fileno: %d\n", max_socket);
        // fflush(stdout);

        int i = select(max_socket+1, &read_fds, NULL, NULL, NULL);

        // if LISTEN socket -- connect the client to the server
        if (FD_ISSET(listen_socket, &read_fds)) {
            if (game->num_clients == 8) {
                printf("Server full: no more users allowed\n");
            }
            else {
                //accept the connection
                int client_socket = server_tcp_handshake(listen_socket);

                // add the new socket to the list of socket connections
                // printf("num clients %d\n", game->num_clients);
                game->client_sockets[game->num_clients] = client_socket;
                // printf("client at index %d\n", game->client_sockets[game->num_clients]);

                // get client username
                read(client_socket, game->usernames[game->num_clients], 20);
                // printf("username is %s\n", game->usernames[game->num_clients]);

                game->num_clients++;
                printf("Connected to new client. Total clients connected: %d\n", game->num_clients);
                // for (int i = 0; i < MAX_CLIENTS; i++) {
                //     // printf("client: %d\n", game->client_sockets[i]);
                // }
                printf("server command: ");
                fflush(stdout);
            }
        }

        // if input to the server, handle commands for the server
        else if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            // printf("server_command called\n");
            game = server_command(game);
            // printf("OUT OF SERVER COMMAND\n");
            printf("server command: ");
            fflush(stdout);
        }

        //if client socket, handle the command read from the client
        else {
            for(int i = 0; i < MAX_CLIENTS; i++) {
                if (FD_ISSET(game->client_sockets[i], &read_fds)) {
                    client_command(i, game);
                }
            }
            // printf("\n");
            // // printf("server command: ");
            // // fflush(stdout);
        }
    }
    return 0;
}