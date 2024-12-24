#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#define BUF_SIZE 1024
#define FILE_SIZE 512
void error_handling(char *message);

int main(int argc, char *argv[])
{
	int sock;
	struct sockaddr_in serv_addr;
	char message[BUF_SIZE], file_name[FILE_SIZE];
	int str_len = 0;
	int recv_len, recv_cnt;

	if (argc != 3)
	{
		printf("Usage : %s <IP> <port>\n", argv[0]);
		exit(1);
	}

	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock == -1)
		error_handling("socket() error");

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(atoi(argv[2]));

	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
		error_handling("connect() error!");

	while (1)
	{
		while ((recv_cnt = read(sock, message, BUF_SIZE)) > 0)
		{
			message[recv_cnt] = '\0';

			if (recv_cnt == -1)
				error_handling("read() error!");
			if (!strcmp(message, "[END]"))
				break;

			printf("%s\n", message);
		}

		printf("\n");
		fputs("Input file number: ", stdout);
		fgets(message, BUF_SIZE, stdin);

		write(sock, message, strlen(message));

		recv_cnt = read(sock, file_name, FILE_SIZE);

		file_name[recv_cnt] = '\0';

		FILE *fp = fopen(file_name, "wb");

		memset(&file_name, 0, sizeof(file_name));
		if (!fp)
			error_handling("fopen() failed");

		while ((recv_cnt = read(sock, message, BUF_SIZE)) > 0)
		{
			message[recv_cnt] = '\0';

			if(recv_cnt < BUF_SIZE)
			{
				fwrite((void *)message, 1, recv_cnt, fp);
				break;	
			}
			fwrite((void *)message, 1, recv_cnt, fp);
		}
		message[recv_cnt] = 0;

		str_len = write(sock, "Thank you!", 10);
		fclose(fp);
	}

	close(sock);
	return 0;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
