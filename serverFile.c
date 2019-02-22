#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h>  // for open
#include <unistd.h> // for close
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <malloc.h>

char server_message[2000];

char *directory;


void append_string(char *str1, char *str2, int index)
{
	for (int i = 0; i < strlen(str2); i++)
	{	
		if (index == 0){
			strcpy(str1,str2);
		}
		else
		{
			memcpy(index+str1,str2,sizeof(str2));
		}
		
		
	}
	//printf("str1 is %s, str2 is %s\n",str1,str2);
}
unsigned char int2one_byte(int x)
{
	unsigned char res;
	res = x & 0xFF;
	return res;
}
void int2two_byte(unsigned char res[2], int x)
{
	res[0] = (x & 0xFF00) >> 8;
	res[1] = x & 0xFF;
	//printf("x is %d, two byte is %02x %02x", x, res[0], res[1]);
}

void prror(char *error_message)
{
	//printer error and exit
	fprintf(stderr, "%s", error_message);
	exit(1);
}

int cfileexists(const char *filename)
{
	/* try to open file to read */
	FILE *file;
	printf("%s", filename);
	if (file = fopen(filename, "r"))
	{
		printf("file is here\n");
		fclose(file);
		return 1;
	}
	return 0;
}
void list(int connection_fd, int *message_count)
{ //send all files to connection_fd

	char buf[1024] = {0};

	DIR *dir;
	struct dirent *item;

	dir = opendir(directory);
	int file_count = 0, name_len = 0;
	char filenames[200] = {0};

	while ((item = readdir(dir)) != NULL)
	{
		if ((strcmp(item->d_name, ".") == 0) || (strcmp(item->d_name, "..") == 0))
			continue;
		// get names of all files

		append_string(filenames, item->d_name, name_len);
		name_len += strlen(item->d_name)+1;
		//filenames[file_count][strlen(item->d_name)+1] = '\0';
		file_count++;
	}
	//printf("malloc done\n");
	unsigned char message_count_byte;
	message_count_byte = int2one_byte(*message_count);
	send(connection_fd, &message_count_byte, sizeof(message_count_byte), 0);
	printf("0x%02x|\n", message_count_byte);

	unsigned char file_count_2_byte[2];
	int2two_byte(file_count_2_byte, file_count);
	send(connection_fd, file_count_2_byte, sizeof(file_count_2_byte), 0);
	printf("%x%x|", file_count_2_byte[0], file_count_2_byte[1]);


	send(connection_fd, filenames, sizeof(filenames), 0);
	for(int i = 0; i < sizeof(filenames); i++)
	{
		printf("%c",filenames[i]);
	}

	

	*message_count++;
}
void download(int connection_fd, char *filename)
{
	char buffer[1024];
	FILE *fp = fopen(filename, "r");

	if (NULL == fp)
	{
		prror("File Not Found\n");
	}
	else
	{
		memset(buffer, 0, sizeof(buffer));
		int length = 0;

		while ((length = fread(buffer, sizeof(char), sizeof(buffer), fp)) > 0)
		{
			printf("length = %d\n", length);
			if (send(connection_fd, buffer, length, 0) < 0)
			{
				printf("Send File Failed.\n");
				break;
			}
			memset(buffer, 0, sizeof(buffer));
		}
		fclose(fp);
		sleep(1);
		printf("File Transfer Successful!\n");
	}
	/* 	struct stat stat_buf;
	fgets(server_message, 2000, stdin);
	send(connection_fd, server_message, strlen(server_message), 0);
	if (cfileexists(filename) == 1)
	{
		char *message = "sending the file";
		send(connection_fd, message, strlen(message), 0);
		int read_fd = open(filename, O_RDONLY);
		fstat(read_fd, &stat_buf);
		sendfile(connection_fd, read_fd, 0, stat_buf.st_size);
	}
	else
	{
		printf("we do not have it\n");
	} */
}

void *socketThread(void *arg)
{

	int message_count = 0x00;
	int read_fd;
	int newSocket = *((int *)arg);
	// Send message to the client socket

	char str[80], client_message[2000];
	char filename[20], temp;
	while (1)
	{
		// if lose connect with client
		if (recv(newSocket, client_message, sizeof(client_message), 0) == 0)
			goto close_socket;
		//printf("received %s\n",client_message);
		char flag;
		sscanf(client_message, " %c", &flag);
		// get first char of command
		switch (client_message[0])
		{
		case 'l':
			// if command is l, it means list all files
			list(newSocket, &message_count);
			break;
		case 'u':
			break;
		case 'd':
			// d command, send

			sscanf(client_message, " %c %s", &temp, filename);
			download(newSocket, filename);
			break;
		case 'q':
			// q command, quit
			goto close_socket;
		default:
			break;
		}
		memset(client_message, 0, sizeof(client_message));
	}
close_socket:
	printf("Exit socketThread \n");
	close(newSocket);
	pthread_exit(NULL);
}

int main(int argc, char **argv)
{
	if (argc != 3)
	{
		prror("Usage: ddupserver directory port\n");
	}
	directory = argv[1];

	int serverSocket, newSocket;
	struct sockaddr_in serverAddr;
	struct sockaddr_storage serverStorage;
	socklen_t addr_size;

	//Create the socket.
	serverSocket = socket(AF_INET, SOCK_STREAM, 0);

	// Configure settings of the server address struct
	// Address family = Internet
	serverAddr.sin_family = AF_INET;

	//Set port number, using htons function to use proper byte order
	serverAddr.sin_port = htons(atoi(argv[2]));

	//Set IP address to localhost
	serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	//Set all bits of the padding field to 0
	memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);

	//Bind the address struct to the socket
	bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr));

	//Listen on the socket, with 40 max connection requests queued
	if (listen(serverSocket, 50) == 0)
		printf("Listening\n");
	else
		printf("Error\n");
	pthread_t tid[60];
	int i = 0;
	while (1)
	{
		//Accept call creates a new socket for the incoming connection
		addr_size = sizeof serverStorage;
		newSocket = accept(serverSocket, (struct sockaddr *)&serverStorage, &addr_size);

		//for each client request creates a thread and assign the client request to it to process
		//so the main thread can entertain next request
		if (newSocket >= 0)
		{
			printf("%d", i);
			//printf("I am making sockets");
			if (pthread_create(&tid[i], NULL, socketThread, &newSocket) != 0)
				printf("Failed to create thread\n");
			i++;
		}
		else if (newSocket < 0)
			printf("Failed to connect");
		// copy from lab code
		if (i >= 50)
		{
			i = 0;
			while (i < 50)
			{
				pthread_join(tid[i++], NULL);
			}
			i = 0;
		}
		printf("finished\n");
	}
	return 0;
}