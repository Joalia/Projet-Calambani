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

#include "user_messageManager.h"

// Definition de la fonction pour mettre le textes en couleur
#define color(param) printf("\033[%sm", param);

#define MSG_SIZE 256
#define NAME_LEN 32

volatile sig_atomic_t flag = 0;
int sockfd = 0;
char name[NAME_LEN];
static int myID = 0;

/*
====================================================================
*/
void recv_msg_handler(){
	char message[MSG_SIZE];
	char sender[32];
	while(1){
		int receive = recv(sockfd, message, MSG_SIZE, 0);

		if(receive > 0){
			int etat = decode_msg(message, sender, 0, MSG_SIZE, NAME_LEN);
			
			printf("Type %d\n", etat);
			// Private message
			if(etat == 1){
				color("32");
				printf("[ %s ] : ", sender);
			}
			// Public message
			else if(etat == 2){
				color("34");
				printf("[ %s ] : ", sender);
				
			}
			else if(etat == 5){
				color("36");
				printf("%s\n", message);
			}
			else{

			}

			color("0");
			printf("%s\n", message);
			print_tag();
		}
		else if(receive == 0){
			break;
		}
		bzero(message, MSG_SIZE);
	}
}
/*
====================================================================
*/
void catch_ctrl_c_and_exit(){
	flag = 1;
}
/*
====================================================================
*/
void send_msg_handler(){
	char buffer[MSG_SIZE];
	char message[MSG_SIZE + NAME_LEN];
	print_tag();
	while(1){
		print_tag();
		fgets(buffer, MSG_SIZE, stdin);
		buffer[strlen(buffer)] = ' ';
		int msg_type = encode_msg(buffer, MSG_SIZE, myID);
		
		if(msg_type == 0){
			sprintf(message, "%s\n", buffer);
			send(sockfd, message, strlen(message), 0);
			int receive = recv(sockfd, message, MSG_SIZE, 0);
			if(receive > 0){
				printf("%s\n", message);
				print_tag();
			}
			break;
		}
		sprintf(message, "%s\n", buffer);
		send(sockfd, message, strlen(message), 0);
		
		bzero(buffer, MSG_SIZE);
		bzero(message, MSG_SIZE + NAME_LEN);
	}
	flag = 1;
}

/*
===================================================================
*/
int main(int argc, char** argv){
	
	char* ip = "127.0.0.1";
	int port = 54000;

	signal(SIGINT, catch_ctrl_c_and_exit);

	struct sockaddr_in serv_addr;

	// Socket Settings
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(ip);
	serv_addr.sin_port = htons(port);

	// Connect to the Server
	int err = connect(sockfd, (struct  sockaddr*) &serv_addr, sizeof(serv_addr));
	if(err == -1){
		printf("ERROR : Connection Error.\n");
		return EXIT_FAILURE;
	}

	// Send the Name
	printf("Enter your name : ");
	fgets(name, NAME_LEN, stdin);
	format_str(name, strlen(name));

	if(strlen(name) > NAME_LEN - 1 || strlen(name) < 2){
		printf("Enter a correct name.\n");
		return EXIT_FAILURE;
	}

	char infos[MSG_SIZE];
	sprintf(infos, "0%d|HELO|%s|", MSG_SIZE,name);
	send(sockfd, infos, NAME_LEN, 0);
	recv(sockfd, infos, MSG_SIZE, 0);

	// Trying to cllect the ID and Name
	
	int etat = decode_msg(infos, "", &myID, MSG_SIZE, 0);
	while(etat == -1){
		color("31");
		printf("[ERROR : xxl] User of this name already exist. Try another name... \n");
		color("0");
		printf("Type another name : ");
		fgets(name, NAME_LEN, stdin);
		format_str(name, strlen(name));
		sprintf(infos, "0%d|HELO|%s|", MSG_SIZE,name);
		send(sockfd, infos, NAME_LEN, 0);
		recv(sockfd, infos, MSG_SIZE, 0);
		etat = decode_msg(infos, " ", &myID, MSG_SIZE, 0);
	}

	system("clear");
	printf("==============================================================\n");
	printf("Connection validated. Infos are : id = %d  Name = %s\n", myID, name);
	printf("\n\t *** %s Welcome to CALAMBANI.po ***\n", name);
	printf("==============================================================\n\n");


	// Pthread for sending message
	pthread_t send_msg_thread;
	if(pthread_create(&send_msg_thread, NULL, (void*)send_msg_handler, NULL) != 0){
		printf("ERROR: pthread.\n");
		return EXIT_FAILURE;
	}

	// Pthread for receiving massages
	pthread_t recv_msg_thread;
	if(pthread_create(&recv_msg_thread, NULL, (void*)recv_msg_handler, NULL) != 0){
		printf("ERROR: pthread.\n");
		return EXIT_FAILURE;
	}

	while(1){
		if(flag){
			break;
		}
	}

	close(sockfd);

	return EXIT_SUCCESS;
}