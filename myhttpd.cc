const char * usage =
                                                            " \n";


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
#include <bits/stdc++.h>
#include <pthread.h>
using namespace std;

time_t startime;
string contentType;

int debug = 1;
int QueueLength = 5;
//global var for mode 1 for -f; 2 for -t; 3 for -p; 0 for basic
int mode = 0; 
//port number
int port = 0;
//flag for compare helper; 0 for asc, 1 for des;
int cmp_flag = 0;
//number of request
int num_req = 0;
pthread_mutex_t mutex1;
// Processes time request
void processRequest( int socket );
void dispatchHTTP(int fd);
void *loopthread(int fd);
void poolOfThreads(int masterSocket);
void createThreadForEachRequest(int masterSocket);
string get_argument(string docpath);
extern "C" void killzombie(int sig);

void poolOfThreads(int masterSocket)
{
  pthread_t tid[4]; 
  pthread_attr_t attr;
  
  pthread_attr_init(&attr);
  pthread_mutex_init(&mutex1, NULL);
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

  for (int i = 0; i < 4; i++)
  {

    pthread_create(&tid[i], NULL, (void * (*)(void *))loopthread, (void *)masterSocket);
  }
  loopthread(masterSocket);
}


void *loopthread (int fd)
{

while (1) {
    pthread_mutex_lock(&mutex1);
    struct sockaddr_in clientIPAddress; 
    int alen = sizeof(clientIPAddress);
    int slaveSocket = accept(fd, (struct sockaddr *)&clientIPAddress, (socklen_t*)&alen);
    ++num_req;
    pthread_mutex_unlock(&mutex1);
    if (slaveSocket >= 0)
    {
       dispatchHTTP(slaveSocket);
    }
  }
}
int
main( int argc, char ** argv )
{

  
     if (argc == 2)
    {
        // Get the port from the arguments
        port  = atoi( argv[1] );
    }
   
  else if (argc == 3)
  {
    //get the concurrency type
    if (argv[1][0] == '-')
    {
        if (argv[1][1] == 'f')
        {
            //create a new process for each request
            mode = 1;
        }
        else if (argv[1][1] == 't')
        {
            //create a new thread for each request
            mode = 2;
        }
        else if (argv[1][1] == 'p')
        {
            //pool of threads
            mode = 3;
            printf("ppppppppppppppppp\n");
            printf("mode:");
            printf("%d",mode);
            printf("\n");
            
        }
        else
        {
            fprintf( stderr, "%s", usage);
            exit(-1);
        }
    }
    port  = atoi( argv[2] );
  }
  else
  {
    fprintf( stderr, "%s", usage );
    exit(-1);
  }
   printf("mmmmmmmmmm\n");
            printf("mode:");
            printf("%d",mode);
            printf("\n");
  struct sigaction sa2;
  sa2.sa_handler = killzombie;
  sigemptyset(&sa2.sa_mask);
  sa2.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD,&sa2, NULL))
  {
    perror("sigaction");
    exit(2);
  }
 //printf("mode:%s\n",mode); 
  // Set the IP address and port for this server
  struct sockaddr_in serverIPAddress; 
  memset( &serverIPAddress, 0, sizeof(serverIPAddress) );
  serverIPAddress.sin_family = AF_INET;
  serverIPAddress.sin_addr.s_addr = INADDR_ANY;
  serverIPAddress.sin_port = htons((u_short) port);
  
  // Allocate a socket
  int masterSocket =  socket(PF_INET, SOCK_STREAM, 0);
  if ( masterSocket < 0) {
    perror("socket");
    exit( -1 );
  }
  // Set socket options to reuse port. Otherwise we will
  // have to wait about 2 minutes before reusing the sae port number
  int optval = 1; 
  int err = setsockopt(masterSocket, SOL_SOCKET, SO_REUSEADDR, 
           (char *) &optval, sizeof( int ) );
   
  // Bind the socket to the IP address and port
  int error = bind( masterSocket,
        (struct sockaddr *)&serverIPAddress,
        sizeof(serverIPAddress) );
  if ( error ) {
    perror("bind");
    exit( -1 );
  }
  
  // Put socket in listening mode and set the 
  // size of the queue of unprocessed connections
  error = listen( masterSocket, QueueLength);
  if ( error ) {
    perror("listen");
    exit( -1 );
  }
printf("mode: %d ==================\n",mode);
  if (mode == 3)
  {
	  printf("p mode===========================================\n");
    poolOfThreads(masterSocket);
    printf("oooooo");
    
  }
  /*else if(mode==2){
    createThreadForEachRequest(masterSocket);
  }*/
  else
  {
    while ( 1 ) {
        // Accept incoming connections
        struct sockaddr_in clientIPAddress;
        int alen = sizeof( clientIPAddress );
        int slaveSocket = accept( masterSocket,
                (struct sockaddr *)&clientIPAddress,
                (socklen_t*)&alen);
        //needs modify, keep server running if there is an error
        if ( slaveSocket < 0 ) {
          perror( "accept" );
          exit( -1 );
        }
        num_req++;
        if (mode == 0)
        {
            processRequest(slaveSocket);
            close(slaveSocket);
        }
        //process based -f
        else if (mode == 1)
        {
          pid_t slave = fork();
          if (slave == 0)
          {
            processRequest(slaveSocket);
            close(slaveSocket);
       
          }
          close(slaveSocket);
        }
        //thread based 
        else if (mode == 2)
        {
          pthread_t thread;
          pthread_attr_t attr;
          pthread_attr_init(&attr);
          pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
          pthread_create(&thread, &attr, (void* (*)(void*))dispatchHTTP, (void *) slaveSocket);
        }
     }
  }
}

void createThreadForEachRequest(int masterSocket) {
    while (1) {
        struct sockaddr_in clientIPAddress;
        int alen = sizeof( clientIPAddress );
        int slaveSocket = accept( masterSocket,
                (struct sockaddr *)&clientIPAddress,
                (socklen_t*)&alen);

        if (slaveSocket >= 0) {
        // When the thread ends resources are recycled 
            num_req++;
            pthread_t thread;
            pthread_attr_t attr; 
            pthread_attr_init(&attr); 
            pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED); 
            pthread_create(&thread, &attr,(void* (*)(void*))dispatchHTTP, (void *) slaveSocket);
        }
    }
 }


void processRequest(int socket)
{
  const int size = 4000;
  int length = 0;
  int n;
  int gotGet = 0;
  int docpath_index = 0;

  unsigned char newChar;
  unsigned char oldChar = 0;
  unsigned char olldchar = 0;
  unsigned char ollldchar = 0;

  string request;
  string request_type;
  string docpath;
  string argument;
//read request
  while (length < size && (n = read(socket, &newChar, sizeof(newChar))) > 0)
  {
    printf("%c",newChar);	  
    if (newChar == ' ')
    {
      if (!gotGet)
      {
        gotGet = length;
        //request_type = request;
      }
      else if (!docpath_index)
      {
        docpath_index = length;
        docpath = request.substr(gotGet+1, length-gotGet-1);
      }
    }
    else if (newChar == '\n' && oldChar == '\r' && olldchar == '\n' && ollldchar == '\r')
    {
      break;
    }
    else
    {
      ollldchar = olldchar;
      olldchar = oldChar;
      oldChar = newChar;
    }
    ++length;
    request += newChar;
  }
  printf("request:%s\n",request);
//autherization
 int auth=1;
 char *occur;
 int l=request.length();
 char rl[l+1];
 strcpy(rl,request.c_str());
printf("docpath: %s\n",docpath);
  occur=strstr(rl,"Authorization: Basic");

  if(occur){
       printf("here");
  	 char * occur2;
        occur2=strstr(occur,"\r\n");
        char cred[100]={0};
        strncpy(cred,occur+21,occur2-occur-21);
        printf("cred=%s\n",cred);
        printf("number %d\n",strcmp(cred,"U3Vubnk6MTIzNDU2Nw=="));
        if(strcmp(cred,"U3Vubnk6MTIzNDU2Nw==")!=0){
		printf("0\n");
                auth=0;
        }
 }

 printf("auth=%d\n",auth);
  if(auth==0|| occur==NULL){
    //print
    write(socket,"HTTP/1.1 401 Unauthorized\r\n",strlen("HTTP/1.1 401 Unauthorized\r\n"));
    write(socket,"WWW-Authenticate: Basic realm=\"myhttpd-cs252\"\r\n",strlen("WWW-Authenticate: Basic realm=\"myhttpd-cs252\"\r\n"));
    return;
  }


  char cwd[size + 1] = {0};
  getcwd(cwd, sizeof(cwd));
  string address;
  address = cwd;
//get adddress
    if (strcmp(docpath.c_str(),"/") == 0 && docpath.length() == 1)
    {
      address +="/http-root-dir/htdocs/index.html";
    }
  else{
      address += "/http-root-dir/htdocs";
      address +=  docpath;
    }
  
  
  char rpp[size + 1] = "";
  char *res = realpath(address.c_str(), rpp);
  printf("rpp:%s",rpp);    
  if (strstr(rpp, ".text") || strstr(rpp, ".text/"))
      {
        contentType = "text/plain";
      }

      
      else if (strstr(rpp, ".gif") || strstr(rpp, ".gif/"))
      {
        contentType = "image/gif";
      }
   
      else
      {
        contentType = "text/html";
      }
    
      DIR *directory = opendir(rpp);
       FILE *document;
      if (strstr(rpp, ".gif") || strstr(rpp, ".gif/"))
      {
       document = fopen(address.c_str(), "rb");
  
      }else{
   
      document = fopen(address.c_str(), "r");
      }

      if (document > 0)
          { 
            const char * protocol = "HTTP/1.0 200 Document follows";
            const char * crlf = "\r\n";
            const char * server = "Server: CS 252 lab5/1.0";
            const char * content_type = "Content-type: ";

            write(socket, protocol, strlen(protocol));
            write(socket, crlf, strlen(crlf));
            write(socket, server, strlen(server));
            write(socket, crlf, strlen(crlf));
            write(socket, content_type, strlen(content_type));
            write(socket, crlf, strlen(crlf));
            write(socket, crlf, strlen(crlf));
           long count = 0;
            char ch;
            while (count = read(fileno(document), &ch,sizeof(ch)))
            {
                if (write(socket, &ch, sizeof(ch)) != count)
                {
                    perror("write");
                }
            }
            fclose(document);
          }
          else
          {
           
              const char * notFound = "HTTP/1.0 404 File Not Found";
              const char * crlf = "\r\n";
              const char * server = "Server: CS 252 lab5/1.0";
              const char * content_type = "Content-type: ";
              const char * err_msg = "File not Found";
              
              write(socket, notFound, strlen(notFound));
              write(socket, crlf, strlen(crlf));
              write(socket, server, strlen(server));
              write(socket, crlf, strlen(crlf));
              write(socket, content_type, strlen(content_type));
              write(socket, crlf, strlen(crlf));
              write(socket, crlf, strlen(crlf));
              write(socket, err_msg, strlen(err_msg)); 
          }
}

void dispatchHTTP (int socket)
{
  processRequest(socket);
  close(socket);
}





extern "C" void killzombie(int sig)
{
  int pid = 1;
  while(waitpid(-1, NULL, WNOHANG) > 0);
}

