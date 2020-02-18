#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

int main( int argc, char **argv) {
	struct hostent *ptrh;
	struct protoent *ptrp;
	struct sockaddr_in sad;
	int sd;
	int port;
	char *host;

	uint8_t K;
	uint8_t N;

	memset((char *)&sad,0,sizeof(sad));
	sad.sin_family = AF_INET;

	if( argc != 3 ) {
		fprintf(stderr,"Error: Wrong number of arguments\n");
		fprintf(stderr,"usage:\n");
		fprintf(stderr,"./client server_address server_port\n");
		exit(EXIT_FAILURE);
	}

	port = atoi(argv[2]);
	if (port > 0)
		sad.sin_port = htons((u_short)port);
	else {
		fprintf(stderr,"Error: bad port number %s\n",argv[2]);
		exit(EXIT_FAILURE);
	}

	host = argv[1];

	ptrh = gethostbyname(host);
	if ( ptrh == NULL ) {
		fprintf(stderr,"Error: Invalid host: %s\n", host);
		exit(EXIT_FAILURE);
	}

	memcpy(&sad.sin_addr, ptrh->h_addr, ptrh->h_length);

	if ( ((long int)(ptrp = getprotobyname("tcp"))) == 0) {
		fprintf(stderr, "Error: Cannot map \"tcp\" to protocol number");
		exit(EXIT_FAILURE);
	}

	sd = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
	if (sd < 0) {
		fprintf(stderr, "Error: Socket creation failed\n");
		exit(EXIT_FAILURE);
	}

	if (connect(sd, (struct sockaddr *)&sad, sizeof(sad)) < 0) {
		fprintf(stderr,"connect failed\n");
		exit(EXIT_FAILURE);
	}

	recv(sd, &K, sizeof(K), 0);

	char guess; // user input for guess
	int first = 1; // first instance of scan
	uint8_t condition = 1; // did not win or lose yet
	char* board = (char*)malloc(sizeof(char)*(K)); // current state of clients guess board

	while(condition == 1){

		recv(sd, &N, sizeof(N), 0);
		recv(sd, board, sizeof(board), MSG_WAITALL);

		printf("Board: %s (%d guesses left)\n", board, N);
		printf("Enter guess: ");

		// get character from user
		if(first == 1){
			scanf("%c", &guess);
			first = 0;
		}
		else{
			scanf(" %c", &guess);
			printf("\n");
		}

		// verify guess with server
		send(sd, &guess, sizeof(guess), 0);

		// check if win or lose
		recv(sd, &condition, sizeof(condition), 0);
		recv(sd, board, sizeof(board), MSG_WAITALL);
		
		if(condition == 0){
			printf("Board: %s\n", board);
			printf("You lost\n");
		}
		else if(condition == 255){
			printf("Board: %s\n", board);
			printf("You won\n");
		}
	}

	close(sd);

	exit(EXIT_SUCCESS);
}
