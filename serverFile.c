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
#include <libxml/parser.h>
#include <openssl/md5.h>
#include <signal.h>

char server_message[2000];
xmlDocPtr doc;

char *directory;

union two_byte {
	int num;
	unsigned char bytes[2];
};
union one_byte {
	int num;
	unsigned char byte;
};

void append_string(char *str1, char *str2, int index)
{
	for (int i = 0; i < strlen(str2); i++)
	{
		if (index == 0)
		{
			strcpy(str1, str2);
		}
		else
		{
			memcpy(index + str1, str2, sizeof(str2));
		}
	}
	//printf("str1 is %s, str2 is %s\n",str1,str2);
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
void create_xml()
{
	xmlNodePtr root_node;
	doc = xmlNewDoc(BAD_CAST "1.0");
	root_node = xmlNewNode(NULL, BAD_CAST "repository");
	xmlDocSetRootElement(doc, root_node);
	char *path;
	path = (char *)malloc(strlen(directory) + strlen(".dedup") + 2);
	strcpy(path, directory);
	strcat(path, "/.dedup");
	xmlSaveFormatFileEnc(path, doc, "UTF-8", 1);

	xmlFreeDoc(doc);
	xmlCleanupParser();
	xmlMemoryDump();
	doc = xmlParseFile(path);
	free(path);
}

void save_xml(char *filename, char hash[MD5_DIGEST_LENGTH * 2])
{

	xmlNodePtr root = xmlDocGetRootElement(doc);

	for (xmlNodePtr curr_node = root->xmlChildrenNode; curr_node != NULL; curr_node = curr_node->next)
	{
		for (xmlNodePtr tag_node = curr_node->xmlChildrenNode; tag_node != NULL; tag_node = tag_node->next)
		{
			if ((!xmlStrcmp(tag_node->name, (const xmlChar *)"hashname")) && !xmlStrcmp(xmlNodeGetContent(tag_node), (const xmlChar *)hash))
			{
				xmlNewTextChild(curr_node, NULL, "knownas", filename);
				return;
			}
		}
		/* 		if (curr_node->xmlChildrenNode != NULL)
		{
			
			printf("curr_node is %s\n", curr_node->name);
			xmlChar *test = xmlNodeGetContent(curr_node->xmlChildrenNode);
			printf("test: %s\n", test);
			for (int i = 0; i < sizeof(test); i++)
			{
				printf("%02x ", test[i]);
			}
			printf("\n");
			xmlFree(test);

			printf("%d\n", xmlStrcmp(xmlNodeGetContent(curr_node->xmlChildrenNode), BAD_CAST hash));
			if (xmlStrcmp(xmlNodeGetContent(curr_node->xmlChildrenNode), BAD_CAST hash) == 0)
			{
				xmlNewTextChild(curr_node, NULL, "knownas", filename);
				return;
			}
		} */
	}
	printf("new child\n");

	xmlNodePtr curr_node = xmlNewTextChild(root, NULL, "file", NULL);
	printf("bash is %s, filename is %s\n", hash, filename);
	xmlNewTextChild(curr_node, NULL, "hashname", hash);
	xmlNewTextChild(curr_node, NULL, "knownas", filename);
}

void remove_xml(char *filename)
{
	xmlNodePtr root = xmlDocGetRootElement(doc);

	for (xmlNodePtr curr_node = root->xmlChildrenNode; curr_node != NULL; curr_node = curr_node->next)
	{
		for (xmlNodePtr tag_node = curr_node->xmlChildrenNode; tag_node != NULL; tag_node = tag_node->next)
		{
			if ((!xmlStrcmp(tag_node->name, (const xmlChar *)"knownas")) && !xmlStrcmp(xmlNodeGetContent(tag_node), (const xmlChar *)filename))
			{
				xmlUnlinkNode(tag_node);
				xmlFreeNode(tag_node);

				if (xmlChildElementCount(curr_node) == 1)
				{
					xmlUnlinkNode(curr_node);
					xmlFreeNode(curr_node);
					continue;
				}
			}
		}
	}
}

void list(int connection_fd, int *message_count)
{ //send all files to connection_fd

	char buf[1024] = {0};

	DIR *dir;
	struct dirent *item;

	dir = opendir(directory);
	int name_len = 0;
	char filenames[20][50] = {0};
	union one_byte prefix;
	union two_byte file_count;
	file_count.num = 0;

	while ((item = readdir(dir)) != NULL)
	{
		if ((strcmp(item->d_name, ".") == 0) || (strcmp(item->d_name, "..") == 0))
			continue;
		// get names of all files
		strcpy(filenames[file_count.num], item->d_name);

		//name_len += strlen(item->d_name)+1;
		//filenames[file_count][strlen(item->d_name)+1] = '\0';
		file_count.num++;
	}
	//printf("malloc done\n");

	prefix.num = *message_count;
	send(connection_fd, &(prefix.byte), 1, 0);
	//printf("0x%02x\n", prefix.byte);

	/* 	unsigned char message_count_byte;
	message_count_byte = int2one_byte(*message_count);
	send(connection_fd, &message_count_byte, sizeof(message_count_byte), 0);
	printf("0x%02x\n", message_count_byte); */
	//sleep(1);
	//printf("|");
	send(connection_fd, file_count.bytes, 2, 0);
	//printf("%x%x|", file_count.bytes[1], file_count.bytes[0]);
	/* 	unsigned char file_count_2_byte[2] = {0};
	int2two_byte(file_count_2_byte, file_count);
	printf("|");
	send(connection_fd, file_count_2_byte, sizeof(file_count_2_byte), 0);
	printf("%x%x|", file_count_2_byte[0], file_count_2_byte[1]); */

	for (int i = 0; i < file_count.num; i++)
	{
		sleep(1);
		//printf("%s\n",filenames[i]);
		send(connection_fd, filenames[i], sizeof(filenames[i]), 0);
	}
	//printf("\n");
}
void remove_file(int connection_fd, char *filename)
{
	char path[200];
	sprintf(path, "%s/%s", directory, filename);
	if (cfileexists(path))
	{
		if (remove(path) == 0)
		{
			printf("Deleted successfully\n");
			remove_xml(filename);
		}

		else
			printf("Unable to delete the file\n");
	}
	else
	{
		printf("%s does not exist\n", path);
	}
}
void receive_upload(int connection_fd, char *filename)
{
	int read_fd;
	char buffer[200] = {0};
	char path[200];
	unsigned char *res;
	sprintf(path, "%s/%s", directory, filename);
	int exist = cfileexists(path);
	send(connection_fd, &exist, sizeof(int), 0);
	if (exist) {
		printf("exist\n");
		return;
	}
	
	FILE *fp = fopen(path, "w");
	if (fp < 0)
	{
		printf("File Can Not Open To Write\n");
		exit(1);
	}

	int length = 0, total_length = 0, received_length = 0;
	memset(buffer, 0, sizeof(buffer));
	printf("start\n");
	int count = 0;

	unsigned char c[MD5_DIGEST_LENGTH];
	MD5_CTX mdContext;
	MD5_Init(&mdContext);

	while ((length = recv(connection_fd, buffer, sizeof(buffer), 0)) > 0)
	{
		if (count == 0)
		{

			memcpy(&total_length, buffer, sizeof(int));
			if (total_length == -1)
			{
				printf("%s does not exits\n", filename);
				fclose(fp);
				remove(path);
				return;
			}
			printf("length = %d\n", total_length);
			count++;
		}
		else
		{
			if (fwrite(buffer, sizeof(char), length, fp) < length)
			{
				printf("File:\t%s Write Failed\n", filename);
				break;
			}
			MD5_Update(&mdContext, buffer, length);
			received_length += length;
			if (received_length >= total_length)
			{
				break;
			}
			memset(buffer, 0, sizeof(buffer));
		}
	}
	MD5_Final(c, &mdContext);
	char signed_c[MD5_DIGEST_LENGTH * 2] = {0};

	for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
	{

		printf("%02x", c[i]);
		char temp[3];
		sprintf(temp, "%02x", c[i]);
		strcat(signed_c, temp);
	}
	printf("converted: %s\n", signed_c);
	save_xml(filename, signed_c);

	printf("Receive File:\t%s From client IP Successful!\n", filename);
	fclose(fp);
	printf("server_socket_fd = %d\n", connection_fd);
}
void download(int connection_fd, char *filename)
{
	int read_fd;
	char buffer[200] = {0};
	char path[200];
	unsigned char *res;
	sprintf(path, "%s/%s", directory, filename);

	FILE *fp = fopen(path, "r");

	if (NULL == fp)
	{
		int error = -1
		printf("File:%s Not Found\n", filename);
		send(connection_fd, &error, sizeof(int), 0);
	}
	else
	{
		printf("start to sending file\n");
		//memset(buffer, 0,sizeof(buffer));
		int length = 0, total_length = 0;
		while ((length = fread(buffer, sizeof(char), sizeof(buffer), fp)) > 0)
		{
			total_length += length;
		}
		fclose(fp);
		send(connection_fd, &total_length, sizeof(int), 0);
		sleep(1);
		fp = fopen(path, "r");
		while ((length = fread(buffer, sizeof(char), sizeof(buffer), fp)) > 0)
		{
			printf("length = %d\n", length);
			if (send(connection_fd, buffer, length, 0) < 0)
			{
				printf("Send File:%s Failed.\n", filename);
				break;
			}
			memset(buffer, 0, sizeof(buffer));
		}
		fclose(fp);
		sleep(1);
		printf("File:%s Transfer Successful!\n", filename);
	}
}
void before_term()
{

	char *path;
	path = (char *)malloc(strlen(directory) + strlen(".dedup") + 2);
	strcpy(path, directory);
	strcat(path, "/.dedup");
	xmlSaveFormatFileEnc(path, doc, "UTF-8", 1);
	free(path);
	xmlFreeDoc(doc);
	xmlCleanupParser();
	xmlMemoryDump();
	exit(0);
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
			sscanf(client_message, " %c %s", &temp, filename);
			receive_upload(newSocket, filename);
			break;
		case 'd':
			// d command, send

			sscanf(client_message, " %c %s", &temp, filename);
			download(newSocket, filename);
			break;
		case 'q':
			// q command, quit
			goto close_socket;
		case 'r':
			sscanf(client_message, " %c %s", &temp, filename);
			remove_file(newSocket, filename);
			break;
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
	char *path;
	path = (char *)malloc(strlen(directory) + strlen(".dedup") + 2);
	strcpy(path, directory);
	strcat(path, "/.dedup");
	if (cfileexists(path))
	{
		doc = xmlParseFile(path);
	}
	else
	{
		create_xml();
	}
	free(path);

	signal(SIGTERM, before_term);
	signal(SIGINT, before_term);
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