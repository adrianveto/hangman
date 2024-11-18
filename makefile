.PHONY: compile server client clean

compile server.out client.out: networking.o hangman_server.o hangman_client.o hangman.o
	@gcc -o server.out hangman_server.o networking.o hangman.o
	@gcc -o client.out hangman_client.o networking.o

hangman_client.o: hangman_client.c networking.h
	@gcc -c hangman_client.c

hangman_server.o: hangman_server.c networking.h hangman.h
	@gcc -c hangman_server.c

networking.o: networking.c
	@gcc -c networking.c

hangman.o: hangman.c
	@gcc -c hangman.c

server: server.out
	@./server.out

client: client.out
	@./client.out $(ARGS)

clean:
	@rm -f *.o
	@rm -f *.out
