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
#include <malloc.h>

#define MAX_SIZE 500
#define NUM_CLIENT 1

void *connection_handler(void *socket_desc);

int read_1_byte(unsigned char prefix)
{
    return (int)(prefix);
}
int read_2_byte_char(unsigned char *arr)
{
    int i = *(signed char *)(&arr[0]);
    i *= 1 << sizeof(char);
    i = i | arr[1];
    return i;
}

void prror(char *error_message)
{
    fprintf(stderr, "%s", error_message);
    exit(1);
}
void request_list(int server_fd)
{
    int count = 0, file_count = 0, file_total = 1;
    char buffer[200], filename[30];
    unsigned char prefix, file_count_2_byte[2];
    while (file_count < file_total && recv(server_fd, buffer, sizeof(buffer), 0) != 0)
    {

        switch (count)
        {
        case 0:
            goto read_prefix;
            break;
        case 1:
            goto read_N1;
            break;
        default:
            goto read_filenams;
        }
    clear_buffer:
        count++;
        memset(buffer, 0, sizeof(buffer));
    }
    return;

read_prefix:

    memcpy(&prefix, buffer, 1);
    printf("0x%02x|", read_1_byte(prefix));
    goto clear_buffer;
read_N1:
    memcpy(file_count_2_byte, buffer, 2);
    file_total=read_2_byte_char(file_count_2_byte);
    printf("%d|", file_total);
    goto clear_buffer;

read_filenams:

    //printf("received:");
	for(int i = 0; i < sizeof(buffer); i++)
	{
		printf("%c",buffer[i]);
	}
	printf("\n");
    //printf("count is: %d, file_count is: %d, file_total is: %d\n", count, file_count, file_total);

    return;
}
void download(int server_fd, char *filename)
{
    char buffer[1024];

    FILE *fp = fopen(filename, "w");
    if (NULL == fp)
    {
        printf("File Can Not Open To Write\n");
        exit(1);
    }
    memset(buffer, 0, sizeof(buffer));
    int len = 0;
    while ((len = recv(server_fd, buffer, sizeof(buffer), 0)) > 0)
    {
        printf("len is %d\n", len);
    }
}

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        prror("Usage: ddupserver address port\n");
    }

    int sock_desc;
    int write_fd;
    struct sockaddr_in serv_addr;
    char sbuff[MAX_SIZE], rbuff[MAX_SIZE], filebuff[MAX_SIZE];
    //off_t offset = 0;

    if ((sock_desc = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        printf("Failed creating socket\n");

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    if (connect(sock_desc, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("Failed to connect to server\n");
    }

    while (1)
    {
        char command = 0;
        fgets(sbuff, MAX_SIZE, stdin);
        sscanf(sbuff, " %c", &command);
        switch (command)
        {
        case 'l':
            // if command is l, it means list all files
            send(sock_desc, sbuff, sizeof(sbuff), 0);
            printf("send l\n");
            request_list(sock_desc);
            break;
        case 'u':
            break;
        case 'd':
            // d command, send
            send(sock_desc, sbuff, sizeof(sbuff), 0);
            char filename[20], temp;
            sscanf(sbuff, " %c %s", &temp, filename);
            download(sock_desc, filename);
            break;
        case 'q':
            // q command, quit

        default:
            break;
        }
    }
    close(sock_desc);
    return 0;
}
