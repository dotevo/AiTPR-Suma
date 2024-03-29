#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <mpi.h>
#include "list.c"

int WHITE = 0;
int BLACK = 1;

//GLOBAL VALUES
//Init head
Item *head=0;
int numbersCount=0;
int numbersSum=0;
int *numbers=0;
int rank=0;
int next=0;
int prev=0;
int size=0;
int flag=0;
int finished=0;
int tokenSent=0;

//TAGS
int TOKEN               = 0;
int TASKS_REQUEST       = 1;
int TASKS_NUMBER        = 2;
int TASKS_VALUES        = 3;
int FINISH              = 4;

//-1 zakres przekroczony
//0 szukaj dalej
// 1 wynik
int result(int *value){
        int i		=0;
        int sum		=0;
        int count	=0;
        int antcount	=0;
        //Sumuj az nie trafisz na -1 lub koniec
        while(i<numbersCount && value[i]!=-1){
                //printf("n: %d v: %d\n",numbers[i],value[i]);
                sum	+=numbers[i]*value[i];
                count	+=value[i];
                antcount+=value[i]==0?1:0;
//		if(rank!=0){
//			printf("V %d\n",value[i]);
//		}
                i++;
        }

//	if(rank!=0){
//		printf("SUM: %d COUNT:%d SUMALL:%d COUNTALL:%d\n",sum,count,numbersSum,numbersCount);
//	}

        //Sprawdzanie czy zakres nie zostal przekroczony
        if( sum*2 > numbersSum || count*2 >numbersCount || antcount*2 >numbersCount)
                return -1;

        //Sprawdzanie czy to nie jest wynik
        if( sum*2 == numbersSum && count *2 == numbersCount)
                return 1;

        //jesli doszlo do konca, a nie wynik!
        if( i>=numbersCount){
                return -1;
        }

        return 0;
}

void tokenService(int rcvToken, int tasksNum) {
    printf("%d token service\n", rank);

    MPI_Request request;
    if(rank == 0) { // jezeli odbiera 0 - token zrobil pelne kolo
	printf("0 received token ");
        if(rcvToken == 0) // token bialy - wszystkie skonczyly prace
	{
	    printf("token is white\n");
            finished = 1;
	    int i;
	    for(i=0; i<size; ++i)
            {
                MPI_Isend(&finished, 1, MPI_INT, i, FINISH, MPI_COMM_WORLD, &request);
	    }
	}
        else // token czarny - ktos cos robi
	{
	    printf("token is black\n");
            tokenSent = 0;
	}
    } else { // jezeli odbiera inny niz 0
        if(rcvToken == 1) // odebral czarny - przesyla dalej
        {
            MPI_Isend(&rcvToken, 1, MPI_INT, next, TOKEN, MPI_COMM_WORLD, &request); // black
	}
        else // odebral bialy - sprawdza swoj stan
	{
            if(tasksNum > 0) // ma zadania - wysyla czarny
            {
                MPI_Isend(&BLACK, 1, MPI_INT, next, TOKEN, MPI_COMM_WORLD, &request); // black
	    }
            else // nie ma zadan
	    {
                if(flag == BLACK) // czy wyslal do ranku mniejszego niz swoj
                {
                    MPI_Isend(&BLACK, 1, MPI_INT, next, TOKEN, MPI_COMM_WORLD, &request); // black
                    flag = WHITE;
		}
                else // jest czysty
                {
                    MPI_Isend(&rcvToken, 1, MPI_INT, next, TOKEN, MPI_COMM_WORLD, &request); // received
		}
	    }
	}
    }
}


int* getHalfTasks(Item** head,int *count){
        //Trzymaj glowe
        Item*tmp=*head;
        *count=listCount(*head);
        //Gdy nie mam czym sie podzielic
        if(*count<=1)
                return 0;

        *count=(*count)/2;
        int *values=(int*)malloc(sizeof(int)*(*count)*numbersCount);
        int i=0;
        while( *head!=0 && (*head)->next!=0 && *count>i ){
                //wez zadanie o nr i+1
                Item* item=listTakeAt(head,i+1);
                if(item!=0){
                        //kopiuj
                        int j=0;
                        for(j=0;j<numbersCount;j++)
                                *(values+i*numbersCount+j)=item->val[j];			
                        free(item->val);
                        free(item);			
                }
                i++;
        }
        return values;
}


//Return new head
Item* tasksToItems(int* data,int count){
        Item* head=0;

        int a=0;
        for(a=0;a<count;a++){
                Item* item=(Item*)malloc(sizeof(Item));
                item->next=0;
                item->val=(int*)malloc(sizeof(int)*numbersCount);
		int w=0;
		for(;w<numbersCount;w++){
			item->val[w]=data[numbersCount*a+w];
		}
                head=listAddItemOnBegin(item, head);
        }

        return head;
}

int main(int argc, char ** argv) {
    int random; // proces ktory bedzie pytal o zadanie
    int tasksNum;
    int msg;
    int waiting;

    MPI_Status status;
    MPI_Request request;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    tasksNum = 0;
    
    next = (rank + 1) % size;
    prev = (rank - 1 + size) % size;
    random = (rank == 0) ? 1 : 0; // tymczasowo

    int i=0;

    //----------Char to Number-------------
    numbersCount	=argc-1;
    numbers         	=(int*)malloc(sizeof(int)*(argc-1));

    //ustaw z argumentow wartosci ciagu
    for(i=0;i<argc-1;i++){
 	 numbers[i]=atoi(argv[i+1]);               
         numbersSum+=numbers[i];
    }

    if(numbersCount%2!=0){
        if(rank==0)
	    printf("DEBIL! jest bledny ciag!\n");
        goto clear;
    }

    if(rank == 0) // 0 inicjalizuje zadanie
    {
	//wyswietl ciag
        printf("Ciag: ");
        for(i=0;i<numbersCount;i++)
                printf("%d,",numbers[i]);
        printf("\n SUM: %d COUNT %d\n",numbersSum,numbersCount);

	//Pierwsze zadanie (wartosci)
	int *firstTask  =(int*)malloc(sizeof(int)*(argc-1));
        for(i=0;i<numbersCount;i++){                
                firstTask[i]=-1;               
        }
       
        //Stworz pierwsze zadanie
        Item *fitem=(Item*)malloc(sizeof(Item));
        fitem->next=0;
        fitem->val=firstTask;
        head=listAddItemOnBegin(fitem, head);
        
        tasksNum = 1;
    }

    finished = 0;
    tokenSent = 0;
    Item *item=0;
    while(finished != 1) // rob az skonczysz ;)
    {
        if(tasksNum == 0) // nie ma zadan
        {
	    if(random == rank)
	    {
		random = next;
	    }
	    
            printf("%d sends request to %d.\n", rank, random);
            int tmp = 1; // cokolwiek - tresc nie jest wazna, tylko tag
            MPI_Isend( &tmp, 1, MPI_INT, random, TASKS_REQUEST, MPI_COMM_WORLD, &request);
	    int received = 0;
            while(received != 1) // nasluchuje, az dostanie to na co czeka (liczbe zadan)
	    {
		MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status); // blokujace
		MPI_Recv(&msg, 1, MPI_INT, status.MPI_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
//		printf("slucham :(\n");
                // po tagu sprawdza co dostal
                if(status.MPI_TAG == TASKS_NUMBER) // liczba zadan jakie dostanie
		{
                    tasksNum = msg;
		    received = 1;
		}
                else if(status.MPI_TAG == TOKEN) // token
		{
                    tokenService(msg, tasksNum);
		}
                else if(status.MPI_TAG == TASKS_REQUEST) // prosba o zadanie
                {
                    //printf("%d received request from %d.\n", rank, status.MPI_SOURCE);
                    int tasksToSendNum = 0;
                    MPI_Isend(&tasksToSendNum, 1, MPI_INT, status.MPI_SOURCE, TASKS_NUMBER, MPI_COMM_WORLD, &request);
                }
                else if(status.MPI_TAG == FINISH) // praca skonczona
		{
                    printf("%d received FINISH from %d.\n", rank, status.MPI_SOURCE);
		    finished = 1;
                    received = 1; // zeby wyskoczyc z petli
		}
	    }
            printf("%d gets %d tasks from %d.\n", rank, tasksNum, random);
            if(tasksNum > 0) // odebral 0 - nie dostanie zadan
            {
                received = 0;
                while(received != 1)
                {
                    MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status); // blokujace
                    // po tagu sprawdza co dostal
                    if(status.MPI_TAG == TOKEN) // token
                    {			
                        MPI_Recv(&msg, 1, MPI_INT, status.MPI_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                        tokenService(msg, tasksNum);
                    }
                    else if(status.MPI_TAG == TASKS_VALUES) // zadania
                    {                        
                        // odebrac zadania (w liczbie wczesniej ustalonej) i dodac do listy			
			int *tasksMSG=(int*)malloc(sizeof(int)*tasksNum * numbersCount);
                        MPI_Recv(tasksMSG, tasksNum * numbersCount, MPI_INT, status.MPI_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);                        
/* Testowanie odbioru
			int z=0;
			printf("\n");
                        for(z=0;z<numbersCount*tasksNum;z++)
				printf("R%d,",tasksMSG[z]);
*/
			head = tasksToItems(tasksMSG, tasksNum);
			free(tasksMSG);
			received = 1;
                    }
                    else if(status.MPI_TAG == TASKS_REQUEST) // prosba o zadanie
                    {
                        //printf("%d received request from %d.\n", rank, status.MPI_SOURCE);
                        int tasksToSendNum = 0;
                        MPI_Isend(&tasksToSendNum, 1, MPI_INT, status.MPI_SOURCE, TASKS_NUMBER, MPI_COMM_WORLD, &request);
        	    }
                    else if(status.MPI_TAG == FINISH) // praca skonczona
                    {
                        printf("%d received FINISH from %d.\n", rank, status.MPI_SOURCE);
                        finished = 1;
                        received = 1; // zeby wyskoczyc z petli
                    }
                }
            }
	    random = (random + 1) % size; // nastepnym razem zapyta kolejnego
	} // end if(tankNum == 0)
	
        if(tasksNum > 0) // ma zadania
        {
            //
            // Obsluga zadania
            //
//            printf("%d | List count %d \n", rank, listCount(head));
            //wez pierwsze zadanie
            item=listTakeAt(&head,0);
            //przetestuj czy jest prawdziwe (nie przekroczono limitow) lub czy nie jest rozwiazaniem
            int n=result(item->val);
            //printf("%d\n",n);
            if(n==0){
                    //Tworzenie nowych zadan
                    Item *a	=(Item*)malloc(sizeof(Item));
                    a->val  =(int*)malloc(sizeof(int)*(argc-1));
                    Item *b	=(Item*)malloc(sizeof(Item));
                    b->val  =(int*)malloc(sizeof(int)*(argc-1));
                    //Ustaw wartosci
                    int p=0;
                    int type=0;


                    for(p=0;p<numbersCount;p++){
                            if((item->val)[p]==-1 && type==0 )
                                    type=1;

                            //kopiowanie
                            if(type==0){
                                    a->val[p]=item->val[p];
                                    b->val[p]=item->val[p];
                            }
                            //Dodawanie
                            else if(type==1){
                                    a->val[p]=1;
                                    b->val[p]=0;
                                    type=2;
                            }
                            //uzupelnianie
                            else{
                                    a->val[p]=-1;
                                    b->val[p]=-1;
                            }
                    }
                    head=listAddItemOnBegin(a, head);
                    head=listAddItemOnBegin(b, head);
		    tasksNum += 2;
            }
            else if(n==1){
                    int z=0;
                    /*printf("%d: Wynik: ",rank);
                    for(z=0;z<numbersCount;z++){
                            if(item->val[z]==1)
                                    printf("%d,",numbers[z]);
                    }
                    printf("\n");*/
            }

            //Wyczysc po zadaniu
            free( item->val);
            free( item );

	    tasksNum--;
	} // end if(taskNum >0)
	
        // na koniec zawsze sprawdza, czy ktos czegos nie chcial
        waiting = 0;
	MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &waiting, &status); // nieblokujace
        if(waiting == 1)
        {
            int waitingCount;
            MPI_Get_count(&status, MPI_INT, &waitingCount);
            printf("%d has %d tasks in queue.\n", rank, waitingCount);
            for(i=0; i<waitingCount; ++i)
            {
                MPI_Recv(&msg, 1, MPI_INT, status.MPI_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                // po tagu sprawdza co dostal
                if(status.MPI_TAG == TASKS_REQUEST) // prosba o zadanie
                {
                    printf("%d received request from %d.\n", rank, status.MPI_SOURCE);
                    // wyslij ile zadan chcesz dac
                    int tasksToSendNum;
                    int * tasksToSend;
                    tasksToSend = getHalfTasks(&head, &tasksToSendNum);
    /* Test wysylania
                    int ii=0;
                    for(;ii<tasksToSendNum*numbersCount;ii++){
                            printf("%d,",tasksToSend[ii]);
                    }
    */

                    MPI_Isend(&tasksToSendNum, 1, MPI_INT, status.MPI_SOURCE, TASKS_NUMBER, MPI_COMM_WORLD, &request);


                    // moze poczekac, zeby byla pewnosc, ze tamten dostanie najpierw liczbe zadan a potem te zadania?
                    // wyslij te zadania
                    printf("%d sends %d tasks to %d.\n", rank, tasksToSendNum, status.MPI_SOURCE);
                    if(tasksToSendNum != 0) {
                        MPI_Isend(tasksToSend, tasksToSendNum * numbersCount, MPI_INT, status.MPI_SOURCE, TASKS_VALUES, MPI_COMM_WORLD, &request);
                        if(status.MPI_SOURCE < rank) // jezeli wyslal do wczesniejszego - ustawia flage na czarna
                        {
                            flag = BLACK;
                        }
                    }
                    free(tasksToSend);

                    tasksNum -= tasksToSendNum;
                }
                else if(status.MPI_TAG == TOKEN) // token
                {
                    tokenService(msg, tasksNum);
                }
                else if(status.MPI_TAG == FINISH) // praca skonczona
                {
                    printf("%d received FINISH from %d.\n", rank, status.MPI_SOURCE);
                    finished = 1;
                }
            }
        }
        if(rank == 0 && tasksNum == 0 && tokenSent != 1 && finished != 1) // 0 nie ma zadan - rozpoczyna rozsylanie tokena
	{
            printf("0 sent token\n");
            MPI_Isend(&WHITE, 1, MPI_INT, next, TOKEN, MPI_COMM_WORLD, &request);
	    tokenSent = 1;
	}
    } // end while(finished != 1)
    
    //
    // !!UWAGA!!
    // Nie wiem, czy moze sie zdazyc, ze jakies zadanie nie zostanie obsluzone,
    // wiec tutaj mozna jeszcze Iprobe wrzucic, bo nie wiem co sie dzieje z tymi
    // nieodebranymi komunikatami...
    //

    printf("rank: %d finished!\n", rank);
    
clear: MPI_Finalize();
    return 0;
}
