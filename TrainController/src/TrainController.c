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
#define ATTACH_POINT "CTC"
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

// This thread will listen to CTCT regarding the given input or key pressed from Central Controller
// Those key press will determine changes of sequences of the traffic light system
void *server(void *data){
	return 0;
}
int main(void) {
	SensorData sensor;
	pthread_t tcThread;

	return EXIT_SUCCESS;
}
