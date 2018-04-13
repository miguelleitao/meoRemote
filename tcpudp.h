
/*
 *                      C L I E N T . U D P
 *
 */
 
#ifndef _TCPUDP_H_ 

#define _TCPUDP_H_ 

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <signal.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
//#include <unistd.h>


//extern int errno;

/*
  void handler(int)
  {
  };
*/

typedef struct _ClientUDP
{ 
  int Soc;                         /* socket descriptor */
 
  struct hostent *hp;            /* pointer to info for nameserver host */
  struct servent *sp;            /* pointer to service information */
 
  struct sockaddr_in myaddr_in;  /* for local socket address */
  struct sockaddr_in sa;	/* for server socket addres */
  struct in_addr reqaddr;        /* for returned internet address */
} ClientUDP; 

  #define ADDRNOTFOUND 0xffffffff /* value returned for unknown host */
  #define RETRIES 5 /* # of times to retry before giving up */

  /*
   *                      H A N D L E R
   *
   *      This routine is the signal handler for the alarm signal.
   *      It simply re-installs itself as the handler and returns.
   */
  void NewClientUDP(ClientUDP *cli)
  {
        /* clear out address structures */
        memset ((char *)&(cli->myaddr_in), 0, sizeof(struct sockaddr_in));
        memset ((char *)&(cli->sa), 0, sizeof(struct sockaddr_in));

  }
  int Init(ClientUDP *cli, char* servername, int portnum)
  {
  	cli->hp = gethostbyname(servername);
	if (cli->hp == NULL) {
                fprintf(stderr, "%s not found in /etc/hosts\n", servername);
                return 1;
        }
	
	/*  build socket structure  */

	memcpy((char *)&cli->sa.sin_addr,(char *)cli->hp->h_addr, cli->hp->h_length);

	cli->sa.sin_family = cli->hp->h_addrtype;
	cli->sa.sin_port = htons(portnum);

	/*  allocate an open socket */

	//soxdes = socket(AF_INET, SOCK_STREAM,IPPROTO_TCP);
	cli->Soc = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (cli->Soc == -1) {
                perror("ClientUDP");
                fprintf(stderr, "%s: unable to create socket\n", "ClientUDP");
		return 2;
        }
	
	#ifdef _DEBUG_
	else
      		printf("socket created\n");
	#endif

	/*  connect to the server */
	if (connect(cli->Soc, (struct sockaddr *)&cli->sa, sizeof cli->sa) < 0)
      	{
      		printf( "connect error - %i\n",errno);
	     	return 3;
      	}	

	printf( "connected\n");
	return 0;
  }

  int Send(ClientUDP *cli, char * msg, int msize)
  {
	int res;
	res = send (cli->Soc, msg, msize, 0);
	// (const sockaddr*) &sa, sizeof(struct sockaddr_in));
	if ( res==-1 )
	{
//                perror("ClientUDP::Send");
//                fprintf(stderr, "%s: unable to send request\n", "ClientUDP::Send");
        }
	return res;
  }

 
//		 S E R V . U D P

typedef struct _ServerUDP
{
    int Soc;                    /* socket descriptor */

    #define BUFFERSIZE  1024  /* max size of packets to be received */
    int cc;                   /* contains the number of bytes read */
    char buffer[BUFFERSIZE];  /* buffer for packets to be read into */

    struct hostent *hp;       /* pointer to info of requested host */
    struct servent *sp;       /* pointer to service information */

    struct sockaddr_in myaddr_in;    /* for local socket address */
    struct sockaddr_in clientaddr_in;/* for client's socket address */
    struct in_addr reqaddr;         /* for requested host's address */
} ServerUDP;

    #define ADDRNOTFOUND 0xffffffff /* return address for unfound host */
    void NewServerUDP(ServerUDP * serv)
    {
	/* clear out address structures */
   	memset ((char *)&serv->myaddr_in, 0, sizeof(struct sockaddr_in));
   	memset ((char *)&serv->clientaddr_in, 0, sizeof(struct sockaddr_in));
	/* Set up address structure for the socket. */
   	serv->myaddr_in.sin_family = AF_INET;
        /* The server should receive on the wildcard address,
         * rather than its own internet address.  This is
         * generally good practice for servers, because on
         * systems which are connected to more than one
         * network at once will be able to have one server
         * listening on all networks at once.  Even when the
         * host is connected to only one network, this is good
         * practice, because it makes the server program more
         * portable.
         */
  	serv->myaddr_in.sin_addr.s_addr = INADDR_ANY;
    };
    int Bind(ServerUDP *serv, int port)
    {
	serv->sp = malloc(sizeof(struct servent));
  	serv->myaddr_in.sin_port = htons(port);

        /* Create the socket. */
  	serv->Soc = socket (AF_INET, SOCK_DGRAM, 0);
  	if (serv->Soc == -1) {
                perror("ServerUDP");
                printf("%s: unable to create socket\n", "ServerUDP");
                return(1);
  	}
        /* Bind the server's address to the socket. */
	if (bind(serv->Soc, (struct sockaddr*)&serv->myaddr_in, sizeof(struct sockaddr_in)) == -1) 
	{
                perror("ServerUDP");
                printf("%s: unable to bind address\n", "ServerUDP");
                return(1);
  	}
	printf("fez bind\n");

               /* This will open the /etc/hosts file and keep
                * it open.  This will make accesses to it faster.
                * If the host has been configured to use the NIS
                * server or name server (BIND), it is desirable
                * not to call sethostent(1), because a STREAM
                * socket is used instead of datagrams for each
                * call to gethostbyname().
                */
        sethostent(1);
	return 0;
    };
    int Receive(ServerUDP *serv, unsigned char *buffer, int bufsize)
    {
		int cc;
		int addrlen;
		 struct sockaddr from;
                /* Note that addrlen passed as a pointer
                 * so that the recvfrom call can return
                 * the size of the returned address.
                 */
                addrlen = sizeof(struct sockaddr_in);

                /* This call will block until a new
                 * request arrives.  Then, it will
                 * return the address of the client,
                 * and a buffer containing its request.
                 * BUFFERSIZE - 1 bytes are read so that
                 * room is left at the end of the buffer
                 * for a null character.
                 */
                cc = recvfrom(serv->Soc, buffer, bufsize , 0, &from, (socklen_t *)&addrlen);
		//cc = read(serv->Soc, buffer, bufsize);
		//printf("recebeu mensagem \n");
                if ( cc == -1)
		{
			printf("Bad message\n");
		}
		//printf("  mensagem (%d bytes) = %s\n", cc, buffer);
		return cc;
    }
    void ServerClose(ServerUDP *serv)
    {
        close(serv->Soc);
    }
#endif


