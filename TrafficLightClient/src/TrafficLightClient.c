#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/iofunc.h>
#include <sys/netmgr.h>
#include <pthread.h>

#define FILE_LOCATION "/tmp/trafficLightServer.info"
//#define FILE_LOCATION "/net/serverAditya/tmp/trafficLightServer.info"
#define BUF_SIZE 100
#define MESSAGE_SIZE 2
#define N_ITERATIONS 10 // Iteration number can be customised

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
	volatile char data;
	int delay;
	enum states currentState;
}messageData;

typedef struct {
	struct _pulse header;
	char buf[BUF_SIZE];
	enum states currentState;
}replyData;

typedef struct {
	int messageNum;
	int stayAlive;
	int living;
	int serverConnectionId;
}serverParams;

typedef struct {
	messageData message;
	replyData replyMessage;
	serverIds serverIdentity;
	serverParams params;
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
	clientSide->message.currentState = state0;
	clientSide->message.delay = 1;
	clientSide->params.serverConnectionId=0;
	pthread_mutex_init(&clientSide->mutex, NULL);
	getServerInfo(&clientSide->serverIdentity);
};

void singlestep_trafficlight_statemachine_send(clientData *cd){
	char buf[MESSAGE_SIZE] = {};
	switch(cd->replyMessage.currentState){
	case state0:
		cd->message.currentState = state1;
		break;
	case state1:
		cd->message.currentState = state2;
		printf("\nPress 'n' to stop cars from North-South:\n");
		gets(buf);
		cd->message.data = buf[0];
		cd->message.delay = 2;
		break;
	case state2:
		cd->message.delay = 1;
		cd->message.currentState = state3;
		break;
	case state3:
		cd->message.currentState = state4;
		break;
	case state4:
		cd->message.currentState = state5;
		printf("\nPress 'e' to stop cars from East-West:\n");
		gets(buf);
		cd->message.data = buf[0];
		cd->message.delay = 2;
		break;
	case state5:
		cd->message.delay = 1;
		cd->message.currentState = state6;
		break;
	case state6:
		cd->message.delay = 1;
		cd->message.currentState = state1;
		break;
	}
}

// client thread
void *client(void *data){
	clientData *td = (clientData*) data;

	printf("THREAD STATUS: Attempting to connect to server with process ID of %d\n", td->serverIdentity.serverProcessId);
	printf("On Channel ID: %d\n", td->serverIdentity.serverChannelId);

	pthread_mutex_lock(&td->mutex);
		td->params.serverConnectionId = ConnectAttach(ND_LOCAL_NODE, td->serverIdentity.serverProcessId, td->serverIdentity.serverChannelId, _NTO_SIDE_CHANNEL, 0);
		printf("ConnectionId %d\n", td->params.serverConnectionId);
		if(td->params.serverConnectionId  == -1){
			printf("ERROR: Unable to connect to the server!\n");
			pthread_exit(EXIT_FAILURE);
		}
	pthread_mutex_unlock(&td->mutex);

	printf("Client connected to the server\n");
	return 0;
}

int main(int argc, char *argv[]) {
	clientData clientSide;
	clientDataInit(&clientSide);
	pthread_t clientThread;
	int index = 0;
	pthread_create(&clientThread, NULL, client, &clientSide);
	pthread_join(clientThread, NULL);

	// After all connections have been made => start iteration of tasks
	for(index = 0; index < N_ITERATIONS; index++){
		printf("\nClient (ID: %d): Sending data packet with state number %d\n", clientSide.message.clientId, clientSide.message.currentState);
		if(MsgSend(clientSide.params.serverConnectionId , &clientSide.message, sizeof(clientSide.message),
				&clientSide.replyMessage, sizeof(clientSide.replyMessage)) == -1){
			printf("ERROR: Data has not been sent successfully or there is no reply being sent back from server\n");
			break;
		}else{
			printf("----> Server side replying with: '%s'", clientSide.replyMessage.buf); // print reply messages when client receive it from server
		}
		singlestep_trafficlight_statemachine_send(&clientSide); // Perform direct data mutation of the task in state machine based on replies
	}
	printf("\nNotify server to close connection\n");
	ConnectDetach(clientSide.params.serverConnectionId);
	printf("(MAIN) THREAD STATUS: Main thread terminating\n");

	return EXIT_SUCCESS;
}
