#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <time.h>

#include <sys/select.h>
#include <sys/stat.h>

#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_CLIENT 5
#define PORT 8080
#define BUFSIZE 256
#define DEFAULT_TYPE_SIZE 6

typedef struct{
	int client_socket;
}Request;

typedef struct{
	int day, month, year;
}Birthday;

extern int server_socket;

void handler(int);

int creating_socket();
void run();
void* process_sending(void*);

void HEADresponse(int, char*, char*);
int count_day(Birthday);
void getInfo(Birthday*, char*, char*);
int POSTresponse(int, char*, char*);
void GETresponse(int, int, int*, char*, char*);
void request_details(char*, char*, char*, char*, char*);
char* HTTP_head(char*);
