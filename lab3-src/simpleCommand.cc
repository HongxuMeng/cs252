#include <cstdio>
#include <cstdlib>
#include <string>
#include <string.h>
#include <iostream>
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>
//#include "simpleCommand.hh"
#include "shell.hh"

using namespace std; 
SimpleCommand::SimpleCommand() {
	_arguments = std::vector<std::string *>();
}

SimpleCommand::~SimpleCommand() {
	// iterate over all the arguments and delete them
	for (auto & arg : _arguments) {
		delete arg;
	}
}

char* SimpleCommand::chkExp(char *argument) {
	if (!strcmp(argument, "${?}")) {
		char *rst = strdup(Shell::_currentCommand._strEnv.q.c_str());
		return rst;
	}
	else if (!strcmp(argument, "${!}")) {
		if (Shell::_currentCommand._strEnv.bang.length()) {
			char *rst = strdup(Shell::_currentCommand._strEnv.bang.c_str());
			return rst;
		}
		else {
			return "";
		}
	}
	else if (!strcmp(argument, "${_}")) {
		char *rst = strdup(Shell::_currentCommand._strEnv.us.c_str());
		return rst;
	}
	char *arg = strdup(argument), *chkD = strchr(arg, '$'), *chkLB = strchr(arg, '{'), *del = arg;
	if (chkD&&chkLB) {
		char *str = (char *)malloc(sizeof(arg) + 1024), *tmp = str;
		for (; *arg != '$'; *(tmp++) = *(arg++));
		*tmp = '\0';
		while (chkD) {
			if (chkD[1] == '{'&&chkD[2] != '}') {
				char *var = chkD + 2, *env = (char*)malloc(1024), *tEnv = env;
				for (; *var != '}'; *(tEnv++) = *(var++));
				*tEnv = '\0';
				//cout << "1.5: " << env << endl;
				//cout << "get: " << getenv(env) << endl;
				char *val = getenv(env);
				free(env);
				if(val) strcat(str, val);
				for (; *(arg - 1) != '}'; ++arg);
				char *m = (char*)malloc(1024), *tM = m;
				for (; *arg != '$'&&*arg; *(tM++) = *(arg++));
				*tM = '\0';
				strcat(str, m);
			}
			chkD++;
			chkD = strchr(chkD, '$');
		}
		char *rst = strdup(str);
		free(del);
		return rst;
	}
	free(del);
	return NULL;
}

char* SimpleCommand::tilde(char* argument) {
	if (*argument == '~') {
		if (strlen(argument) == 1) {
			argument = strdup(getenv("HOME"));
			return argument;
		}
		else {
			if (argument[1] == '/') {
				char *dir = strdup(getenv("HOME"));
				++argument;
				argument = strcat(dir, argument);
				return argument;
			}
			char *na = (char*)malloc(strlen(argument) + 32), *un = (char*)malloc(64), *usr = un, *tmp = argument;
			++tmp;
			for (; *tmp != '/'&&*tmp; *(un++) = *(tmp++));
			*un = '\0';
			if (*tmp) {
				na = strcat(getpwnam(usr)->pw_dir, tmp);
				argument = strdup(na);
				return argument;
			}
			else {
				argument = strdup(getpwnam(usr)->pw_dir);
				return argument;
			}
		}
	}
	return NULL;
}

void SimpleCommand::insertArgument(std::string * argument) {
	// simply add the argument to the vector
	char *str = strdup(argument->c_str()), *argm = chkExp(str);
	if (argm) {
		delete argument;
		argument = new string(argm);
	}
	free(str);
	free(argm);
	str = strdup(argument->c_str());
	argm = tilde(str);
	if (argm) {
		delete argument;
		argument = new string(argm);
	}
	free(str);
	free(argm);
	_arguments.push_back(argument);

}

// Print out the simple command
void SimpleCommand::print() {
	for (auto & arg : _arguments) {
		std::cout << "\"" << *arg << "\" \t";
	}
	// effectively the same as printf("\n\n");
	std::cout << std::endl;
}
