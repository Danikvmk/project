#include "parser.h"
#include "execute.h"

const char* terminal = "\e[1;31m new string:\e[m\t";

void run() {
    signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	signal(SIGINT, SIG_IGN);
    job* buf = NULL;
	while(1){
		printf("%s", terminal);
		char* string = get_string();
		char** cmd = split(string);
		check(&buf);
		queue* cmds = get_commands(cmd);
		execute(cmds, &buf);
		check(&buf);
	}
}