#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

int counter = 0; // Global counter
pthread_mutex_t mutex;
void increment_loop(int max){
    for(int i=0;i<max;i++){
      pthread_mutex_lock(&mutex);
      int tmp = counter;
      tmp=tmp+1;
      counter=tmp;
      pthread_mutex_unlock(&mutex);
} 
}



int main(){

  pthread_t t1,t2;
  pthread_mutex_init(&mutex,NULL);
  
  pthread_create(&t1,NULL,
                increment,10000000);
  
  pthread_create(&t2,NULL,
                increment,10000000);
  //wait until threads finish
  
  pthread_join(t1);
  pthread_join(t2);
  printf(“counter total=%d”,counter);
   
}
