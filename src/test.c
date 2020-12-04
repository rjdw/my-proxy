#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>


static int counter = 0;



void *func(void *t_arg){
   
    pthread_detach(pthread_self());
    
    int num = *(int*)t_arg;

    free(t_arg);
    
    while (counter < num){
	//loop
    }

    counter++;
    printf("%d\n", num);

    return NULL;
}


int main(){

    for (int i = 0; i<3; i++){

	int *num = malloc(sizeof(int));
	*num=i;


	pthread_t thr;
	pthread_create(&thr, NULL, func, num);
    
    }

    pthread_exit(NULL);

}
