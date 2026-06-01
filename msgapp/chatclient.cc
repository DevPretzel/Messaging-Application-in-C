/* chatclient.cc
 *
 * Modified by: Roland Pelzel
 *
 * This program is the client part of the message relay network application.
 * It connects to the server using a basic protocol. At the command line, the 
 * client must provide the server address, port number, and must identify
 * themself as a host or guest. If they are a host, they must set the session
 * id; if they are the guest, they must connect via the session id set by the
 * host. 
 *
 *   chatclient <server-addr> <portnum> (host or guest) <sess-id>
 *
 */

#include "socket.h"
#include "selector.h"
#include "chatpacket.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

/* The max size of the relayed string messages, which includes '\n'. */
const int MSG_SIZE = 128;

/* The functions used in the top-down design of the problem solution. */
char * getServerInfo(int argc, char *argv[], int *port);
void connectToServer(char *server, int port, char *argv[]);
void closeConnection();
void initSelector();
void processMessages(char *argv[]);
bool processUserInput();
bool processServerResponse(char *argv[]);
int getSessId(const char *str);
bool isHost(char *argv[]);
int isValidId(const char *str);
void sigHandler(int sig);

/* Create a client socket and input selector as global variables. */
Socket clientSocket;
InputSelector inputSet;
int tag;


int main(int argc, char *argv[])
{  
  char *server;
  int port;
  
   /* Set the Ctrl-C signal handler. */
  signal(SIGINT, sigHandler);
 
   /* Get the server address and port number from the command-line. */
  server = getServerInfo(argc, argv, &port);
  
   /* Connect the client to the server. */
  connectToServer(server, port, argv);
  
   /* Initialize the input selector. */
  initSelector();
  
   /* Process the user and server messages. */
  processMessages(argv); 
  
   /* Close the connection to the server and shutdown. */
  closeConnection();
}


void processMessages(char *argv[])
{
  int *activeSet = 0;
  char userMessage[MSG_SIZE];

  bool done = false;
  while(!done) {
    printf(">"); 
    fflush(stdout);
    
    activeSet = inputSet.select();

    int i = 0;
    while(activeSet[i] >= 0 && !done) {
      if(activeSet[i] == 0) 
        done = processUserInput();
      else 
        done = processServerResponse(argv);
      i++;
    }
  }
}  

 // Processes the possible responses to receive from the server.
bool processServerResponse(char *argv[])
{
  ChatPacket responsePacket;

  clientSocket.recv(&responsePacket, sizeof(responsePacket));
  if(responsePacket.op == REQ_ACCEPTED) {
  	  tag = responsePacket.tag;			// sets the global variable tag
  	  return false;
  }
  else if(responsePacket.op == CONNECT_NOTIF) {
  	  printf("Guest connected.\n");
  	  return false;
  }
  else if(responsePacket.op == MSG_NOTIF) {
  	  printf("<<<< %s", responsePacket.message);
  	  return false;
  }
  else if(responsePacket.op == ERR_BAD_ID) {
  	  fprintf(stderr, "Error: bad session id number.\n");
  	  fprintf(stderr, "Disconnecting...\n");
  	  return true;
  }
  else if(responsePacket.op == ERR_ACTIVE_SESSION) {
  	  fprintf(stderr, "Error: cannot join active session.\n");
  	  exit(1);
  }
  else if(responsePacket.op == ERR_NO_HOST) {
  	  fprintf(stderr, "Connection failed: no host connected.\n");
  	  exit(1);
  }
  else if(responsePacket.op == ERR_NO_GUEST) {
  	  fprintf(stderr, "Error: no guest connected.\n");
  	  fprintf(stderr, "        Message was dropped.\n");
  	  return false;
  }
  else if(responsePacket.op == ERR_BAD_OP) {
  	  fprintf(stderr, "Unable to join session.\n");
  	  return true;
  }
  else if(responsePacket.op == DISCONNECT_NOTIF) {
  	  if(isHost(argv)) {
  	  	  printf("Guest disconnected.\n");
  	  	  return false;
  	  }
  	  else {
  	  	  printf("Host disconnected.\n");
  	  	  return true;
  	  }
  }
  else 
    return true;
}  

 // Takes user input and sends it to the server via packet.
bool processUserInput()
{
  char userMessage[MSG_SIZE];

  fgets(userMessage, MSG_SIZE, stdin);
  
  if(feof(stdin)) 
    return true;
  
  ChatPacket messagePacket = {.op = MSG_NOTIF, .tag = tag};
  strcpy(messagePacket.message, userMessage);
  clientSocket.send(&messagePacket, sizeof(messagePacket));

  return false;
}  


void initSelector()
{
   // add client’s socket fd and the standard in fd to the input set.
  inputSet.add(clientSocket.fd());
  inputSet.add(0);
}  

 // Connects the client to the server given a valid session id.
void connectToServer(char *server, int port, char *argv[])
{
  bool connected;
  // ChatPacket requestConnect;
  int sessId = isValidId(argv[4]);

  if(sessId > 0) {
  	  connected = clientSocket.connect(server, port);
	  if(connected) {
	  	  if(isHost(argv)) {
			  ChatPacket requestConnect = {.op = REQ_CONNECT_HOST, .tag = sessId, .message = {0}};
			  clientSocket.send(&requestConnect, sizeof(requestConnect));
		  }
		  else {
			  ChatPacket requestConnect = {.op = REQ_CONNECT_GUEST, .tag = sessId, .message = {0}};
			  clientSocket.send(&requestConnect, sizeof(requestConnect));
		  }
		printf("Connected to the server.\n");
	  }
	  else {
		printf("Error: can not connect to the server.\n");
		exit(1);
	  }
  }
  else {
  	  exit(1);
  }
}

 // Closes client connection to the server.
void closeConnection()
{
  clientSocket.close();
  printf("\nDone!! connection closed.\n");
}

 // Gets the port number and returns the server address.
char *getServerInfo(int argc, char *argv[], int *port)
{
     /* Get the server address and port number from the command-line. */
  if( argc < 5 ) {
    fprintf(stderr, "Error: Invalid number of arguments.\n" ); 
    fprintf(stderr, "usage: chatclient <server-addr> <portnum> " 
    				"(host or guest) <sess-id>\n\n" );
    exit(1);
  }
  else {
    *port = atoi( argv[2] );
    return argv[1]; // the server address 
  }
}

 // Checks that the host or guest argument is valid. 
bool isHost(char *argv[])
{
	if(strcmp(argv[3], "host") == 0) 
		return true;
	else if(strcmp(argv[3], "guest") == 0)
		return false;
	else {
		fprintf(stderr, "Error: Must be a host or guest.\n");
		fprintf(stderr, "usage: chatclient <server-addr> <portnum> " 
						"(host or guest) <sess-id>\n\n");
		exit(1);
	}
}

 // Checks that the session id is valid. 
int isValidId(const char *str)
{
	int sessId = atoi(str);
	
	if(sessId > 999)
		return sessId;
	else {
		fprintf(stderr, "Error: sess-id must be greater than 999.\n");
		return -1;
	}
}

 // Sends a disconnect request to the server and closes its connection.
void sigHandler(int sig)
{
  ChatPacket disconnect = {.op = REQ_DISCONNECT, .tag = tag, .message = {0}};
  clientSocket.send(&disconnect, sizeof(disconnect));
  printf("\nClosing connection to the server.\n");
  closeConnection();
  exit(0);
}