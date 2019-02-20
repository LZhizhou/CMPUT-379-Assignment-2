#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>


#define MAX_SIZE 50
#define NUM_CLIENT 1
void *connection_handler(void *socket_desc);

void read_res(char *string, int size){
    printf("received: ");
    for(int i = 0; i < size; i++)
    {
        //printf("%d",string[i]);
        printf("%c",string[i]);
    }
    printf("\n");
    
}
void prror(char *error_message){
	fprintf(stderr, "%s",error_message);
	exit(1);
}
int main(int argc, char** argv)
{
    if (argc != 3){
		prror("Usage: ddupserver address port\n");
	}

    int sock_desc;
	int write_fd;
    struct sockaddr_in serv_addr;
    char sbuff[MAX_SIZE],rbuff[MAX_SIZE], filebuff[MAX_SIZE];
	//off_t offset = 0;

    if((sock_desc = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        printf("Failed creating socket\n");

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    if (connect(sock_desc, (struct sockaddr *) &serv_addr, sizeof (serv_addr)) < 0) {
        printf("Failed to connect to server\n");
    }

    while(1)
    {
        fgets(sbuff, MAX_SIZE , stdin);
        send(sock_desc,sbuff,strlen(sbuff),0);

        while (recv(sock_desc,rbuff,MAX_SIZE,0)==0)
            printf("Error");
        
		read_res(rbuff,sizeof(rbuff));

        //sleep(2);
        memset(rbuff,0,sizeof(rbuff));
    }
    close(sock_desc);
    return 0;
}
