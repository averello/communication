//
//  testCommunication.c
//  communication
//
//  Created by averello on 29/3/13.
//  Copyright (c) 2013 George Boumis. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <communication.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>

typedef struct _message {
	int type;
	char *string;
} CMMessage;

int InitConnexion(const char *hostname, int port);

bool_t xdr_message(XDR *xdrs, const void *message);

int main (int argc, char ** argv) {
	char idString[SHA_DIGEST_LENGTH*2+1];
	const char *string = "hello biatch";
	CMMessage *message = calloc(1, sizeof(CMMessage));
	if (  message == NULL ) fprintf(stderr, "can't allocate message\n"), exit(EXIT_FAILURE);
	
	
	
	message->type = 1;
	message->string = (char *)string;
	CMConvertDigestToHexString(idString, (const unsigned char *)string);
	
	printf("message:%p, type:%d, string:\"%s\" hex:\"%s\"\n", (void *)message, message->type, message->string, idString);
	
	int sockServer;
	int sockServerClient = -1;
	int sockClient;
	
	{
		struct sockaddr_in addr;
		
		
		// Init à 0 de la struct
		memset(&addr, 0, sizeof(addr));
		addr.sin_port = htons(12345);
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
		
		if ( (sockServer = socket(AF_INET, SOCK_STREAM, 0)) < 0)
			perror("socket"),
			free(message),
			exit(EXIT_FAILURE);
		
		
		int reuse = 1;
		setsockopt(sockServer, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
		
		if ( bind(sockServer, (const struct sockaddr *) &addr, sizeof(addr)) != 0 )
			perror("bind"),
			free(message),
			close(sockServer),
			exit(EXIT_FAILURE);
		
		if ( listen(sockServer, 5) != 0 )
			perror("listen"),
			free(message),
			close(sockServer),
			exit(EXIT_FAILURE);

		puts("** will sleep");
		sleep(2);
		puts("** woke up");
		
		sockClient = InitConnexion("localhost", 12345);
		if ( sockClient == -1 )
			perror("InitConnexion"), 
			free(message),
			close(sockServer),
			exit(EXIT_FAILURE);
		
		fd_set fdset;
		FD_ZERO(&fdset);
		FD_SET(sockServer, &fdset);
		struct timeval tv;
		tv.tv_sec = 2;
		tv.tv_usec = 0;
		int retval = select(sockServer+1, &fdset, NULL, NULL, &tv);
		if ( retval < 0 )
			perror("select()"), free(message), close(sockServer), exit(EXIT_FAILURE);
		else if ( retval == 0 ) {
			if ( connect(sockClient, (const struct sockaddr *)&addr, sizeof(addr)) == -1 )
				perror("connect 2"),
					free(message),
					close(sockServer),
					exit(EXIT_FAILURE);
		}
		sockServerClient = accept(sockServer, NULL, NULL);
	}

	int descClient = CMInitCommunicationWithSocketAndConverter(sockClient, (xdrproc_t)xdr_message);
	int descServer = CMInitCommunicationWithSocketAndConverter(sockServerClient, (xdrproc_t)xdr_message);
	
	assert(CMGetConverterF(descClient) == (xdrproc_t)xdr_message);

	CMSendMessage(descClient, message);

	message->type = -1;
	message->string = NULL;
	CMReceiveMessage(descServer, message);

	
	CMConvertDigestToHexString(idString, (const unsigned char *)message->string);
	printf("message:%p, type:%d, string:\"%s\" hex:\"%s\"\n", (void *)message, message->type, message->string, idString);
	
	CMDestroyMessage(message, (xdrproc_t)xdr_message);
		
	CMFinishCommunicationWithCommunicationDescriptor(descClient);
	CMFinishCommunicationWithCommunicationDescriptor(descServer);

	{
		shutdown(sockServer, SHUT_RDWR),
		shutdown(sockClient, SHUT_RDWR),
		close(sockServer),
		close(sockClient);
	}

	return EXIT_SUCCESS;
}


int InitConnexion(const char *hostname, int port) {
	struct addrinfo hints, *res, *res0;
	int error;
	int s;
	char *cause = NULL, stringPort[10];
	sprintf(stringPort, "%d",  port);

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_INET;
	hints.ai_socktype = SOCK_STREAM;
	error = getaddrinfo(hostname, stringPort, &hints, &res0);
	if (error) {
		fprintf(stderr, "%s\n", gai_strerror(error));
		return -1;
	}
	s = -1;
	for (res = res0; res; res = res->ai_next) {
		s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (s < 0) {
			cause = "socket";
			continue;
		}

		if (connect(s, res->ai_addr, res->ai_addrlen) < 0) {
			cause = "connect";
			close(s);
			s = -1;
			continue;
		}

		break;  /* okay we got one */
	}
	freeaddrinfo(res0);
	if (s < 0)
		fprintf(stderr, "getaddrinfo %s", cause);
	return s;
}


bool_t xdr_message(XDR *xdrs, const void *mesg) {
	CMMessage *message = (CMMessage *)mesg;
	int length = 0;
	if (message->string)
		length = (int)strlen(message->string);
	return (
			xdr_int(xdrs, &(message->type))
			&&
			xdr_int(xdrs, &length)
			&&
			xdr_string(xdrs, &(message->string), length)
			);
}





