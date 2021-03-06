#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/dispatch.h>
#include <sys/syspage.h>

#define ATTACH_POINT "/net/CTC/dev/name/local/CTC"
#define BUF_SIZE 100
#define COLOR_CODE_SIZE 2
#define N_ITERATIONS 10
#define RED "R"
#define YELLOW "Y"
#define GREEN "G"
#define RY_DELAY 2
#define G_DELAY 4

enum states {
	state0, state1, state2, state3, state4, state5, state6
};

typedef union{
	union{
		_Uint32t sival_int;
		void *sival_ptr;
	};
	_Uint32t dummy[4];
}_CustomSignalValue;

typedef struct _CustomPulse{
	_Uint16t type;
	_Uint16t subtype;
	_Int8t code;
	_Uint8t zero[3];
	_CustomSignalValue value;
	_Uint8t zero2[2];
	_Int32t serverConnectionId;
}msg_header_t;

typedef struct {
	int name[BUF_SIZE];
	int id;
	int waitTime;
	int living;
	char message[BUF_SIZE];
	char color[COLOR_CODE_SIZE];
} TrafficLightSettings;

typedef struct {
	msg_header_t header;
	TrafficLightSettings trafficLight;
}MessageData;

typedef struct {
	msg_header_t header;
	char buf[BUF_SIZE];
	char replySourceName[BUF_SIZE];
}ReplyData;

typedef struct {
	MessageData message;
	ReplyData reply;
	pthread_mutex_t mutex;
	pthread_cond_t condVar;
	name_attach_t *attach;
	enum states currentState;
	int dataIsReady;
}SensorData;

typedef struct {
	msg_header_t header;
	volatile char data;
	int id;
	char sourceName[BUF_SIZE];
} InstructionData;

typedef struct{
	InstructionData instruction;
	ReplyData reply;
	pthread_mutex_t mutex;
	pthread_cond_t condVar;
	name_attach_t *attach;
	int dataIsReady;
}InstructionCommand;

// Initialize sensor data struct
void SensorDataInit(SensorData *sensor,_Uint16t type, _Uint16t subtype, int clientId, char *hostname ){
	sensor->currentState = state0;
	sensor->message.trafficLight.id = clientId;
	sensor->message.trafficLight.waitTime = RY_DELAY;
	strcpy(sensor->message.trafficLight.color, RED);
	strcpy(sensor->message.trafficLight.name, hostname);
	sensor->message.header.type = type;
	sensor->message.header.subtype = subtype;
	pthread_mutex_init(&sensor->mutex, NULL);
	pthread_cond_init(&sensor->condVar, NULL);
	sensor->message.trafficLight.living = 1;
}

// Initialize instruction data as a reply to TC
void InstructionCommandInit(InstructionCommand *ic, char *hostname,_Uint16t type, _Uint16t subtype){
	ic->reply.header.type = type;
	ic->reply.header.subtype = subtype;
	strcpy(ic->reply.replySourceName, hostname);
	pthread_mutex_init(&ic->mutex, NULL);
	pthread_cond_init(&ic->condVar, NULL);
	ic->dataIsReady = 0;
};

// TODO: Modify the state machine of the traffic light according to our design
void trafficLightStateMachine(SensorData *sensor){
	switch(sensor->currentState){
	case state0:
		sprintf(sensor->message.trafficLight.message,
				"Traffic Light %s Starting\n", sensor->message.trafficLight.name);
		sensor->currentState = state1;
		break;
	case state1:
		sprintf(sensor->message.trafficLight.message,
				"EWR-NSR(%d) -> Wait for %d second", sensor->currentState,
				sensor->message.trafficLight.waitTime);
		sensor->currentState = state2;
		break;
	case state2:
		sensor->message.trafficLight.waitTime = G_DELAY;
		sprintf(sensor->message.trafficLight.message,
				"EWG-NSR(%d) -> Wait for %d seconds", sensor->currentState,
				sensor->message.trafficLight.waitTime);
		sensor->currentState = state3;
		break;
	case state3:
		sensor->message.trafficLight.waitTime = RY_DELAY;
		sprintf(sensor->message.trafficLight.message,
				"EWY-NSR(%d) -> Wait for %d second", sensor->currentState,
				sensor->message.trafficLight.waitTime);
		sensor->currentState = state4;
		break;
	case state4:
		sprintf(sensor->message.trafficLight.message,
				"EWR-NSR(%d) -> Wait for %d second", sensor->currentState,
				sensor->message.trafficLight.waitTime);
		sensor->currentState = state5;
		break;
	case state5:
		sensor->message.trafficLight.waitTime = G_DELAY;
		sprintf(sensor->message.trafficLight.message,
				"EWR-NSG(%d) -> Wait for %d seconds", sensor->currentState,
				sensor->message.trafficLight.waitTime);
		sensor->currentState = state6;
		break;
	case state6:
		sensor->message.trafficLight.waitTime = RY_DELAY;
		sprintf(sensor->message.trafficLight.message,
				"EWR-NSY(%d) -> Wait for %d second", sensor->currentState,
				sensor->message.trafficLight.waitTime);
		sensor->currentState = state1;
		break;
	}
}

// GENERIC PULSE STATE MACHINE
void pulseStateMachine(InstructionData instruction, int stayAlive, int messageNum){
	printf("----> CTC Received a pulse from ClientID(%d)\n", instruction.id);
	printf("----> Received Message Header Code: %d\n", instruction.header.code);
	switch(instruction.header.code){
	case _PULSE_CODE_DISCONNECT:
		printf("Pulse STATUS CODE: %d\n", _PULSE_CODE_DISCONNECT);
		if(stayAlive == 0){
			// if received pulse to disconnect => disconnect all its connection by running name_close
			// for each name_open()
			ConnectDetach(instruction.header.serverConnectionId); // pass serverConnectionId to detach
			printf("Detaching: Server is requested to detach from ClientId(%d)\n", instruction.id);
		}else{
			printf("Rejected: Server is requested to detach from ClientId(%d) but rejected\n", instruction.id);
		}
		break;
	case _PULSE_CODE_UNBLOCK: // receiveId associated with the blocking instruction
		// Gives the option to reply back to the client now or later after being hit by a signal or a time out
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

// Client thread which will send the current state and settings of traffic light to be displayed
// on CTC monitor
void *client(void *data){
	SensorData *cd = (SensorData*)data;
	int serverConnectionId = 0;

	printf("ATTEMPTING TO CONNECT: Attempting to connect to server %s\n", ATTACH_POINT);
	serverConnectionId = name_open(ATTACH_POINT, 0);
	printf("Returned Connection ID: %d\n", serverConnectionId);

	if(serverConnectionId == -1){
		// Logs error and exit the program early if it is connection is failed to be performed
		printf("ERROR: Unable to connect to the server with the given name of %s\n", ATTACH_POINT);
		return EXIT_FAILURE;
	}

	printf("SUCCESS: Connected to the server %s\n", ATTACH_POINT);
	printf("THREAD STARTING: %s client thread starting...\n", cd->message.trafficLight.name);

	// start event loop when the trafiic light is ON
	while(cd->message.trafficLight.living){
		pthread_mutex_lock(&cd->mutex);

		// Await for data to be consumed by CTC
		while(!cd->dataIsReady){
			pthread_cond_wait(&cd->condVar, &cd->mutex);
		}

		// Implement traffic light state machine
		trafficLightStateMachine(cd);

		cd->dataIsReady = 1; // set status as data is ready to be consumed after being mutated in trafficLightStateMachine
		printf("SENDING: ClientID(%d) sending value of state %d with %d bytes of memory size\n",
				cd->message.trafficLight.id, cd->currentState, sizeof(cd->message));

		// Send message data and get reply messages from consumer which is CTC
		if(MsgSend(serverConnectionId, &cd->message, sizeof(cd->message),
				&cd->reply, sizeof(cd->reply))==-1){
			printf("ERROR: Message of size %d bytes and value of %d is failed to be sent\n",
					sizeof(cd->message), cd->currentState);
			break;
		}else{
			// Logs out replid messages from consumer to confirm that data has been displayed
			printf("----> RECEIVED REPLY from %s: %s\n", cd->reply.replySourceName, cd->reply.buf);
		}

		// send data to CTC according according to traffic light color waitTime
		sleep(cd->message.trafficLight.waitTime);
		pthread_cond_signal(&cd->condVar); // Send signal to CTC that data is ready to be consumed
		pthread_mutex_unlock(&cd->mutex);
	}

	printf("CLOSE CONNECTION: Sending message to server of closing connection\n");
	name_close(serverConnectionId); // Close connection
	return 0;
}

void *server(void *data){
	InstructionCommand *ic = (InstructionCommand*)data;
	int receiveId = 0, messageNum = 0;
	int isLiving = 0, stayAlive = 0;

	if((ic->attach = name_attach(NULL, ic->reply.replySourceName, 0)) == NULL){
		printf("ERROR: Failed to compute name_attach on %s: %s\n", ic->reply.replySourceName,ic->reply.replySourceName);
		printf("----> Another server may run the same ATTACH_POINT_X1 name or GNS service has not yet started");
		return EXIT_FAILURE;
	}
	printf("THREAD STARTING: %s server thread is starting...\n", ic->reply.replySourceName);
	printf("%s listening to X1 ATTACH_POINT_%s: %s\n",ic->reply.replySourceName
			,ic->reply.replySourceName,ic->reply.replySourceName);
	isLiving = 1;
	while(isLiving){
		pthread_mutex_lock(&ic->mutex);
		receiveId = MsgReceive(ic->attach->chid, &ic->instruction, sizeof(ic->instruction), NULL);
		if(receiveId == -1){
			printf("ERROR: Failed to receive message from MsgReceive\n");
			break;
		}
		if(receiveId == 0){
			pulseStateMachine(ic->instruction, stayAlive, messageNum);
		}
		if(receiveId > 0){
			messageNum++;
			if(ic->instruction.header.type == _IO_CONNECT){
				MsgReply(receiveId, EOK, NULL, 0);
				printf("\n----------> Replying with: EOK('no error')\n");
				messageNum--;
				continue;
			}
			if(ic->instruction.header.type > _IO_BASE && ic->instruction.header.type <= _IO_MAX){
				MsgError(receiveId, ENOSYS);
				printf("ERROR: %s received other IO messages and reject it\n", ic->reply.replySourceName);
				continue;
			}
			sprintf(ic->reply.buf, "Message number %d received", messageNum);
			printf("%s RECEIVED KEY COMMAND from %s(ClientID: %d) with value of %c:\n----> %s\n",
					ic->reply.replySourceName, ic->instruction.sourceName, ic->instruction.id, ic->instruction.data,
					ic->reply.buf);

			MsgReply(receiveId, EOK, &ic->reply, sizeof(ic->reply));
			ic->dataIsReady = 0;
			pthread_cond_signal(&ic->condVar);
		}else{
			printf("\nERROR: Server received unrecognized entity but could not handle it correctly\n");
		}
		pthread_mutex_unlock(&ic->mutex);
	}
	name_detach(ic->attach, 0);
	printf("THREAD TERMINATING: %s server terminating...\n", ic->reply.replySourceName);
	return 0;
}

int main(int agrc, char *argv[]) {
	SensorData sensor;
	InstructionCommand cmd;
	pthread_t clientThread, serverThread;

	char hostname[100];
	time_t secondsFromEpoch = time(NULL);
	srand(secondsFromEpoch);
	int clientId = rand();

	memset(hostname, '\0', 100);
	hostname[99] = '\n';
	gethostname(hostname, sizeof(hostname));

	printf("STARTING: %s is Running...\n", hostname);

	SensorDataInit(&sensor, 0x22, 0x00, clientId, hostname);
	InstructionCommandInit(&cmd, hostname, 0x55, 0x00);

	pthread_create(&serverThread, NULL, server, &cmd);
	pthread_create(&clientThread, NULL, client, &sensor);

	pthread_join(serverThread, NULL);
	pthread_join(clientThread, NULL);

	printf("TERMINATING: %s is Terminating...\n", hostname);

	return EXIT_SUCCESS;
}

