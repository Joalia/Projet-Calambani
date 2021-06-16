void print_tag(){
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

void get_cmd(char* msg, char indicator[]){
	int i = 0;
	while(msg[i] != 32){
		indicator[i] = msg[i];
		i++;
	}
	indicator[i] = ' ';
}

void str_cpy(char msg1[], char msg2[], int pos, int MSG_LEN){
	bzero(msg2, MSG_LEN);
	for(int i = pos; i < strlen(msg1); i++){
		msg2[i-pos] = msg1[i];
	}
	bzero(msg1, MSG_LEN);
}

int encode_msg(char msg[], int msg_size, int ID){

	// Test pour verifier si c'est une commande
	//format_str(msg, strlen(msg));	msg[strlen(msg)] = ' ';
	char indicator[10];
	get_cmd(msg, indicator);
	
	if(str_cmp(indicator, "EXIT") == 0 || str_cmp(indicator, "BYEE") == 0){
		bzero(msg, msg_size);
		sprintf(msg, "0%d|BYEE|00%d|", msg_size, ID);

		return 0;
	}
	else if(str_cmp(indicator, "PRIVATE") == 0){

		// We have a private message, so we try to get a Pseudo
		char pseudo[32];
		for(int i = 11; i < strlen(msg); i++){
			if(msg[i] != 32){
				pseudo[i-11] = msg[i];
			}
			else{
				pseudo[i-11] = '\n';
				break;
			}
		}
		format_str(pseudo, 32);

		// Now we extract the message
		char message[256];
		str_cpy(msg, message, 11+strlen(pseudo)+1, msg_size);
		sprintf(msg, "0%d|PRVT|00%d|%s|%s|", msg_size, ID, pseudo, message);
		format_str(msg, strlen(msg));
	}
	else if(str_cmp(indicator, "LIST") == 0){
		bzero(msg, msg_size);
		sprintf(msg, "0%d|LIST|00%d|", msg_size, ID);
	}
	else if(str_cmp(indicator, "SEND") == 0){
		printf("The commande is also now : %s\n", indicator);
		//For later Studies
	}
	else{
		msg[strlen(msg)] = '\n';
		char message[256];
		str_cpy(msg, message, 0, msg_size);
		sprintf(msg, "0%d|BCST|00%d|%s|", msg_size, ID, message);
		format_str(msg, strlen(msg));
	}

	
	return 1;
}

int decode_msg(char msg[], char sender[], int* ID, int msg_size, int name_size){

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
	for (i = j; i < j + 4; i++){
		type[i-j] = msg[i];
	}
	j = ++i;

	if (str_cmp(type, "OKOK") == 0){

		char id[5];

		for(i = j; i < msg_size; i++){
			if(msg[i] == '|'){
				break;
			}
			else{
				id[i-j] = msg[i];
			}
		}
		(*ID) = atoi(id);
		//sender = {'S', 'E', 'R', 'V', 'E', 'R'};

		return 0;
	}
	else if(str_cmp(type, "BADD") == 0){

		return -1;
	}

	else if(str_cmp(type, "PRVT") == 0){

		for(i = j; i < msg_size; i++){
			if(msg[i] == '|'){
				break;
			}
			else{
				sender[i-j] = msg[i];
			}
		}
		j = ++i;

		char text[msg_size];
		for(i = j; i < msg_size; i++){
			if(msg[i] == '|'){
				break;
			}
			else{
				text[i-j] = msg[i];
			}
		}
		text[i-j] = '\0';

		bzero(msg, msg_size);
		strcpy(msg, text);

		return 1;
	}
	else if(str_cmp(type, "BCST") == 0){

		for(i = j; i < msg_size; i++){
			if(msg[i] == '|'){
				break;
			}
			else{
				sender[i-j] = msg[i];
			}
		}
		j = ++i;

		char text[msg_size];
		for(i = j; i < msg_size; i++){
			if(msg[i] == '|'){
				break;
			}
			else{
				text[i-j] = msg[i];
			}
		}
		text[i-j] = '\0';

		bzero(msg, msg_size);
		strcpy(msg, text);

		return 2;
	}
	else if(str_cmp(type, "BYEE") == 0){

		char text[msg_size];
		for(i = j; i < msg_size; i++){
			if(msg[i] == '|'){
				break;
			}
			else{
				text[i-j] = msg[i];
			}
		}
		text[i-j] = '\0';

		bzero(msg, msg_size);
		strcpy(msg, text);

		return 5;
	}
	else{


		return 6;
	}
}
