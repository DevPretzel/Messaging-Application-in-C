/* chatserver.cc
 *
 * Modified by: Roland Pelzel
 *
 * This program is the server part of the message relay network application.
 * It accepts connections from two clients and relays text messages 
 * between the two clients using a very basic and simple protocol. The 
 * listening port number can optionally be provided as a command-line. If 
 * no argument is provided, port number 30000 is used.
 * 
 *   chatserver [listening-port]
 */
 

#include "socket.h"
#include "selector.h"
#include "chatpacket.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>

/* The default port number for the server's listening socket. */
const int PORT_NUMBER = 30000;

/* The max size of the relayed string messages, which includes '\n'. */
const int MSG_SIZE = 128;

/* The functions used in the top-down design of the problem solution. */
void sigHandler(int sig);
int getPortNumber(int argc, char *argv[]);
void initServerSocket(int portNum);
void initSelector();
void processRequests();
void handleClientConnection();
void handleClientRequest(int fd);
void processClientMessage(ChatPacket message, Socket *toClient);
void disconnectClient(int fd);



/* Create the server's listening socket, an input selector and two 
 * client socket pointers as global variables. */
ServerSocket theServer;   
InputSelector inputSet;

Socket *host = 0;
Socket *guest = 0;

int sessId = 0;
int hTag;
int gTag;


int main(int argc, char *argv[]) 
{
     /* Set the Ctrl-C signal handler. */
  signal(SIGINT, sigHandler);
  
   /* Get the port number to use for the listening socket. */
  int portNum;
  portNum = getPortNumber(argc, argv);
  
   /* Initialize the listening socket. */
  initServerSocket(portNum);
  
   /* Initialize the input selector. */
  initSelector();
  
   /* Process protocol requests. */
  processRequests();  
}

 // Processes requests sent by the clients.
void processRequests()
{
  int *activeSet;
  while(true) {
    activeSet = inputSet.select();

    int i = 0;
    while(activeSet[i] >= 0) {
      int fd = activeSet[i];
      if(fd == theServer.fd())
        handleClientConnection();
      else
        handleClientRequest(fd);

      i++;
    }
  }
}

 // Receives packets sent by the clients. 
void handleClientRequest(int fd)
{
  if(host != NULL && fd == host->fd()) {
  	ChatPacket clMessage;
	int number = host->recv(&clMessage, sizeof(clMessage));
  	if(guest != NULL) {
		printf("Recv message from host.\n");
		if(number > 0) {
			if(clMessage.op == MSG_NOTIF) {
				if(clMessage.tag == hTag) {
					processClientMessage(clMessage, guest);
				}
				else {
					fprintf(stderr, "Error: incorrect tag. Message dropped.");
				}
			}
			else if(clMessage.op == REQ_DISCONNECT) {
				ChatPacket resp = {.op = DISCONNECT_NOTIF, .tag = 0, .message = {0}};
				guest->send(&resp, sizeof(resp));
				disconnectClient(host->fd());
				disconnectClient(guest->fd());
			}
		}
		else {
			printf("Host disconnected.\n");
			guest->send(&clMessage, sizeof(clMessage));
		}
	}
	else if(clMessage.op == REQ_DISCONNECT) {
		ChatPacket resp = {.op = DISCONNECT_NOTIF, .tag = 0, .message = {0}};
		host->send(&resp, sizeof(resp));
		disconnectClient(host->fd());
	}
	else {
		ChatPacket clMessage = {.op = ERR_NO_GUEST, .tag = 0, .message = {0}};
		host->send(&clMessage, sizeof(clMessage));
		printf("Failed to relay message: no guest.\n");
  	}
  }
  else if(guest != NULL && fd == guest->fd()) {
    ChatPacket clMessage;
    int number = guest->recv(&clMessage, sizeof(clMessage));
    if(number > 0) {
    	if(clMessage.op == MSG_NOTIF) {
    		if(clMessage.tag == gTag) {
    			printf("Recv message from guest.\n");
    			processClientMessage(clMessage, host);
    		}
    		else {
    			fprintf(stderr, "Error: incorrect tag. Message dropped.");
    		}
    	}
    	else if(clMessage.op == REQ_DISCONNECT) {
    		ChatPacket resp = {.op = DISCONNECT_NOTIF, .tag = 0, .message = {0}};
    		host->send(&resp, sizeof(resp));
    		disconnectClient(guest->fd());
    	}
    }
    else {
    	printf("Guest disconnected.\n");
    	guest->send(&clMessage, sizeof(clMessage));
    }
  }
}

 // Sets the packet tag to 0 and sends the message to the client.
void processClientMessage(ChatPacket message, Socket *toClient)
{
  message.tag = 0;
  toClient->send(&message, sizeof(message));
}

 // Disconnects either client from the server.
void disconnectClient(int fd)
{
  inputSet.remove(fd);

  if(host != NULL && fd == host->fd()) {
    printf("Host disconnected.\n");
    host->close();
    delete host;
    host = NULL;
  }
  else if(guest != NULL && fd == guest->fd()) {
    printf("Guest disconnected.\n");
    guest->close();
    delete guest;
    guest = NULL;
  }
}

 // Handles properly connected the host and guest to the server.
void handleClientConnection()
{
  Socket *theClient;
  theClient = theServer.accept();
  ChatPacket requestPacket;
//   ChatPacket responsePacket;

   // Is this the first client to connect?
  if(host == NULL) {
    host = theClient;
    srand(time(0));
    int number = host->recv(&requestPacket, sizeof(requestPacket));
    if(number > 0) {
    	if(requestPacket.op == REQ_CONNECT_HOST) {
    		sessId = requestPacket.tag;
    		hTag = rand();
    		ChatPacket responsePacket = {.op = REQ_ACCEPTED, .tag = hTag, .message = {0}};
    		host->send(&responsePacket, sizeof(responsePacket));
    		printf("Host connected.\n");
    		inputSet.add(theClient->fd());
    	}
    	else if(requestPacket.op == REQ_CONNECT_GUEST) {
    		ChatPacket responsePacket = {.op = ERR_NO_HOST, .tag = 0, .message = {0}};
    		host->send(&responsePacket, sizeof(responsePacket));
    		host->close();
    		delete host;
    		host = NULL;
    		fprintf(stderr, "Guest failed to connect: no host.\n");
    	}
    	else {
    		ChatPacket responsePacket = {.op = ERR_BAD_OP, .tag = 0, .message = {0}};
    		host->send(&responsePacket, sizeof(responsePacket));
    		fprintf(stderr, "Invalid operation code.\n");
    	}
    }
    
  }
   // or is this the second client to connect?
  else if(guest == NULL) {
    guest = theClient;
    srand(time(0));
    int number = guest->recv(&requestPacket, sizeof(requestPacket));
    if(number > 0) {
    	if(requestPacket.op == REQ_CONNECT_GUEST) {
    		if(requestPacket.tag == sessId) {
    			gTag = rand();
    			ChatPacket connPacket = {.op = REQ_ACCEPTED, .tag = gTag, .message = {0}};
    			guest->send(&connPacket, sizeof(connPacket));
    			printf("Guest connected.\n");
    			
    			ChatPacket responsePacket = {.op = CONNECT_NOTIF, .tag = 0, .message = {0}};
    			host->send(&responsePacket, sizeof(responsePacket));
    			printf("Notified host that guest connected.\n");
    			
    			inputSet.add(theClient->fd());
    		}
    		else {
    			ChatPacket responsePacket = {.op = ERR_BAD_ID, .tag = 0, .message = {0}};
    			guest->send(&responsePacket, sizeof(responsePacket));
    			guest->close();
    			delete guest;
    			guest = NULL;
    			fprintf(stderr, "Error: bad session id number.\n");
    			fprintf(stderr, "Guest disconnected.\n");
    		}
    	}
    	else if(requestPacket.op == REQ_CONNECT_HOST) {
    		ChatPacket responsePacket = {.op = ERR_BAD_OP, .tag = 0, .message = {0}};
    		guest->send(&responsePacket, sizeof(responsePacket));
    		guest->close();
    		delete guest;
    		guest = NULL;
    		fprintf(stderr, "Guest tried to connect as host.\n");
    	}
    }
  }
  else { // we already have two clients.
  	int number = theClient->recv(&requestPacket, sizeof(requestPacket));
  	if(number > 0) {
  		if(requestPacket.op == REQ_CONNECT_HOST || requestPacket.op == REQ_CONNECT_GUEST) {
  			ChatPacket responsePacket = {.op = ERR_ACTIVE_SESSION, .tag = 0, .message = {0}};
  			theClient->send(&responsePacket, sizeof(responsePacket));
  		}
  	}
    theClient->close();
    delete theClient;
    printf("Only two clients can connect at the same time.\n"); 
  }
}



void initSelector()
{
  inputSet.add(theServer.fd());
}
 


void initServerSocket(int portNum)
{
  bool bound = theServer.bind(portNum);
  if(bound) 
    printf("Server bound to port #%d\n", portNum); 
  else {
    printf("Error: the socket could not be bound"
           " to port #%d\n", portNum);
    exit(1);
  }
}  


 /* Returns the number to use for the listing port. If a value is provided on
    the commandline, that number is used for the listening port. Otherwise, 
    the default port number is used. */
int getPortNumber(int argc, char *argv[])
{
  if(argc > 1) 
    return atoi(argv[1]);
  else
    return PORT_NUMBER;
}


void sigHandler(int sig)
{
  printf("\nShutting down the server.\n");
  theServer.close();
  exit(0);
}