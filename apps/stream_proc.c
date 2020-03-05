#include <xinu.h>
#include <stream.h>
#inculde "tscdf_input.h"

uint pcport;
struct stream **array_of_streams;
int work_queue_depth;

int stream_proc(int nargs, char* args[]) {
	ulong secs, msecs, time;
	secs = clktime;
	msecs = clkticks;

	if((pcport = ptcreate(num_streams)) == SYSERR) {
		printf("ptcreate failed\n");
		return(-1);
	}
	 
	// Parse arguments
	char usage[] = "Usage: -s num_streams -w work_queue_depth -t time_window -o output_time\n";
	int num_streams, time_window, output_time, i, j, st, ts, v;
	char *ch, c, *a;
	
	/* Parse arguments out of flags */
	/* if not even # args, print error and exit */
	if (!(nargs % 2)) {
		printf("%s", usage);
		return(-1);
	}
		i = nargs - 2;
		while (i > 0) {
		ch = args[i-1];
		c = *(++ch);
		                 
		switch(c) {
			case 's':
		 		num_streams = atoi(args[i]);
		  		break;
			case 'w':
		  		work_queue_depth = atoi(args[i]);
		  		break;                                                                                                          
			case 't':                                                                                                      			
				time_window = atoi(args[i]);
				break;
			case 'o':
				output_time = atoi(args[i]);
		  	        break;
			default:
		  		printf("%s", usage);
				return(-1);     
		}  
		i -= 2;
		}
		 
		// Create streams
		array_of_streams = (struct stream **) getmem(sizeof() * num_streams);
		//Create consumer processes and initialize streams
		// Use `i` as the stream id.
		for(i = 0; i < num_streams; i++) {
			array_of_streams[i] = (stream*)getmem(sizeof(stream));
		  	array_of_streams[i]->queue = (de **)getmem(sizeof(de *) * work_queue_depth);
		        
			for(j = 0; j < work_queue_depth; j++)                                 	        						
				array_of_streams[i]->queue[j] = (de *)getmem(sizeof(de))                                 	        							
			array_of_streams[i]->spaces = semcreate(work_queue_dept);
			array_of_streams[i]->items = semcreate(0);
			array_of_streams[i]->mutex = semcreate(0);
		  	array_of_streams[i]->head = 0;
			array_of_streams[i]->tail  = -1;

		  	resume(create(stream_consumer, 1024, 20,'consumer', 2, i,array_of_streams[i]));				 
		}
	                                            	        															// Parse input header file data and populate work queue
		for (i = 0; i < n_input; i++){
			                                                                                                                                                                    	        															      	
			a = (char *)stream_input[i]; 
			st = atoi(a);
			while (*a++ != '\t');
			ts = atoi(a);
		  	while (*a++ != '\t');
			v = atoi(a);
			wait(array_of_streams[st]->spaces);
		 	wait(array_of_streams[st]->mutex);
		                                                                                                                                                                                               	        															      			array_of_streams[st]->tail = (array_of_streams[st]->tail + 1)% work_queue_depth;			      		    						array_of_streams[st]->queue[array_of_streams[st]->tail]->time = ts;
			array_of_streams[st]->queue[array_of_streams[st]->tail]->value = v;                           	        															      		                    										signal(array_of_streams[st]->mutex);		     		                    											signal(array_of_streams[st]->items);

		}
		for(i=0; i < num_streams; i++) {
		                                                                                                                                                                                                   	        															      		                    											       uint32 pm;
		  //                                                                                                                                                                                                 	        															      		                    											             pm = ptrecv(pcport);
		  //                                                                                                                                                                                                 	        															      		                    											                   printf("process %d exited\n", pm);
		  //                                                                                                                                                                                                 	        															      		                    											                     }
		  //
		  //                                                                                                                                                                                                 	        															      		                    											                       ptdelete(pcport, 0);
		  //
		  //                                                                                                                                                                                                 	        															      		                    											                         time = (((clktime * 1000) + clkticks) - ((secs * 1000) + msecs));
		  //                                                                                                                                                                                                 	        															      		                    											                           printf("time in ms: %u\n", time);
		  //                                                                                                                                                                                                 	        															      		                    											                           	
		  //                                                                                                                                                                                                 	        															      		                    											                           		
		  //                                                                                                                                                                                                 	        															      		                    											                           		  return 0;
		  //                                                                                                                                                                                                 	        															      		                    											                           		  }
		  //
		  //
		  //
		  //                                                                                                                                                                                                 	        															      		                    											                           		  void stream_consumer(int32 id, struct stream *str){
		  //                                                                                                                                                                                                 	        															      		                    											                           		   kprintf("stream_consumer id:%d (pid:%d)\n",id,getpid());
		  //                                                                                                                                                                                                 	        															      		                    											                           		    char s[2];
		  //                                                                                                                                                                                                 	        															      		                    											                           		     sprintf(s, "s%d", id);
		  //                                                                                                                                                                                                 	        															      		                    											                           		      while (str->queue[str->head]->time != 0){
		  //                                                                                                                                                                                                 	        															      		                    											                           		      	
		  //                                                                                                                                                                                                 	        															      		                    											                           		      		wait(str->items);
		  //                                                                                                                                                                                                 	        															      		                    											                           		      			wait(str->mutex);
		  //                                                                                                                                                                                                 	        															      		                    											                           		      				
		  //                                                                                                                                                                                                 	        															      		                    											                           		      					kprintf("%s: %d\n",s,str->queue[str->head]->value);
		  //                                                                                                                                                                                                 	        															      		                    											                           		      						str->head = (str->head + 1)% work_queue_depth;
		  //                                                                                                                                                                                                 	        															      		                    											                           		      							
		  //                                                                                                                                                                                                 	        															      		                    											                           		      								signal(str->mutex);
		  //                                                                                                                                                                                                 	        															      		                    											                           		      									signal(str->spaces);
		  //                                                                                                                                                                                                 	        															      		                    											                           		      										}
		  //                                                                                                                                                                                                 	        															      		                    											                           		      										 kprintf("stream_consumer exiting\n");
		  //                                                                                                                                                                                                 	        															      		                    											                           		      										 }
