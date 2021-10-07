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
#include <time.h>

#define COLOR_CODE_SIZE 2
#define ATTACH_POINT_CTC "CTC"
#define ATTACH_POINT_TC "/net/TC/dev/name/local/TC"
#define BUF_SIZE 100
#define COMMAND_SIZE 2
#define RECONNECT_INTERVAL 2

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

void SensorDataInit(SensorData *sensor, char *hostname,
		_Uint16t type, _Uint16t subtype){
	sensor->reply.header.type = type;
	sensor->reply.header.subtype = subtype;
	strcpy(sensor->reply.replySourceName, hostname);
	pthread_mutex_init(&sensor->mutex, NULL);
	pthread_cond_init(&sensor->condVar, NULL);
	sensor->dataIsReady = 0;
}

void InstructionCommandInit(InstructionCommand *ic,int clientId, char *hostname,
		_Uint16t type, _Uint16t subtype){
	ic->instruction.header.type = type;
	ic->instruction.header.subtype = subtype;
	ic->instruction.id = clientId;
	strcpy(ic->instruction.sourceName, hostname);
	pthread_mutex_init(&ic->mutex, NULL);
	pthread_cond_init(&ic->condVar, NULL);
}

int _keyboardEventListener() {
	static const int STDIN = 0;
	static bool initialized = false;
	if (!initialized) {
		// Use termios to turn off line buffering
		struct termios term;
		tcgetattr(STDIN, &term);
		term.c_lflag &= ~ICANON; // apply bitwise compliment operator on Canonical input mode => not this will enable line editing mode in console
		tcsetattr(STDIN, TCSANOW, &term);// check if change is made immediately
		setbuf(stdin, NULL);
		initialized = true;
	}
	int bytesWaiting;
	ioctl(STDIN, FIONREAD, &bytesWaiting);// apply command to get numbber of bytes to read
	return bytesWaiting;
}

// Standard Pulse State Machine Handler
void pulseStateMachine(MessageData message, int stayAlive, int messageNum){
	printf("----> CTC Received a pulse from ClientID(%d)\n", message.trafficLight.id);
	printf("----> Received Message Header Code: %d\n", message.header.code);
	switch(message.header.code){
	case _PULSE_CODE_DISCONNECT:
		printf("Pulse STATUS CODE: %d\n", _PULSE_CODE_DISCONNECT);
		if(stayAlive == 0){
			// if received pulse to disconnect => disconnect all its connection by running name_close
			// for each name_open()
			ConnectDetach(message.header.serverConnectionId); // pass serverConnectionId to detach
			printf("Detaching: Server is requested to detach from ClientId(%d)\n", message.trafficLight.id);
		}else{
			printf("Rejected: Server is requested to detach from ClientId(%d) but rejected\n", message.trafficLight.id);
		}
		break;
	case _PULSE_CODE_UNBLOCK: // receiveId associated with the blocking message
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


void *client(void *data){
	int ch;
	int serverConnectionId = 0;
	int buf[COMMAND_SIZE] = {};
	InstructionCommand *ic = (InstructionCommand*)data;
	printf("ATTEMPTING TO CONNECT: Attempting to connect to server %s\n", ATTACH_POINT_TC);
	serverConnectionId = name_open(ATTACH_POINT_TC, 0);
	printf("Returned Connection ID: %d\n", serverConnectionId);

	// When TC is not started yet, it will attempt to find connection with it, until
	// TC is activated without interrupting the other process which is to listen to
	// any changes made by traffic light
	while(serverConnectionId == -1){
		// Logs error and exit the program early if it is connection is failed to be performed
		printf("ERROR: Unable to connect to the server with the given name of %s\n", ATTACH_POINT_TC);
		printf("ATTEMPTING TO CONNECT: Attempting to connect to server %s\n", ATTACH_POINT_TC);
		serverConnectionId = name_open(ATTACH_POINT_TC, 0);
		if(serverConnectionId != -1){
			break;
		}
		sleep(RECONNECT_INTERVAL); // attempting
	}
	printf("SUCCESS: Connected to the server %s\n", ATTACH_POINT_TC);
	printf("THREAD STARTING: Keyboard Event thread starting...\n"); // Logs out that the connection has been made and confirmed
	while(serverConnectionId){
		if(_keyboardEventListener()){
			pthread_mutex_lock(&ic->mutex);
			gets(buf);
			ch = buf[0];
			if(ch!='\0' && ch!='\n'){
				ic->instruction.data = ch;
				printf("Keyboard Event detected: %c\n", ic->instruction.data);
				ic->dataIsReady = 1; // flasg that data is ready
			}
			// Awaiting for data to be consumed by TC
			while(!ic->dataIsReady){
				pthread_cond_wait(&ic->condVar, &ic->mutex);
			}

			printf("SENDING: ClientID(%d) sending command key of '%c' with %d bytes of memory size\n",
							ic->instruction.id, ic->instruction.data, sizeof(ic->instruction));
			// Send Messages and receive reply from TC
			if(MsgSend(serverConnectionId, &ic->instruction, sizeof(ic->instruction),
					&ic->reply, sizeof(ic->reply)) == -1){
				printf("ERROR: Instruction of size %d bytes is failed to be sent\n",
						sizeof(ic->instruction));
				break;
			}else{
				printf("----> RECEIVED REPLY from %s: %s\n",ic->reply.replySourceName, ic->reply.buf);
			}
			pthread_cond_signal(&ic->condVar); // signal TC that is ready to be consumed
			pthread_mutex_unlock(&ic->mutex);
		}
	}
	printf("CLOSE CONNECTION: Sending message to server of closing connection\n");
	name_close(serverConnectionId);
	printf("THREAD TERMINATING: Keyboard Event thread terminating...\n");
	return 0;
}

void *server(void *data){
	SensorData *sd = (SensorData*)data;
	int receiveId = 0, isLiving = 0;
	int messageNum = 0, stayAlive = 0;
	// creating a global name which is located in /dev/<hostname>/local/<ATTACH_POINT_CTC>
	if((sd->attach = name_attach(NULL, ATTACH_POINT_CTC, 0)) == NULL){
		// if name is not attached successfully exit the program early
		printf("ERROR: Failed to compute name_attach on ATTACH_POINT_CTC: %s\n", ATTACH_POINT_CTC);
		printf("----> Another server may run the same ATTACH_POINT_CTC name or GNS service has not yet started");
		return EXIT_FAILURE;
	}

	printf("THREAD STARTING: %s server thread is starting...\n", ATTACH_POINT_CTC);
	printf("CTC Listening for L1, and L2 on ATTACH_POINT_CTC: %s\n", ATTACH_POINT_CTC);

	pthread_mutex_lock(&sd->mutex);
	isLiving = 1; // set thread status that it is living as connection has been made;
	while(isLiving){
		while(sd->dataIsReady){
			pthread_cond_wait(&sd->condVar, &sd->mutex);
		}

		receiveId = MsgReceive(sd->attach->chid, &sd->message, sizeof(sd->message), NULL);
		if(receiveId == -1){
			// break the loop early if there is no received message Id returned from MsgReceive
			printf("ERROR: Failed to receive message from MsgReceive\n");
			break;
		}

		// Condition when pulse is received, note that message now has the credentials of clients identity, where in this case are from L1, and L2
		if(receiveId == 0){
			pulseStateMachine(sd->message, stayAlive, messageNum);
		}

		// Condition when message is received from clients
		if(receiveId > 0){
			messageNum++;

			if(sd->message.header.type == _IO_CONNECT){
				// The case when client sending message that GNS service is running/succesfully connected
				MsgReply(receiveId, EOK, NULL, 0); // reply with EOK (a constant that means no error)
				printf("\nClient Send Message: GNS Server is running...\n");
				printf("\n----------> Replying with: EOK('no error')\n");
				messageNum--; // reduce number of messages because it has been replied
				continue; // go back to the start if the loop
			}

			// receiving some other IO => reject it
			if(sd->message.header.type > _IO_BASE && sd->message.header.type <= _IO_MAX){
				MsgError(receiveId, ENOSYS); // Send error of Function isn't implemented (ENOSYS)
				printf("ERROR: CTC received other IO messages and reject it\n");
				continue;
			}

			sprintf(sd->reply.buf, "Message number %d received", messageNum);
			printf("CTC RECEIVED CONFIG From %s(ClientID:%d):\n----> %s\n",
					sd->message.trafficLight.name, sd->message.trafficLight.id, sd->message.trafficLight.message);
			printf("----> REPLYING to %s: '%s'\n", sd->message.trafficLight.name, sd->reply.buf);
			MsgReply(receiveId, EOK, &sd->reply, sizeof(sd->reply)); // Send reply to clients (L1, and L2)
			sd->dataIsReady = 0;
			pthread_cond_signal(&sd->condVar);
		}else{
			printf("\nError: Server received unrecognized entity, but could not handle it correctly\n");
		}
	}
	name_detach(sd->attach, 0);
	pthread_mutex_unlock(&sd->mutex);
	printf("THREAD TERMINATING: %s server terminating...\n", ATTACH_POINT_CTC);
	return 0;
}



int main(int argc, char *argv[]){
	printf("_CustomSignalValue = %d bytes\n", sizeof(_CustomSignalValue));
	printf("msg_header_t = %d bytes\n", sizeof(msg_header_t));
	printf("MessageData = %d bytes\n", sizeof(MessageData));
	printf("ReplyData = %d bytes\n", sizeof(ReplyData));
	printf("SensorData = %d bytes\n", sizeof(SensorData));
	printf("InstructionCommand = %d bytes\n", sizeof(InstructionCommand));

	SensorData sensor;
	InstructionCommand cmd;
	pthread_t ctcServerThread, ctcClientThread;

	char hostname[100];
	time_t secondsFromEpoch = time(NULL);
	srand(secondsFromEpoch);
	int clientId = rand();
	memset(hostname, '\0', 100);
	hostname[99] = '\n';
	gethostname(hostname, sizeof(hostname));

	printf("STARTING: %s is Running...\n", hostname);

	SensorDataInit(&sensor, hostname, 0x01, 0x00);
	InstructionCommandInit(&cmd, clientId, hostname, 0x03, 0x00);

	pthread_create(&ctcServerThread, NULL, server, &sensor);
	pthread_create(&ctcClientThread, NULL, client, &cmd);

	pthread_join(ctcServerThread, NULL);
	pthread_join(ctcClientThread, NULL);

	printf("TERMINATING: %s is Terminating...\n", hostname);

	return EXIT_SUCCESS;
}


