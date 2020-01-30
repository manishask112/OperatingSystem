#include <xinu.h>
#include <prodcons.h>

void producer(int count) {
	for (int i =1; i<=count; i++){
		n=i;
		printf("produced : %d\n",n);
	}		
}
