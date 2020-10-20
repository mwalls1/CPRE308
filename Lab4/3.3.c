#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
void divZero();
int main()
{
	signal(SIGFPE, divZero);
	int a = 4;
	a = a/0;
	return 0;
}
void divZero()
{
	printf("Caught a SIGFPE.\n");
	exit(0);
}
