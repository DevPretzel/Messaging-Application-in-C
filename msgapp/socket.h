/* socket.h
 *
 * Wrapper classes for the C socket API and related functions.
 */

#ifndef _SOCKET_H
#define _SOCKET_H


/*
   A wrapper class for a standard client side Internet socket which 
   is used to connect to and communicate with a server. 
 */
 
class Socket {
public:  
   /* Create the socket object. */
  Socket();

   /* Open the socket for data transmission by connecting to the 
      indicated port on the given host. [host] can be either the Inet 
      hostname or the Inet address. */  
  bool 
  connect(const char *host, int port);
      
   /* Close the socket. */
  void 
  close();

   /* Transmit data across a connected socket. */
  int 
  send(void *data, int len);
  
   /* Receive data from the connected socket. Returns 0 if the socket was
      closed by the peer or the number of bytes actually received. */ 
  int 
  recv(void *data, int maxLen);
  
   /* Reads a line of text from the socket and stores it into the buffer.
      It stops reading when either maxLen-1 characters are read, the 
      newline character is read, or the end-of-file is reached. If read, the
      newline character IS included in the buffer. Returns 0 if the 
      end-of-file is encountered, -1 if an error occurs, or the number of 
      characters read (less the null character). */
  int
  readstr(char *buffer, int maxLen);
  
   /* Is the socket connected to a server? */
  bool
  isActive() const { return Active; }
      
   /* Get the sockets file descriptor number or -1 if the socket
      is not connected. */   
  int 
  fd( void ) const { return SockNum; }
  
   /* Returns the remote port to which the socket is connected. Can only 
      be used on an active socket. */  
  int 
  port() const;
    
   /* Returns a string containing the address to which the socket is 
      connected. Can only be used on an active socket. */  
  const char *
  address() const;
  
   /* Returns the source port of the socket. Can only be used on an 
      active socket. */  
  int 
  srcPort() const;
    
   /* Returns a string containing the address of the source socket. Can 
      only be used on an active socket. */    
  const char *
  srcAddress() const;  
  
              
protected:
   // Used internally to create a new socket.
  Socket(int thesock);

   // Makes ServerSocket a friend of this class.
  friend class ServerSocket;
  
protected:
  int SockNum;
  bool Active;
};


/*
   A wrapper class for a standard server side socket which listens
   for connections from clients. 
 */
 
class ServerSocket {
public:
   /* Create the server socket. */  
  ServerSocket();
  
   /* Open the socket for listening and binds it to the given port.
      The socked it bound to the default address of this host.
        maxClients - indicates the size of the client connection queue.
    */  
  bool 
  bind(int port, int maxClients = 5);
 
   /* Accepts a connection on the socket. Returns the new socket as a 
      dynamic Socket object. */
  Socket *
  accept();
 
   /* Close the socket. */
  void 
  close();
  
   /* Is the socket connected to a server? */
  bool
  isActive() const { return Active; }
      
   /* Get the sockets file descriptor number or -1 if the socket
      is not connected. */   
  int 
  fd( void ) const { return SockNum; }

      
protected:
  int SockNum;
  bool Active;  
};

#endif
