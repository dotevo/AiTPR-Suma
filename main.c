#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <mpi.h>
#include <stdbool.h>

void tokenService(int rank, int size, int next, int rcvToken, int tasksNum, int * flag, MPI_Request request, int tokenTag, int finishTag, int * tokenSent) {
    printf("rank: %d token service\n", rank);
    if(rank == 0) {
	printf("0 received token ");
	if(rcvToken == 0)
	{
	    printf("token is white\n");
	    int finished = 1;
	    int i;
	    for(i=0; i<size; ++i)
	    {
		MPI_Isend(&finished, 1, MPI_INT, i, finishTag, MPI_COMM_WORLD, &request);
	    }
	}
	else
	{
	    printf("token is black\n");
	    *tokenSent = 0;
	    // do nothing
	}
    } else { // other than 0
	if(rcvToken == 1)
	{
	    MPI_Isend(&rcvToken, 1, MPI_INT, next, tokenTag, MPI_COMM_WORLD, &request); // black
	}
	else
	{
	    if(tasksNum > 0)
	    {
		int black = 1;
		MPI_Isend(&black, 1, MPI_INT, next, tokenTag, MPI_COMM_WORLD, &request); // black
	    }
	    else
	    {
		if(*flag == 1)
		{
		    int black = 1;
		    MPI_Isend(&black, 1, MPI_INT, next, tokenTag, MPI_COMM_WORLD, &request); // black
		    flag = 0;
		}
		else
		{
		    MPI_Isend(&rcvToken, 1, MPI_INT, next, tokenTag, MPI_COMM_WORLD, &request); // received
		}
	    }
	}
    }
}

int main(int argc, char ** argv) {
    int rank;
    int next;
    int prev;
    int random; // proces ktory bedzie pytal o zadanie
    int token;
    int size;
    int tasksNum;
    int flag;
    int finished;
    int msg;
    int tokenSent;
    
    int tokenTag = 0;
    int tasksRequestTag = 1;
    int tasksNumTag = 2;
    int tasksSendTag = 3;
    int finishTag = 4;
    
    MPI_Status status;
    MPI_Request request;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    tasksNum = rank;
    int i=0;
    
    next = (rank + 1) % size;
    prev = (rank - 1 + size) % size;
    random = next; // tymczasowo
    
    finished = 0;
    tokenSent = 0;
    while(finished != 1)
    {
	if(tasksNum == 0) 
	{
	    //printf("rank: %d sent req for task\n", rank);
	    int tmp = 1;
	    //MPI_Isend(&tmp, 1, MPI_INT, random, tasksRequestTag, MPI_COMM_WORLD, &status);
	    int received = 0;
	    while(received != 0)
	    {
		MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status); // blokujace
		MPI_Recv(&msg, 1, MPI_INT, status.MPI_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		if(status.MPI_TAG == tasksNumTag) // liczba zadan jakie dostanie
		{
		    // zapisac ta liczbe
		    received = 1;
		}
		else if(status.MPI_TAG == tokenTag) // token
		{
		    tokenService(rank, size, next, msg, tasksNum, &flag, request, tokenTag, finishTag, &tokenSent);
		}
		else if(status.MPI_TAG == finishTag) // praca skonczona
		{
		    finished = 1;
		}
	    }
	    received = 0;
	    while(received != 0)
	    {
		MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status); // blokujace
		MPI_Recv(&msg, 1, MPI_INT, status.MPI_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		if(status.MPI_TAG == tokenTag) // token
		{
		    tokenService(rank, size, next, msg, tasksNum, &flag, request, tokenTag, finishTag, &tokenSent);
		}
		else if(status.MPI_TAG == tasksSendTag) // zadania
		{
		    // odebrac zadania (w liczbie wczesniej ustalonej)
		}
		else if(status.MPI_TAG == finishTag) // praca skonczona
		{
		    finished = 1;
		}
	    }
	    random = (random + 1) % size; // nastepnym razem zapyta kolejnego
	} // end if(tankNum == 0)
	
	if(tasksNum > 0) 
	{
	    // obsloz 1 zadanie
	    tasksNum--;
	} // end if(taskNum >0)
	
	int waiting = 0;
	MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &waiting, &status); // nieblokujace
	if(waiting == 1) // nie wiem jak w tym IProbe sie to sprawdza...
	{
	    //printf("rank: %d probing => waiting: %d\n", rank, waiting);
	    MPI_Recv(&msg, 1, MPI_INT, status.MPI_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
	    //printf("rank: %d received with tag: %d\n", rank, status.MPI_TAG);
	    
	    if(status.MPI_TAG == tasksRequestTag) // prosba o zadanie
	    {
		printf("rank: %d taskReq\n", rank);
		//MPI_Isend(0, 1, MPI_INT, status.MPI_SOURCE, tasksNumTag, MPI_COMM_WORLD, &request);
		//MPI_Isend(0, 1, MPI_INT, status.MPI_SOURCE, tasksSendTag, MPI_COMM_WORLD, &request);
		//if(status.MPI_SOURCE < rank)
		//{
		    flag = 1;
		//}
		// wyslij ile zadan chcesz dac
		// moze poczekac, zeby byla pewnosc, ze tamten dostanie najpierw liczbe zadan a potem te zadania?
		// wyslij te zadania
	    }
	    else if(status.MPI_TAG == tokenTag) // token
	    {
		//printf("rank: %d token\n", rank);
		tokenService(rank, size, next, msg, tasksNum, &flag, request, tokenTag, finishTag, &tokenSent);
	    }
	    else if(status.MPI_TAG == finishTag) // praca skonczona
	    {
		printf("rank: %d fin\n", rank);
		finished = 1;
	    }
	}
	if(rank == 0 && tasksNum == 0 && tokenSent != 1)
	{
	    printf("0 sent token\n");
	    int white = 0;
	    MPI_Isend(&white, 1, MPI_INT, next, tokenTag, MPI_COMM_WORLD, &request);
	    tokenSent = 1;
	}
    } // end while(finished != 1)
    
    printf("rank: %d finished!\n", rank);
    
    MPI_Finalize();
    return 0;
}
