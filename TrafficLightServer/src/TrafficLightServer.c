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
//	int data;
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
	pthread_mutex_t mutex;
}sensorData;

// This function will perform direct mutation to serverIds
// and store the current process pid number and channel id during channel creation to the typedef
// and will store it into a file in the location described by FILE_LOCATION
void writeServerInfo(serverIds *ids){
	FILE *file;
	file = fopen(FILE_LOCATION, "w");
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
	sensor->replyMessage.currentState = state0;
	pthread_mutex_init(&sensor->mutex, NULL);
	writeServerInfo(&sensor->serverIdentity);
};

// A helper function that will manage the pulse state machine
// and will perform direct mutation to the server's status parameters
// if it receives request to detach from client
void pulseStateMachine(struct _pulse header, serverParams *params, int clientId){
	switch (header.code){
	case _PULSE_CODE_DISCONNECT:
		if(params->stayAlive == 0){
			ConnectDetach(header.scoid);
			printf("\nServer is requested to detach from ClientID: %d\n", clientId);
			params->living = 0;
		}else{
			printf("\nERROR: Server is requested to detach from ClientID %d but rejected\n");
		}
		break;
	case _PULSE_CODE_UNBLOCK:
		printf("\nServer received _PULSE_CODE_UNBLOCK(code: %d) after %d messages\n", _PULSE_CODE_UNBLOCK, params->messageNum);
		break;
	case _PULSE_CODE_COIDDEATH:
		printf("\nServer received _PULSE_CODE_COIDDEATH(code: %d) after %d messages\n", _PULSE_CODE_COIDDEATH, params->messageNum);
		break;
	case _PULSE_CODE_THREADDEATH:
		printf("\nServer received _PULSE_CODE_THREADDEATH(code: %d) after %d messages\n", _PULSE_CODE_THREADDEATH, params->messageNum);
		break;
	default:
		printf("\nServer received unrecognized PULSE CODE entity after %d messages\n", params->messageNum);
		break;
	}
}

void singlestep_trafficlight_statemachine_receive(sensorData *sd){
	switch(sd->message.currentState){
	case state0:
		printf("EWR-NSR(%d) -> Wait for 1 second\n", sd->message.currentState);
		break;
	case state1:
		printf("EWR-NSR(%d) -> Wait for 1 second\n", sd->message.currentState);
		printf("Waiting to receive signal...\n");
		break;
	case state2:
		printf("EWG-NSR(%d) -> Wait for 2 seconds\n", sd->message.currentState);
		printf("Received '%c' -> stopping car from North-South Road\n", sd->message.data);
		break;
	case state3:
		printf("EWY-NSR(%d) -> Wait for 1 second\n", sd->message.currentState);
		break;
	case state4:
		printf("EWR-NSR(%d) -> Wait for 1 second\n", sd->message.currentState);
		printf("Waiting to receive signal...\n");
		break;
	case state5:
		printf("Received '%c' -> stopping car from East-West Road\n", sd->message.data);
		printf("EWR-NSG(%d) -> Wait for 2 seconds\n", sd->message.currentState);
		break;
	case state6:
		printf("EWR-NSY(%d) -> Wait for 1 second\n", sd->message.currentState);
		break;
	}
	sd->replyMessage.currentState = sd->message.currentState;
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
		pthread_mutex_lock(&td->mutex);
			td->params.receiveId = MsgReceive(td->serverIdentity.serverChannelId, &td->message,
					sizeof(td->message), NULL);
			if(td->params.receiveId < 0){ // Receiving no messages
				printf("ERROR: Failed to receive message\n");
				break;
			}

			if(td->params.receiveId == 0){ // Receiving pulse
				pulseStateMachine(td->message.header, &td->params, td->message.clientId);
				continue;
			}else if(td->params.receiveId > 0){ // Receiving message
				td->params.messageNum++;

				// check if GNS service is running
				if(td->message.header.type == _IO_CONNECT){
					MsgReply(td->params.receiveId, EOK, NULL, 0);
					printf("\nSERVICE STATUS: GNS Service is running\n");
					continue;
				}

				// check if other header type is received
				if(td->message.header.type == _IO_BASE && td->message.header.type <= _IO_MAX){
					MsgError(td->params.receiveId, ENOSYS);
					printf("ERROR: Server received, and rejected unrecognized IO message header type\n");
					continue;
				}

				sprintf(td->replyMessage.buf, "State %d received", td->message.currentState);
				printf("\nServer (TD: %d): Receive data packet from ClientID(%d) of state %d\n", td->serverIdentity.serverProcessId,
						td->message.clientId, td->message.currentState);
				printf("----> Replying with: '%s'\n", td->replyMessage.buf);
				singlestep_trafficlight_statemachine_receive(td);
				sleep(td->message.delay);
				MsgReply(td->params.receiveId, EOK, &td->replyMessage, sizeof(td->replyMessage));
			}else{
				printf("\nERROR: Server received unrecognized entity, but could not handle it properly\n");
			}
		pthread_mutex_unlock(&td->mutex);
	}

	printf("\nTHREAD STATUS: Server thread terminating\n");
	ChannelDestroy(td->serverIdentity.serverChannelId);
	return 0;
}

// the main thread of server side application
int main(int argc, char *argv[]) {
	pthread_t serverThread;
	sensorData sensor;
	sensorDataInit(&sensor);
	pthread_create(&serverThread, NULL, server, &sensor);
	pthread_join(serverThread, NULL);
	printf("(MAIN) THREAD STATUS: Main thread terminating\n");
	return EXIT_SUCCESS;
}