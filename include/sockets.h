/*
 *
 * (C) 2003-2011 Anope Team
 * Contact us at team@anope.org
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of Epona by Lara.
 * Based on the original code of Services by Andy Church.
 */

#ifndef SOCKETS_H
#define SOCKETS_H

#include "anope.h"

#define NET_BUFSIZE 65535

#ifdef _WIN32
# define CloseSocket closesocket
#else
# define CloseSocket close
#endif

/** A sockaddr union used to combine IPv4 and IPv6 sockaddrs
 */
union CoreExport sockaddrs
{
	sockaddr sa;
	sockaddr_in sa4;
	sockaddr_in6 sa6;

	/** Construct the object, sets everything to 0
	 */
	sockaddrs();

	/** Memset the object to 0
	 */
	void clear();

	/** Get the size of the sockaddr we represent
	 * @return The size
	 */
	size_t size() const;

	/** Get the port represented by this addr
	 * @return The port, or -1 on fail
	 */
	int port() const;

	/** Get the address represented by this addr
	 * @return The address
	 */
	Anope::string addr() const;

	/** Check if this sockaddr has data in it
	 */
	bool operator()() const;

	/** Compares with sockaddr with another. Compares address type, port, and address
	 * @return true if they are the same
	 */
	bool operator==(const sockaddrs &other) const;
	/* The same as above but not */
	inline bool operator!=(const sockaddrs &other) const { return !(*this == other); }

	/** The equivalent of inet_pton
	 * @param type AF_INET or AF_INET6
	 * @param address The address to place in the sockaddr structures
	 * @param pport An option port to include in the  sockaddr structures
	 * @throws A socket exception if given invalid IPs
	 */
	void pton(int type, const Anope::string &address, int pport = 0);

	/** The equivalent of inet_ntop
	 * @param type AF_INET or AF_INET6
	 * @param address The in_addr or in_addr6 structure
	 * @throws A socket exception if given an invalid structure
	 */
	void ntop(int type, const void *src);
};

class CoreExport cidr
{
	sockaddrs addr;
	Anope::string cidr_ip;
	unsigned char cidr_len;
 public:
 	cidr(const Anope::string &ip);
	cidr(const Anope::string &ip, unsigned char len);
	Anope::string mask() const;
	bool match(sockaddrs &other);
};

class SocketException : public CoreException
{
 public:
	/** Default constructor for socket exceptions
	 * @param message Error message
	 */
	SocketException(const Anope::string &message) : CoreException(message) { }

	/** Default destructor
	 * @throws Nothing
	 */
	virtual ~SocketException() throw() { }
};

enum SocketType
{
	SOCKTYPE_BASE,
	SOCKTYPE_BUFFERED,
	SOCKTYPE_CONNECTION,
	SOCKTYPE_CLIENT,
	SOCKTYPE_LISTEN
};

enum SocketFlag
{
	SF_DEAD,
	SF_WRITABLE
};

static const Anope::string SocketFlagStrings[] = { "SF_DEAD", "SF_WRITABLE", "" };

class Socket;
class ClientSocket;
class ListenSocket;
class ConnectionSocket;

class CoreExport SocketIO
{
 public:
	/** Receive something from the buffer
 	 * @param s The socket
	 * @param buf The buf to read to
	 * @param sz How much to read
	 * @return Number of bytes received
	 */
	virtual int Recv(Socket *s, char *buf, size_t sz);

	/** Write something to the socket
 	 * @param s The socket
	 * @param buf What to write
	 * @return Number of bytes written
	 */
	virtual int Send(Socket *s, const Anope::string &buf);

	/** Accept a connection from a socket
	 * @param s The socket
	 * @return The new socket
	 */
	virtual ClientSocket *Accept(ListenSocket *s);

	/** Check if a connection has been accepted
	 * @param s The client socket
	 * @return -1 on error, 0 to wait, 1 on success
	 */
	virtual int Accepted(ClientSocket *cs);

	/** Bind a socket
	 * @param s The socket
	 * @param ip The IP to bind to
	 * @param port The optional port to bind to
	 */
	virtual void Bind(Socket *s, const Anope::string &ip, int port = 0);

	/** Connect the socket
	 * @param s The socket
	 * @param target IP to connect to
	 * @param port to connect to
	 */
	virtual void Connect(ConnectionSocket *s, const Anope::string &target, int port);

	/** Check if this socket is connected
	 * @param s The socket
	 * @return -1 for error, 0 for wait, 1 for connected
	 */
	virtual int Connected(ConnectionSocket *s);

	/** Called when the socket is destructing
	 */
	virtual void Destroy() { }
};

class CoreExport Socket : public Flags<SocketFlag, 2>
{
 protected:
	/* Socket FD */
	int Sock;
	/* Is this an IPv6 socket? */
	bool IPv6;

 public:
	/* Sockaddrs for bind() (if it's bound) */
	sockaddrs bindaddr;

	/* I/O functions used for this socket */
 	SocketIO *IO;

	/* Type this socket is */
	SocketType Type;

	/** Empty constructor, used for things such as the pipe socket
	 */
	Socket();

	/** Default constructor
	 * @param sock The socket to use, 0 if we need to create our own
	 * @param ipv6 true if using ipv6
	 * @param type The socket type, defaults to SOCK_STREAM
	 */
 	Socket(int sock, bool ipv6, int type = SOCK_STREAM);

	/** Default destructor
	 */
	virtual ~Socket();

	/** Get the socket FD for this socket
	 * @return the fd
	 */
	int GetFD() const;

	/** Check if this socket is IPv6
	 * @return true or false
	 */
	bool IsIPv6() const;

	/** Mark a socket as blockig
	 * @return true if the socket is now blocking
	 */
	bool SetBlocking();

	/** Mark a socket as non-blocking
	 * @return true if the socket is now non-blocking
	 */
	bool SetNonBlocking();

	/** Bind the socket to an ip and port
	 * @param ip The ip
	 * @param port The port
	 */
	void Bind(const Anope::string &ip, int port = 0);

	/** Called when there is something to be received for this socket
	 * @return true on success, false to drop this socket
	 */
	virtual bool ProcessRead();

	/** Called when the socket is ready to be written to
	 * @return true on success, false to drop this socket
	 */
	virtual bool ProcessWrite();

	/** Called when there is an error for this socket
	 * @return true on success, false to drop this socket
	 */
	virtual void ProcessError();
};

class CoreExport BufferedSocket : public Socket
{
 protected:
	/* Things to be written to the socket */
	Anope::string WriteBuffer;
	/* Part of a message sent from the server, but not totally received */
	Anope::string extrabuf;
	/* How much data was received from this socket */
	int RecvLen;

 public:
	/** Blank constructor
	 */
	BufferedSocket();

	/** Constructor
	 * @param fd FD to use
	 * @param ipv6 true for ipv6
	 * @param type socket type, defaults to SOCK_STREAM
	 */
	BufferedSocket(int fd, bool ipv6, int type = SOCK_STREAM);

	/** Default destructor
	 */
	virtual ~BufferedSocket();

	/** Called when there is something to be received for this socket
	 * @return true on success, false to drop this socket
	 */
	bool ProcessRead();

	/** Called when the socket is ready to be written to
	 * @return true on success, false to drop this socket
	 */
	bool ProcessWrite();

	/** Called with a line received from the socket
	 * @param buf The line
	 * @return true to continue reading, false to drop the socket
	 */
	virtual bool Read(const Anope::string &buf);

	/** Write to the socket
	* @param message The message
	*/
	void Write(const char *message, ...);
	void Write(const Anope::string &message);

	/** Get the length of the read buffer
	 * @return The length of the read buffer
	 */
	int ReadBufferLen() const;

	/** Get the length of the write buffer
	 * @return The length of the write buffer
	 */
	int WriteBufferLen() const;
};

class CoreExport ListenSocket : public Socket
{
 public:
	/** Constructor
	 * @param bindip The IP to bind to
	 * @param port The port to listen on
	 * @param ipv6 true for ipv6
	 */
	ListenSocket(const Anope::string &bindip, int port, bool ipv6);

	/** Destructor
	 */
	virtual ~ListenSocket();

	/** Process what has come in from the connection
	 * @return false to destory this socket
	 */
	bool ProcessRead();

	/** Called when a connection is accepted
 	 * @param fd The FD for the new connection
 	 * @param addr The sockaddr for where the connection came from
	 * @return The new socket
	 */
	virtual ClientSocket *OnAccept(int fd, const sockaddrs &addr);
};

class CoreExport ConnectionSocket : public BufferedSocket
{
 public:
	/* Sockaddrs for connection ip/port */
	sockaddrs conaddr;
	/* True if connected */
	bool connected;

	/** Constructor
	 * @param ipv6 true to use IPv6
	 * @param type The socket type, defaults to SOCK_STREAM
	 */
	ConnectionSocket(bool ipv6 = false, int type = SOCK_STREAM);

	/** Connect the socket
	 * @param TargetHost The target host to connect to
	 * @param Port The target port to connect to
	 */
	void Connect(const Anope::string &TargetHost, int Port);

	/** Called when there is something to be received for this socket
	 * @return true on success, false to drop this socket
	 */
	bool ProcessRead();

	/** Called when the socket is ready to be written to
	 * @return true on success, false to drop this socket
	 */
	bool ProcessWrite();

	/** Called when there is an error for this socket
	 * @return true on success, false to drop this socket
	 */
	void ProcessError();

	/** Called on a successful connect
	 */
	virtual void OnConnect();

	/** Called when a connection is not successful
	 * @param error The error
	 */
	virtual void OnError(const Anope::string &error);
};

class CoreExport ClientSocket : public BufferedSocket
{
 public:
	/* Listen socket this connection came from */
	ListenSocket *LS;
	/* Clients address */
	sockaddrs clientaddr;

	/** Constructor
	 * @param ls Listen socket this connection is from
	 * @param fd New FD for this socket
	 * @param addr Address the connection came from
	 */
	ClientSocket(ListenSocket *ls, int fd, const sockaddrs &addr);

	/** Called when there is something to be received for this socket
	 * @return true on success, false to drop this socket
	 */
	bool ProcessRead();

	/** Called when the socket is ready to be written to
	 * @return true on success, false to drop this socket
	 */
	bool ProcessWrite();
};

class CoreExport Pipe : public BufferedSocket
{
 public:
 	/** The FD of the write pipe (if this isn't evenfd)
	 * this->Sock is the readfd
	 */
 	int WritePipe;

 	/** Constructor
	 */
	Pipe();

	/** Called when data is to be read
	 */
	bool ProcessRead();

	/** Function that calls OnNotify
	 */
	bool Read(const Anope::string &);

	/** Called when this pipe needs to be woken up
	 */
	void Notify();

	/** Should be overloaded to do something useful
	 */
	virtual void OnNotify();
};

#endif // SOCKET_H
