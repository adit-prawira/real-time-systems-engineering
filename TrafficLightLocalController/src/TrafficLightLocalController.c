#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/dispatch.h>
#include <sys/syspage.h>

#define ATTACH_POINT "/net/CTC/dev/name/local/CTC"
//#define ATTACH_POINT "CTC"
#define BUF_SIZE 100
#define N_ITERATIONS 10
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
	msg_header_t header;
	int clientId;
	char clientName[BUF_SIZE];
	int data;
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
	int dataIsReady;
}SensorData;


void SensorDataInit(SensorData *sensor,_Uint16t type, _Uint16t subtype, int clientId, char *hostname ){
	sensor->message.clientId = clientId;
	sensor->message.header.type = type;
	sensor->message.header.subtype = subtype;
	strcpy(sensor->message.clientName, hostname);
	pthread_mutex_init(&sensor->mutex, NULL);
	pthread_cond_init(&sensor->condVar, NULL);
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
	printf("THREAD STARTING: %s client thread starting...\n", cd->message.clientName);
	for(index = 0; index < N_ITERATIONS; index++){
		pthread_mutex_lock(&cd->mutex);
		while(!cd->dataIsReady){
			pthread_cond_wait(&cd->condVar, &cd->mutex);
		}
		cd->message.data = 10+index;
		cd->dataIsReady = 1;
		printf("SENDING: ClientID(%d) sending value of %d with %d bytes of memory size\n",cd->message.clientId,
				cd->message.data, sizeof(cd->message));
		if(MsgSend(serverConnectionId, &cd->message, sizeof(cd->message),
				&cd->reply, sizeof(cd->reply))){
			printf("ERROR: Message of size %d bytes and value of %d is failed to be sent\n",
					sizeof(cd->message), cd->message.data);
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

	memset(hostname, '\0', 100);
	hostname[99] = '\n';
	gethostname(hostname, sizeof(hostname));

	printf("STARTING: %s is Running...\n", hostname);
	SensorDataInit(&sensor, 0x22, 0x00, 620, hostname);
	pthread_create(&clientThread, NULL, client, &sensor);
	pthread_join(clientThread, NULL);
	printf("TERMINATING: %s is Terminating...\n", hostname);

	return EXIT_SUCCESS;
}

