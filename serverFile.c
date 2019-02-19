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

char client_message[2000];
char server_message[2000];
char buffer[1024];
char *directory;

int total_strlen(char *string, int size)
{
	int count = size;
	for (int i = 0; i < size - 1; i++)
	{
		if (string[i] == '\0' && string[i + 1] == '\0')
		{
			count = i;
			break;
		}
	}
	return count;
}

void print_whole_string(char *string, int size)
{
	for (int i = 0; i < size; i++)
	{
		printf("%c", string[i]);
	}
	printf("\n");
}

void stradd(char *a, char *b, int size_a,int size_b)
{
	if (size_a == 0)
	{
		strcat(a, b);
		return;
	}
	for (int i = 0; i < size_b; i++)
	{
		a[size_a + i + 1] = b[i];
	}
}
void prror(char *error_message)
{
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
void list(int connection_fd)
{
	char buf[1024] = {0};

	DIR *dir;
	struct dirent *item;

	dir = opendir(directory);
	int file_count = 0;
	while ((item = readdir(dir)) != NULL)
	{

		if ((strcmp(item->d_name, ".") == 0) || (strcmp(item->d_name, "..") == 0))
			continue;
		//printf("this file name is %s\n", item->d_name);
		stradd(buf, item->d_name, total_strlen(buf, sizeof(buf)),strlen(item->d_name));
		file_count++;
	}
	char res[1024] = {0};
	sprintf(res, "%d", file_count);
	stradd(res, buf, strlen(res),total_strlen(buf, sizeof(buf)));
	send(connection_fd, res, 1024, 0);
	print_whole_string(buf, sizeof(buf));
	close(connection_fd);
}
void download(int connection_fd, char *client_message)
{
	char filename[20], temp;
	sscanf(client_message, " %c %s", &temp, filename);
	struct stat stat_buf;
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
	}
}

void *socketThread(void *arg)
{

	int read_fd;
	int newSocket = *((int *)arg);
	// Send message to the client socket

	char str[80];
	if (recv(newSocket, client_message, 2000, 0) == 0)
		printf("Error");
	else
	{
		char flag;
		sscanf(client_message, " %c", &flag);
		switch (client_message[0])
		{
		case 'l':
			// if command is l, it means list all files
			list(newSocket);
			break;
		case 'u':
			break;
		case 'd':
			// d command, send
			download(newSocket, client_message);
			break;
		case 'q':
			break;
		default:
			break;
		}
	}

	send(newSocket, buffer, 13, 0);
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
			printf("I am making sockets");
			if (pthread_create(&tid[i], NULL, socketThread, &newSocket) != 0)
				printf("Failed to create thread\n");
			i++;
		}
		else if (newSocket < 0)
			printf("Failed to connect");

		if (i >= 50)
		{
			i = 0;
			while (i < 50)
			{
				pthread_join(tid[i++], NULL);
			}
			i = 0;
		}
	}
	return 0;
}