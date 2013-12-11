/*!
 *  @file communication.h
 *  @brief Communication Module.
 *  @details This module describes all data structures and functions used for communicating between two ends (client-server) using a socket.
 *
 *  Created by @author George Boumis
 *  @date 27/3/13.
 *	@version 2.0
 *  @copyright Copyright (c) 2013 George Boumis <georgios.boumis@etu.upmc.fr>. All rights reserved.
 * 
 *  @defgroup communication Communication Module
*/

#ifndef communication_communication_h
#define communication_communication_h

#include <openssl/sha.h>
#include <stdint.h>
#include <rpc/types.h>
#include <rpc/xdr.h>

#ifdef __cplusplus
extern "C" {
#endif


///*!
// *  @typedef typedef bool_t (*xdr_f)(XDR *, const void *)
// *  @brief The converter function.
// *  @ingroup communication
// *  @details The function to be used for converting custom structures with xdr.
// */
//typedef bool_t (*xdr_f)(XDR *, const void *);

/*!
 *  @fn int CMInitCommunicationWithSocketAndConverter(int socket, xdr_f converter)
 *  @brief Initializes a communication session.
 *  @ingroup communication
 *  @details Initializes a communication session with the given socket. A communication descriptor is returned. This communication descriptor *should* be used for future function calls of this module.
 *
 *  @par Thread-safety:
 *  Calling this function from multiple threads will **always** return a valid index (unless another error occurs, as described in errors section).
 *
 *  ~~~~~~~~~~~~~~~~~~~~{.c}
 *		int socket = ...;
 *		xdr_f converter = ...;
 *		int communicationDescriptor = CMInitCommunicationWithSocket(socket, converter);
 *		...
 *		CMFinishCommunicationWithCommunicationDescriptor(communicationDescriptor);
 *  ~~~~~~~~~~~~~~~~~~~~
 *
 *  @par Possible errors:
 *		- **EINVAL** The socket passed in is a negative number.
 *		- **ENOMEM** Insufficient memory is available for internal structures.
 *
 *  @param[in] socket the socket to be used for the communciation session.
 *  @param[in] converter the converter function (@ref xdr_f) that will convert the custom structure.
 *  @returns on success a non-negative integer, termed	a **communication descriptor**. On error, -1 is returned, and @a errno is set appropriately.
 */
int CMInitCommunicationWithSocketAndConverter(int socket, xdrproc_t converter);

/*!
 *  @fn void CMFinishCommunicationWithCommunicationDescriptor(int communicationDescriptor)
 *  @brief Terminates a communication session.
 *  @ingroup communication
 *  @details Deletes and terminates the  communication session associated to the specified communication descriptor. Further calls, using this communication descriptor, will fail after the call of this function.
 *
 *  @par Thread-safety:
 *  Calling this function from multiple threads will **always** terminate correctly (unless another error occurs, as described in errors section).
 *
 *  @par Possible errors:
 *		- **EINVAL** The communication descriptor is invalid.
 *
 *  @param[in] communicationDescriptor the communication descriptor.
 */
void CMFinishCommunicationWithCommunicationDescriptor(int communicationDescriptor);

/*!
 *  @fn void CMSetConverterF(int communicationDescriptor, xdr_f converterf)
 *  @brief Changes the converter function coupled to the communication descriptor.
 *  @ingroup communication
 *  @details Replaces the converter function with the one specified as argument.
 *
 *  @par Thread-safety:
 *  Calling this function from multiple threads will **always** terminate correctly (unless another error occurs, as described in errors section).
 *
 *  @par Possible errors:
 *		- **EINVAL** The communication descriptor is invalid.
 *
 *  @param[in] communicationDescriptor the communication descriptor.
 *  @param[in] converterf the converter function.
 */
void CMSetConverterF(int communicationDescriptor, xdrproc_t converterf);

/*!
 *  @fn xdr_f CMGetConverterF(int communicationDescriptor)
 *  @brief Gets the converter function coupled to the communication descriptor.
 *  @ingroup communication
 *
 *  @par Thread-safety:
 *  Calling this function from multiple threads will **always** terminate correctly (unless another error occurs, as described in errors section).
 *
 *  @par Possible errors:
 *		- **EINVAL** The communication descriptor is invalid.
 *
 *  @param[in] communicationDescriptor the communication descriptor.
 *	@returns the converter function coupled to the communicationDescriptor
 */
xdrproc_t CMGetConverterF(int communicationDescriptor);

/*!
 *  @fn int CMSendMessage(int communicationDescriptor, void *message)
 *  @brief Send a @a message.
 *  @ingroup communication
 *  @details Sends the specified @a message to the socket associated to the communcation descriptor, obtained from a successful call to @ref CMInitCommunicationWithSocketAndConverter. If the call fails then nothing is written in the socket associated to this communication descriptor. This function uses the converter method specified via @ref CMInitCommunicationWithSocketAndConverter or @ref CMSetConverterF.
 *  @warning Calling this function concurrently from multiple threads with the same communication descriptor results in **undefined behaviour**.
 *
 *  ~~~~~~~~~~~~~~~~~~~~{.c}
 *		int socket = ...;
 *		xdr_f converter = ...;
 *		int communicationDescriptor = CMInitCommunicationWithSocket(socket, converter);
 *		void *message = malloc(...);
 *		CMSendMessage(communicationDescriptor, message);
 *		free(message);
 *		CMFinishCommunicationWithCommunicationDescriptor(communicationDescriptor);
 *  ~~~~~~~~~~~~~~~~~~~~
 *
 *  @par Possible errors:
 *		- **EINVAL** The communication descriptor is invalid.
 *		- **EINVAL** The message is @a NULL or a field of the message is not valid.
 *
 *  @param[in] communicationDescriptor the communication descriptor.
 *  @param[in] message the message to be send.
 *  @returns 0 on success. On error, -1 is returned, and @a errno is set appropriately.
 */
int CMSendMessage(int communicationDescriptor, void *message);

/*!
 *  @fn int CMReceiveMessage(int communicationDescriptor, void *message)
 *  @brief Receives a @a message.
 *  @ingroup communication
 *  @details Receives a @a message from the socket associated to the the communcation descriptor, the latter obtained from a successful call to @ref CMInitCommunicationWithSocketAndConverter. Then fills the @a message passed as argument with the data received. If the call fails the @a message contents are **undefined**. This function uses the converter method specified via @ref CMInitCommunicationWithSocketAndConverter or @ref CMSetConverterF.
 *  @warning Calling this function concurrently from multiple threads with the same communication descriptor results in **undefined behaviour**.
 *
 *  ~~~~~~~~~~~~~~~~~~~~{.c}
 *		int socket = ...;
 *		xdr_f converter = ...;
 *		int communicationDescriptor = CMInitCommunicationWithSocket(socket, converter);
 *		void *message = malloc(...);
 *		if (CMReceiveMessage(communicationDescriptor, message) == 0) {
 *			// message is valid
 *		}
 *		free(message);
 *		CMFinishCommunicationWithCommunicationDescriptor(communicationDescriptor);
 *  ~~~~~~~~~~~~~~~~~~~~
 *
 *  @par Possible errors:
 *		- **EINVAL** The communication descriptor is invalid.
 *		- **EINVAL** The message is @a NULL.
 *
 *  @param[in] communicationDescriptor the communcation descriptor.
 *  @param[in,out] message the message to be filled with the received data.
 *  @returns 0 on success. On error, -1 is returned, and @a errno is set appropriately.
 */
int CMReceiveMessage(int communicationDescriptor, void *message);
	
/*!
 *  @fn void CMDestroyMessage(int communicationDescriptor, void *message)
 *  @brief Destroys memory allocated by @ref CMReceiveMessage.
 *  @ingroup communication
 *
 *  @par Possible errors:
 *		- **EINVAL** The communication descriptor is invalid.
 *
 *  @param[in] message the message that its data will be desallocated.
 */
void CMDestroyMessage(void *message, xdrproc_t converter);

/*!
 *  @fn int CMConvertDigestToHexString(char *restrict dest, unsigned char *restrict src)
 *  @brief Convenience function for tranforming a SHA1 digest to a hex string.
 *  @ingroup communication
 *  @note Prefer using the @ref copyDigestToHexString function.
 *
 *  ~~~~~~~~~~~~~~~~~~~~{.c}
 *		char hexString[SHA_DIGEST_LENGTH*2+1];
 *		char digest[SHA_DIGEST_LENGTH] = ...;
 *		CMConvertDigestToHexString(hexString, digest);
 *  ~~~~~~~~~~~~~~~~~~~~
 *
 *  @par Possible errors:
 *		- **EINVAL** The destination or the source is invalid.
 *
 *  @param[in,out] dest the destination string. The size of this array should be `SHA_DIGEST_LENGTH*2 + 1`. This argument **can not** be @a NULL.
 *  @param[in] src the digest. The size of this array should be *SHA_DIGEST_LENGTH*. This argument **can not** be @a NULL.
 *  @returns 0 on success. On error, -1 is returned, and @a errno is set appropriately.
 */
int CMConvertDigestToHexString(char *restrict dest, const unsigned char *restrict src) __attribute__((nonnull (1, 2)));

/*!
 *  @fn int CMCopyDigestToHexString(char **restrict dest, unsigned char *restrict src)
 *  @brief Convenience function for tranforming a SHA1 digest to a hex string.
 *  @ingroup communication
 *  @details After calling this function the @a *dest points to an hex string. On error @a dest is not changed.
 *
 *  ~~~~~~~~~~~~~~~~~~~~{.c}
 *		char *hexString = NULL;
 *		char digest[SHA_DIGEST_LENGTH] = ...;
 *		if ( CMCopyDigestToHexString( & hexString, digest) != -1 ) {
 *			puts(hexString);
 *			free(hexString);
 *		}
 *  ~~~~~~~~~~~~~~~~~~~~
 *
 *  @par Possible errors:
 *		- **EINVAL** The destination or the source is invalid.
 *		- **ENOMEM** Insufficient memory is available.
 *
 *  @param[out] dest a holder to be filled. This argument **can not** be @a NULL.
 *  @param[in] src the digest. The size of this array should be *SHA_DIGEST_LENGTH*. This argument **can not** be @a NULL.
 *  @returns 0 on success and @a dest points to a hex string, that should be freed by a call to `free()`. On error, -1 is returned, and @a errno is set appropriately.
 */
int CMCopyDigestToHexString(char **restrict dest, const unsigned char *restrict src) __attribute__((nonnull (1, 2)));

/*!
 *  @fn const char *CMCommunicationErrorString(int errorCode);
 *  @brief Error function for @ref communication errrors.
 *  @ingroup communication
 *  @details This function returns a pointer to a string that describes the error code passed in the argument @a errorCode. This string **must not** be modified by the application.
 *
 *  @par Possible errors:
 *		- **EINVAL** The errorCode is invalid.
 *
 *  @param[in] errorCode the error code.
 *  @returns a pointer to a string that describes the error code or @a NULL if the @a errorCode is not valid.
 */
//const char *CMCommunicationErrorString(int errorCode);

#ifdef __cplusplus
}
#endif

#endif /* communication_communication_h */
