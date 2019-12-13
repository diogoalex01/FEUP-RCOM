/*      (C)2000 FEUP  */

#include "application.h"

url_syntax url_arg;

int main(int argc, char **argv)
{
	// int sockfd;
	// struct sockaddr_in server_addr;
	// char buf[] = "Mensagem de teste na travessia da pilha TCP/IP\n";
	// int bytes;

	// /*server address handling*/
	// bzero((char *)&server_addr, sizeof(server_addr));
	// server_addr.sin_family = AF_INET;
	// server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDR); /* 32 bit Internet address network byte ordered */
	// server_addr.sin_port = htons(SERVER_PORT);			  /* server TCP port must be network byte ordered */

	// /*open an TCP socket*/
	// if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	// {
	// 	perror("socket()");
	// 	exit(0);
	// }

	// /*connect to the server*/
	// if (connect(sockfd,
	// 			(struct sockaddr *)&server_addr,
	// 			sizeof(server_addr)) < 0)
	// {
	// 	perror("connect()");
	// 	exit(0);
	// }

	// /*send a string to the server*/
	// bytes = write(sockfd, buf, strlen(buf));
	// printf("Bytes escritos %d\n", bytes);

	// close(sockfd);
	parse_arguments(argv[1]);
	exit(0);
}

void get_IP(const char *name)
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
		exit(1);
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
		char *token, *credentials, *tk;
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
			strcpy(url_arg.url, tk);
			tk = strtok(NULL, "/");
		}
	}
	// in case there's no login info
	else
	{
		// parse host and url-path - host/url-path
		char *tk = strtok(url, "/");
		tk = strtok(NULL, "/");
		strcpy(url_arg.host, tk);

		while (tk != NULL)
		{
			strcpy(url_arg.url, tk);
			tk = strtok(NULL, "/");
		}
	}

	print_parsed_url();

	return 0;
}

void print_parsed_url()
{
	printf(" Server: %s\n User: %s\n Password: %s\n Host: %s\n URL: %s\n",
		   url_arg.server, url_arg.user, url_arg.password, url_arg.host, url_arg.url);
}

// ftp://host/url-path
// ftp://user:password@host/url-path