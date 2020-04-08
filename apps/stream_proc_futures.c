#include <xinu.h>
#include <future.h>
#include "tscdf.h"
#include <stream.h>

uint pcport;
uint work_queue_depth, time_window,output_time;

int stream_proc_futures(int nargs, char* args[]) {
	ulong secs, msecs, time;
	secs = clktime;
	msecs = clkticks;

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
	future_t **farray = (future_t **) getmem(sizeof(future_t *) * num_streams);
	for(i = 0; i < num_streams; i++) {
		farray[i] =future_alloc(FUTURE_QUEUE, sizeof(de), work_queue_depth);
		resume(create(stream_consumer_future, 1024, 20,'consumers', 2, i, farray[i]));	
	 
	}
	// Parse input header file data and populate work queue
	for (i = 0; i < n_input; i++){

		a = (char *)stream_input[i]; 
		st = atoi(a);
		while (*a++ != '\t');
		ts = atoi(a);
		while (*a++ != '\t');
		v = atoi(a);
			
        de *data = (de *)getmem(sizeof(de));
        data->time = ts;
        data->value = v;

        future_set(farray[st], data);
	}
	for(i=0; i < num_streams; i++) {
	      uint32 pm;
	      pm = ptrecv(pcport);
	      printf("process %d exited\n", pm);
	  }

    for (i = 0; i < num_streams; i++) {
        future_free(farray[i]);
    }
  ptdelete(pcport, 0);

  time = (((clktime * 1000) + clkticks) - ((secs * 1000) + msecs));
  printf("time in ms: %u\n", time);


  return 0;
}



void stream_consumer_future(int32 id, future_t *f){
 kprintf("stream_consumer id:%d (pid:%d)\n",id,getpid());
 char s[50];
 int count = 0;
 struct tscdf *tc = tscdf_init(time_window);
 int32 *qarray;
 sprintf(s, "s%d", id);
 while(1){
    de *data = (de*)getmem(sizeof(de));
    future_get(f,data);
	if (data->time == 0){
		ptsend(pcport,getpid());
		return;
	}
	if(tscdf_update(tc, data->time, data->value)){
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
 }
}