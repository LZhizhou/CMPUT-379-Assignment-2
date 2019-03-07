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
union two_byte {
    int num;
    unsigned char bytes[2];
};
union one_byte {
    int num;
    unsigned char byte;
};

int read_1_byte(unsigned char prefix)
{
    return (int)(prefix);
}
int read_2_byte_char(unsigned char *arr)
{
    /*     int i = *(signed char *)(&arr[0]);
    i *= 1 << sizeof(char);
    i = i | arr[1];
    return i;  */
    union two_byte b;
    memset(&b, 0, sizeof(b));
    b.bytes[0] = arr[0];
    b.bytes[1] = arr[1];
    //printf("arr: %x%x|\n", arr[1],arr[0]);
    //printf("union: %x%x, num is %d\n", b.bytes[1], b.bytes[0],b.num);

    return b.num;
}

void prror(char *error_message)
{
    fprintf(stderr, "%s", error_message);
    exit(1);
}
void request_list(int server_fd)
{
    int count = 0, file_count = 0, file_total = 3;
    unsigned char buffer[200];
    while (file_count < file_total)
    {

        if (count == 0)
        {
            recv(server_fd, buffer, 1, 0);
            //printf("0x%02x|\n", read_1_byte(buffer[0]));
        }
        else if (count == 1)
        {
            recv(server_fd, buffer, 2, 0);

            file_total = read_2_byte_char(buffer);
            //printf("%d\n",file_total);
            //printf("%x%x|\n", buffer[1],buffer[0]);
        }
        else
        {
            recv(server_fd, buffer, sizeof(buffer), 0);
            char filenames[20] = {0};

            printf("%s\n", buffer);
            file_count++;
        }
        count++;
        //fflush(stdout);
        //printf("%s\n",buffer);
        //sleep(1);
        //printf("\n");
        memset(buffer, 0, sizeof(buffer));
    }
    return;
}
void download(int server_fd, char *filename)
{
    char buffer[200];

    FILE *fp = fopen(filename, "w");
    if (fp < 0)
    {
        printf("File Can Not Open To Write\n");
        exit(1);
    }

    int length = 0;
    memset(buffer, 0, sizeof(buffer));
    printf("start\n");
    while ((length = recv(server_fd, buffer, sizeof(buffer), 0)) > 0)
    {
        printf("length = %d\n", length);
        if (strcmp(buffer, "OK") == 0)
        {
            break;
        }
        if (fwrite(buffer, sizeof(char), length, fp) < length)
        {
            printf("File:\t%s Write Failed\n", filename);
            break;
        }
        memset(buffer, 0, sizeof(buffer));
    }
    printf("Receive File:\t%s From Server IP Successful!\n", filename);
    fclose(fp);
    printf("client_socket_fd = %d\n", server_fd);
}
void upload(int server_fd, char *filename)
{

    char buffer[200];

    FILE *fp = fopen(filename, "r");

    if (NULL == fp)
    {
        printf("File:%s Not Found\n", filename);
    }
    else
    {
        memset(buffer, 0, sizeof(buffer));
        int length = 0;
        while ((length = fread(buffer, sizeof(char), sizeof(buffer), fp)) > 0)
        {
            printf("length = %d\n", length);
            if (send(server_fd, buffer, length, 0) < 0)
            {
                printf("Send File:%s Failed.\n", filename);
                break;
            }
            memset(buffer, 0, sizeof(buffer));
        }
        sleep(1);
        fclose(fp);
        send(server_fd, "OK", 2, 0);
        printf("File:%s Transfer Successful!\n", filename);
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
        char filename[20], temp;
        fgets(sbuff, MAX_SIZE, stdin);
        sscanf(sbuff, " %c", &command);
        switch (command)
        {
        case 'l':
            // if command is l, it means list all files
            send(sock_desc, sbuff, strlen(sbuff) + 1, 0);
            //printf("send l\n");
            request_list(sock_desc);
            //printf("received\n");
            break;
        case 'u':
            send(sock_desc, sbuff, sizeof(sbuff), 0);
            sscanf(sbuff, " %c %s", &temp, filename);
            upload(sock_desc, filename);
            break;
        case 'd':
            // d command, send
            send(sock_desc, sbuff, sizeof(sbuff), 0);
            sscanf(sbuff, " %c %s", &temp, filename);
            download(sock_desc, filename);
            break;
        case 'q':
            // q command, quit
            goto close;

        default:
            break;
        }
        memset(sbuff, 0, sizeof(sbuff));
    }
close:
    close(sock_desc);
    return 0;
}
