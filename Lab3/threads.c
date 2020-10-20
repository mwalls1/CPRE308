#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
void * thread1(void * ptr);
void * thread2(void * ptr);
int main()
{	
	void* val;
	pthread_t t1;
	pthread_t t2;
	pthread_create(NULL, NULL, thread1, NULL);//create thread using t1's address, and the function it will run
	pthread_create(NULL, NULL, thread2, NULL);//create thread using t2's address and the function it will run
	printf("Hello from the main thread\n");
	return 0;
}
void * thread1(void * ptr)
{
	sleep(5);
	printf("Hello from thread 1\n");
}
void * thread2(void * ptr)
{
	sleep(5);
	printf("Hello from thread 2\n");
}
