#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>

#define bzero(b,len) (memset((b), '\0', (len)), (void) 0)
#define GAME_SPEED 0

int start_server(int portno);
int start_client(char *host, int portno);



int other_engine; //Socket file descriptor for the other chess engine
char *buffer;

int error(char *msg)
{
  perror(msg);
  return -1;
}

int start_network(int role, char *host, int portno)
{
  if (role == 1)
    return start_server(portno);
  else if (role == 2)
    return start_client(host, portno);
  else
    return error("Error, wrong role specified when starting network.\n");
}

int start_server(int portno)
{
  int sockfd, clilen;
  struct sockaddr_in serv_addr, cli_addr;
  int n;
  
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
    return error("Error opening socket.\n");

  bzero((char *) &serv_addr, sizeof(serv_addr));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);

  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    return error("Error on binding.\n");

  listen(sockfd, 5);
  clilen = sizeof(cli_addr);

  fprintf(stderr, "Waiting for client to connect...\n");

  for (n = 0; n < 5; n++)
  {
    fprintf(stderr, "Attempting to connect... (%d)\n", n+1);
    other_engine = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if (!(other_engine < 0))
    {
      fprintf(stdout, "Connected!!\n");
      return 1;
    }
    sleep(3);
  }   

  return -1;
}

int start_client(char *host, int portno)
{
    struct sockaddr_in serv_addr;
    struct hostent *server;
	
    other_engine = socket(AF_INET, SOCK_STREAM, 0);
    if (other_engine < 0) 
        error("ERROR opening socket\n");
    server = gethostbyname(host);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
		  (char *)&serv_addr.sin_addr.s_addr,
		  server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(other_engine,&serv_addr,sizeof(serv_addr)) < 0) 
        return -1;
    return 0;
}

void close_network()
{
  close(other_engine);
}

int send_move(char *move)
{
  sleep(GAME_SPEED);
  return send(other_engine, move, strlen(move), 0);
}

int get_move(char *buff, int len)
{
  bzero(buff,len);
  int n;

  n = recv(other_engine,buff,len,0);

  if (n <= 0)
    return -1;
  return 1;
}
