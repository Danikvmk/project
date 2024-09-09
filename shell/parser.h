#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

struct simpleCommand{
	char** cmd;
	char** InOut;
	int rf, wf, ef;
	char** cmdName;
	//rf = 1 <
	//er = 1 |&
	//wr = 1 > | wr = 2 >> | wr = 3 >& | wr = 4 &>>
};
typedef struct conveyor{
	struct simpleCommand Node;
	struct conveyor* next;
}conveyor;
typedef struct queue{
	struct conveyor* commands;
	int flag;// 1 = ';'  2 = '||'  0 = '&&'  3 = '&'
	struct queue* next;
}queue;

queue* get_commands(char**);
void push_back1(queue**, char**, int*);
queue* CreatingQueue(char**, int*);
void push_back2(conveyor**, char**, int*);
conveyor* CreatingNode(char**, int*);

char* get_string();
char** split(char*);
