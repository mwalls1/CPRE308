#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include "Bank.c"
//All comments from server.c are applicable to server2.c
//This project was copied before the comments were added
struct trans { // structure for a transaction pair
	int acc_id; // account ID
	int amount; // amount to be added, could be positive or negative
};
struct request {
	struct request * next; // pointer to the next request in the list
	int request_id; // request ID assigned by the main thread
	int check_acc_id; // account ID for a CHECK request
	struct trans * transactions; // array of transaction data
	int num_trans; // number of accounts in this transaction
	struct timeval starttime, endtime; // starttime and endtime for TIME
	int type;
};
struct queue {
	struct request * head, * tail; // head and tail of the list
	int numJobs; // number of jobs currently in queue
};
void * request(void * ptr);
struct request *dequeue(struct queue *q);
char** string_parse(char* input);
void printq();
void destroy(struct queue *q);
void enqueue(struct queue *q, struct request *req);
struct queue *initQueue();
int end = 0;
pthread_mutex_t qLock;
pthread_mutex_t bLock;
struct queue *q;
pthread_cond_t jobs;
int idleThreads;
FILE *file;
int NUM_ACTS;
int NUM_THREADS;
int activeThreads;
void swap(int *xp, int *yp);
int main(int argc, char** argv)
{	
	struct timeval time;
	pthread_mutex_init(&qLock, NULL);
	pthread_mutex_init(&bLock, NULL);
	q = initQueue();
	int requestID = 1;
	void *val;
	if(argc !=4){
		printf("Not enough arguments!\n");
		exit(0);
	}
	else if(argc == 4){
		file = fopen(argv[3], "w");
		NUM_ACTS = atoi(argv[2]);
		NUM_THREADS = atoi(argv[1]);
		
	}
	idleThreads = 0;
	initialize_accounts(NUM_ACTS);
	pthread_t threads[NUM_THREADS];
	activeThreads = NUM_THREADS;
	int i = 0;
	for(i = 0; i < NUM_THREADS; i ++)
	{
		pthread_create(&threads[i], NULL, request, val);
	}
	char *input = malloc(sizeof(char) * 100);
	char **args;





	while(1){
		size_t bufferLen = 100;
		getline(&input, &bufferLen, stdin);
	     	args = string_parse(input);
		int k = 0;
		if(strcmp(args[0], "CHECK")==0)
		{
			printf("ID %d\n",requestID);
			struct request *r = (struct request*) malloc(sizeof(struct request)*10);
			r->request_id = requestID;
			r->check_acc_id = atoi(args[1]);
			r->type = 1;
			struct timeval time;
			gettimeofday(&time, NULL);
			r-> starttime = time;
			pthread_mutex_lock(&qLock);
			enqueue(q, r);
			pthread_cond_broadcast(&jobs);
			pthread_mutex_unlock(&qLock);
			requestID++;
			
		}
		if(strcmp(args[0], "TRANS")==0)
		{	
			printf("ID %d\n",requestID);
			int k = 0;
			while(args[k] != NULL)
				k++;
			if((k-1)%2 != 0)
			{
				printf("Missing arguments\n");
			}
			else
			{
				struct request *r = (struct request*) malloc(sizeof(struct request)*10);
				r-> request_id = requestID;
				r->num_trans = (k-1)/2;
				r->type = 0;
				struct trans *tran = malloc(sizeof(struct trans)*k);
				int i = 0;
				int j = 1;
				for(i; i < r->num_trans; i ++)
				{
					tran[i].acc_id = atoi(args[j]);
					j++;
					tran[i].amount = atoi(args[j]);
					j++;
				}
				for(i = 0; i < r->num_trans; i ++)
				{
					for(j = 0; j < r->num_trans; j++)
					{
						if(tran[j].acc_id > tran[i].acc_id)
						{	
							swap(&tran[j].acc_id, &tran[i].acc_id);
							swap(&tran[j].amount, &tran[i].amount);
						}
					}
				}	
				r->transactions = tran;
				struct timeval time;
				gettimeofday(&time, NULL);
				r-> starttime = time;
				pthread_mutex_lock(&qLock);
				enqueue(q, r);
				pthread_cond_broadcast(&jobs);
				pthread_mutex_unlock(&qLock);
			}
			requestID++;
		}
		if(strcmp(args[0], "END")==0)
		{	
			
			int i = 0;
			while(idleThreads<NUM_THREADS)
			{
				//wait
			}
			end = 1;
			free_accounts();
			return 0;
		}
	}
	
}
void * request(void * ptr)
{
	int error = 0;
	while(end == 0)
	{
		pthread_mutex_lock(&qLock);
		while(q->numJobs==0)
		{
			idleThreads++;
			pthread_cond_wait(&jobs, &qLock);
			idleThreads--;
		}
		struct request *req = q->head;
		dequeue(q);
		pthread_mutex_unlock(&qLock);
		if(req->type == 1)
		{
			pthread_mutex_lock(&bLock);
			if(req->check_acc_id < 1 || req->check_acc_id > NUM_ACTS)
			{
				printf("Invalid Account\n");
				error = 1;
			}
			struct timeval end;
			gettimeofday(&end, NULL);
			req-> endtime = end;
			int reqid = req->request_id;
			int bal = read_account(req->check_acc_id);
			fprintf(file,"%d BAL %d TIME %ld.%06ld %ld.%06ld\n", reqid, bal, req->starttime.tv_sec, req->starttime.tv_usec, req->endtime.tv_sec, req->endtime.tv_usec);
			pthread_mutex_unlock(&bLock);
		}
		else
		{
			int reqid = req->request_id;
			int i = 0;
			if(req->num_trans > 10)
			{
					printf("Too many transactions\n");
					error = 1;
			}
			else
			{
				int j = 0;
				pthread_mutex_lock(&bLock);
				for(i = 0; i < req->num_trans; i++)
				{
					int id = req->transactions[i].acc_id;
					int add = req->transactions[i].amount;
					int curBal = read_account(id);
					if(id < 1 || id > NUM_ACTS)
					{
						printf("Invalid Account\n");
						error = 1;
						break;
					}	
					if(curBal + add < 0)
					{
						printf("Not enough funds\n");
						struct timeval end;
						gettimeofday(&end, NULL);
						req-> endtime = end;
						fprintf(file,"%d ISF %d TIME %ld.%06ld %ld.%06ld\n", reqid, id, req->starttime.tv_sec, req->starttime.tv_usec, req->endtime.tv_sec, req->endtime.tv_usec);
						error = 1;
						break;
					}
				}
			}
			if(error == 0)
			{
				for(i = 0; i < req->num_trans; i++)
			 	{
			 		int id = req->transactions[i].acc_id;
					int curBal = read_account(id);
					int add = req->transactions[i].amount;
					write_account(id, curBal+add);
				}
				struct timeval end;
				gettimeofday(&end, NULL);
				req->endtime = end;
				fprintf(file,"%d OK TIME %ld.%06ld %ld.%06ld\n", reqid, req->starttime.tv_sec, req->starttime.tv_usec, req->endtime.tv_sec, req->endtime.tv_usec);
			}
			pthread_mutex_unlock(&bLock);
		}
		error = 0;
	}
	printf("Thread complete\n");
	return NULL;
}
void enqueue(struct queue *q, struct request *req)
{
	if(q->numJobs == 0)
	{
		q->head = req;
		q->tail = req;
	}
	else
	{
		q->tail->next = req;
		q->tail = req;
	}
	
	q->numJobs++;
}
struct request *dequeue(struct queue *q)
{
	if(q->numJobs == 0) return NULL;
	
	struct request *req = q->head;
	
	q->head = q->head->next;
	q->numJobs--;
	return NULL;
	
}

void destroy(struct queue *q)	
{
	struct request *req;
	while (q->numJobs != 0) 
	{
		req = dequeue(q);
		//free(req);
   	}
//free(q);
}
struct queue *initQueue()
{
	struct queue *q = (struct queue*) malloc(sizeof(struct queue));
	q->numJobs = 0;
	q->head = NULL;
	q->tail = NULL;
	
	return q;
}
char** string_parse(char* input)
{
	char** arguments = malloc(100*sizeof(char*));
	char *delim = " ";
	char *ptr = malloc(100 * sizeof(char));
	ptr =  strtok(input, delim);
	if(ptr[strlen(ptr)-1] == '\n')
	{
		ptr[strlen(ptr)-1] = '\0';
	}
	else
	{
		ptr[strlen(ptr)] = '\0';
	}
	arguments[0] = ptr;
	int i = 1;
	ptr = strtok(NULL, delim);
	while(ptr != NULL)
	{
		if(ptr[strlen(ptr)-1] == '\n')
		{
			ptr[strlen(ptr)-1] = '\0';
		}
		else
		{
			ptr[strlen(ptr)] = '\0';
		}
		arguments[i] = ptr;
		ptr = strtok(NULL, delim);
		i++;
	}
        arguments[i] = NULL;
	return arguments;
}
void swap(int *xp, int *yp)
{
	int temp = *xp;
	*xp = *yp;
	*yp = temp;
}
