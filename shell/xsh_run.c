#include <xinu.h>
#include <prodcons_bb.h>  
#include <stdlib.h>
#include <prodcons_f.h>
//#include <stdio.h>
//#include <string.h>
//#include <semaphore.h>              

// definition of array, semaphores and indices 
int arr_q[BUFFSIZE], writeq, readq, one, two;
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

int futures_test(char *args[]){
	int fib = -1, i;
	future_t* f_exclusive, * f_shared;
	if(strncmp(args[2], "-pc", 3) == 0){
		f_exclusive = future_alloc(FUTURE_EXCLUSIVE, sizeof(int), 1);
		f_shared    = future_alloc(FUTURE_SHARED, sizeof(int), 1);
		
		// Test FUTURE_EXCLUSIVE
		resume( create(future_cons, 1024, 20, "fcons1", 1, f_exclusive) );
		resume( create(future_prod, 1024, 20, "fprod1", 2, f_exclusive, (char*) &one) );
		
		// Test FUTURE_SHARED
		resume( create(future_cons, 1024, 20, "fcons2", 1, f_shared) );
		resume( create(future_cons, 1024, 20, "fcons3", 1, f_shared) );
		resume( create(future_cons, 1024, 20, "fcons4", 1, f_shared) );
		resume( create(future_cons, 1024, 20, "fcons5", 1, f_shared) );
		resume( create(future_prod, 1024, 20, "fprod2", 2, f_shared, (char*) &two) );
	}
	
	else if(strncmp(args[2], "-f", 2) == 0){
		fib = atoi(args[3]);
		if (fib > -1) {
			int final_fib;
			int future_flags = FUTURE_SHARED; // TODO - add appropriate future mode here
			
			// create the array of future pointers
			if ((fibfut = (future_t **)getmem(sizeof(future_t *) * (fib + 1))) == (future_t **) SYSERR) {
				printf("getmem failed\n");
				return(SYSERR);
			}
			
			// get futures for the future array
			for (i=0; i <= fib; i++) {
				if((fibfut[i] = future_alloc(future_flags, sizeof(int), 1)) == (future_t *) SYSERR) {
					printf("future_alloc failed\n");
					return(SYSERR);
				}
			}
			// spawn fib threads and get final value
			// TODO - you need to add your code here
			for(i=0;i<=fib;i++)
				ffib(i);
			
			future_get(fibfut[fib], (char*) &final_fib);
			
			for (i=0; i <= fib; i++) {
				future_free(fibfut[i]);
			}
			
			freemem((char *)fibfut, sizeof(future_t *) * (fib + 1));
			printf("Nth Fibonacci value for N=%d is %d\n", fib, final_fib);
			return(OK);
		}
	}
}	

shellcmd xsh_run(int nargs, char *args[]){
	//args++;
        //nargs--;
	if ((nargs == 1) || (strncmp(args[1], "list", 5) == 0)) {
		printf("prodcons_bb\n");
		printf("futures_test\n");
		return OK;
	}
	//printf("%s %s %s",args[0],args[1],args[2]);
        if(strncmp(args[1], "prodcons_bb", 11) == 0){
		/* create a process with the function as an entry point. */
        	resume (create((void *)prodcons_bb, 4096, 20, "prodcons_bb", 2, nargs, args));
	}
	else if(strncmp(args[1],"futures_test",12) == 0){
		if(nargs<=2){
			printf("Need an option (-pc/-f)\n");
			return 1;
		}
		if(strncmp(args[2],"-pc",3)!=0 && strncmp(args[2],"-f",2)!=0){
			printf("Only two options allowed (-pc/-f).\n");
			return 1;
		}
		if(strncmp(args[2],"-f",2)==0){
			if (nargs!=4){
				printf("Need to provide a value for sum upto Nth value\n");
				return 1;
			}
		}	
		resume (create((void *)futures_test, 4096, 20, "futures_test", 1, args));
	}
}	
