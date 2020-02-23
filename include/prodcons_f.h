#include <xinu.h>
#include<future.h>

uint future_prod(future_t* fut, char* value);
uint future_cons(future_t* fut);
int ffib(int);
extern future_t **fibfut;
extern int one, two, zero;
