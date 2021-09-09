#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/iofunc.h>
#include <sys/netmgr.h>
#include <pthread.h>

#define FILE_LOCATION "/tmp/trafficLightServer.info"
//#define FILE_LOCATION "/net/serverAditya/tmp/trafficLightServer.info"
#define BUF_SIZE 100

enum states {
	state0, state1, state2, state3, state4, state5, state6
};

typedef struct {
	int serverProcessId;
	int serverChannelId;
}serverIds;

typedef struct {
	struct _pulse header;
	int clientId;
	int data;
//	volatile char messageChar;
}messageData;

typedef struct {
	struct _pulse header;
	char buf[BUF_SIZE];
}replyData;

typedef struct {
	int messageNum;
	int stayAlive;
	int living;
}serverParams;

typedef struct {
	messageData message;
	replyData replyMessage;
	serverIds serverIdentity;
	serverParams params;
	enum states currentState;
	pthread_mutex_t mutex;
}clientData;


// return the size of data that has been stored in the read file
// if it fails to read then return 0
size_t read_data( FILE *fp, serverIds *p )
{
    return( fread( p, sizeof( serverIds ), 1, fp ) );
}

// This function will perform direct mutation to accessIds
// and store serverChannelId, and serverProcessId, where those two properties were created by server-side
void getServerInfo(serverIds *accessIds){
	FILE *file;
	serverIds ids;
	file = fopen(FILE_LOCATION, "r");
	if(file != NULL && read_data(file, &ids)!= 0){
		accessIds->serverChannelId =ids.serverChannelId;
		accessIds->serverProcessId =ids.serverProcessId;
		printf("Successfully get Server Connection and Channel IDs from file\n"); // logs successfully file opening and read methods
	}else{
		printf("Cannot open the file\n"); // logs unsuccessful file access process
	}
	fclose(file); // close file after performing the process
}

// helper function to initialize the clientData typedef
void clientDataInit(clientData *clientSide){
	clientSide->message.header.type = 0x00;
	clientSide->message.header.subtype = 0x00;
	clientSide->message.clientId = 500;
	clientSide->params.stayAlive = 0;
	clientSide->params.living = 0;
	clientSide->params.messageNum = 0;
	clientSide->currentState = state0;
	pthread_mutex_init(&clientSide->mutex, NULL);
	getServerInfo(&clientSide->serverIdentity);
};

void *client(void *data){
	clientData *td = (clientData*) data;
	int index = 0;
	int serverConnectionId;
	printf("THREAD STATUS: Attempting to connect to server with process ID of %d\n", td->serverIdentity.serverProcessId);
	printf("On Channel ID: %d\n", td->serverIdentity.serverChannelId);

	serverConnectionId = ConnectAttach(ND_LOCAL_NODE, td->serverIdentity.serverProcessId, td->serverIdentity.serverChannelId, _NTO_SIDE_CHANNEL, 0);
	printf("ConnectionId %d\n", serverConnectionId);
	if(serverConnectionId == -1){
		printf("ERROR: Unable to connect to the server!\n");
		pthread_exit(EXIT_FAILURE);
	}
	printf("Client connected to the server\n");
	for(index = 0; index < 5; index++){
		pthread_mutex_lock(&td->mutex);
			td->message.data = 10+index;
			printf("\nClient (ID: %d): Sending data packet with integer of %d\n", td->message.clientId, td->message.data);
			if(MsgSend(serverConnectionId, &td->message, sizeof(td->message),
					&td->replyMessage, sizeof(td->replyMessage)) == -1){
				printf("ERROR: Data has not been sent successfully or there is no reply being sent back from server\n");
				break;
			}else{
				printf("----> Server side replying with: '%s'", td->replyMessage.buf);
			}
		pthread_mutex_unlock(&td->mutex);
	}
	printf("\nNotify server to close connection\n");
	ConnectDetach(serverConnectionId);
	return 0;
}
int main(int argc, char *argv[]) {
	clientData clientSide;
	clientDataInit(&clientSide);
	pthread_t clientThread;

	pthread_create(&clientThread, NULL, client, &clientSide);
	pthread_join(clientThread, NULL);
	printf("(MAIN) THREAD STATUS: Main thread terminating\n");

	return EXIT_SUCCESS;
}
