#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/dispatch.h>

#define ATTACH_POINT "jaeMamon2801"
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
	_Int32t scoid;
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
		if(receivId == -1){ // if receiveId doesn't exist break and terminate early
			printf("\n Failed to MsgReceive\n");
			break;
		}

		// Check the condition whether a message or pulse was received

		/*********************************** CONDITION FOR PULSE ******************************************/
		if(receiveId == 0){
			printf("\nServer receive a pulse from ClientID: %d...\n", message.clientId);
			printf("Pulse receive: %d\n", message.header.code);

			// state machine:
			switch(message.header.code){
			case _PULSE_CODE_DISCONNECT:
				break;
			}
		}
	}

	return EXIT_SUCCESS;
}
