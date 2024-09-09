#pragma once

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdlib.h>
#include "parser.h"
#include <errno.h>

typedef enum State{
	NEUTRAL, EXITED, TERMINATED, RUNNING, STOPPED
} State;

typedef struct procDet{
	char** cmd;
	char** InOut;
	int rf, wf, ef;//rf = 1 <
		       //ef = 1 |&
		       //wr = 1 >   wr = 2 >>   wr = 3 >&   wr = 4 &>>
}procDet;

typedef struct process{
	State state;
	pid_t pid;
	unsigned index;
	int result;
	procDet Det;
	struct process* next;
}process;
typedef struct job{
	process *first;
	process *last;
	char* cmd_name;
	unsigned index;
	State state;
	unsigned status;//0 fg   1 bg
	pid_t pgid;
	struct job *next;
}job;

int fg(job*, job*);
int bg(job*, job*);
int cd(process*);
int kill2(job*);
void exit2(job*);
void deleting(process*);

void help(job*);
void history();
void jobs(job*);

int Infunct(job*, process*, job*);

void execute(queue*, job**);
int executeConveyor(queue*, job**);
void fileDescriptors(process*);
void launchProcess(job*, process*, job*);
void update_process_status(job*, pid_t, int);
void check(job**);

void removeJob(job*);
void removeProc(process*);

void addJob(job**, job*);
job* creatingJob(queue*);
char* get_name(conveyor*);
void pushbackProc(process**, process**, conveyor*);
process* CreatingProc(conveyor*, unsigned);
