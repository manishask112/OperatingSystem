#include <xinu.h>
#include <prodcons_bb.h>  
#include <stdlib.h>
//#include <stdio.h>
//#include <string.h>
//#include <semaphore.h>              

// definition of array, semaphores and indices 
int arr_q[BUFFSIZE], writeq, readq;
sid32 psem, csem, mutex;
int prodcons_bb(int nargs, char *args[]) {
//create and initialize semaphores to necessary values
	psem = semcreate(BUFFSIZE);
	csem = semcreate(0);
        mutex = semcreate(1);
	char str[10];
	//char process[10];
       //initialize read and write indices for the queue
       writeq = 0;
       readq = 0;
       //create producer and consumer processes and put them in ready queue
       int no_of_producers=atoi(args[2]);
       int no_of_consumers=atoi(args[3]);
       if (nargs>6){
	printf("Only 5 arguments allowed\n");	
	return (1);	
       }
       else if(nargs<6){
	printf("5 arguments are needed\n");
	return(1);
       }	
       if (no_of_producers*atoi(args[4]) != no_of_consumers*atoi(args[5])){
	printf("Total producer iterations and the number of total consumer iterations are not equal\n");
	return (1);	
       }	       
       //kprintf("no_of_producers %d\n", no_of_producers);
       //kprintf("no_of_consumers %d\n", no_of_consumers);
       for(int i=1;i<=no_of_producers;i++){
	       sprintf(str, "producer_%d", i);
	       resume(create(producer_bb, 1024, 20, str, 1, atoi(args[4])));
               }
       for(int i=1;i<=no_of_consumers;i++){
	       sprintf(str, "consumer_%d", i);
	       //strcpy(process,"consumer_");
	       //strcat(process,str);
	       resume(create(consumer_bb, 1024, 20, str, 1, atoi(args[5])));
	       }
       return (0);
}

shellcmd xsh_run(int nargs, char *args[]){
	//args++;
        //nargs--;
        if(strncmp(args[1], "prodcons_bb", 11) == 0){
		/* create a process with the function as an entry point. */
        	resume (create((void *)prodcons_bb, 4096, 20, "prodcons_bb", 2, nargs, args));
	}	  		      			  		  
}
