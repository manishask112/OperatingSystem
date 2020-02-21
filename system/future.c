#include <xinu.h>
#include <future.h>

future_t* future_alloc(future_mode_t mode, uint size, uint nelems){
	intmask mask;
	mask = disable();
	future_t* new_future = (future_t*)getmem(sizeof(future_t));
	new_future->data = (char *)getmem(sizeof(char) * size);
	new_future->state = FUTURE_EMPTY;
	new_future->mode = mode;
	new_future->size = size;
	new_future->pid= 0;
	new_future->set_queue = NULL;
	new_future->get_queue = NULL;
	restore(mask);
	return new_future;
}

syscall future_free(future_t* f){
	intmask mask;
	mask = disable();
	if (f!=NULL){
		if(f->get_queue != NULL){
			queue* temp;
			while(f->get_queue != NULL){
				temp = f->get_queue->next;
				freemem((char*)f->get_queue,sizeof(queue));
				f->get_queue = temp;
			}
		}	
		if(f->set_queue != NULL){
			queue* temp;
			while(f->set_queue != NULL){
				temp = f->set_queue->next;
				freemem((char*)f->set_queue,sizeof(queue));
				f->set_queue = temp;
			}
		}
		//freemem((char*)f->set_queue,sizeof(queue));
											
		freemem((char*)f,sizeof(future_t));
		restore(mask);
		return OK;
	}
	restore(mask);
	return SYSERR;
}	
				

syscall future_get(future_t* f, char* data){
	intmask mask;
	mask = disable();
	if (f == NULL){
		restore(mask);
		return SYSERR;
	}	
	if (f->mode == FUTURE_EXCLUSIVE){
		if (f->pid != 0)
		{	
			restore(mask);
			return SYSERR; //to allow only one process to access future.
		}
		f->pid = getpid();
		if (f->state != FUTURE_READY) suspend(f->pid);
		memcpy(data,f->data,f->size);			
	}
	else if(f->mode == FUTURE_SHARED){
		//pid32 pid = getpid();
		if (f->state != FUTURE_READY){
			pid32 pid = getpid();
			if (f->get_queue == NULL){ //First process to request for access to future
				f->get_queue = (queue*)getmem(sizeof(queue));
				f->get_queue->pid = pid;
				f->get_queue->next = NULL;
			}
			else{
				queue* temp = f->get_queue;
				while(temp->next != NULL)
					temp = temp->next;
				temp->next = (queue*)getmem(sizeof(queue));
				temp->next->pid = pid;
				temp->next->next = NULL;
			}
			suspend(pid);
		}
		memcpy(data,f->data,f->size);
	}
	restore(mask);	
	return OK;
}

syscall future_set(future_t* f, char* data){
	intmask mask;
	mask = disable();
	if (f == NULL){
		restore(mask);
		return SYSERR;
	}
	if (f->mode == FUTURE_EXCLUSIVE){
		if (f->state == FUTURE_EMPTY){
			f->state = FUTURE_WAITING;
			memcpy(f->data,data,f->size);
			f->state = FUTURE_READY;
			
			if (f->pid != 0) resume(f->pid); //Resume the only other thread allowed to get value, if it exists.
		}
		else
		{
			restore(mask);
			return SYSERR;//Future data can be set only once
		}	
	}
	else if(f->mode == FUTURE_SHARED){
		if (f->state == FUTURE_EMPTY){
			f->state = FUTURE_WAITING;
			memcpy(f->data,data,f->size);
			f->state = FUTURE_READY;
			queue* temp = f->get_queue;
			while(temp){	//While loop to start all processes waiting to get future from queue
				resume(temp->pid);
				f->get_queue = f->get_queue->next;
				temp = f->get_queue;
			}
		}
		else
		{
			restore(mask);
			return SYSERR;//Future data can be set only once
		}
	}
	restore(mask);
	return OK;
}
