#include "parser.h"
#define STRING_SIZE 64
#define COMMAND_SIZE 8

const char* cmd_enders[] = {"|", "|&", "<", ">", ">>", ">&", "&>", "&>>"};
const char* convey_enders[] = { "&&", ";", "||", "&"};

conveyor* CreatingNode(char** strings, int* row){
	conveyor* temp = (conveyor*) calloc(1, sizeof(conveyor));
	temp -> next = NULL;
	int n = 2, words = 2;
	int j = 0;

	temp -> Node.cmd = (char**) calloc(n, sizeof(char*));
	temp -> Node.InOut = (char**) calloc(2, sizeof(char*));
	temp -> Node.cmdName = (char**) calloc(words, sizeof(char*));
	while(strings[*row] != NULL){
		int i = 0;
		for(; convey_enders[i] != NULL; i++){
			if(strcmp(convey_enders[i], strings[*row]) == 0){
				return temp;
			}
		}

		temp -> Node.cmdName[words - 2] = (char*) calloc(strlen(strings[*row]), sizeof(char));
		strcpy(temp -> Node.cmdName[words - 2], strings[*row]);

		words++;
		temp -> Node.cmdName = realloc(temp -> Node.cmdName, words * sizeof(char*));

		i = 0;
		for(; i < 8; i++){
			if(strcmp(strings[*row], cmd_enders[i]) == 0){
				if(i == 0){
					(*row)++;
					return temp;
				}else if(i == 1){//|&
					temp -> Node.ef = 1;
					(*row)++;
					return temp;
				}else if(i == 2){//<
					(*row)++;
					if(temp -> Node.InOut[1] != NULL){
						free(temp -> Node.InOut[1]);
					}
					temp -> Node.InOut[1] = (char*) calloc(strlen(strings[*row]), sizeof(char));
					strcpy(temp ->  Node.InOut[1], strings[*row]);

					temp -> Node.cmdName[words - 2] = (char*) calloc(strlen(strings[*row]), sizeof(char));
					strcpy(temp -> Node.cmdName[words - 2], strings[*row]);
					words++;
					temp -> Node.cmdName = realloc(temp -> Node.cmdName, words * sizeof(char*));

					(*row)++;
					temp -> Node.rf = 1;
					break;
				}
				if(temp -> Node.wf == 2){
					int file = open(temp -> Node.InOut[0], O_WRONLY | O_CREAT | O_APPEND, 0660);
					close(file);
					free(temp -> Node.InOut[0]);
				}else if(temp -> Node.wf == 1){
					int file = open(temp -> Node.InOut[0], O_WRONLY | O_CREAT | O_TRUNC, 0660);
					close(file);
					free(temp -> Node.InOut[0]);
				}
				if(i == 3){//>
					temp -> Node.wf = 1;
				}else if(i == 4){//>>
					temp -> Node.wf = 2;
				}else if(i == 5 || i == 6){//>& &>
					temp -> Node.wf = 3;
				}else if(i == 7){//&>>
					temp -> Node.wf = 4;
				}
				(*row)++;
				temp -> Node.InOut[0] = (char*) calloc(strlen(strings[*row]), sizeof(char));
				strcpy(temp -> Node.InOut[0], strings[*row]);

				temp -> Node.cmdName[words - 2] = (char*) calloc(strlen(strings[*row]), sizeof(char));
				strcpy(temp -> Node.cmdName[words - 2], strings[*row]);
				words++;
				temp -> Node.cmdName = realloc(temp -> Node.cmdName, words * sizeof(char*));

				(*row)++;
				break;
			}
		}
		if(i != 8){
			continue;
		}
		temp -> Node.cmd[j] = (char*) calloc(strlen(strings[*row]), sizeof(char));
		strcpy(temp -> Node.cmd[j], strings[*row]);
		(*row)++;
		j++;
		n++;
	}
	temp -> Node.cmd[j] = NULL;
	return temp;
}

void push_back2(conveyor** conv, char** strings, int* row){
	if(*conv == NULL){
		*conv = CreatingNode(strings, row);
		return;
	}
	conveyor* curr = *conv;
	while(curr -> next != NULL){
		curr = curr -> next;
	}
	curr -> next = CreatingNode(strings, row);
}

queue* CreatingQueue(char** strings, int* row){
	queue* temp = (queue*) calloc(1, sizeof(queue));
	temp -> next = NULL;
	while(strings[*row] != NULL){
		push_back2(&(temp -> commands), strings, row);
		if(strings[*row] != NULL){
			for(int i = 0; convey_enders[i] != NULL; i++){
				if(strcmp(strings[*row], convey_enders[i]) == 0){
					temp -> flag = i;
					(*row)++;
					return temp;
				}
			}
		}else{
			break;
		}
	}
	return temp;
}

void push_back1(queue** cmdSet, char** strings, int* row){
	if(*cmdSet == NULL){
		*cmdSet = CreatingQueue(strings, row);
		return;
	}
	queue* curr = *cmdSet;
	while(curr -> next != NULL){
		curr = curr -> next;
	}
	curr -> next = CreatingQueue(strings, row);
}

queue* get_commands(char** strings){
	unsigned row = 0;
	queue* temp = NULL;
	while(strings[row] != NULL){
		push_back1(&temp, strings, &row);
	}
	return temp;
}

char* get_string(){
	int n = STRING_SIZE;
	char* str = (char*) calloc(n, sizeof(char));
	char c;
	int i = 0;
	while((c = getchar()) != '\n'){
		if(i >= n){
			n *= 2;
			str = realloc(str, n * sizeof(char));
		}
		if(c == 39 || c == '"'){
			char a = c;
			str[i] = c;
			i++;
			while((c = getchar()) != a){
				if(i >= n){
					n *= 2;
					str = realloc(str, n * sizeof(char));
				}
				str[i] = c;
				i++;
			}
		}
		str[i] = c;
		i++;
	}
	return str;
}

char** split(char* string){
	int n = 2, i = 0, j = 0;
	char** temp = (char**) calloc(n, sizeof(char*));
	while(string[i] != '\0'){
		while(string[i] == ' ' || string[i] == '\t') i++;
		int size = COMMAND_SIZE, k = 0;
		temp = realloc(temp, n * sizeof(char*));
		temp[j] = (char*) calloc(size, sizeof(char));
		while(string[i] != ' ' && string[i] != '\0' && string[i] != '\t'){
			if(string[i] == '|' || string[i] == '&'){
				if(string[i - 1] != ' ' && string[i - 1] != '\t'){
					i--;
					string[i] = ' ';
					break;
				}
				temp[j][k] = string[i];
				k++;
				if(string[i + 1] == '|' || string[i + 1] == '&'){
					i++;
					temp[j][k] = string[i];
				}else if(string[i + 1] == '>'){
					i++;
					temp[j][k] = string[i];
					k++;
					if(string[i + 1] == '>'){
						i++;
						temp[j][k] = string[i];
					}
				}
				break;
			}
			if(string[i] == ';'){
				if(string[i - 1] != ' ' && string[i - 1] != '\t'){
					i--;
					string[i] = ' ';
					break;
				}
				temp[j][k] = string[i];
				break;
			}
			if(string[i] == '>' || string[i] == '<'){
				if(string[i - 1] != ' ' && string[i - 1] != '\t'){
					i--;
					string[i] = ' ';
					break;
				}
				temp[j][k] = string[i];
				k++;
				if(string[i + 1] == '>'){
					i++;
					temp[j][k] = string[i];
					k++;
				}
				else if(string[i + 1] == '&'){
					i++;
					temp[j][k] = string[i];
				}
				break;
			}
			if(string[i] == '"' || string[i] == 39){
				char a = string[i];
				i++;
				while(string[i] != a){
					if(k >= size){
						size *= 2;
						temp[j] = realloc(temp[j], size * sizeof(char));
					}
					temp[j][k] = string[i];
					k++;
					i++;
				}
				break;
			}
			if(k >= size){
				size *= 2;
				temp[j] = realloc(temp[j], size * sizeof(char));
			}
			temp[j][k] = string[i];
			k++;
			i++;
		}
		i++;
		j++;
		n++;
	}
	temp[j] = NULL;
	return temp;
}
