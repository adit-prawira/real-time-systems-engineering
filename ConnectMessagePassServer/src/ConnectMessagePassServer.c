#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/iofunc.h>
#include <unistd.h>

#define BUF_SIZE 100

enum states{
	state0, state1, state2, state3, state4, state5, state6
};

typedef struct {
	struct _pulse header;
	int clientId;
	int data;
}messageData; // message data struct as type definition

typedef struct {
	struct _pulse header;
	char buf[BUF_SIZE];
}reply;

typedef struct{
	int serverProcessId;
	int serverChannelId;
}serverIds;

int server();

int main(int argc, char *argv[]) {
	printf("Server side running...\n");
	int returnValue = server();
	printf("Server side terminating...\n");
	return returnValue;
}

int server(){
	int serverProcessID = 0, channelID = 0;

	serverIds idToStore;

	serverProcessID = getpid();
	channelID = ChannelCreate(_NTO_CHF_DISCONNECT); // the flag will be utilise to allow detach


	if(channelID == -1){
		// the case where channel ID is not defined/found
		printf("ERROR: Failed to create communication channel\n");
		return EXIT_FAILURE;
	}

	printf("Server side listening to Client on:\n");
	printf("------> Process ID: %d\n", serverProcessID);
	printf("------> Channel ID: %d\n", channelID);

	FILE *file;
	file = fopen("/tmp/myServer.info", "w+");
	if(file != NULL){
		idToStore.serverProcessId = serverProcessID;
		idToStore.serverChannelId = channelID;
		int dataNum = fwrite(&idToStore, sizeof(serverIds), 1, file);
		printf("Successfully write %d data\n", dataNum);
	}else{
		printf("Error opening file");
	}
	fclose(file);

	messageData message;
	int receiveId, messageNum = 0;
	int stayAlive = 0, living = 0;

	reply replyMessage;
	replyMessage.header.type = 0x01;
	replyMessage.header.subtype = 0x00;
	living = 1;
	while(living){
		receiveId = MsgReceive(channelID, &message, sizeof(message), NULL);
		if(receiveId == -1){
			// if the server does not receive message from client exit early
			printf("ERROR: Failed to receive message\n");
			break;
		}

		// Pulse
		if(receiveId == 0){
			switch(message.header.code){
			case _PULSE_CODE_DISCONNECT:
				if(stayAlive == 0){
					ConnectDetach(message.header.scoid); // pass server connection ID to ConnectDetach
					printf("\nServer is requested to detach from ClientID: %d\n", message.clientId);
					living = 0;
					continue;
				}else{
					printf("\nERROR: Server is requested to detach from ClientID %d but rejected\n", message.clientId);
				}
				break;
			case _PULSE_CODE_UNBLOCK:
				printf("\nServer received _PULSE_CODE_UNBLOCK(code: %d) after %d messages\n", _PULSE_CODE_UNBLOCK, messageNum);
				break;
			case _PULSE_CODE_COIDDEATH:
				printf("\nServer received _PULSE_CODE_COIDDEATH(code: %d) after %d messages\n", _PULSE_CODE_COIDDEATH, messageNum);
				break;
			case _PULSE_CODE_THREADDEATH:
				printf("\nServer received _PULSE_CODE_THREADDEATH(code: %d) after %d messages\n", _PULSE_CODE_THREADDEATH, messageNum);
				break;
			default:
				printf("\nServer received unrecognized PULSE CODE entity after %d messages\n", messageNum);
				break;
			}
			continue;
		}else if(receiveId > 0){
			messageNum++;
			if(message.header.type == _IO_CONNECT){
				MsgReply(receiveId, EOK, NULL, 0);
				printf("\nGNS Service is running...\n");
				continue;
			}

			if(message.header.type > _IO_BASE && message.header.type <= _IO_MAX){
				MsgError(receiveId, ENOSYS);
				printf("ERROR: Server received, and rejected unrecognized IO message\n");
				continue;
			}

			sprintf(replyMessage.buf, "Message %d received", messageNum);
			printf("\nServer data packet from ClientID(%d) with value of %d", message.clientId, message.data);
			fflush(stdout);
			sleep(1);
			printf("\n-----> replying with: '%s'\n", replyMessage.buf);
			MsgReply(receiveId, EOK, &replyMessage, sizeof(replyMessage));
		}else{
			printf("\nERROR: Server received unrecognized entity, but could not handle it properly\n");
		}
	}
	printf("\nServer is requested to Destroy connection Channel\n");
	ChannelDestroy(channelID);

	return EXIT_SUCCESS;
}
