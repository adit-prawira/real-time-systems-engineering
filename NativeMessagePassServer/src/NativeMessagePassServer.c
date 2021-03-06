#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/dispatch.h>
#define ATTACH_POINT "adityaNet"

#define BUF_SIZE 100

// create private custom union sigval
typedef union{
	union{
		_Uint32t sival_int;
		void *sival_ptr; // will have different size in 32-bit and 64-bit
	};
	_Uint32t dummy[4]; // create dummy variable to create space
}_customSigval;

// replace the standtard (typedef struct _pulse msg_header_t)
typedef struct _CustomPulse{
	_Uint16t type;
	_Uint16t subtype;
	_Int8t code;
	_Uint8t zero[3]; // same padding used in standard _pulse struct
	_customSigval value;
	_Uint8t zero2[2]; // added extra padding for ensuring alignment access
	_Int32t serverConnectionId;
}msg_header_t;

typedef struct {
	msg_header_t header;
	int clientId; // unique id from client
	int data;
}msgData;

typedef struct {
	msg_header_t header; // use the one that has been built with _CustomPulse sturcture
	char buf[BUF_SIZE]; // Message that will be send back to send back to other thread
}reply;


// prototype
int server();
int main(int argc, char *argv[]) {
	puts("Server side running...\n");
	int ret = 0;
	ret = server();
	puts("Server side terminating...\n");
	return EXIT_SUCCESS;
}

int server(){
	printf("_customSigval size = %d\n", sizeof(_customSigval));
	printf("header size = %d\n", sizeof(msg_header_t));
	printf("msgData size = %d\n", sizeof(msgData));
	printf("reply size = %d\n", sizeof(reply));

	name_attach_t *attach;
	msgData message;
	reply replyMessage; // replayMessage structure that will send message back to client from server
	replyMessage.header.type = 0x01; // number to help client process reply message
	replyMessage.header.subtype = 0x00; // number to help client process reply message

	// Create a global name (/dev/hostname/local/...)
	if((attach = name_attach(NULL, ATTACH_POINT, 0)) == NULL){
		printf("\nFailed to name_attach on ATTACH_POINT: %s\n", ATTACH_POINT);
		printf("\nAnother server may run the same ATTACH_POINT name or gns service has not yet started\n");
		return EXIT_FAILURE;
	}

	printf("Server Listening for client on ATTACH_POINT: %s\n", ATTACH_POINT);

	// Set up server loop
	int receiveId = 0, messageNum = 0;
	int stayAlive = 0, living = 0;
	living = 1;
	// server loop will always listening as long as it is living (Duh)
	while(living){
		// Perform MsgReceive with the channel id (chid)
		receiveId = MsgReceive(attach->chid, &message, sizeof(message), NULL);
		if(receiveId == -1){ // if receiveId doesn't exist break and terminate early
			printf("\n Failed to MsgReceive\n");
			break;
		}

		// Check the condition whether a message or pulse was received

		/*********************************** CONDITION FOR PULSE (receivedId is 0) ******************************************/
		if(receiveId == 0){
			printf("\nServer receive a pulse from ClientID: %d...\n", message.clientId);
			printf("Pulse receive: %d\n", message.header.code);

			// state machine:
			switch(message.header.code){

			case _PULSE_CODE_DISCONNECT: // no value is being defined
				printf("Pulse Case Status: %d\n", _PULSE_CODE_DISCONNECT);
				if(stayAlive == 0){
					// a client disconnect all its connections by running name_close() for each name_open()
					// or thread terminated
					ConnectDetach(message.header.serverConnectionId); // detach by passing server connection id
					printf("\nServer Received: Detach ClientID: %d...\n", message.clientId);
					continue; // break the loop;
				}else{
					printf("\nServer Received but Rejected: Detach ClientID: %d...\n", message.clientId);
				}
				break;
			case _PULSE_CODE_UNBLOCK: // receiveId associated with the blocking message
				// GIves the option to reply back to the client now or later after being hit by a signal or a time out
				printf("\nServer Received: _PULSE_CODE_UNBLOCK after %d messages\n", messageNum);
				break;
			case _PULSE_CODE_COIDDEATH: // Received an ID of a connection that was attached to a destroyed channel
				printf("\nServer Received: _PULSE_CODE_COIDDEATH after %d messages\n", messageNum);
				break;
			case _PULSE_CODE_THREADDEATH: // Received an ID of a thread that just died
				printf("\nServer Received: _PULSE_CODE_THREADDEATH after %d messages\n", messageNum);
				break;
			default:
				printf("\nServer Received: Unrecognized pulse detected after %d messages\n", messageNum);
				break;
			}
		}
		/*********************************** CONDITION FOR MESSAGE (receivedId is larger than 0) ******************************************/
		if(receiveId > 0){
			// when the condition is met => a message has been received => add messageNum
			messageNum++;

			// The case where Global Name Service (GNS Server) is running
			// Then, name_open() send a connect message => Server must EOK it. (Approve it if you will)
			if(message.header.type == _IO_CONNECT){
				MsgReply(receiveId, EOK, NULL, 0); // reply with EOK (a constant that means no error)
				printf("\nClient Send Message: GNS Server is running...\n");
				printf("\n----------> Replying with: EOK('no error')\n");
				messageNum--; // reduce number of messages because it has been replied
				continue; // go back to the start if the loop
			}

			// receiving some other IO => reject it
			if(message.header.type > _IO_BASE && message.header.type <= _IO_MAX){
				MsgError(receiveId, ENOSYS); // Send error of Function isn't implemented (ENOSYS)
				printf("\nServer Received: Other IO Message and rejected it...\n");
				continue; // go back to the start of the loop
			}

			// At this point a message is received
			sprintf(replyMessage.buf, "Message number %d received", messageNum);
			printf("Server received data packet with value of %d from client (ID: %d)\n", message.data, message.clientId);
			fflush(stdout);
			sleep(1);
			printf("---------> replying with: '%s'\n", replyMessage.buf);
			MsgReply(receiveId, EOK, &replyMessage, sizeof(replyMessage)); // give back reply to client with no error
		}else{
			printf("\nError: Server received unrecognized entity, but could not handle it correctly\n");
		}

	}
	// Remove the attach point name form the file system (e.g. /dev/name/local/<myname>)
	name_detach(attach,0);
	return EXIT_SUCCESS;
}
