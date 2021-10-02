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
//#define ATTACH_POINT "CTC"
#define BUF_SIZE 100
#define COLOR_CODE_SIZE 2
#define N_ITERATIONS 10
#define RED "R"
#define YELLOW "Y"
#define GREEN "G"

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


void SensorDataInit(SensorData *sensor,_Uint16t type, _Uint16t subtype, int clientId, char *hostname ){
	sensor->currentState = state0;
	sensor->message.trafficLight.id = clientId;
	sensor->message.trafficLight.waitTime = 1;
	strcpy(sensor->message.trafficLight.color, RED);
	strcpy(sensor->message.trafficLight.name, hostname);
	sensor->message.header.type = type;
	sensor->message.header.subtype = subtype;
	pthread_mutex_init(&sensor->mutex, NULL);
	pthread_cond_init(&sensor->condVar, NULL);
	sensor->message.trafficLight.living = 1;
}
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
		sensor->message.trafficLight.waitTime = 3;
		sprintf(sensor->message.trafficLight.message,
				"EWG-NSR(%d) -> Wait for %d seconds", sensor->currentState,
				sensor->message.trafficLight.waitTime);
		sensor->currentState = state3;
		break;
	case state3:
		sensor->message.trafficLight.waitTime = 1;
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
		sensor->message.trafficLight.waitTime = 3;
		sprintf(sensor->message.trafficLight.message,
				"EWR-NSG(%d) -> Wait for %d seconds", sensor->currentState,
				sensor->message.trafficLight.waitTime);
		sensor->currentState = state6;
		break;
	case state6:
		sensor->message.trafficLight.waitTime = 1;
		sprintf(sensor->message.trafficLight.message,
				"EWR-NSY(%d) -> Wait for %d second", sensor->currentState,
				sensor->message.trafficLight.waitTime);
		sensor->currentState = state1;
		break;
	}
}
void *client(void *data){
	SensorData *cd = (SensorData*)data;
	int serverConnectionId = 0, index = 0;
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

	while(cd->message.trafficLight.living){
		pthread_mutex_lock(&cd->mutex);
		while(!cd->dataIsReady){
			pthread_cond_wait(&cd->condVar, &cd->mutex);
		}
		trafficLightStateMachine(cd);
		cd->dataIsReady = 1;
		printf("SENDING: ClientID(%d) sending value of state %d with %d bytes of memory size\n",
				cd->message.trafficLight.id, cd->currentState, sizeof(cd->message));
		if(MsgSend(serverConnectionId, &cd->message, sizeof(cd->message),
				&cd->reply, sizeof(cd->reply))){
			printf("ERROR: Message of size %d bytes and value of %d is failed to be sent\n",
					sizeof(cd->message), cd->currentState);
			break;
		}else{
			printf("----> RECEIVED REPLY: %s\n", cd->reply.buf);
		}
		pthread_cond_signal(&cd->condVar);
		pthread_mutex_unlock(&cd->mutex);
	}
	printf("CLOSE CONNECTION: Sending message to server of closing connection\n");
	name_close(serverConnectionId);
	return 0;
}

int main(int agrc, char *argv[]) {
	printf("_CustomSignalValue = %d bytes\n", sizeof(_CustomSignalValue));
	printf("msg_header_t = %d bytes\n", sizeof(msg_header_t));
	printf("MessageData = %d bytes\n", sizeof(MessageData));
	printf("ReplyData = %d bytes\n", sizeof(ReplyData));

	SensorData sensor;
	pthread_t clientThread;
	char hostname[100];
	time_t secondsFromEpoch = time(NULL);
	srand(secondsFromEpoch);
	int clientId = rand();
	memset(hostname, '\0', 100);
	hostname[99] = '\n';
	gethostname(hostname, sizeof(hostname));

	printf("STARTING: %s is Running...\n", hostname);
	SensorDataInit(&sensor, 0x22, 0x00, clientId, hostname);
	pthread_create(&clientThread, NULL, client, &sensor);
	pthread_join(clientThread, NULL);
	printf("TERMINATING: %s is Terminating...\n", hostname);

	return EXIT_SUCCESS;
}
