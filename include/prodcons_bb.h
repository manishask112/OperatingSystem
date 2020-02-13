// declare globally shared variable
#define BUFFSIZE 5
extern int arr_q[BUFFSIZE];

// declare globally shared semaphores
extern sid32 psem,csem,mutex;
// declare globally shared read and write indices
extern int writeq, readq;
// function prototypes
void consumer_bb(int count);
void producer_bb(int count);
