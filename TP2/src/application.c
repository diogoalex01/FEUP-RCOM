/*      (C)2000 FEUP  */

#include "application.h"

url_syntax url_arg;
char server_response[3];

int main(int argc, char **argv)
{
	int socket_fd, ret;
	struct sockaddr_in server_addr;
	// int bytes;

	if (argc != 2)
	{
		printf("Wrong number of arguments.\n");
		printf("Usage: %s ftp://[<user>:<password>@]<host>/<url-path>\n");
		return 1;
	}

	if (parse_arguments(argv[1]))
		return 1;

	char *server_ip = get_IP(url_arg.host);

	/*server address handling*/
	bzero((char *)&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(server_ip); /* 32 bit Internet address network byte ordered */
	server_addr.sin_port = htons(SERVER_PORT);			/* server TCP port must be network byte ordered */

	/*open an TCP socket*/
	if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket()");
		exit(0);
	}

	/*connect to the server*/
	if (connect(socket_fd,
				(struct sockaddr *)&server_addr,
				sizeof(server_addr)) < 0)
	{
		perror("connect()");
		exit(0);
	}

	ret = get_reply(server_response, 1024, socket_fd);
	if (ret == CONNECTED)
	{
		printf("Connection Established\n");
	}

	if (strlen(url_arg.user))
	{
		if (login(url_arg.user, url_arg.password, socket_fd))
		{
			printf("Invalid login\n");
			return 1;
		}
	}

	if (set_passive_mode(socket_fd))
	{
		printf("Could not set passive mode\n");
		return 1;
	}

	if (download(socket_fd))
	{
		printf("Could not download\n");
		return 1;
	}

	// /*send a string to the server*/
	// bytes = write(socket_fd, buf, strlen(buf));
	// printf("Bytes escritos %d\n", bytes);

	// close(socket_fd);

	exit(0);
}

char *get_IP(const char *name)
{
	struct hostent *h;

	/*
	struct hostent
	{
		char *h_name; Official name of the host.char **h_aliases; A NULL-terminated array of alternate names for the host. 
		int h_addrtype; The type of address being returned; usually AF_INET.
		int h_length; The length of the address in bytes.
		char **h_addr_list;	A zero-terminated array of network addresses for the host. Host addresses are in Network Byte Order.
	};

	#define h_addr h_addr_list[0] The first address in h_addr_list.
	*/

	if ((h = gethostbyname(name)) == NULL)
	{
		herror("gethostbyname");
		exit(1);
	}

	printf("Host name  : %s\n", h->h_name);
	printf("IP Address : %s\n", inet_ntoa(*((struct in_addr *)h->h_addr)));

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
		printf("Expected 'ftp://', got '%s'\n", url_arg.server);
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
		// parse host and url-path - host/url-path
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
		url_arg.url[strlen(url_arg.url)] = '\0';
	}

	print_parsed_url();

	return 0;
}

void print_parsed_url()
{
	printf(" Server: %s\n User: %s\n Password: %s\n Host: %s\n URL: 9%s99\n",
		   url_arg.server, url_arg.user, url_arg.password, url_arg.host, url_arg.url);
}

static int get_reply(char *resp, int len, int socket_fd)
{
	usleep(100 * 1000);
	memset(resp, 0, 1024);
	read(socket_fd, resp, 1024);
	char *asn_server = strndup(resp, 3);
	printf("respond:%s\n", resp);
	return atoi(asn_server);
}

int login(char *user, char *password, int socket_fd)
{
	char user_cmd[1024];
	char password_cmd[1024];
	int ret;
	// send USER
	sprintf(user_cmd, "USER %s\r\n", user);
	ret = write(socket_fd, user_cmd, strlen(user_cmd));

	if (!ret)
	{
		printf("Could not send username\n");
		close(socket_fd);
		return 1;
	}

	ret = get_reply(server_response, 1024, socket_fd);

	if (ret != REQ_PASSWORD)
	{
		printf("Invalid username\n");
		close(socket_fd);
		return 1;
	}

	// send PASS
	sprintf(password_cmd, "PASS %s\r\n", password);
	ret = write(socket_fd, password_cmd, strlen(password_cmd));

	if (!ret)
	{
		printf("Could not send password\n");
		close(socket_fd);
		return 1;
	}

	ret = get_reply(server_response, 1024, socket_fd);

	if (ret != CORRECT_PASSWORD)
	{
		printf("Invalid password\n");
		close(socket_fd);
		return 1;
	}

	printf("User authenticated\n");
	return 0;
}

int set_passive_mode(socket_fd)
{
	int ret, a, b, c, d, pa, pb;
	ret = write(socket_fd, PASSIVE_CMD, strlen(PASSIVE_CMD));

	if (!ret)
	{
		printf("Could not send passive mode request\n");
		close(socket_fd);
		return 1;
	}

	ret = get_reply(server_response, 1024, socket_fd);

	if (ret != PASSIVE_MODE)
	{
		printf("Passive mode not enabled\n");
		close(socket_fd);
		return 1;
	}

	char *reply;
	reply = strrchr(server_response, '(');
	sscanf(reply, "(%d,%d,%d,%d,%d,%d)", &a, &b, &c, &d, &pa, &pb);
	sprintf(url_arg.ipaddr, "%d.%d.%d.%d", a, b, c, d);
	url_arg.port = pa * 256 + pb;
	printf("addr %s\n", url_arg.ipaddr);
	printf("port %d\n", url_arg.port);

	printf("Passive mode enabled\n");
	return 0;
}

int send_retr(int socket_fd)
{
	char retr_cmd[1024];
	int ret;
	printf("\n2.1\n");
	sprintf(retr_cmd, "RETR /parrot/last-sync.txt\r\n");
	printf("\n2.2\n");

	ret = write(socket_fd, retr_cmd, strlen(retr_cmd));
	printf("\n2.3\n");

	if (!ret)
	{
		close(socket_fd);
		return 1;
	}
	printf("\n2.4\n");

	ret = get_reply(server_response, 1024, socket_fd);
	printf("\n2.5\n");
	printf("%d", ret);

	if (ret != RETR)
	{
		close(socket_fd);
		return 1;
	}

	printf("RETR sent\n");
	return 0;
}

int download(int socket_fd)
{
	int socket_client;
	struct sockaddr_in server_addr_client;
	FILE *new_file;
	char buffer[1024];
	int ret;

	if (!(new_file = fopen(url_arg.url, "w")))
	{
		printf("Unable to open file %s\n", url_arg.url);
		return 1;
	}

	/*server address handling*/
	bzero((char *)&server_addr_client, sizeof(server_addr_client));
	server_addr_client.sin_family = AF_INET;
	server_addr_client.sin_addr.s_addr = inet_addr(url_arg.ipaddr); /*32 bit Internet address network byte ordered*/
	server_addr_client.sin_port = htons(url_arg.port);				/*server TCP port must be network byte ordered */

	/*open an TCP socket*/
	if ((socket_client = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket()");
		exit(0);
	}
	/*connect to the server*/
	printf("\n1\n");

	if (connect(socket_client, (struct sockaddr *)&server_addr_client, sizeof(server_addr_client)) < 0)
	{
		perror("connect()");
		exit(0);
	}
	printf("\n2\n");

	if (send_retr(socket_fd))
	{
		printf("Unable to send RETR\n");
		return 1;
	}
	printf("\n3\n");
	while ((ret = read(socket_client, buffer, 1024)))
	{
		if ((ret = fwrite(buffer, ret, 1, new_file)) < 0)
		{
			printf("Unable to write to file %s\n");
			return 1;
		}
	}
	printf("\n4\n");
	if (ret < 0)
	{
		printf("Unable to read from socket\n");
		return 1;
	}
	printf("\n5\n");
	/*ret = get_reply(server_response, 1024, socket_fd);
	printf("\n6\n");
	if (ret != DOWNLOAD_COMPLETED)
	{	

		close(socket_client);
		return 1;
	}*/
	fclose(new_file);
	printf("Download completed\n");
	return 0;
}
// ftp://host/url-path
// ftp://user:password@host/url-path