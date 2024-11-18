#ifndef GAME_INFO
#define GAME_INFO
struct game_info {
    int num_clients;
    int* client_sockets;
    char** usernames;
    int* guessing_order;
    int guesser_index;
    char gamemode;
    int chooser;
    int guesser;
    char* real_word;
    char* current_word;
    int num_guesses;
    char* failed_guesses;
};
#endif

#ifndef HANGMAN_SERVER_H
#define HANGMAN_SERVER_H
struct game_info* user_start_word(struct game_info* game);
void message_blast(struct game_info* game, char* message, int exclude_index);
void client_guess(int index, struct game_info* game);
void client_guess_word(int index, struct game_info* game);
void client_status(int index, struct game_info* game);
void client_command(int client_socket, struct game_info* game);
void client_chat(int index, struct game_info* game);
struct game_info* server_command(struct game_info* game);
struct game_info* change_gamemode(struct game_info* game);
struct game_info* change_num_guesses(struct game_info* game);
struct game_info* change_chooser(struct game_info* game);
void print_status(struct game_info* game);
#endif

#define COMPUTER_CHOOSING 0
#define USER_CHOOSING 1
