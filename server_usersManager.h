#define NAME_LEN 32

// Client structure
typedef struct{
	struct  sockaddr_in address;
	int sockfd;
	int userID;
	char name[NAME_LEN];
}users_t;

void add_user(users_t* users[], users_t* usr, pthread_mutex_t usr_mut, int length){
	pthread_mutex_lock(&usr_mut);

	for(int i = 0; i < length; i++){
		if(!users[i]){
			users[i] = usr;
			break;
		}
	}
	pthread_mutex_unlock(&usr_mut);
}

void remove_user(users_t* users[], int uid, pthread_mutex_t usr_mut, int length){
	pthread_mutex_lock(&usr_mut);

	for(int i = 0; i < length; i++){
		if(users[i]->userID == uid){
			users[i] = NULL;
			break;
		}
	}

	pthread_mutex_unlock(&usr_mut);
}