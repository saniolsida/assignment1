#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <dirent.h>
#include <sys/stat.h>

#define BUF_SIZE 1024
#define FILE_SIZE 512

typedef struct
{
	char file_name[FILE_SIZE];
	char file_path[FILE_SIZE];
	int bytes;
} file_info_t;

void error_handling(char *messgae);
void get_file_info(const char *folder_name, file_info_t file_info[], int *index);

int main(int argc, char *argv[])
{
	int serv_sock, clnt_sock;
	int str_len;
	int file_count;
	int index = 0;
	struct sockaddr_in serv_addr;
	struct sockaddr_in clnt_addr;
	file_info_t file_info[BUF_SIZE];

	socklen_t clnt_addr_size;
	char * curr_dir = getcwd(NULL, 0);

	char message[BUF_SIZE];

	if (argc != 2)
	{
		printf("Usage: %s <port>\n", argv[0]);
		exit(1);
	}

	serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	if (serv_sock == -1)
		error_handling("socket() error");

	get_file_info(curr_dir, file_info, &index);

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(atoi(argv[1]));

	if (bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
	{
		perror("bind() error");
		error_handling("bind() error");
	}

	if (listen(serv_sock, 5) == -1)
		error_handling("listen() error");

	clnt_addr_size = sizeof(clnt_addr);
	clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
	if (clnt_sock == -1)
		error_handling("accept() error");

	while (1)
	{
		for (int i = 0; i < index; i++)
		{
			snprintf(message, sizeof(message), "%d) %s, %d bytes", i + 1, file_info[i].file_name, file_info[i].bytes);
			printf("%s\n", message);
			write(clnt_sock, message, sizeof(message));
		}
		write(clnt_sock, "[END]", 5);

		str_len = read(clnt_sock, message, BUF_SIZE - 1);
		message[str_len] = '\0';
		printf("Received number: %s\n", message);

		int input = atoi(message) - 1;

		char file_name_bytes[BUF_SIZE];

		snprintf(file_name_bytes, sizeof(file_name_bytes), "%s %d",file_info[input].file_name, file_info[input].bytes);
		write(clnt_sock, file_name_bytes, strlen(file_name_bytes));


		FILE *fp = fopen(file_info[input].file_path, "rb");

		if (!fp)
			error_handling("fopen() error");

		while (1)
		{
			str_len = fread((void *)message, 1, BUF_SIZE, fp);
			if (str_len < BUF_SIZE)
			{
				write(clnt_sock, message, str_len);
				break;
			}
			write(clnt_sock, message, BUF_SIZE);
		}
		// shutdown(clnt_sock, SHUT_WR);
		memset(&message, 0, sizeof(message));
		str_len = read(clnt_sock, message, BUF_SIZE);
		message[str_len] = '\0';

		printf("Message from client: %s \n", message);

		fclose(fp);
	}

	close(clnt_sock);
	close(serv_sock);
	return 0;
}

void get_file_info(const char *folder_name, file_info_t file_info[], int *index)
{
	DIR *dir = opendir(folder_name);
	struct stat sb;
	if (!dir)
		error_handling("opendir() error");

	struct dirent *entry;
	struct stat st;

	while ((entry = readdir(dir)) != NULL)
	{
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
		{
			continue;
		}

		char path[FILE_SIZE];
		snprintf(path, sizeof(path), "%s/%s", folder_name, entry->d_name);
		if (stat(path, &st) == -1)
			error_handling("stat() failed");

		if (S_ISREG(st.st_mode))
		{
			strcpy(file_info[*index].file_path, path); // 파일 경로 저장

			char *ptr = strtok(path, "/");
			char *file_name;
			while (ptr != NULL)
			{
				file_name = ptr;
				ptr = strtok(NULL, "/");
			}
			strcpy(file_info[*index].file_name, file_name); // 파일 이름 저장

			// stat
			stat(file_info[*index].file_path, &st);

			file_info[*index].bytes = st.st_size;
			(*index)++;
		}
		if (S_ISDIR(st.st_mode))
		{
			get_file_info(path, file_info, index);
		}
	}

	closedir(dir);
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
