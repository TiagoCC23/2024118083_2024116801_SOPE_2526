#include "R4_3_prodConsBoundedBuffer.h"

pthread_mutex_t mutex_prod = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_cons = PTHREAD_MUTEX_INITIALIZER;
sem_t podeProduzir;
sem_t podeConsumir;
int prodptr = 0, constptr = 0;