#include <xinu.h>
#include <stdio.h> 

/*-----------------------------------------
*xsh_hello  -  Simple Hello World program
*------------------------------------------
*/
shellcmd xsh_hello (int nargs, char *args[]){
	if(nargs!=2){
		if(nargs>2){
			fprintf(stderr,"Only one argument allowed\n");
		}
		else{
			fprintf(stderr,"One argument needed\n");
		}	
		return 1;
	}

	printf("Hello %s, Welcome to the world of Xinu!!\n", args[1]);
	return 0;
}
