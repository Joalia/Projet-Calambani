
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "server_usersManager.h"
#include "server_messageManager.h"


#define MAX_CONNEXION 10
#define MSG_SIZE 256
#define NAME_LEN 32

#define IP "127.0.0.1"
#define port 54000

static _Atomic unsigned int users_count = 0;
static int userID = 10;
volatile sig_atomic_t flag = 0;
users_t* users[MAX_CONNEXION];

pthread_mutex_t user_mutex = PTHREAD_MUTEX_INITIALIZER;


/*
==================================================================
*/

void validateConnection(int connfd, users_t* usr, struct sockaddr_in usr_addr)
{
	char message[MSG_SIZE];
	// Initialize users parameters
	usr->address = usr_addr;
	usr->sockfd = connfd;
	usr->userID = userID++;

	// Request for the user's name
	char name[NAME_LEN];
	recv(usr->sockfd, message, MSG_SIZE, 0);
	decode_msg(users, message, 0, MSG_SIZE, MAX_CONNEXION);
	strcpy(name, message);
	printf("The detected name is : %s\n", name);
	format_str(name, strlen(name));

	int exist = 0;
	for(int i=0; i < MAX_CONNEXION; i++){
		if(users[i]){
			if(str_cmp(users[i]->name, name) ==0){
				exist = 1;
			}
		}
	}

	// Here we verify that the name enterred is not already used by an existing user
	while(exist == 1){
		int compteur = 0;
		char* response = "0256|BADD|";
		//leave_flag = 1;
		send(usr->sockfd, response, strlen(response),0);
		bzero(message, MSG_SIZE);
		recv(usr->sockfd, message, MSG_SIZE, 0);

		decode_msg(users, message, 0, MSG_SIZE, MAX_CONNEXION);
		strcpy(name, message);
		format_str(name, strlen(name));

		for(int i=0; i < MAX_CONNEXION; i++){
			if(users[i]){
				if(str_cmp(users[i]->name, name) == 0){
					compteur++;
				}
			}
		}

		if(compteur == 0){
			break;
		}
	}

	// If the name is not yet used, we then validate the user's connexion
	strcpy(usr->name, name);
	sprintf(message, "0256|BCST|%s|has joined the Group.\n", usr->name);
	printf("%s\n", message);
	distribute_massage(users, message, usr->userID, user_mutex, MAX_CONNEXION);

	// Notify user that he was accepted
	char response[MSG_SIZE];
	sprintf(response, "0256|OKOK|%d|", usr->userID);
	send(connfd, response, strlen(response), 0);

	// Add the user to the table of users
	add_user(users, usr, user_mutex, MAX_CONNEXION);
}

/*
==================================================================
*/

void *handle_user(void *arg)
{
	char message[MSG_SIZE];
	char leavingMessage[MSG_SIZE];
	char name[NAME_LEN];
	int leave_flag = 0;
	users_count++;

	users_t *usr = (users_t*)arg;

	bzero(message, MSG_SIZE);

	while(1){
		if(leave_flag){
			printf("Flag have signaled.\n");
			break;
		}

		
		int receive = recv(usr->sockfd, message, MSG_SIZE, 0);
		if(receive > 0){
			format_str(message, strlen(message));
			if(strlen(message) > 0){
				int receiverID = 0;
				int msg_type = decode_msg(users, message, &receiverID, MSG_SIZE, MAX_CONNEXION);
				printf("Message type is : %d\n", msg_type);
				if(msg_type == 1){
					send(receiverID, message, strlen(message), 0);
				}
				else if(msg_type == 2){
					distribute_massage(users, message, usr->userID, user_mutex, MAX_CONNEXION);
				}
				else if(msg_type == 3){
					send(usr->sockfd, message, strlen(message), 0);
				}
				else if(msg_type == 4){
					sprintf(message, "0256|BYEE|%s has left.\n", usr->name);
					printf("%s\n",message);
					distribute_massage(users, message, usr->userID, user_mutex, MAX_CONNEXION);
					strcpy(leavingMessage, "0256|BYEE|Goodbye!!! See you nextly.|");
					leave_flag = 1;
				}
			}
		}
		else if(receive == 0 || strcmp(message, ":exit") == 0){
			sprintf(message, "%s has left.\n", usr->name);
			printf("%s\n",message);
			distribute_massage(users, message, usr->userID, user_mutex, MAX_CONNEXION);
			leave_flag = 1;
		}
		else{
			printf("ERROR : -1\n");
			leave_flag = 1;
		}

		bzero(message, MSG_SIZE);
	}

	send(usr->sockfd, leavingMessage, strlen(leavingMessage),0);
	close(usr->sockfd);
	remove_user(users, usr->userID, user_mutex, MAX_CONNEXION);
	free(usr);
	users_count--;
	pthread_detach(pthread_self());

	return NULL;
}

/*
==================================================================
*/
void catch_ctrl_c_and_exit(){
	char* message = "Server is down. We are working on it. Goodbye";
	distribute_massage(users, message, 0, user_mutex, strlen(message));
	for(int i = 0; i < MAX_CONNEXION; i++){
		close(users[i]->sockfd);
		remove_user(users, users[i]->userID, user_mutex, MAX_CONNEXION);
		free(users[i]);
		users_count--;
	}
	flag = 1;
}
/*
==================================================================
*/
int main(int argc, char** argv)
{
	int option = 1;
	int listenfd = 0; // Listening socket for the server.
	int connfd = 0; //
	struct sockaddr_in serv_addr; // Server socket structure
	struct sockaddr_in usr_addr; // Client socket structure
	pthread_t tid; // Parent thrread from which all other threads will depend

	// Setup the server socket
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(IP);
	serv_addr.sin_port = htons(port);

	// Signals
	signal(SIGPIPE, SIG_IGN);
	if(setsockopt(listenfd, SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR), (char*)&option, sizeof(option)) < 0){
		printf("ERROR : Failed to set the socket option.\n");
		return EXIT_FAILURE;
	}

	// Bind the server
	if(bind(listenfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0){
		printf("[--X X X--] Error while binding.\n");
		return EXIT_FAILURE;
	}

	// Listening
	if(listen(listenfd, 10) < 0){
		printf("ERROR : Server cannot listen...\n");
		return EXIT_FAILURE;
	}

	printf("====================================================\n");
	printf("\tWelcome to CALAMBANI.PO\n");
	printf("====================================================\n\n");

	while(1){
		socklen_t usr_len = sizeof(usr_addr);
		connfd = accept(listenfd, (struct sockaddr*)&usr_addr, &usr_len);

		// Check if MAX_CONNEXION have been attained
		if((users_count + 1) == MAX_CONNEXION){
			printf("Maximum clients connected. Connection rejected.\n");
			char* response = "No possible connection. Already full.";
			//send(connfd, "0", 1, 0);
			send(connfd, response, strlen(response),0);
			//Have to print the ID of the user
			close(connfd);
			continue;
		}

		// Create a user
		users_t* usr = (users_t*)malloc(sizeof(users_t));

		//validate the users connexion
		validateConnection(connfd, usr, usr_addr);

		// Create user's personal thread
		pthread_create(&tid, NULL, &handle_user, (void*)usr);

		// Reduce CPU usage;
		sleep(1);

	}
	printf("Bonjour\n");

	return EXIT_SUCCESS;
}