#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define QLEN 6
char* guessword; // main word
uint8_t globalN; // default number of guesses
uint8_t K; // size of word

void rungame(int sd2);

int main(int argc, char **argv) {
	struct protoent *ptrp;
	struct sockaddr_in sad;
	struct sockaddr_in cad;
	int sd, sd2;
	int port;
	int alen;
	int optval = 1;

	if( argc != 4 ) {
		fprintf(stderr,"Error: Wrong number of arguments\n");
		fprintf(stderr,"usage:\n");
		fprintf(stderr,"./server server_port numguesses guessword\n");
		exit(EXIT_FAILURE);
	}

	memset((char *)&sad,0,sizeof(sad));
	sad.sin_family = AF_INET;
	sad.sin_addr.s_addr = INADDR_ANY;

	port = atoi(argv[1]);
	if (port > 0) {
		sad.sin_port = htons((u_short)port);
	} else {
		fprintf(stderr,"Error: Bad port number %s\n",argv[1]);
		exit(EXIT_FAILURE);
	}

	if ( ((long int)(ptrp = getprotobyname("tcp"))) == 0) {
		fprintf(stderr, "Error: Cannot map \"tcp\" to protocol number");
		exit(EXIT_FAILURE);
	}

	sd = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
	if (sd < 0) {
		fprintf(stderr, "Error: Socket creation failed\n");
		exit(EXIT_FAILURE);
	}

	if( setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0 ) {
		fprintf(stderr, "Error Setting socket option failed\n");
		exit(EXIT_FAILURE);
	}

	if (bind(sd, (struct sockaddr *)&sad, sizeof(sad)) < 0) {
		fprintf(stderr,"Error: Bind failed\n");
		exit(EXIT_FAILURE);
	}

	if (listen(sd, QLEN) < 0) {
		fprintf(stderr,"Error: Listen failed\n");
		exit(EXIT_FAILURE);
	}

	// initialize variables
	globalN = atoi(argv[2]);
	guessword = argv[3];
	K = strlen(guessword);

	while (1) {
		alen = sizeof(cad);
		if ( (sd2=accept(sd, (struct sockaddr *)&cad, &alen)) < 0) {
			fprintf(stderr, "Error: Accept failed\n");
			exit(EXIT_FAILURE);
		}

		if(fork() == 0){
			rungame(sd2);
		}
	}
}

void rungame(int sd2){

	char guess; // user input for guess
	int guessix = 0; // guesslist index
	int guessdec = 0; // check if need to decrement guesses
	uint8_t N = globalN; // guesses left for this instance of the game
	uint8_t condition = 1; // did not win or lose yet
	char* board = (char*)malloc(sizeof(char)*K); // current state of the board for this instance of the game
	char* guesslist = (char*)malloc(sizeof(char)*(N*2)); // characters already guessed

	// create board
	for(int i = 0; i < K; i++){
		board[i] = '_';
	}

	send(sd2, &K, sizeof(K), 0);

	while(condition == 1){

		send(sd2, &N, sizeof(N), 0);
		send(sd2, board, sizeof(board), 0);
		recv(sd2, &guess, sizeof(guess), 0);

		// guess not in guesslist
		if(strchr(guesslist, guess) == NULL){
			// add to guest list
			guesslist[guessix] = guess;
			guessix++;
			// go through board
			for(int i = 0; i < K; i++){
				// guess correctly
				if(guess == guessword[i]){
					board[i] = guess;
					guessdec = -1;
				}
			}
		}
		// wrong guess
		if(guessdec == 0){
			N--;
		}
		guessdec = 0;

		// client won
		if(strstr(board, guessword) != NULL){
			condition = 255;
		}
		// out of guesses
		else if(N == 0){
			condition = 0;
		}
		send(sd2, &condition, sizeof(condition), 0);
		send(sd2, board, sizeof(board), 0);
	}

	close(sd2);
}
