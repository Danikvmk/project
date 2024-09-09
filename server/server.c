#include "server.h"

volatile sig_atomic_t flag = 1;

struct sockaddr_in server_addr;

void handler(int sig) {
	flag = 0;
}

const char* filetypes[] = {"txt", "jpg", "png", "html", "pdf", "css"};
const char* content_types[] = { "Content-Type: text/plain\r\n",
				"Content-Type: image/jpeg\r\n",
				"Content-Type: image/png\r\n",
				"Content-Type: text/html\r\n",
				"Content-Type: application/pdf"
				"Content-Type: text/css\r\n"};
const char response_404[] = "HTTP/1.1 404 Not Found\r\nServer: Custom HTTP server\r\nContent-Type: text/html\r\n\r\n<html><head></head><body><p>404 Not Found</p></body></html>";
const char response_501[] = "HTTP/1.1 501 Not Implemented\r\nServer: Custom HTTP server\r\nContent-Type: text/html\r\n\r\n<html><head></head><body><p>501 Not Implemented</p></body></html>";

void request_details(char* method, char* addres, char* type, char* request, char* postInfo) {
	int str_size = BUFSIZE;
	for (int i = 0;;i++) {
		if (request[i] == ' ') {
			i += 2;
			int j = 0;
			int filetypebegin = 0;
			while (request[i] != ' ') {
				if (request[i] == '.') {
					filetypebegin = i;
				}
				if (j >= str_size) {
					str_size *= 2;
					addres = (char*) realloc(addres, str_size * sizeof(char));
				}
				addres[j++] = request[i++];
			}
			j = 0;
			i = filetypebegin + 1;
			while (request[i] != ' ') {
				type[j++] = request[i++];
			}
			if (strcmp(method, "POST") == 0) {
				while (request[i] != '\0') {
					if (request[i] == '\r' && request[i + 1] == '\n' && request[i + 2] == '\r' && request[i + 3] == '\n') {
						break;
					}
					i++;
				}
				i += 4;
				str_size = BUFSIZE;
				for (int j = 0; request[i] != '\n'; j++, i++) {
					if (j >= str_size) {
						str_size *= 2;
						postInfo = (char*) realloc(postInfo, str_size * sizeof(char));
					}
					postInfo[j] = request[i];
				}
			}
			break;
		}
		method[i] = request[i];
	}
}

void getInfo(Birthday* data, char* name, char* postInfo) {
	int n = 16;
	for (int i = 0;; i++) {
		if (postInfo[i] == '=') {
			i++;
			for (int j = 0; postInfo[i] != '&'; j++, i++) {
				if (j >= n) {
					n *= 2;
					name = (char*) realloc(name, n * sizeof(char));
				}
				name[j] = postInfo[i];
			}
			while(postInfo[i] != '=') i++;
			i++;
			char temp[5] = "";
			for (int j = 0; postInfo[i] != '\0'; j++, i++) {
				if (postInfo[i] == '.') {
					temp[j] = '\0';
					if (data -> day == 0) {
						data -> day = atoi(temp);
					} else if (data -> month == 0) {
						data -> month = atoi(temp);
					}
					j = 0;
					i++;
				}
				temp[j] = postInfo[i];
			}
			data -> year = atoi(temp);
			break;
		}
	}
}

int days_in_month(int month, int year) {
	if (month == 4 || month == 6 || month == 9 || month == 11) return 30;
	else if (month == 2) {
		if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) return 29;
		else return 28;
	} else return 31;
}

int count_day(Birthday data) {
	time_t t = time(NULL);
	struct tm* now = localtime(&t);
	int curr_day = now->tm_mday, curr_month = now->tm_mon + 1, curr_year = now->tm_year + 1900;
	if (data.month < 1 || data.month > 12 || data.day < 1 || data.day > days_in_month(data.month, data.year)) {
		return -1;
	}
	int res = 0;
	if (data.month < curr_month || (data.month == curr_month && data.day < curr_day)) {
		curr_year++;
	}
	while (!(data.month == curr_month && data.day == curr_day)) {
		res++;
		curr_day++;
		if (curr_day > days_in_month(curr_month, curr_year)) {
			curr_day = 1;
			curr_month++;
			if (curr_month > 12) {
				curr_month = 1;
				curr_year++;
			}
		}
	}
	return res;
}

void HEADresponse(int client_socket, char* addres, char* type) {
	struct stat filestats;
	stat(addres, &filestats);

	char* response = (char*) calloc(BUFSIZE, sizeof(char));
	strcat(response, "HTTP/1.1 200 OK\r\nServer: Custom HTTP server\r\n");

	for(int i = 0; filetypes[i] != NULL; i++) {
		if (strcmp(type, filetypes[i]) == 0) {
			strcat(response, content_types[i]);
			break;
		}
	}

	char length[32];
	sprintf(length, "Content-Length: %ld\r\n", filestats.st_size);
	strcat(response, length);

	char last_mod[64];
	time_t lm_info = filestats.st_mtim.tv_sec;
	struct tm* timeinfo = localtime(&lm_info);
	sprintf(last_mod, "Last-Modified: %d.%d.%d  %d:%d:%d\r\n\r\n", timeinfo -> tm_mday, timeinfo -> tm_mon + 1, timeinfo -> tm_year + 1900, timeinfo -> tm_hour, timeinfo -> tm_min, timeinfo -> tm_sec);
	strcat(response, last_mod);

	write(client_socket, response, strlen(response));
	free(response);
}

int POSTresponse(int client_socket, char* addres, char* postInfo) {
	if (strcmp(addres, "post.html") == 0) {
		Birthday data = {};
		char* name = (char*) calloc(16, sizeof(char));
		getInfo(&data, name, postInfo);
		int res;
		if((res = count_day(data)) == -1) {
			return 1;
		}
		char* response = (char*) calloc(BUFSIZE, sizeof(char));
		strcat(response, "HTTP/1.1 200 OK\r\nServer: Custom HTTP server\r\nContent-Type: text/html\r\n\r\n<html><head></head><body><p>");
		strcat(response, name);
		char str[64];
		sprintf(str, "%d days left until your birthday</p>", res);
		strcat(response, str);
		strcat(response, "</body></html>");
		write(client_socket, response, strlen(response));
		free(response);
//		free(name);
	}
	return 0;
}

void GETresponse(int Num, int client_socket, int* FD_ptr, char* addres, char* type) {
	if (Num == 404) {
		write(client_socket, response_404, strlen(response_404));
	} else if (Num == 501) {
		write(client_socket, response_501, strlen(response_501));
	} else if (Num == 200) {
		if (addres == NULL) {
			struct stat filestats;
			stat("standart.html", &filestats);
			char* body = (char*) calloc(filestats.st_size, sizeof(char));
			read(*FD_ptr, body, filestats.st_size);

			char* response = (char*) calloc(BUFSIZE, sizeof(char));
			strcat(response, "HTTP/1.1 200 OK\r\nServer: Custom HTTP server\r\nContent-Type: text/html\r\n");

			char length[32];
			sprintf(length, "Content-Length: %ld\r\n\r\n", filestats.st_size);
			strcat(response, length);

			size_t size = strlen(response) + filestats.st_size;
			response = (char*) realloc(response, size * sizeof(char));
			for (int i = strlen(response), j = 0; j < filestats.st_size; i++, j++) {
				response[i] = body[j];
			}

			write(client_socket, response, size);
			free(body);
			free(response);
		} else {
			struct stat filestats;
			stat(addres, &filestats);
			char* body = (char*) calloc(filestats.st_size, sizeof(char));
			read(*FD_ptr, body, filestats.st_size);

			char* response = (char*) calloc(BUFSIZE, sizeof(char));
			strcat(response, "HTTP/1.1 200 OK\r\nServer: Custom HTTP server\r\n");

			for(int i = 0; filetypes[i] != NULL; i++) {
				if (strcmp(type, filetypes[i]) == 0) {
					strcat(response, content_types[i]);
					break;
				}
			}

			char length[32];
			sprintf(length, "Content-Length: %ld\r\n", filestats.st_size);
			strcat(response, length);

			char last_mod[64];
			time_t lm_info = filestats.st_mtim.tv_sec;
			struct tm* timeinfo = localtime(&lm_info);
			sprintf(last_mod, "Last-Modified: %d.%d.%d  %d:%d:%d\r\n\r\n", timeinfo -> tm_mday, timeinfo -> tm_mon + 1, timeinfo -> tm_year + 1900, timeinfo -> tm_hour, timeinfo -> tm_min, timeinfo -> tm_sec);
			strcat(response, last_mod);

			size_t size = strlen(response) + filestats.st_size;
			response = (char*) realloc(response, size * sizeof(char));
			for (int i = strlen(response), j = 0; j < filestats.st_size; i++, j++) {
				response[i] = body[j];
			}

			write(client_socket, response, size);
			free(body);
			free(response);
		}
	}
}

void* process_sending(void* args) {
	Request* rqst = args;

	fd_set rset;
	FD_ZERO(&rset);
	FD_SET(rqst -> client_socket, &rset);
	if (select(rqst -> client_socket + 1, &rset, NULL, NULL, NULL) == -1) {
		perror("select");
		pthread_exit((void*) 1);
	}
	size_t bufsize = 0;
	if (FD_ISSET(rqst -> client_socket, &rset)) {
		while (!bufsize) {
			if (ioctl(rqst -> client_socket, FIONREAD, &bufsize) == -1) {
				perror("ioctl");
				pthread_exit((void*) 1);
			}
		}
	}
	char* request = (char*) calloc(bufsize + 1, sizeof(char));
	if (read(rqst -> client_socket, (void*) request, bufsize) == -1) {
		perror("read");
		pthread_exit((void*) 1);
	}

	write(STDOUT_FILENO, request, strlen(request));

	char* method = (char*) calloc(8, sizeof(char));
	char* addres = (char*) calloc(BUFSIZE, sizeof(char));
	char* type = (char*) calloc(DEFAULT_TYPE_SIZE, sizeof(char));
	char* postInfo = (char*) calloc(BUFSIZE, sizeof(char));

	request_details(method, addres, type, request, postInfo);
	free(request);

	if (strcmp(addres, "") == 0) {
		int fd = open("standart.html", O_RDONLY);
		GETresponse(200, rqst -> client_socket, &fd, NULL, NULL);
		free(method);
	} else {
		int fd = open(addres, O_RDONLY);
		if (fd == -1) {
			GETresponse(404, rqst -> client_socket, NULL, NULL, NULL);
		} else {
			if (!strcmp(method, "POST")) {
				POSTresponse(rqst -> client_socket, addres, postInfo);
	//			free(postInfo);
			} else if (!strcmp(method, "HEAD")) {
				HEADresponse(rqst -> client_socket, addres, type);
			} else {
				GETresponse(200, rqst -> client_socket, &fd, addres, type);
			}
			close(fd);
		}
		free(method);
		free(addres);
		free(type);
	}
	shutdown(rqst -> client_socket, SHUT_RDWR);

	close(rqst->client_socket);
	pthread_exit((void*) 0);
}

void run() {
	fd_set rset;
	while (flag) {
		FD_ZERO(&rset);
		FD_SET(server_socket, &rset);
		struct timeval timer = {};
		timer.tv_sec = 60;
		int res = select(server_socket + 1, &rset, NULL, NULL, &timer);
		if (res == -1) {
			perror("select");
			exit(1);
		} else if (res == 0) {
			return;
		}
		struct sockaddr_in client_addr = {};
		int sock;
		if (FD_ISSET(server_socket, &rset)) {
			socklen_t client_addr_len = sizeof(client_addr);
			sock = accept(server_socket, (struct sockaddr*) &client_addr, &client_addr_len);
			if (sock == -1) {
				perror("server: accept: ");
				exit(1);
			}
		}

		Request args;
		args.client_socket = sock;
		pthread_t tid;
		pthread_create(&(tid), NULL, process_sending, (void*)&(args));
	}
}

int creating_socket() {
	if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("server: socket: ");
		return -1;
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(PORT);

	int opt = 1;
	setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	if (bind(server_socket, (struct sockaddr*) &server_addr, sizeof(server_addr)) == -1) {
		perror("server: bind: ");
		return -1;
	}

	if (listen(server_socket, MAX_CLIENT) == -1) {
		perror("server: listen ");
		return -1;
	}

	return 0;
}


