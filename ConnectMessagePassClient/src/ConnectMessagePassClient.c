#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/iofunc.h>
#include <sys/netmgr.h>

#define BUF_SIZE 100
#define NUM_SIZE 2
#define FILE_LOCATION "/tmp/myServer.info"
//#define FILE_LOCATION "/net/serverAditya/tmp/myServer.info"
typedef struct{
	struct _pulse header;
	int clientId;
	int data;
}messageData;

typedef struct {
	struct _pulse header;
	char buf[BUF_SIZE];
}reply;

typedef struct{
	int serverProcessId;
	int serverChannelId;
}serverIds;
int client(int serverProcessId, int serverChannelId);
size_t read_data( FILE *fp, serverIds *p )
{
    return( fread( p, sizeof( serverIds ), 1, fp ) );
}
int main(int argc, char *argv[]) {
	printf("Client side running...\n");
	serverIds ids;

	FILE *file;

	int serverProcessId = 0, serverChannelId = 0;
	file = fopen(FILE_LOCATION, "r");
	if(file != NULL){
		if(read_data(file, &ids)!= 0){
			serverProcessId = ids.serverProcessId;
			serverChannelId = ids.serverChannelId;
		}else{
			printf("ERROR: Cannot read file\n");
		}

	}else{
		printf("Cannot open the file\n");
	}
	fclose(file);


	int returnVal= client(serverProcessId, serverChannelId);

	printf("Client side terminating...\n");
	return returnVal;
}

int client(int serverProcessId, int serverChannelId){
	messageData message;
	reply reply;

	int serverConnectionId;
	int index = 0; // initialize the first index for constructing loop iteration number

	printf("----> Attempting to connect to server with process ID of %d\n", serverProcessId);
	printf("----> On Channel: %d\n", serverChannelId);

	// set server connection ID
	serverConnectionId = ConnectAttach(ND_LOCAL_NODE, serverProcessId, serverChannelId, _NTO_SIDE_CHANNEL, 0);

	// Check if connection has been made
	if(serverConnectionId == -1){
		printf("ERROR: Unable to connect to the server!\n");
		return EXIT_FAILURE;
	}

	printf("Client connected to the server\n");

	// pre-define the message data
	message.header.type = 0x00;
	message.header.subtype = 0x00;
	message.clientId = 500;

	for(index = 0; index < 5; index ++){
		message.data = 10+index;
		printf("Client (ID: %d): Sending data packet with integer of %d", message.clientId, message.data);
		fflush(stdout);

		// wants to check if client receive a reply from server side. If yes, break the loop early
		if(MsgSend(serverConnectionId, &message, sizeof(message), &reply, sizeof(reply)) == -1){
			printf("ERROR: Data has not been sent successfully or there is no reply being sent back from server\n");
			break;
		}else{
			printf("----> Server side replying with: '%s'", reply.buf);
		}
		sleep(5);
	}
	printf("\n Sending message queue to server to close connection\n");
	ConnectDetach(serverConnectionId);

	return EXIT_SUCCESS;
}
