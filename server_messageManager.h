void print_stdout(){
	printf("\r%s", "> ");
	fflush(stdout);
}

void format_str(char* msg, int length){
	for(int i=0; i<length; i++){
		if(msg[i] == '\n'){
			msg[i] = '\0';
			break;
		}
	}
}

int str_cmp(char* msg1, char* msg2){
	int a = strlen(msg1);
	int b = strlen(msg2);

	if(a < b){
		for(int i = 0; i < a; i++){
			if(msg1[i] != msg2[i]){
				return -1;
			}
		}
		return 0;
	}
	else{
		for(int i = 0; i < b; i++){
			if(msg1[i] != msg2[i]){
				return -1;
			}
		}
		return 0;
	}
	
}

void print_ip_addr(struct sockaddr_in addr){
	printf("%d.%d.%d.%d", addr.sin_addr.s_addr & 0xff,
			(addr.sin_addr.s_addr & 0xff00) >> 8,
			(addr.sin_addr.s_addr & 0xff0000) >> 16,
			(addr.sin_addr.s_addr & 0xff000000) >> 24);
}

void distribute_massage(users_t* users[], char* msg, int uid, pthread_mutex_t usr_mut, int length){
	pthread_mutex_lock(&usr_mut);

	for(int i = 0; i < length; i++){
		if(users[i]){
			if(users[i]->userID != uid){
				if(write(users[i]->sockfd, msg, strlen(msg)) < 0){
					printf("ERROR: Write to descriptor failed\n");
					break;
				}
			}
		}
	}

	pthread_mutex_unlock(&usr_mut);
}

int decode_msg(users_t* users[], char msg[], int* receiverID, int msg_size, int max_connexion){
	char total_len[5];
	char type[5];
	int i = 0;
	int j = 0;
	// Search for the first break
	bzero(total_len, 5);
	for(i = 0; i < msg_size; i++){
		if(msg[i] == '|'){
			i++;
			break;
		}
		else{
			total_len[i] = msg[i];
		}
	}
	j = i;

	// Get the type of the message
	for (i = j; i < j + 4; i++){
		type[i-j] = msg[i];
	}
	type[i-j] = '\0';
	j = ++i;

	// Compare the type and act consequently
	// I we have a private message
	if(str_cmp(type, "HELO") == 0){

		char sender[32];
		for(i = j; i < msg_size; i++){
			if(msg[i] == '|'){
				break;
			}
			else{
				sender[i-j] = msg[i];
			}
		}
		sender[i-j] = '\0';

		bzero(msg, msg_size);
		strcpy(msg, sender);

		return 0;
	}
	else if(str_cmp(type, "PRVT") == 0){

		char senderID[5];
		for(i = j; i < strlen(msg); i++){
			if(msg[i] == '|'){
				break;
			}
			else{
				senderID[i-j] = msg[i];
			}
		}
		senderID[5] = '\0';
		printf("%s\n", senderID);
		j = ++i;
		int ID = atoi(senderID);

		// With the sender's ID, let us get its name
		int k = 0;
		for(k = 0; k < max_connexion; k++){
			if(users[k]->userID == ID){
				break;
			}
		}
		char senderName[32];
		int a = 0;
		for(a = 0; a < strlen(users[k]->name); a++){
			senderName[a] = users[k]->name[a];
		}
		senderName[a] = '\0';

		// Now we get the name of the sender's socket
		char receiverName[32];
		printf("%d\n", i);
		for(i = j; i < strlen(msg); i++){
			if(msg[i] == '|'){
				break;
			}
			else{
				receiverName[i-j] = msg[i];
			}
		}
		receiverName[i-j] = '\0';
		j = ++i;

		// Search the receive in dataBase and return it's socket
		k = 0;
		for(k = 0; k < max_connexion; k++){
			if(users[k]){
				if(str_cmp(users[k]->name, receiverName) == 0){
					break;
				}
			}
		}

		if(!users[k]){
			printf("No member of this name.\n");
			return -1;
		}
		else{
			*receiverID = users[k]->sockfd;

			char message[msg_size];
			for(i = j; i < strlen(msg); i++){
				if(msg[i] == '|'){
					break;
				}
				else{
					message[i-j] = msg[i];
				}
			}
			message[i-j] = '\0';

			// Now we format the message to send to the different users
			format_str(message, strlen(message));
			sprintf(msg, "%s|%s|%s|%s|", total_len, type, senderName, message);

			return 1;
		}
	}

	// If we have a message of type LIST
	else if(str_cmp(type, "BCST") == 0){
		char senderID[5];
		char text[msg_size];
		for(i = j; i < strlen(msg); i++){
			if(msg[i] == '|'){
				break;
			}
			else{
				senderID[i-j] = msg[i];
			}
		}
		j = ++i;
		senderID[5] = '\0';

		for(i = j; i < msg_size; i++){
			if(msg[i] == '|'){
				break;
			}
			else{
				text[i-j] = msg[i];
			}
		}
		text[i-j] = '\0';


		int ID = atoi(senderID);

		// With the sender's ID, let us get its name
		int k = 0;
		for(k = 0; k < max_connexion; k++){
			if(users[k]->userID == ID){
				break;
			}
		}
		char senderName[32];
		int a = 0;
		for(a = 0; a < strlen(users[k]->name); a++){
			senderName[a] = users[k]->name[a];
		}
		senderName[a] = '\0';


		format_str(msg, strlen(msg));
		sprintf(msg, "%s|%s|%s|%s|", total_len, type, senderName, text);

		return 2;
	}

	// If we have a message of type LIST
	else if(str_cmp(type, "LIST") == 0){
		printf("It's a message of type LIST\n");

		char message[msg_size];
		bzero(message, msg_size);
		int j = 0;
		int compteur = 0;
		for(int i = 0; i < max_connexion; i++){
			if(users[i]){
				message[j] = '\n';
				j++;
				compteur++;
				message[j] = compteur+'0';
				j++;
				message[j] = '.';
				j++;
				message[j] = ' ';
				j++;
				for(int k = 0; k < strlen(users[i]->name); k++){
					message[j] = users[i]->name[k];
					j++;
				}
			}
			else{
				//break;
			}
		}

		sprintf(msg, "%s|%s|%d|%s|", total_len, type, compteur, message);
		//strcpy(msg, message);

		return 3;
	}
	else if(str_cmp(type, "BYEE") == 0){
		printf("Exiting message type.\n");
							
		return 4;
	}
	else if(str_cmp(type, "SEND") == 0){
		printf("Message of type sending file.\n");

		return 5;
	}
	else{
		printf("Not yet detected\n");
	}

}