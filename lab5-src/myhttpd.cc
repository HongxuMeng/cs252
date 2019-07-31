//implement httpd server
//401 Unauthorized The_Great_Realm_of_CS252
// WWW-Authenticate: Basic realm="The_Great_Realm_of_CS252"
// username:password Authorization header

//const char * usage;

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;

typedef struct dirF;
{
 struct dirent *dir;
 char *Path;
 char *Dpath;
 struct stat file_At;
}dirF;

pthread_mutex_t mutex;
pthread_mutexattr_t mattr;
time_t startime;
string contentType;

int debug=1;
int QueueLength=5;
//global var for mode 1 for -f; 2 for -t; 3 for -p; 0 for basic
int mode=0;
int port=0;
int cmp_flag=0;//flag for compare helper; 0 for asc, 1 for des;
int num_req=0;

//Porcesses time request
void processRequest(int socket);
void dispatchHTTP(int fd);
void *loopthread(int fd);
void poolOfThreads(int masterSocket);
void CGI(int socket, string docpath, char* rpp, char *args);
char *write_parent(int socket, string docpath, const char *main_msg);
void sorting (dirF **files, int count_file, string argument);
int cmp_name(const void *f1, const void * f2);
int cmp_mod(const void *f1, const void * f2);
int cmp_size(const void *f1, const void * f2);
void Log(int socket, char *rpp);
void send_stats(int socket, char *rpp);
string get_argument(string docpath);
extern "C" void killzombie(int sig);

int cmp_name (const void *f1, const void *f2)
{
  struct dirF *dirf1 = *(struct dirF **)f1;
  struct dirF *dirf2 = *(struct dirF **)f2;
  if (cmp_flag == 0){
    return strcmp(dirf1->dir->d_name, dirf2->dir->d_name);
  }
  else
  {
    return strcmp(dirf2->dir->d_name, dirf1->dir->d_name);
  }
}


int cmp_mod(const void *f1, const void *f2)
{
  struct dirF *dirf1 = *(struct dirF **)f1;
  struct dirF *dirf2 = *(struct dirF **)f2;
  //compare the time of last modification
  if (cmp_flag == 0){
    return strcmp(ctime(&(dirf1->file_At.st_mtime)), ctime(&(dirf2->file_At.st_mtime)));
  }
  else
  {
    return strcmp(ctime(&(dirf2->file_At.st_mtime)), ctime(&(dirf1->file_At.st_mtime))); 
  }
}

int cmp_size(const void *f1, const void *f2)
{
  struct dirF *dirf1 = *(struct dirF **)f1;
  struct dirF *dirf2 = *(struct dirF **)f2;
  if (cmp_flag == 0){
    return dirf1->file_At.st_size - dirf2->file_At.st_size;
  }
  else
  {
    return dirf2->file_At.st_size - dirf1->file_At.st_size;
  }
}

int cmp_des_name (const void *f1, const void *f2)
{
  struct dirF *dirf1 = *(struct dirF **)f1;
  struct dirF *dirf2 = *(struct dirF **)f2;
  return strcmp(dirf1->dir->d_name, dirf2->dir->d_name);
}


int main( int argc, char **argv) {


// if Authorization header field isn't present, you responde with a status code of 401 Unauthorized with the following additional fields:WWW-Authenticate : Basic realm="The_Great_Realm_of_CS252"
// when your browser receives this response, it knows to prompt you for a username and password. Your browser will encode this in Base 64 in the following format:
// username:password that get supplied in the Authorization  header field
// your browser will repeat the request with the Authorization header.
// you should create your own username/password combination and encode it using a Base64 encoder online 
//
//Client Request :
//GET /index.html HTTP/1.1
//Server Response:
//HTTP/1.1 401 Unauthorized
//WWW-Authenticate:Basic realm="myhttpd-cs252"

//Client broser prompts for username/password.
//user supplies cs252 as the username
//password as the password and 
//the client encodes in the format of cs252:password in base 64
//
//Client Request:
//GET /index.html HTTP/1.1
//Authorization: Basic Y3MyNTI6cGFzc3dvcmQ=
//create your own username and password 
//not purdue credential
//
//include the line "AUthorization: Basic <User-password in base 64>"
//then respond.
//if the request does not include this line , return an error
//the Line will be included in all the subsequent requests, so you do not need to type user password again
//After you add the basic HTTP authentication, you may serve other documents besides index.html
//

//ADDing concurrency 

time(&startime);
//Print usage if not enough arguemnts
if(argc<3){
    if(argc==1)
    {
        port=6789;
    }
    else if(argc==2)
    {
        //get the port from teh arguments
        port=atoi(argv[1]);
    } 
    else
    {
        fprintf(stderr,"%s",usage);
        exit(-1);
    }
    }else if(argc==3)
    {
        //get the concurrency type
        if(argv[1][0]=='-'){
            if(argv[1][1]=='f')
            {
                //create a new process for eac request
                mode=1;
            }
            else if(argv[1][1]=='t'){
                //create a new thread for each request
                mode=2;
            }
            else if (argv[1][1]=='p'){
                //pool of threaeds
                mode=3;
            }
            else
            {
                fprintf(stderr, "%s", usage);
                exit(-1);
            }
        }
        port =atoi(argv[2]);
    }
    else
    {
        fprintf(stderr, "%s", usage);
        exit(-1);
    }

    struct sigaction sa2;
    sa2.sa_handler=killzombie;
    sigemptyset(&sa2.sa_mask);
    sa2.sa_flags=SA_RESTART;
    if(sigaction(SIGCHLD,&sa2,NULL)){
        perror("sigaction");
        exit(2);
    }
    //set the IP address and port for this server
    struct sockaddr_in serverIPAddress;
    memset( &serverIPAddress, 0, sizeof(serverIPAddress));
    serverIPAddress.sin_family=AF_INET;
    serverIPAddress.sin_addr.s_addr=INADDR_ANY;
    serverIPAddress.sin_port=htons((u_short) port);

    //Allocate a socket
    int masterSocket=socket(PF_INET, SOCK_STREAM, 0);
    if(masterSocket<0){
        perror("socket");
        exit(-1);
    }

    //set socket options to reuse port. Otherwise we will have to wait
    //about 2 min before reusing the same port
    int optval=1;
    int err = setsockopt(masterSocket, SQL_SOCKET, SO_REUSEADDR,(char *) &optval, sizeof( int ));
    //bind the socket to the IP address and port
    int error =bind(masterSocket, (struct sockaddr *)&serverIPAddress, sizeof(serverIPAddress));
    if(error){
        perror("bind");
        exit(-1);
    }

    //put socket in listening mode and set teh size of the queue of unprocessed connections
    error=listen(masterSocket, QueueLength);
    if(error){
        perror("listen");
        exit(-1);
    }
    if(mode==3)
    {
        poolOfThreads(masterSocket);
    }
    else
    {
        while(1){
            //Accept incoming connections
            struct sockaddr_in clientIPAddress;
            int alen=sizeof(clientIPAddress);
            int slaveSocket=accept(masterSocket, (struct sockaddr *)&clientIPAddress,(socklen_t)&alen);
            //needs modify, keep server running if there is an error
            if(slaveSocket <0){
                perror("accept");
                exit(-1);
                num_req++;
                if(mode==0){
                    processRequest(slaveSocket);
                    close(slaveSocket);
                }
                //process based -f
                else if (mode==1)
                {
                    pid_t slave=fork();
                    if(slave ==0)
                    {
                        processRequest(slaveSocket);
                        close(slaveSocket);
                        exit(EXIT_SUCCESS);
                    }
                    close(slaveSocket);
                }
                //thread based
                else if(mode ==2)
                {
                    pthread_t thread;
                    pthread_attr_t attr;
                    pthread_attr_init(&attr);
                    pthread_attr_setdetachstate(&attr, PTHREAD
                    _CREATE_DETACHED);
                    pthread_create(&thread,&attr,(void* (*)(void*))dispatchHTTP,(void *) slaveSocket);
                }
            }
        }
    }
    void processRequest(int socket){}
    void dispatchHTTP(int socket){}
    void poolOfThreads(int masterSocket){}
    void *loopthread(int fd){}
    void CGI(int socket, string docpath, char* rpp, char *args){}
    char *write_parent(int socket, string docpath, const char *main_msg){}
    string get_argument(string docpath){}
    void sorting(dirF **files, int count_file, string argument){}
    void Log(int socket, char *rpp){}
    void send_stats(int socket, char * rpp){
        const char *msg4="</h4>";
        const char *msg1="HTTP/1.0 200 Document follows\r\nServer: CS 252 lab5/1.0\r\nContent-type: text/html\r\n\r\n<html>\n<head>\n"
    }
    extern "C" void killzombie(int sig){
        int pid=1;
        while(waitpid(-1,NULL,WNOHANG)>0);
    }
}

