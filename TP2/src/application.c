/*      (C)2000 FEUP  */

#include "application.h"

url_syntax url_arg;
char server_response[3];

int main(int argc, char **argv)
{
	int socket_fd, socket_client, ret;
	struct sockaddr_in server_addr;

	if (argc != 2)
	{
		printf("     **Wrong number of arguments**\n");
		printf("\tUsage: %s ftp://[<user>:<password>@]<host>/<url-path>\n", argv[0]);
		return 1;
	}

	if (parse_arguments(argv[1]))
		return 1;

	char *server_ip = get_IP(url_arg.host);

	/* server address handling */
	bzero((char *)&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(server_ip); /* 32 bit Internet address network byte ordered */
	server_addr.sin_port = htons(SERVER_PORT);			/* server TCP port must be network byte ordered */

	/* open an TCP socket */
	if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("     **Unable to open TCP socket.**\n\n");
		exit(0);
	}

	/* connect to the server */
	if (connect(socket_fd,
				(struct sockaddr *)&server_addr,
				sizeof(server_addr)) < 0)
	{
		perror("     **Unable to connect to the server.**\n\n");
		exit(0);
	}

	ret = get_reply(server_response, socket_fd);

	if (ret == CONNECTED)
	{
		printf("   [Connection Established]\n");
	}

	if (strlen(url_arg.user))
	{
		if (login(url_arg.user, url_arg.password, socket_fd))
		{
			printf("     **Invalid login**\n\n");
			return 1;
		}
	}

	if (set_passive_mode(socket_fd))
	{
		printf("     **Could not set passive mode**\n\n");
		return 1;
	}

	if (download(socket_fd, &socket_client))
	{
		printf("     **Could not download**\n\n");
		return 1;
	}

	if (disconnect_sockets(socket_fd, socket_client))
		return 1;

	return 0;
}

char *get_IP(const char *name)
{
	struct hostent *h;

	if ((h = gethostbyname(name)) == NULL)
	{
		herror("        **Unable to get host name.**");
		exit(1);
	}

	return inet_ntoa(*((struct in_addr *)h->h_addr));
}

// ftp://[<user>:<password>@]<host>/<url-path>
int parse_arguments(char *arguments)
{
	char arg_copy[100];
	strcpy(arg_copy, arguments);
	char *auth = strtok(arg_copy, "@"), *url;

	// parse server - ftp://
	memcpy(url_arg.server, arguments, 6);
	if (strcmp(url_arg.server, "ftp://"))
	{
		printf("\n     **Expected 'ftp://' host, got '%s'**\n\n", url_arg.server);
		return 1;
	}

	if (!strcmp(arguments, auth))
	{
		url = auth;
		auth = NULL;
	}
	else
	{
		url = strtok(NULL, "@");
	}

	// in case there's login info
	if (auth != NULL)
	{
		// parse user and password - user:password
		char *token, *tk;
		char *credentials = (char *)malloc(sizeof(char) * 512);
		token = strtok(auth, "/");

		while (token != NULL)
		{
			strcpy(credentials, token);
			token = strtok(NULL, "/");
		}

		strcpy(url_arg.user, strtok(credentials, ":"));

		while (credentials != NULL)
		{
			strcpy(url_arg.password, credentials);
			credentials = strtok(NULL, ":");
		}
		// parse host and url-path - host/folder/filename
		tk = strtok(url, "/");
		strcpy(url_arg.host, tk);

		while (tk != NULL)
		{
			tk = strtok(NULL, "/");
			if (tk == NULL)
				break;
			strcat(url_arg.url, "/");
			strcat(url_arg.url, tk);
		}

		memcpy(url_arg.url, &url_arg.url[1], strlen(url_arg.url));
		strcpy(url_arg.filename, strchr(url_arg.url, '/'));
		memcpy(url_arg.filename, &url_arg.filename[1], strlen(url_arg.filename));
	}
	// in case there's no login info
	else
	{
		// parse host and url-path - ftp://host/folder/filename
		char *tk = strtok(url, "/");
		tk = strtok(NULL, "/");
		strcpy(url_arg.host, tk);

		while (tk != NULL)
		{
			tk = strtok(NULL, "/");
			if (tk == NULL)
				break;
			strcat(url_arg.url, "/");
			strcat(url_arg.url, tk);
		}

		memcpy(url_arg.url, &url_arg.url[1], strlen(url_arg.url));
		strcpy(url_arg.filename, strchr(url_arg.url, '/'));
		memcpy(url_arg.filename, &url_arg.filename[1], strlen(url_arg.filename));
	}

	print_parsed_url();

	return 0;
}

void print_parsed_url()
{
	printf("\n   [Information parsed]\n    - Server: %s\n    - User: %s\n    - Password: %s\n    - Host: %s\n    - URL: %s\n    - Filename: %s\n",
		   url_arg.server, url_arg.user, url_arg.password, url_arg.host, url_arg.url, url_arg.filename);
}

static int get_reply(char *resp, int socket_fd)
{
	usleep(100 * 1000);
	memset(resp, 0, MAX_BUFFER_SIZE);
	read(socket_fd, resp, MAX_BUFFER_SIZE);
	char *asn_server = strndup(resp, 3);
	//printf("respond:%s\n", resp);
	return atoi(asn_server);
}

int login(char *user, char *password, int socket_fd)
{
	char user_cmd[MAX_BUFFER_SIZE];
	char password_cmd[MAX_BUFFER_SIZE];
	int ret;
	// send USER
	sprintf(user_cmd, "USER %s\r\n", user);
	ret = write(socket_fd, user_cmd, strlen(user_cmd));

	if (!ret)
	{
		printf("     **Could not send username**\n");
		close(socket_fd);
		return 1;
	}

	ret = get_reply(server_response, socket_fd);

	if (ret != REQ_PASSWORD)
	{
		printf("     **Invalid username**\n");
		close(socket_fd);
		return 1;
	}

	// send PASS
	sprintf(password_cmd, "PASS %s\r\n", password);
	ret = write(socket_fd, password_cmd, strlen(password_cmd));

	if (!ret)
	{
		printf("     **Could not send password**\n");
		close(socket_fd);
		return 1;
	}

	ret = get_reply(server_response, socket_fd);

	if (ret != CORRECT_PASSWORD)
	{
		printf("     **Invalid password**\n");
		close(socket_fd);
		return 1;
	}

	printf("   [User authenticated]\n");
	return 0;
}

int set_passive_mode(int socket_fd)
{
	int ret, a, b, c, d, pa, pb;
	ret = write(socket_fd, PASSIVE_CMD, strlen(PASSIVE_CMD));

	if (!ret)
	{
		printf("     **Could not send passive mode request**\n");
		close(socket_fd);
		return 1;
	}

	ret = get_reply(server_response, socket_fd);

	if (ret != PASSIVE_MODE)
	{
		printf("     **Passive mode not enabled**\n");
		close(socket_fd);
		return 1;
	}

	char *reply;
	reply = strrchr(server_response, '(');
	sscanf(reply, "(%d,%d,%d,%d,%d,%d)", &a, &b, &c, &d, &pa, &pb);
	sprintf(url_arg.ipaddr, "%d.%d.%d.%d", a, b, c, d);
	url_arg.port = pa * MAX_BUFFER_SIZE / 4 + pb;
	printf("   [Passive mode enabled]\n");
	return 0;
}

int send_retr(int socket_fd)
{
	char retr_cmd[MAX_BUFFER_SIZE];
	int ret;

	sprintf(retr_cmd, "RETR %s\r\n", url_arg.url);
	ret = write(socket_fd, retr_cmd, strlen(retr_cmd));

	if (!ret)
	{
		close(socket_fd);
		return 1;
	}

	ret = get_reply(server_response, socket_fd);

	if (ret != RETR)
	{
		close(socket_fd);
		return 1;
	}

	printf("   [Sending RETR command]\n");
	return 0;
}

int download(int socket_fd, int *socket_client)
{
	struct sockaddr_in server_addr_client;
	FILE *new_file;
	char buffer[MAX_BUFFER_SIZE];
	int ret;

	if (!(new_file = fopen(url_arg.filename, "w")))
	{
		printf("     **Unable to open file %s**\n", url_arg.filename);
		return 1;
	}

	/*server address handling*/
	bzero((char *)&server_addr_client, sizeof(server_addr_client));
	server_addr_client.sin_family = AF_INET;
	server_addr_client.sin_addr.s_addr = inet_addr(url_arg.ipaddr); /*32 bit Internet address network byte ordered*/
	server_addr_client.sin_port = htons(url_arg.port);				/*server TCP port must be network byte ordered */

	/*open an TCP socket*/
	if (((*socket_client) = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("     **Unable to open TCP socket**\n");
		exit(0);
	}
	/*connect to the server*/
	if (connect((*socket_client), (struct sockaddr *)&server_addr_client, sizeof(server_addr_client)) < 0)
	{
		perror("     **Unable connect to the server**\n");
		exit(0);
	}

	if (send_retr(socket_fd))
	{
		printf("     **Unable to send RETR**\n");
		return 1;
	}

	while ((ret = read((*socket_client), buffer, MAX_BUFFER_SIZE)))
	{
		if ((ret = fwrite(buffer, ret, 1, new_file)) < 0)
		{
			printf("     **Unable to write to file %s**\n", url_arg.filename);
			return 1;
		}
	}

	if (ret < 0)
	{
		printf("     **Unable to read from socket**\n");
		return 1;
	}

	/*
	ret = get_reply(server_response, socket_fd);

	if (ret != DOWNLOAD_COMPLETED)
	{

		close(socket_client);
		return 1;
	}*/

	fclose(new_file);
	printf("   [Download completed]\n");
	return 0;
}

int disconnect_sockets(int socket_fd, int socket_client)
{
	if (close(socket_fd) || close(socket_client))
	{
		printf("     **Error closing sockets**\n");
		return 1;
	}

	printf("   [Sockets disconnected]\n\n");
	return 0;
}
