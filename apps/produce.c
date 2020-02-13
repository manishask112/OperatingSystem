#include <xinu.h>
#include <prodcons.h>
#include <prodcons_bb.h>  

//int arr_q, write, read, BUFFSIZE;
//sid32 psem,csem,mutex;

void producer(int count) {
	for (int i =1; i<=count; i++){
		n=i;
		printf("produced : %d\n",n);
	}		
}

void producer_bb(int count) {
	  // Iterate from 0 to count and for each iteration add iteration value to the global array `arr_q`, 
        char *proc_name = proctab[getpid()].prname;  
	for(int i = 1; i<=count; i++){
	   	wait(psem);
	   	wait(mutex);
	  	arr_q[writeq]=i;
	  	kprintf("name : %s, write : %d\n",proc_name,i);
	  	writeq = (writeq+1)%BUFFSIZE;
	  	signal(mutex);
	  	signal(csem);
	}
}
