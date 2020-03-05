#include <xinu.h>
#include <stream.h>
#include "tscdf.h"

uint pcport;
struct stream **array_of_streams;
int work_queue_depth, time_window,output_time;

int stream_proc(int nargs, char* args[]) {
	ulong secs, msecs, time;
	secs = clktime;
	msecs = clkticks;

	/*if((pcport = ptcreate(num_streams)) == SYSERR) {
	printf("ptcreate failed\n");
	return(-1);
	}*/

	// Parse arguments
	char usage[] = "Usage: -s num_streams -w work_queue_depth -t time_window -o output_time\n";
	int num_streams, i, j, st, ts, v;
	char *ch, c, *a;

	/* Parse arguments out of flags */
	/* if not even # args, print error and exit */
	if ((nargs % 2)!= 0) {
		return(-1);
	}
	i = nargs - 1;
	while (i > 2) {
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
				kprintf("%s\n", usage);
				return(-1);     
		}  
		i -= 2;
	}
	if((pcport = ptcreate(num_streams)) == SYSERR) {
		printf("ptcreate failed\n");
		return(-1);
	}
	// Create streams
	array_of_streams = (struct stream **) getmem(sizeof(struct stream *) * num_streams);
	//Create consumer processes and initialize streams
	// Use `i` as the stream id.
	for(i = 0; i < num_streams; i++) {
		array_of_streams[i] =(struct stream *) getmem(sizeof(struct stream));
		array_of_streams[i]->queue = (de **) getmem(sizeof(de ) * work_queue_depth);

		for (j = 0; j < work_queue_depth; j++)                                 	        						
			array_of_streams[i]->queue[j] =(de*)getmem(sizeof(de)) ;  
			
		array_of_streams[i]->spaces = semcreate(work_queue_depth);
		array_of_streams[i]->items = semcreate(0);
		array_of_streams[i]->mutex = semcreate(1);
		array_of_streams[i]->head = -1;
		array_of_streams[i]->tail  = -1;

		resume(create(stream_consumer, 1024, 20,'consumers', 2, i, array_of_streams[i]));				 
	}
																						// Parse input header file data and populate work queue
	for (i = 0; i < n_input; i++){

		a = (char *)stream_input[i]; 
		st = atoi(a);
		while (*a++ != '\t');
		ts = atoi(a);
		while (*a++ != '\t');
		v = atoi(a);
			
		//resume(create(stream_consumer, 1024, 20,'consumers', 2, i, array_of_streams[st]));
		
		wait(array_of_streams[st]->spaces);
		wait(array_of_streams[st]->mutex);
		
		array_of_streams[st]->tail = (array_of_streams[st]->tail + 1)% work_queue_depth;
		array_of_streams[st]->queue[array_of_streams[st]->tail]->time = ts;
		array_of_streams[st]->queue[array_of_streams[st]->tail]->value = v;
		//kprintf("time %d\n",array_of_streams[st]->queue[array_of_streams[st]->tail]->time);
		//kprintf("value %d\n",array_of_streams[st]->queue[array_of_streams[st]->tail]->value); 


		signal(array_of_streams[st]->mutex);
		signal(array_of_streams[st]->items);  

	}
	for(i=0; i < num_streams; i++) {
	      uint32 pm;
	      pm = ptrecv(pcport);
	      printf("process %d exited\n", pm);
	  }

  ptdelete(pcport, 0);

  time = (((clktime * 1000) + clkticks) - ((secs * 1000) + msecs));
  printf("time in ms: %u\n", time);


  return 0;
}



void stream_consumer(int32 id, struct stream *str){
 kprintf("stream_consumer id:%d (pid:%d)\n",id,getpid());
 char s[50];
 int count = 0;
 struct tscdf *tc = tscdf_init(time_window);
 int32 *qarray;
 sprintf(s, "s%d", id);
 while(1){
	wait(str->items);
	wait(str->mutex);

	str->head = (str->head+1)% work_queue_depth;
	if (str->queue[str->head]->time == 0){
		kprintf("stream_consumer exiting\n");
		ptsend(pcport,getpid());
		return;
	}
	if(tscdf_update(tc, str->queue[str->head]->time, str->queue[str->head]->value)){
		count+= 1;
		if(count == output_time){

			qarray = tscdf_quartiles(tc);
			if(qarray == NULL) {
				kprintf("tscdf_quartiles returned NULL\n");
				continue;
			}

			sprintf(s, "s%d: %d %d %d %d %d", id, qarray[0], qarray[1], qarray[2], qarray[3], qarray[4]);
			kprintf("%s\n", s);
			freemem((char *)qarray, (6*sizeof(int32)));

			count = 0;
		}	
	}

	signal(str->mutex);
	signal(str->spaces);
 }
}
			
