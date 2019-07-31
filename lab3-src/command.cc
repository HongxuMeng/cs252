/*
 * CS252: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 * DO NOT PUT THIS PROJECT IN A PUBLIC REPOSITORY LIKE GIT. IF YOU WANT 
 * TO MAKE IT PUBLICALLY AVAILABLE YOU NEED TO REMOVE ANY SKELETON CODE 
 * AND REWRITE YOUR PROJECT SO IT IMPLEMENTS FUNCTIONALITY DIFFERENT THAN
 * WHAT IS SPECIFIED IN THE HANDOUT. WE OFTEN REUSE PART OF THE PROJECTS FROM  
 * SEMESTER TO SEMESTER AND PUTTING YOUR CODE IN A PUBLIC REPOSITORY
 * MAY FACILITATE ACADEMIC DISHONESTY.
 */

#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h> 
#include <sys/wait.h>
#include <libgen.h>
#include <fcntl.h>

#include "command.hh"
#include "shell.hh"
extern char **environ;
extern void mysource(FILE *fp);
extern vector<string> delFIFO;
#define YY_BUF_SIZE 32768

bool onError = false;

using namespace std;

Command::Command() {
    // Initialize a new vector of Simple Commands
    _simpleCommands = std::vector<SimpleCommand *>();

    _outFile = NULL;
    _inFile = NULL;
    _errFile = NULL;
	_strEnv.q = "0";
	_strEnv.bang = "";
	_strEnv.us = "LD_LIBRARY PATH";
	char *rst=(char*)malloc(256*sizeof(char)), *path;
	ssize_t count = readlink("/proc/self/exe", rst, 255);
	if (count != -1) {
		path = dirname(rst);
		string tmp = string(path);
		tmp += "/shell";
		free(rst);
		rst = strdup(tmp.c_str());
	}
	setenv("SHELL", rst, 1);
	_strEnv.sh = string(rst);
	free(rst);
    _background = 0;
	_append = 0;
	_oCount = 0;
	_iCount = 0;
}

void Command::insertSimpleCommand( SimpleCommand * simpleCommand ) {
    // add the simple command to the vector
    _simpleCommands.push_back(simpleCommand);
}

void Command::clear() {
	size_t scs = _simpleCommands.size();
    if (!_background&&scs>0) {
		size_t scas = _simpleCommands[scs - 1]->_arguments.size();
		_strEnv.us = string(_simpleCommands[scs - 1]->_arguments[scas - 1]->c_str());
	}
	for(int i=0;i<delFIFO.size();++i){
		unlink(delFIFO[i].c_str());
		string folder=delFIFO[i].c_str();
		folder[folder.length()-2]='\0';
		rmdir(folder.c_str());
	}
	delFIFO.clear();
	/*
	if(scs>0){
		size_t scas = _simpleCommands[scs - 1]->_arguments.size();
		for(int i=0;i<scas;++i){
			if(_simpleCommands[scs - 1]->_arguments[i]->find("tmpZGY/file")>=0){
				unlink(_simpleCommands[scs - 1]->_arguments[i]->c_str());
			}
		}
	}
	int del=rmdir("tmpZGY");*/
	// deallocate all the simple commands in the command vector
    for (auto simpleCommand : _simpleCommands) {
        if(simpleCommand) delete simpleCommand;
    }

    // remove all references to the simple commands we've deallocated
    // (basically just sets the size to 0)
    _simpleCommands.clear();

    if ( _outFile ) {
	if( _outFile == _errFile ) _errFile=NULL;
        delete _outFile;
    }
    _outFile = NULL;

    if ( _inFile ) {
        delete _inFile;
    }
    _inFile = NULL;

    if ( _errFile ) {
        delete _errFile;
    }
	

    _errFile = NULL;
	_outFile = NULL;
	_inFile = NULL;
	_errFile = NULL;
	_background = 0;
	_append = 0;
	_oCount = 0;
	_iCount = 0;
}

void Command::print() {
    printf("\n\n");
    printf("              COMMAND TABLE                \n");
    printf("\n");
    printf("  #   Simple Commands\n");
    printf("  --- ----------------------------------------------------------\n");

    int i = 0;
    // iterate over the simple commands and print them nicely
    for ( auto & simpleCommand : _simpleCommands ) {
        printf("  %-3d ", i++ );
        simpleCommand->print();
    }

    printf( "\n\n" );
    printf( "  Output       Input        Error        Background\n" );
    printf( "  ------------ ------------ ------------ ------------\n" );
    printf( "  %-12s %-12s %-12s %-12s\n",
            _outFile?_outFile->c_str():"default",
            _inFile?_inFile->c_str():"default",
            _errFile?_errFile->c_str():"default",
            _background?"YES":"NO");
    printf( "\n\n" );
}

int Command::BuiltIn(int i) {
	char *str = strdup(_simpleCommands[i]->_arguments[0]->c_str());
	if (!strcmp(str, "setenv")) {
		//_strEnv.q = "0";
		int err = setenv(_simpleCommands[i]->_arguments[1]->c_str(), _simpleCommands[i]->_arguments[2]->c_str(), 1);
		if (err) {
			//_strEnv.q = "1";
			perror("setenv");
		}
		clear();
		free(str);
		Prompt();
		return 1;
	}
	if (!strcmp(str, "unsetenv")) {
		//_strEnv.q = "0";
		int err = unsetenv(_simpleCommands[i]->_arguments[1]->c_str());
		if (err) {
			//_strEnv.q = "1";
			perror("unsetenv");
		}
		clear();
		free(str);
		Prompt();
		return 1;
	}
	if (!strcmp(str, "cd")) {
		int err;
		if (_simpleCommands[i]->_arguments.size() == 1) {
			err = chdir(getenv("HOME"));
		}
		else {
			char *tgt = strdup(_simpleCommands[i]->_arguments[1]->c_str());
			err = chdir(tgt);
			free(tgt);
		}
		if (err < 0) {
			string scd = string("cd: can't cd to "), sarg = string(_simpleCommands[i]->_arguments[1]->c_str());
			string serr = scd + sarg;
			perror(serr.c_str());
			//_strEnv.q = "1";
		}
		else {
			//_strEnv.q = "0";
		}
		clear();
		free(str);
		Prompt();
		return 1;
	}
	free(str);
	return 0;
}

void Command::fdClear() {
	for (int i = 3; i < 1024; ++i) {
		if (fcntl(i, F_GETFD) != -1 || errno != EBADF) {
			close(i);
		}
	}
}

void Command::execute() {
	// Don't do anything if there are no simple commands
	if (_simpleCommands.size() == 0) {
		Prompt();
		return;
	}
	char *str = strdup(_simpleCommands[0]->_arguments[0]->c_str());
	if (!strcmp(str, "exit")) {
		fdClear(); 
		if(isatty(0)) printf("Good bye!!\n");
		exit(1);
	}
	if (_iCount > 1 || _oCount > 1) {
		printf("Ambiguous output redirect.\n");
		clear();
		Prompt();
		return;
	}
	int tIn = dup(0), tOut = dup(1), tErr = dup(2), fIn, fOut, fErr, PID;
	fIn = _inFile ? open(_inFile->c_str(), O_RDONLY) : dup(tIn);
	fErr = _errFile ? open(_errFile->c_str(), (_append ? O_WRONLY | O_APPEND | O_CREAT : O_WRONLY | O_CREAT | O_TRUNC), 0664) : dup(tErr);
	for (size_t i = 0; i < _simpleCommands.size(); ++i) {
		if (BuiltIn(i)) return;
		else if (!strcmp(_simpleCommands[i]->_arguments[0]->c_str(), "source")) {
			FILE *fp = fopen(_simpleCommands[i]->_arguments[1]->c_str(), "r");
			if (!fp) {
				dup2(tIn, 0);
				dup2(tOut, 1);
				dup2(tErr, 2);
				if (i + 1 != _simpleCommands.size()) close(fIn);
				perror("source");
				return;
			}
			mysource(fp);
		}
		else {
			dup2(fIn, 0);
			close(fIn);
			dup2(fErr, 2);
			close(fErr);
			if (i == _simpleCommands.size() - 1) fOut = _outFile ? open(_outFile->c_str(), (_append ? O_WRONLY | O_APPEND | O_CREAT : O_WRONLY | O_CREAT | O_TRUNC), 0664) : dup(tOut);
			else {
				int fdPip[2];
				pipe(fdPip);
				fOut = fdPip[1];
				fIn = fdPip[0];
			}
			dup2(fOut, 1);
			close(fOut);
			PID = fork();
			if (_background) {
				_strEnv.bang = to_string(PID);
			}
			if (PID < 0) {
				perror("fork");
				close(fIn);
				exit(2);
			}
			if (!PID) {
				char *zstr = strdup(_simpleCommands[i]->_arguments[0]->c_str());
				if (!strcmp(zstr, "printenv")) {
					char **env = environ;
					while (*env) {
						printf("%s\n", *env);
						env++;
					}
					close(fIn);
					exit(1);
				}
				int sz = _simpleCommands[i]->_arguments.size();
				char **arr = new char*[sz];
				int x;
				for (x = 0; x < sz; ++x) {
					arr[x] = strdup(_simpleCommands[i]->_arguments[x]->c_str());
				}
				arr[x] = NULL;
				execvp(_simpleCommands[i]->_arguments[0]->c_str(), arr);
				perror("execvp");
				fflush(stdout);
				free(zstr);
				close(fIn);
				for (x = 0; x < sz; ++x) {
					free(arr[x]);
				}
				delete arr;
				_exit(1);
				free(zstr);
			}
		}
	}
	free(str);
	dup2(tIn, 0);
	dup2(tOut, 1);
	dup2(tErr, 2);
	close(tIn);
	close(tOut);
	close(tErr);
	close(fIn);
	if (!_background) {
		int wret;
		waitpid(PID, &wret, 0);
		if(WIFEXITED(wret))
			_strEnv.q = to_string(WEXITSTATUS(wret));
	}
    clear();
	Prompt();
}

void Command::Prompt() {
	char* prompt = getenv("PROMPT");
	char* error = getenv("ON_ERROR");
	bool printErr = (Shell::_currentCommand._strEnv.q.compare("0"));
	if (isatty(0) && prompt == NULL) {
		printf("myshell>");
		fflush(stdout);
	}
	if (isatty(0) && prompt && printErr) {
		printf("%s\n", error);
		fflush(stdout);
	}
	if (isatty(0) && prompt) {
		printf("%s", prompt);
		fflush(stdout);
	}
}

SimpleCommand * Command::_currentSimpleCommand;
