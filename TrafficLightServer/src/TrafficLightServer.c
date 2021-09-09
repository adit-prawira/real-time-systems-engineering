#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/iofunc.h>
#include <pthread.h>
#include <unistd.h>

#define MESSAGE_SIZE 2
#define BUF_SIZE 100
#define FILE_LOCATION "/tmp/trafficLightServer.info"
//#define FILE_LOCATION "/net/clientAditya/tmp/trafficLightServer.info"

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
	volatile char messageChar;
}messageData;

typedef struct {
	struct _pulse header;
	char buf[BUF_SIZE];
}replyData;
typedef struct {
	int receiveId;
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
	pthread_rwlock_t dataSource;
}sensorData;

// This function will perform direct mutation to serverIds
// and store the current process pid number and channel id during channel creation to the typedef
// and will store it into a file in the location described by FILE_LOCATION
void writeServerInfo(serverIds *ids){
	FILE *file;
	file = fopen(FILE_LOCATION, "w+");
	if(file != NULL){
		ids->serverProcessId = getpid();
		ids->serverChannelId = ChannelCreate(_NTO_CHF_DISCONNECT);
		int dataNum = fwrite(ids, sizeof(serverIds), 1, file);
		printf("Successfully store %d data to server info file\n", dataNum); // logs success file writing process
	}else{
		ids->serverChannelId = 0;
		ids->serverProcessId = 0;
		printf("ERROR: Cannot create server info file\n");
	}
	fclose(file);
}

// helper function to initialize the sensorData typedef
void sensorDataInit(sensorData *sensor){
	sensor->replyMessage.header.type = 0x01;
	sensor->replyMessage.header.subtype = 0x00;
	sensor->params.stayAlive = 0;
	sensor->params.living = 0;
	sensor->params.messageNum = 0;
	sensor->currentState = state0;
	pthread_rwlock_init(&sensor->dataSource, NULL);
	writeServerInfo(&sensor->serverIdentity);
};

void serverStateMachine(struct _pulse header, serverParams *params, int clientId){
	switch (header.code){
	case _PULSE_CODE_DISCONNECT:
		ConnectDetach(header.scoid);

	}
}
// server thread
void *server(void *data){
	sensorData *td = (sensorData *)data;
	printf("\nTHREAD STATUS: Server thread starting\n");
	printf("SERVER SIDE STATUS: Server side listening to client...\n");

	// Terminate thread early if server channel id is not exist
	if(td->serverIdentity.serverChannelId == -1){
		printf("ERROR: Unable to create communication to channel\n");
		printf("\nTHREAD STATUS: Server thread terminating\n");
		pthread_exit(EXIT_FAILURE);
	}
	printf("---> Server Process ID: %d\n", td->serverIdentity.serverProcessId);
	printf("---> Server Channel ID: %d\n", td->serverIdentity.serverChannelId);
	td->params.living = 1;
	while(td->params.living){
		td->params.receiveId = MsgReceive(td->serverIdentity.serverChannelId, &td->message,
				sizeof(td->message), NULL);
		if(td->params.receiveId == -1){
			printf("ERROR: Failed to receive message\n");
			break;
		}
	}
	printf("\nTHREAD STATUS: Server thread terminating\n");
	return 0;
}

int main(int argc, char *argv[]) {
	pthread_t serverThread;
	sensorData sensor;
	sensorDataInit(&sensor);
	pthread_create(&serverThread, NULL, server, &sensor);
	pthread_join(serverThread, NULL);
	printf("(MAIN) THREAD STATUS: Main thread terminating\n");
	return EXIT_SUCCESS;
}
