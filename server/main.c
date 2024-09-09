#include "server.h"

int server_socket;

int main() {
	if (creating_socket() == -1) {
		printf("ERROR\n");
	}
	signal(SIGINT, handler);
	run();
	signal(SIGINT, SIG_DFL);
	printf("\nServer is shutting down\n");
	close(server_socket);
	return 0;
}
