#include <my_global.h>
#include <my_sys.h>
#include <my_atomic.h>

my_atomic_32_t a32,b32,c32;
my_atomic_rwlock_t rwl;

pthread_attr_t thr_attr;
pthread_mutex_t mutex;
pthread_cond_t cond;
int N;

/* add and sub a random number in a loop. Must get 0 at the end */
pthread_handler_t test_atomic_add_handler(void *arg)
{
  int    m=*(int *)arg;
  int32 x;
  for (x=((int)(&m)); m ; m--)
  {
    x=x*m+0x87654321;
    my_atomic_add32(&a32, x,  &rwl);
    my_atomic_add32(&a32, -x, &rwl);
  }
  pthread_mutex_lock(&mutex);
  N--;
  if (!N) pthread_cond_signal(&cond);
  pthread_mutex_unlock(&mutex);
}

/*
  1. generate thread number 0..N-1 from b32
  2. add it to a32
  3. swap thread numbers in c32
  4. (optionally) one more swap to avoid 0 as a result
  5. subtract result from a32
  must get 0 in a32 at the end
*/
pthread_handler_t test_atomic_swap_handler(void *arg)
{
  int    m=*(int *)arg;
  uint32  x=my_atomic_add32(&b32, 1, &rwl);

  my_atomic_add32(&a32, x, &rwl);

  for (; m ; m--)
    x=my_atomic_swap32(&c32, x,&rwl);

  if (!x)
    x=my_atomic_swap32(&c32, x,&rwl);

  my_atomic_add32(&a32, -x, &rwl);

  pthread_mutex_lock(&mutex);
  N--;
  if (!N) pthread_cond_signal(&cond);
  pthread_mutex_unlock(&mutex);
}

/*
  same as test_atomic_add_handler, but my_atomic_add32 is emulated with
  (slower) my_atomic_cas32
*/
pthread_handler_t test_atomic_cas_handler(void *arg)
{
  int    m=*(int *)arg;
  int32 x;
  for (x=((int)(&m)); m ; m--)
  {
    uint32 y=my_atomic_load32(&a32, &rwl);
    x=x*m+0x87654321;
    while (!my_atomic_cas32(&a32, &y, y+x, &rwl)) ;
    while (!my_atomic_cas32(&a32, &y, y-x, &rwl)) ;
  }
  pthread_mutex_lock(&mutex);
  N--;
  if (!N) pthread_cond_signal(&cond);
  pthread_mutex_unlock(&mutex);
}

void test_atomic(const char *test, pthread_handler handler, int n, int m)
{
  pthread_t t;
  ulonglong now=my_getsystime();

  my_atomic_store32(&a32, 0, &rwl);
  my_atomic_store32(&b32, 0, &rwl);
  my_atomic_store32(&c32, 0, &rwl);

  printf("Testing %s with %d threads, %d iterations... ", test, n, m);
  for (N=n ; n ; n--)
    pthread_create(&t, &thr_attr, handler, &m);

  pthread_mutex_lock(&mutex);
  while (N)
    pthread_cond_wait(&cond, &mutex);
  pthread_mutex_unlock(&mutex);
  now=my_getsystime()-now;
  printf("got %lu in %g secs\n", my_atomic_load32(&a32, &rwl),
         ((double)now)/1e7);
}

int main()
{
  int err;

#ifdef _IONBF
  setvbuf(stdout, 0, _IONBF, 0);
#endif
  printf("N CPUs: %d\n", my_getncpus());

  if ((err= my_atomic_initialize()))
  {
    printf("my_atomic_initialize() failed. Error=%d\n", err);
    return 1;
  }

  pthread_attr_init(&thr_attr);
  pthread_attr_setdetachstate(&thr_attr,PTHREAD_CREATE_DETACHED);
  pthread_mutex_init(&mutex, 0);
  pthread_cond_init(&cond, 0);
  my_atomic_rwlock_init(&rwl);

  test_atomic("my_atomic_add32", test_atomic_add_handler, 100,1000000);
  test_atomic("my_atomic_swap32", test_atomic_swap_handler, 100,1000000);
  test_atomic("my_atomic_cas32", test_atomic_cas_handler, 100,1000000);

  pthread_mutex_destroy(&mutex);
  pthread_cond_destroy(&cond);
  pthread_attr_destroy(&thr_attr);
  my_atomic_rwlock_destroy(&rwl);
  return 0;
}

