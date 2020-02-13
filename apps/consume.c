#include <xinu.h>
#include <prodcons.h>
#include <prodcons_bb.h>  

void consumer(int count) {
	for (int i =1; i<=count; i++){
		printf("consumed : %d\n",n);

	}
}

void consumer_bb(int count) { 
	  char *proc_name = proctab[getpid()].prname;
	  for(int i = 0; i<count; i++){
		wait(csem);
		wait(mutex);
		kprintf("name : %s, read : %d\n",proc_name,arr_q[readq]);
		readq = (readq+1)%BUFFSIZE;
		signal(mutex);
		signal(psem);
	}	
}

