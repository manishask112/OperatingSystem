#include <xinu.h>
#include<future.h>

uint future_prod(future_t* fut, char* value);
uint future_cons(future_t* fut);
int ffib(int);
future_t **fibfut;
