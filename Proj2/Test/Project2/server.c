#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include "Bank.c"
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
//Function prototypes
void * request(void * ptr);
struct request *dequeue(struct queue *q);
char** string_parse(char* input);
void printq();
void destroy(struct queue *q);
void enqueue(struct queue *q, struct request *req);
struct queue *initQueue();
int end = 0;//Stops threads
pthread_mutex_t qLock;//Mutexes for queue and accounts
pthread_mutex_t *locks;
struct queue *q;//queue
pthread_cond_t jobs;//wait variable
int idleThreads;
FILE *file;//file to write to
//num accounts and threads
int NUM_ACTS;
int NUM_THREADS;
//used to determine when program terminates
int activeThreads;
void swap(int *xp, int *yp);//prototype
int main(int argc, char** argv)
{	
    //initialize some starter mutexes etc.
	struct timeval time;
	pthread_mutex_init(&qLock, NULL);
	q = initQueue();
	int requestID = 1;
	void *val;
    //Get user input, make sure its enough
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
	pthread_mutex_t *lockArr = malloc(sizeof(pthread_mutex_t)*NUM_ACTS);//initialize memory for global mutex array
    //initialize accounts, instantiate threads
	initialize_accounts(NUM_ACTS);
	pthread_t threads[NUM_THREADS];
	activeThreads = NUM_THREADS;
	int i = 0;
	for(i = 0; i < NUM_THREADS; i ++)//create all threads
	{
		pthread_create(&threads[i], NULL, request, val);
	}
	for(i = 0; i < NUM_ACTS; i++)//init all mutex locks
	{
		pthread_mutex_init(&lockArr[i], NULL);	
	}
	locks = malloc(sizeof(lockArr));
	locks = lockArr;//allocate and set global locks
	char *input = malloc(sizeof(char) * 100);
	char **args;





	while(1){//wait for input
		size_t bufferLen = 100;
		getline(&input, &bufferLen, stdin);
	    args = string_parse(input);
		int k = 0;
		if(strcmp(args[0], "CHECK")==0)//look for a check request
		{
			printf("ID %d\n",requestID);
			struct request *r = (struct request*) malloc(sizeof(struct request)*10);//create a request
            //set request info
			r->request_id = requestID;
			r->check_acc_id = atoi(args[1]);
			r->type = 1;
            //get start time
			struct timeval time;
			gettimeofday(&time, NULL);
			r-> starttime = time;
			pthread_mutex_lock(&qLock);//lock queue
			enqueue(q, r);//add request to queue
			pthread_cond_broadcast(&jobs);//tell threads there is a job
			pthread_mutex_unlock(&qLock);//unlock queue
			requestID++;
			
		}
		if(strcmp(args[0], "TRANS")==0)//Check for a trans request
		{	
			printf("ID %d\n",requestID);
			int k = 0;
			while(args[k] != NULL)//check for enough args
				k++;
			if((k-1)%2 != 0)
			{
				printf("Missing arguments\n");
			}
			else
			{
				struct request *r = (struct request*) malloc(sizeof(struct request)*10);//create enough request memory, set proper dataa
				r-> request_id = requestID;
				r->num_trans = (k-1)/2;
				r->type = 0;
				struct trans *tran = malloc(sizeof(struct trans)*k);//create a transaction array
				int i = 0;
				int j = 1;
				for(i; i < r->num_trans; i ++)//add trans data to array
				{
					tran[i].acc_id = atoi(args[j]);
					j++;
					tran[i].amount = atoi(args[j]);
					j++;
				}
				for(i = 0; i < r->num_trans; i ++)//sort trans data using selection sort. Slow but ok for a max of 10 items
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
				struct timeval time;//get the start time
				gettimeofday(&time, NULL);
				r-> starttime = time;
				pthread_mutex_lock(&qLock);//lock the queue
				enqueue(q, r);//add job
				pthread_cond_broadcast(&jobs);//tell threads there is work
				pthread_mutex_unlock(&qLock);//unlock queue
			}
			requestID++;
		}
		if(strcmp(args[0], "END")==0)//check for an end request
		{	
			
			int i = 0;
			while(idleThreads<NUM_THREADS)//wait for threads to finish their jobs
			{
				//wait
			}
			end = 1;//end the while loop
			free_accounts();//free all bank accounts
			return 0;// end program
		}
	}
	
}
void * request(void * ptr)//request handler
{
	int error = 0;
	while(end == 0)
	{
		pthread_mutex_lock(&qLock);//lock the queue
		while(q->numJobs==0)
		{
			idleThreads++;
			pthread_cond_wait(&jobs, &qLock);//wait for a job
			idleThreads--;
		}
		struct request *req = q->head;//take the first job
		dequeue(q);//remove from queue
		pthread_mutex_unlock(&qLock);//unlock queue
		if(req->type == 1)//Check request
		{
			pthread_mutex_lock(&locks[req->check_acc_id-1]);//lock account
			if(req->check_acc_id < 1 || req->check_acc_id > NUM_ACTS)//Make sure its valid
			{
				printf("Invalid Account\n");
				error = 1;
			}
			struct timeval end;//get end time
			gettimeofday(&end, NULL);
			req-> endtime = end;
			int reqid = req->request_id;
			int bal = read_account(req->check_acc_id);//read balance
			fprintf(file,"%d BAL %d TIME %ld.%06ld %ld.%06ld\n", reqid, bal, req->starttime.tv_sec, req->starttime.tv_usec, req->endtime.tv_sec, req->endtime.tv_usec);//print info
			pthread_mutex_unlock(&locks[req->check_acc_id-1]);//unlock account
		}
		else//Trans request
		{
			int reqid = req->request_id;
			int i = 0;
			if(req->num_trans > 10)//Make sure not too many transactions
			{
					printf("Too many transactions\n");
					error = 1;
			}
			else
			{
				int j = 0;
				for(i = 0; i < req->num_trans; i++)
				{
					pthread_mutex_lock(&locks[req->transactions[i].acc_id-1]);//Lock all associated accounts
				}
				for(i = 0; i < req->num_trans; i++)//Process all requests
				{
                    //gather temp data
					int id = req->transactions[i].acc_id;
					int add = req->transactions[i].amount;
					int curBal = read_account(id);
					if(id < 1 || id > NUM_ACTS)//Check for valid accounts
					{
						printf("Invalid Account\n");
						error = 1;
						break;
					}	
					if(curBal + add < 0)//check for fund problems
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
			if(error == 0)//if no errors, proceed
			{
				for(i = 0; i < req->num_trans; i++)//write balance to account
			 	{
			 		int id = req->transactions[i].acc_id;
					int curBal = read_account(id);
					int add = req->transactions[i].amount;
					write_account(id, curBal+add);
				}
				struct timeval end;//get end time
				gettimeofday(&end, NULL);
				req->endtime = end;
				fprintf(file,"%d OK TIME %ld.%06ld %ld.%06ld\n", reqid, req->starttime.tv_sec, req->starttime.tv_usec, req->endtime.tv_sec, req->endtime.tv_usec);//print data
			}
			for(i = 0; i < req->num_trans; i++)
			{
				pthread_mutex_unlock(&locks[req->transactions[i].acc_id-1]);//unlock all accounts
			}
		}
		error = 0;
	}
	printf("Thread complete\n");
	return NULL;
}
void enqueue(struct queue *q, struct request *req)//enqueue functions
{
	if(q->numJobs == 0)//when no jobs, add it to the first
	{
		q->head = req;
		q->tail = req;
	}
	else//add to the end
	{
		q->tail->next = req;
		q->tail = req;
	}
	
	q->numJobs++;
}
struct request *dequeue(struct queue *q)//remove from queue
{
	if(q->numJobs == 0) return NULL;//if empty, do nothing
	
	struct request *req = q->head;//give the first element
	
	q->head = q->head->next;//make first element the proceeding
	q->numJobs--;
	return NULL;
	
}

void destroy(struct queue *q)//remove the queue	
{
	struct request *req;
	while (q->numJobs != 0) 
	{
		req = dequeue(q);
		//free(req);
   	}
//free(q);
}
struct queue *initQueue()//initialize queue for memory etc
{
	struct queue *q = (struct queue*) malloc(sizeof(struct queue));
	q->numJobs = 0;
	q->head = NULL;
	q->tail = NULL;
	
	return q;
}
char** string_parse(char* input)//string parsing from Proj 1
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
void swap(int *xp, int *yp)//swap data for sorting
{
	int temp = *xp;
	*xp = *yp;
	*yp = temp;
}
