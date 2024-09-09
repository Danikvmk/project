#include "execute.h"
#define SIZE_NAME_CMDS 32

int jc_flag = 1;

const char* signals[] = {"-SIGHUP", "-SIGINT", "-SIGQUIT", "-SIGILL", "-SIGTRAP","-SIGABRT", "-SIGBUS", "-SIGFPE", "-SIGKILL", "-SIGUSR1", "-SIGSEGV", "-SIGUSR2", "-SIGPIPE", "-SIGALRM", "-SIGTERM", "-SIGSTKFLT", "-SIGCHLD", "-SIGCONT", "-SIGSTOP", "-SIGTSTP", "-SIGTTIN", "-SIGTTOU", "-SIGURG", "-SIGXCPU"};

void check(job** j){
	if(*j == NULL) return;
	for (job* ptr = *j; ptr; ptr = ptr -> next) {
		int status;
		while(1) {
 			pid_t wpid = waitpid(-ptr -> pgid, &status, WNOHANG | WCONTINUED | WUNTRACED);
			if (wpid < 0 && errno == ECHILD) {
				break;
			} else if (wpid < 0 && errno != ECHILD) {
				perror("waitpid");
				break;
			} else if (wpid == 0) break;
			update_process_status(ptr, wpid, status);
		}
	}

	job* before = *j;
	for (job* Jptr = *j; Jptr; Jptr = Jptr -> next) {
		for (process* Pptr = Jptr->first; Pptr; Pptr = Pptr -> next) {
			if (Pptr -> state == STOPPED) {
				Jptr -> state = STOPPED;
				break;
			} else if (Pptr -> state == RUNNING) {
				Jptr -> state = RUNNING;
			} else if ((Pptr -> state == EXITED || Pptr -> state == TERMINATED) && Pptr -> next == NULL) {
				Jptr -> state = EXITED;
			}
		}
		if (Jptr -> state == EXITED) {
			if(Jptr == *j) {
				*j = Jptr -> next;
				removeJob(Jptr);
				free(Jptr);
			} else {
				before -> next = Jptr -> next;
				removeJob(Jptr);
				free(Jptr);
				Jptr = before;
			}
		}
		before = Jptr;
	}
}

void removeJob(job* j){
	removeProc(j -> first);
}
void removeProc(process* p){
	if(p == NULL) return;
	for(int i = 0; p -> Det.cmd[i] != NULL; i++){
		free(p -> Det.cmd[i]);
	}
	free(p -> Det.InOut[0]);
	free(p -> Det.InOut[1]);
	removeProc(p -> next);
	free(p);
}


void update_process_status(job* j, pid_t curr_pid, int status){
	process* p = j -> first;
	while(p != NULL){
		if(p -> pid == curr_pid){
			if(WIFEXITED(status) == 1){
				p -> state = EXITED;//если закончился
				p -> result = WEXITSTATUS(status);
			}
			if(WIFCONTINUED(status) == 1){
				p -> state = RUNNING;//если продолжился
			}
			if(WIFSIGNALED(status) == 1){
				
				p -> state = TERMINATED;//если убит сигналом
				p -> result = 1;
			}
			if(WIFSTOPPED(status) == 1){
				p -> state = STOPPED;//если был остановлен
				p -> result = 1;
			}
			return;
		}
		p = p -> next;
	}
}

void execute(queue* cmds, job** buf){
	queue* curr = cmds;
	while(curr != NULL){
		//1 = ; 2 = || 0 = &&  4 = &
		int res = executeConveyor(curr, buf);
		if(curr -> flag == 1 || curr -> flag == res || curr -> flag == 3){
			curr = curr -> next;
			continue;
		}
		break;
	}
}

int executeConveyor(queue* cmds, job** buf){
	job* j = creatingJob(cmds);
	if(j -> first -> next == NULL && j -> status != 1){
			int res = Infunct(j, j -> first, *buf);
			if (res != 2){
				return res;
			}
	}
	int fd[2], outfile, infile = STDIN_FILENO;
	for(process* curr = j -> first; curr != NULL; curr = curr -> next){
		if(curr -> next != NULL){
			if(pipe(fd) == -1){
				perror("pipe");
				return 2;
			}
			outfile = fd[1];
		}else{
			outfile = STDOUT_FILENO;
		}

		pid_t curr_pid = fork();
		if(curr_pid == -1){
			perror("fork");
			return 2;
		}else if(curr_pid == 0){
			setpgid(0, j -> pgid);
			if (j -> pgid == 0) {
				j -> pgid = getpid();
			}
			if(infile != STDIN_FILENO){
				if(dup2(infile, STDIN_FILENO) == -1){
					perror("dup2");
					return 2;
				}
				close(fd[0]);
			}
			if(outfile != STDOUT_FILENO){
				if(dup2(fd[1], STDOUT_FILENO) == -1){
					perror("dup2");
					return 2;
				}
				if(curr -> Det.ef == 1){
					if(dup2(fd[1], STDERR_FILENO) == -1){
						perror("dup2");
						return 2;
					}
				}
				close(fd[1]);
			}
			launchProcess(*buf, curr, j);
		}

		if(j -> pgid == 0){
			j -> pgid = curr_pid;
		}
		curr -> pid = curr_pid;
		setpgid(curr_pid, j -> pgid);
		if(infile != STDIN_FILENO){
			close(infile);
		}
		if(outfile != STDOUT_FILENO){
			close(outfile);
		}
		infile = fd[0];
	}
	int status;
	if(j -> status == 0){
		while(1){
			pid_t pid = waitpid(-(j -> pgid), &status, WUNTRACED);
			if(pid < 0 && errno == ECHILD) break;
			else if(pid < 0){
				perror("waitpid: ");
				return 1;
			}
			update_process_status(j, pid, status);
			if (WIFSTOPPED(status) == 1) {
				addJob(buf, j);
				tcsetpgrp(STDIN_FILENO, getpid());
				putchar('\n');
				return 1;
			} else if (WIFSIGNALED(status) == 1) {
				if (WTERMSIG(status) == SIGINT) putchar('\n');
				else if (WTERMSIG(status) == SIGKILL) printf("Убито\n");
				else if (WTERMSIG(status) == SIGTERM) printf("Завершено\n");
				else if (WTERMSIG(status) == SIGQUIT) printf("Выход (образ памяти сброшен на диск\n");
			}
		}
		tcsetpgrp(STDIN_FILENO, getpid());
		return j -> last -> result;
	}else{
		j -> state = RUNNING;
		addJob(buf, j);
		printf("[%d] %d\n", j -> index, j -> pgid);
	}
	return 0;
}

void launchProcess(job* buf, process* curr, job* j){
	if (j -> status == 0) {
		tcsetpgrp(STDIN_FILENO, j -> pgid);
	}
	jc_flag = 0;
	int res = Infunct(j, curr, buf);
	if (res != 2) {
		exit(res);
	}
    	fileDescriptors(curr);
        signal(SIGINT, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTERM, SIG_DFL);
	execvp(curr -> Det.cmd[0], curr -> Det.cmd);
	perror("execvp");
	exit(2);
}

int Infunct(job* newj, process* curr, job* j){
	if (strcmp(curr -> Det.cmd[0], "exit") == 0) {//конец работы баша
		exit2(j);
	}
	if (strcmp(curr -> Det.cmd[0], "jobs") == 0) {
		jobs(j);
		return 0;
	}
	if (strcmp(curr -> Det.cmd[0], "fg") == 0) {
		if (jc_flag == 0) {
			perror("fg");
			return 1;
		}
		if (fg(j, newj) == 0) {
			return 0;
		} else {
			return 1;
		}
	}
	if (strcmp(curr -> Det.cmd[0], "bg") == 0) {
		if (jc_flag == 0) {
			perror("fg");
			return 1;
		}
		if (bg(j, newj) == 0) {
			return 0;
		} else {
			return 1;
		}
	}
	if (strcmp(curr -> Det.cmd[0], "cd") == 0) {
		if (cd(curr) == 0) {
			return 0;
		} else {
			return 1;
		}
	}
	if (strcmp(curr -> Det.cmd[0], "kill") == 0) {
		if (kill2(newj) == 0) {
			return 0;
		} else {
			return 1;
		}
	}

	return 2;
}

int fg(job* jc, job* j){
	if (jc == NULL) {
		printf("fg: текущий: нет такого задания\n");
		return 1;
	}

	if (j->first->Det.cmd[1] != NULL) {
		int digit = atoi(j->first->Det.cmd[1]);
		while (jc->index != digit) {
			jc = jc -> next;
			if (jc == NULL) {
		 		printf("fg: %d: нет такого задания\n", digit);
				return 1;
			}
		}

	}

	tcsetpgrp(STDIN_FILENO, jc -> pgid);
	kill(-(jc -> pgid), SIGCONT);
	int status;
	while(1){
		pid_t pid = waitpid(-(jc -> pgid), &status, WUNTRACED);
		if(pid < 0 && errno == ECHILD) break;
		else if(pid < 0){
			perror("waitpid");
			break;
		}
		update_process_status(jc, pid, status);
	}
	tcsetpgrp(STDIN_FILENO, getpid());
	return jc->last->result;
}

int bg(job* jc, job* j){
	if (jc == NULL) {
		printf("bg: текущий: нет такого задания\n");
		return 1;
	}
	if (j->first->Det.cmd[1] != NULL) {
		int digit = atoi(j->first->Det.cmd[1]);
		while (jc->index != digit) {
			jc = jc -> next;
			if (jc == NULL) {
				printf("bg: %d: нет такого задания\n", digit);
				return 1;
			}
		}
	}
	kill(-(jc -> pgid), SIGCONT);
	printf("[%d] %s\n", jc -> index, jc -> cmd_name);
	return 0;
}

void jobs(job* j){
	if(j == NULL) return;
	while(j != NULL){
		if(j -> state == EXITED || j -> state == TERMINATED){
			printf("[%d] Завершен %s\n", j -> index, j -> cmd_name);
		}else if(j -> state == STOPPED){
			printf("[%d] Остановлен %s\n", j -> index, j -> cmd_name);
		}else if(j -> state == RUNNING){
			printf("[%d] Запущен %s\n", j -> index, j -> cmd_name);
		}
		j = j -> next;
	}
}

int cd(process* p){
	int res  = chdir(p -> Det.cmd[1]);
	if(res != 0){
		printf("cd: %s: Нет такого файла или каталога\n", p -> Det.cmd[1]);
		return 1;
	}
	return 0;
}

int kill2(job* j){
	for(int i = 0; signals[i] != NULL; i++){
		if(strcmp(j -> first -> Det.cmd[1], signals[i]) == 0){
			int number = atoi(j -> first -> Det.cmd[2]);
			int res = kill(number, i + 1);
			if(res == -1){
				perror("kill");
				return 1;
			} else {
				return 0;
			}
		}
	}
	printf("%s такого сигнала не существует\n", j -> first -> Det.cmd[1]);
	return 1;
}

void exit2(job* begin){
	for(job* j = begin; j;){
		kill(-(j -> pgid), SIGHUP);
		deleting(j -> first);
		job* temp = j;
		j = j -> next;
		free(temp -> cmd_name);
		free(temp);
	}
	remove("history");
	raise(SIGHUP);
}

void deleting(process* b){
	if(b == NULL) return;
	process* temp = b;
	for(int i = 0; b -> Det.cmd[i] != NULL; i++)
		free(b -> Det.cmd[i]);
	free(b -> Det.InOut[0]);
	free(b -> Det.InOut[1]);
	deleting(b -> next);
	free(temp);
}

void help(job* j){
}


void history(){
	FILE* fptr = fopen("history", "r");
	int i = 1;
	while(!feof(fptr)){
		char s[45];
		fscanf(fptr, "%s", s);
		if(strcmp(s, "\n") == 0) break;
		printf(" %d  %s\n", i, s);
		i++;
	}
	fclose(fptr);
}


void wait2(job* j){
}

void fileDescriptors(process* proc){
	if(proc -> Det.rf == 1){
		int Read = open(proc -> Det.InOut[1], O_RDONLY);
		if(Read == -1){
			perror("open");
			exit(2);
		}
		if(dup2(Read, STDIN_FILENO) == -1){
			perror("dup2");
			exit(2);
		}
		close(Read);
	}
	if(proc -> Det.wf == 1 || proc -> Det.wf == 3){
		int Write = open(proc -> Det.InOut[0], O_WRONLY | O_CREAT | O_TRUNC, 0660);
		if(Write == -1){
			perror("open");
			exit(2);
		}
		if(dup2(Write, STDOUT_FILENO) == -1){
			perror("dup2");
			exit(2);
		}
		if(proc -> Det.wf == 3){
			if(dup2(Write, STDERR_FILENO) == -1){
				perror("dup2");
				exit(2);
			}
		}
		close(Write);
	}
	if(proc -> Det.wf == 2 || proc -> Det.wf == 4){
		int Write = open(proc -> Det.InOut[0], O_WRONLY | O_CREAT | O_APPEND, 0660);
		if(Write == -1){
			perror("open");
			exit(2);
		}
		if(dup2(Write, STDOUT_FILENO) == -1){
			perror("dup2");
			exit(2);
		}
		if(proc -> Det.wf == 4){
			if(dup2(Write, STDERR_FILENO) == -1){
				perror("dup2");
				exit(2);
			}
		}
		close(Write);
	}
}

void addJob(job** jobbuf, job* newJob){
	if(*jobbuf == NULL){
		*jobbuf = newJob;
		newJob -> index = 1;
		return;
	}
	job* j = *jobbuf;
	while(j -> next != NULL) j = j -> next;
	j -> next = newJob;
	newJob -> index = (j -> index) + 1;
}

job* creatingJob(queue* cmds){
	job* newJob = (job*) calloc(1, sizeof(job));
	newJob -> index = 0;
	newJob -> cmd_name = get_name(cmds -> commands);
	if (cmds -> flag == 3) {
		newJob -> status = 1;
	} else {
		newJob -> status = 0;
	}
	conveyor* curr = cmds -> commands;
	while(curr != NULL){
		pushbackProc(&(newJob -> first), &(newJob -> last), curr);
		curr = curr -> next;
	}
	return newJob;
}

void pushbackProc(process** first, process** last, conveyor* cmd){
	if(*first == NULL){
		*first = CreatingProc(cmd, 1);
		*last = *first;
		return;
	}
	(*last) -> next = CreatingProc(cmd, ((*last) -> index) + 1);
	*last = (*last) -> next;
}

process* CreatingProc(conveyor* conv, unsigned num){
	process* newProc = (process*) calloc(1, sizeof(process));

	newProc -> index = num;
	newProc -> Det.cmd = conv -> Node.cmd;
	newProc -> Det.InOut = conv -> Node.InOut;
	newProc -> Det.rf = conv -> Node.rf;
	newProc -> Det.wf = conv -> Node.wf;
	newProc -> Det.ef = conv -> Node.ef;
	newProc -> next = NULL;
	return newProc;
}

char* get_name(conveyor* cmd){
	unsigned n = SIZE_NAME_CMDS;
	unsigned pos = 0;
	char* name = (char*) calloc(n, sizeof(char));
	while(cmd != NULL){
		for(int i = 0; cmd -> Node.cmdName[i] != NULL; i++){
			for(int j = 0; cmd -> Node.cmdName[i][j] != '\0'; j++){
				if(pos >= n){
					n *= 2;
					name = realloc(name, n * sizeof(char));
				}
				name[pos] = cmd -> Node.cmdName[i][j];
				pos++;
			}
			name[pos] = ' ';
			pos++;
		}
		cmd = cmd -> next;
	}
	return name;
}
