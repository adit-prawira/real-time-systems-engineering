#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/dispatch.h>
#include <pthread.h>
#include <sys/syspage.h>
#include <unistd.h>
#include <sys/select.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <stdbool.h>

#define COLOR_CODE_SIZE 2
#define ATTACH_POINT_TC "TC"
#define BUF_SIZE 100

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
typedef struct{
	msg_header_t header;
	int clientId;
	volatile char data;
	int instructionReady;
}InstructionCommand;
typedef struct {
	msg_header_t header;
	char buf[BUF_SIZE];
	char replySourceName[BUF_SIZE];
}ReplyData;

typedef struct {
	MessageData message;
	ReplyData reply;
	InstructionCommand instruction;
	pthread_mutex_t mutex;
	pthread_cond_t condVar;
	name_attach_t *attach;
	enum states currentState;
	int dataIsReady;
}SensorData;
void pulseStateMachine(InstructionCommand instruction, int stayAlive, int messageNum){
	printf("----> CTC Received a pulse from ClientID(%d)\n", instruction.clientId);
	printf("----> Received Message Header Code: %d\n", instruction.header.code);
	switch(instruction.header.code){
	case _PULSE_CODE_DISCONNECT:
		printf("Pulse STATUS CODE: %d\n", _PULSE_CODE_DISCONNECT);
		if(stayAlive == 0){
			// if received pulse to disconnect => disconnect all its connection by running name_close
			// for each name_open()
			ConnectDetach(instruction.header.serverConnectionId); // pass serverConnectionId to detach
			printf("Detaching: Server is requested to detach from ClientId(%d)\n", instruction.clientId);
		}else{
			printf("Rejected: Server is requested to detach from ClientId(%d) but rejected\n", instruction.clientId);
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
// This thread will listen to CTCT regarding the given input or key pressed from Central Controller
// Those key press will determine changes of sequences of the traffic light system
void *server(void *data){
	SensorData *sd = (SensorData*)data;
	int receiveId = 0, isLiving = 0;
	int messageNum = 0, stayAlive = 0;
	if((sd->attach = name_attach(NULL, ATTACH_POINT_TC, 0))==NULL){
		// if name is not attached successfully exit the program early
		printf("ERROR: Failed to compute name_attach on ATTACH_POINT_TC: %s\n", ATTACH_POINT_TC);
		printf("----> Another server may run the same ATTACH_POINT_TC name or GNS service has not yet started");
		return EXIT_FAILURE;
	}
	printf("THREAD STARTING: %s server thread is starting...\n", ATTACH_POINT_TC);
	printf("TC Listening for CTC ATTACH_POINT_TC: %s\n", ATTACH_POINT_TC);

	pthread_mutex_lock(&sd->mutex);
	isLiving = 1;
	while(isLiving){
		while(sd->instruction.instructionReady){
			pthread_cond_wait(&sd->condVar, &sd->mutex);
		}
		receiveId = MsgReceive(sd->attach->chid, &sd->instruction, sizeof(sd->instruction), NULL);

		if(receiveId == -1){
			// break the loop early if there is no received message Id returned from MsgReceive
			printf("ERROR: Failed to receive message from MsgReceive\n");
			break;
		}

		if(receiveId == 0){
			pulseStateMachine(sd->instruction, stayAlive, messageNum);
		}
		if(receiveId > 0){
			messageNum++;

			if(sd->instruction.header.type == _IO_CONNECT){
				// The case when client sending message that GNS service is running/succesfully connected
				MsgReply(receiveId, EOK, NULL, 0); // reply with EOK (a constant that means no error)
				printf("\nClient Send Message: GNS Server is running...\n");
				printf("\n----------> Replying with: EOK('no error')\n");
				messageNum--; // reduce number of messages because it has been replied
				continue; // go back to the start if the loop
			}

			// receiving some other IO => reject it
			if(sd->instruction.header.type > _IO_BASE && sd->instruction.header.type <= _IO_MAX){
				MsgError(receiveId, ENOSYS); // Send error of Function isn't implemented (ENOSYS)
				printf("ERROR: CTC received other IO messages and reject it\n");
				continue;
			}

			sprintf(sd->reply.buf, "Message number %d received", messageNum);
			printf("RECEIVED KEY COMMAND FROM (ClientID:%d) with value of %c\n----> %s\n",
					sd->instruction.clientId, sd->instruction.data);
			printf("----> REPLYING: '%s'\n",sd->reply.buf);
			MsgReply(receiveId, EOK, &sd->reply, sizeof(sd->reply)); // Send reply to clients (L1, and L2)
			sd->dataIsReady = 0;
			pthread_cond_signal(&sd->condVar);
		}else{
			printf("ERROR: CTC received unrecognized entity, but unable to handle it properly\n");
		}
	}
	name_detach(sd->attach, 0);
	pthread_mutex_unlock(&sd->mutex);
	printf("THREAD TERMINATING: %s server is terminating...\n", ATTACH_POINT_TC);
	return 0;
}
void SensorDataInit(SensorData *sensor, char *hostname,_Uint16t type, _Uint16t subtype){
	sensor->reply.header.type = type;
	sensor->reply.header.subtype = subtype;
	strcpy(sensor->reply.replySourceName, hostname);
	pthread_mutex_init(&sensor->mutex, NULL);
	pthread_cond_init(&sensor->condVar, NULL);
	sensor->dataIsReady = 0;
}

int main(void) {
	printf("_CustomSignalValue = %d bytes\n", sizeof(_CustomSignalValue));
	printf("msg_header_t = %d bytes\n", sizeof(msg_header_t));
	printf("MessageData = %d bytes\n", sizeof(MessageData));
	printf("ReplyData = %d bytes\n", sizeof(ReplyData));
	printf("SensorData = %d bytes\n", sizeof(SensorData));

	SensorData sensor;
	pthread_t tcServerThread;

	char hostname[100];
	memset(hostname, '\0', 100);
	hostname[99] = '\n';
	gethostname(hostname, sizeof(hostname));

	printf("STARTING: %s is Running...\n", hostname);
	SensorDataInit(&sensor, hostname, 0x03, 0x00);
	pthread_create(&tcServerThread, NULL, server, &sensor);
	pthread_join(tcServerThread, NULL);
	printf("TERMINATING: %s is Terminating...\n", hostname);

	return EXIT_SUCCESS;
}