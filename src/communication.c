//
//  communication.c
//  communication
//
//  Created by averello on 27/3/13.
//  Copyright (c) 2013 George Boumis. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <pthread.h>
#include <errno.h>
#include <wchar.h>

/* XDR */
#include <rpc/types.h>
#include <rpc/xdr.h>

#include <communication.h>

#include <openssl/sha.h>



#if DEBUG
#define DEBUGF(format,...) printf(format, __VA_ARGS__)
#else
#define DEBUGF(format,...)
#endif

struct _communicationDescriptionContext {
	int socket;
	int communicationDescriptor;
	xdrproc_t converterf;
	XDR xdrs;
	char digest[SHA_DIGEST_LENGTH];
};
typedef struct _communicationDescriptionContext CMCommunicationDescriptionContext;

struct _communicationInternalData {
	CMCommunicationDescriptionContext *CMCommunicationContexts;
	unsigned int capacity;
	pthread_mutex_t mutex;
};
typedef struct _communicationInternalData CMCommunicationInternalData;

static CMCommunicationInternalData CMInternalData = { NULL, 0, PTHREAD_MUTEX_INITIALIZER };

#if defined(__APPLE__) && defined(__MACH__)
static int readit(void *handler, void *buffer, int nbytes);
static int writeit(void *handler, void *buffer, int nbytes);
#else
static int readit(char *handler, char *buffer, int nbytes);
static int writeit(char *handler, char *buffer, int nbytes);
#endif

bool_t xdr_digest(XDR *xdrs, CMCommunicationDescriptionContext *context);

//static const char *const _CErrors[] = {
//	"(null)",
//};

/******************************/

int CMInitCommunicationWithSocketAndConverter(int socket, xdrproc_t converterf) {
	if ( socket < 0 || NULL == converterf ) return errno = EINVAL, -1;

	unsigned int index = -1U;
	
	pthread_mutex_t *mutex = &CMInternalData.mutex;
	pthread_mutex_lock(mutex);
	CMCommunicationDescriptionContext *communicationContexts = CMInternalData.CMCommunicationContexts;
	pthread_cleanup_push( (void (*)())pthread_mutex_lock, mutex);
	
	
	if ( NULL == communicationContexts ) { /* On initialization */
		/* alloc */
		const unsigned int initialSize = 1<<4; /* 16 */
		communicationContexts = calloc((size_t)initialSize, sizeof(CMCommunicationDescriptionContext));
		if ( communicationContexts == NULL ) return pthread_mutex_unlock(mutex), errno = ENOMEM, -1;
		CMInternalData.CMCommunicationContexts = communicationContexts;
		CMInternalData.capacity = initialSize;
		/* initialize at -1 */
		for (unsigned int i=0; i<CMInternalData.capacity; communicationContexts[i].socket = -1, i++);
	}
	for (unsigned int i=0; i<CMInternalData.capacity; i++)
		if (communicationContexts[i].socket == -1) {
			index = i;
			break;
		}
	
	if (index >= CMInternalData.capacity) { /* No space left => should make some space */
		/* If doubling the size of the array will exceed limits then */
		if (UINT_MAX - CMInternalData.capacity >= CMInternalData.capacity) return pthread_mutex_unlock(mutex), errno = ENOMEM, -1;
		unsigned int newCapacity = CMInternalData.capacity<<1; /* double the size */
		CMCommunicationDescriptionContext *newCommunicationContexts = realloc(communicationContexts, newCapacity * sizeof(CMCommunicationDescriptionContext));
		if (NULL == newCommunicationContexts) return pthread_mutex_unlock(mutex), errno = ENOMEM, -1;
		for (unsigned int i=CMInternalData.capacity; i<newCapacity; newCommunicationContexts[i].socket=-1, i++);
		CMInternalData.CMCommunicationContexts = communicationContexts = newCommunicationContexts;
		CMInternalData.capacity = newCapacity;
	}
	
	/* Initialization */
	CMCommunicationDescriptionContext *context = &(communicationContexts[index]);
	context->socket = socket;
	context->converterf = converterf;
	context->communicationDescriptor = (int)index;
#if defined(__APPLE__) && defined(__MACH__)
	xdrrec_create( &(context->xdrs), 0, 0, (void *)context, readit, writeit);
#else
	xdrrec_create( &(context->xdrs), 0, 0, (void *)context, (int (*)(char*,char*,int))readit, (int (*)(char*,char*,int))writeit);
#endif
	
	pthread_cleanup_pop(0);
	pthread_mutex_unlock(mutex);
	
	return (int)index;
}

void CMFinishCommunicationWithCommunicationDescriptor(int communicationDescriptor) {
	if ( CMInternalData.capacity < UINT_MAX && (unsigned int)communicationDescriptor >= CMInternalData.capacity) { errno = EINVAL; return; }
	
	pthread_mutex_t *mutex = &CMInternalData.mutex;
	pthread_mutex_lock(mutex);
	
	CMCommunicationDescriptionContext *communicationContexts = CMInternalData.CMCommunicationContexts;
	CMCommunicationDescriptionContext *context = &(communicationContexts[communicationDescriptor]);
	if (context->communicationDescriptor != communicationDescriptor) { return; };
	context->socket = -1;
	context->communicationDescriptor = -1;
	xdr_destroy(&context->xdrs);
	
	/* If by any chance the array is empty free it */
	unsigned int i;
	for (i=0; i<CMInternalData.capacity; i++)
		if ( communicationContexts[i].socket != -1 )
			break;
	if ( i >= CMInternalData.capacity )
		free(communicationContexts), CMInternalData.CMCommunicationContexts = NULL, CMInternalData.capacity = 0;
	
	pthread_mutex_unlock(mutex);
}

int CMSendMessage(int communicationDescriptor, void *message) {
	if ( message == NULL ) return  errno = EINVAL, -1;
	if ( CMInternalData.capacity < UINT_MAX && (unsigned int)communicationDescriptor >= CMInternalData.capacity) return errno = EINVAL, -1;
	
	CMCommunicationDescriptionContext *communicationContexts = CMInternalData.CMCommunicationContexts;
	CMCommunicationDescriptionContext *context = &(communicationContexts[communicationDescriptor]);
	
	if (context->communicationDescriptor != communicationDescriptor) return errno = EINVAL, -1;
	if (context->socket == -1) return errno = EINVAL, -1;
	if (context->converterf == NULL) return errno = EINVAL, -1;

//	SHA1((const unsigned char *)line->content, size, line->id);
	
	XDR *xdrs = &(context->xdrs);
	xdrs->x_op = XDR_ENCODE;
	bool_t result = context->converterf(xdrs, message, 0);
	if ( result == (TRUE) )
		return (xdrrec_endofrecord(xdrs, (TRUE) ) == 1) ? 0 : (errno = EINVAL, -1);
	return -1;
}

int CMReceiveMessage(int communicationDescriptor, void *message) {
	int retval = -1;
	if ( message == NULL ) return  errno = EINVAL, -1;
	if ( CMInternalData.capacity < UINT_MAX && (unsigned int)communicationDescriptor >= CMInternalData.capacity) return errno = EINVAL, -1;
	
	
	CMCommunicationDescriptionContext *communicationContexts = CMInternalData.CMCommunicationContexts;
	CMCommunicationDescriptionContext *context = &(communicationContexts[communicationDescriptor]);
	
	if (context->communicationDescriptor != communicationDescriptor) return errno = EINVAL, -1;
	if (context->socket == -1) return errno = EINVAL, -1;
	if (context->converterf == NULL) return errno = EINVAL, -1;

	XDR *xdrs = &(context->xdrs);
	xdrs->x_op = XDR_DECODE;
	if ( xdrrec_skiprecord(xdrs) == (FALSE) ) return retval;

	if ( context->converterf(xdrs, message, 0)  == (TRUE) )
		retval = 0;
	return retval;
}

void CMDestroyMessage(void *message, xdrproc_t converter) {
	if (message == NULL) { errno = EINVAL; return; }
	xdr_free(converter, message);
}


void CMSetConverterF(int communicationDescriptor, xdrproc_t converterf) {
	if ( NULL == converterf ) { errno = EINVAL; return; }
	if ( CMInternalData.capacity < UINT_MAX && (unsigned int)communicationDescriptor >= CMInternalData.capacity) { errno = EINVAL; return; }
	
	pthread_mutex_t *mutex = &CMInternalData.mutex;
	pthread_mutex_lock(mutex);
	
	CMCommunicationDescriptionContext *communicationContexts = CMInternalData.CMCommunicationContexts;
	CMCommunicationDescriptionContext *context = &(communicationContexts[communicationDescriptor]);
	context->converterf = converterf;
	
	pthread_mutex_unlock(mutex);
}

xdrproc_t CMGetConverterF(int communicationDescriptor) {
	if ( CMInternalData.capacity < UINT_MAX && (unsigned int)communicationDescriptor >= CMInternalData.capacity) return errno = EINVAL, (xdrproc_t)NULL;
	
	pthread_mutex_t *mutex = &CMInternalData.mutex;
	pthread_mutex_lock(mutex);
	
	CMCommunicationDescriptionContext *communicationContexts = CMInternalData.CMCommunicationContexts;
	CMCommunicationDescriptionContext *context = &(communicationContexts[communicationDescriptor]);
	xdrproc_t converter = context->converterf;
	
	pthread_mutex_unlock(mutex);
	
	return converter;
}

int CMCopyDigestToHexString(char **restrict dest, const unsigned char *restrict src) {
	if ( dest == NULL || src == NULL ) return errno = EINVAL, -1;
	
	char *restrict out = calloc(SHA_DIGEST_LENGTH*2+1, sizeof(char));
	if ( out == NULL ) return errno = ENOMEM -1;
	
	*dest = out;
	return CMConvertDigestToHexString(out, src);
}

int CMConvertDigestToHexString(char *restrict dest, const unsigned char *restrict src) {
	if ( dest == NULL || src == NULL ) return errno = EINVAL, -1;

	memset(dest, 0, SHA_DIGEST_LENGTH*2+1);
	for (int i=0; i<SHA_DIGEST_LENGTH; i++)
		sprintf( dest+(i<<1), "%02x", src[i] );
	return 0;
}

#if defined(__APPLE__) && defined(__MACH__)
static int readit(void *handler, void *buffer, int nbytes) {
#else
static int readit(char *handler, char *buffer, int nbytes) {
#endif
	CMCommunicationDescriptionContext *context = handler;
	int bytes =
	(int)read(context->socket, buffer, (size_t)nbytes);
#if DEBUG
	int err = errno;
	if (bytes < 0) perror("readit/read()"), printf("[%s] the errno is %d\n", __FUNCTION__, err);
	printf("[%s] int:%d\n", __FUNCTION__, bytes);
#endif
	return (bytes==0) ? -1 : bytes;
}

#if defined(__APPLE__) && defined(__MACH__)
	static int writeit(void *handler, void *buffer, int nbytes) {
#else
	static int writeit(char *handler, char *buffer, int nbytes) {
#endif
	CMCommunicationDescriptionContext *context = handler;
	int bytes = (int)write(context->socket, buffer, (size_t)nbytes);
#if DEBUG
	printf("[%s] int:%d\n", __FUNCTION__, nbytes);
	int err = errno;
	if (bytes < 0) perror("writeit/read()"), printf("[%s] the errno is %d\n", __FUNCTION__, err);
#endif
	return bytes;
}

bool_t xdr_digest(XDR *xdrs, CMCommunicationDescriptionContext *context) {
	if (xdrs == NULL || context == NULL) return errno = EINVAL, FALSE;
	return xdr_opaque(xdrs, context->digest, SHA_DIGEST_LENGTH);
}

//const char *CMCommunicationErrorString(int errorCode) {
////	if ( errorCode < 1 || errorCode > _CMCommandErrorCount ) return errno = EINVAL, (const char *)NULL;
//	return _CErrors[errorCode];
//}

