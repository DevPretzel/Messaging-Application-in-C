
/* Define the protocol packet structure used for the client/server network 
   chat application.
*/

 // Used by the server.
const char REQ_ACCEPTED = 10;
const char CONNECT_NOTIF = 20;
const char DISCONNECT_NOTIF = 22;

 // Used by the clients.
const char REQ_CONNECT_HOST = 11;
const char REQ_CONNECT_GUEST = 12;
const char REQ_DISCONNECT = 13;

 // Used by the server and clients.
const char MSG_NOTIF = 21;

 // Errors.
const char ERR_BAD_OP = -10;
const char ERR_BAD_ID = -20;
const char ERR_ACTIVE_SESSION = -21;
const char ERR_NO_HOST = -22;
const char ERR_NO_GUEST = -23;


struct ChatPacket {
	char op;			    // the request or response code
	char gap[3];		    // not used
	int tag;			    // the session id number
	char message[128];		// the chat message
};