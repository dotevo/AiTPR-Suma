#include <stdio.h>
#include <stdlib.h>
#include "list.c"


//GLOBAL VALUES
//Init head
Item *head=0;
int numbersCount=0;
int numbersSum=0;
int *numbers=0;

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
		i++;
	}

	//Sprawdzanie czy zakres nie zostal przekroczony
	if( sum*2 > numbersSum || count*2 >numbersCount || antcount*2 > numbersCount){
		return -1;
	}

	//Sprawdzanie czy to nie jest wynik
	if( sum*2 == numbersSum && count *2 == numbersCount)
		return 1;

	//jesli doszlo do konca, a nie wynik!
	if( i>=numbersCount){
		return -1;
	}

	return 0;	
}

//rturn array of ints
int* getHalfTasks(Item** head,int *count){
        //Trzymaj glowe
        Item*tmp=*head;

        *count=listCount(*head);

        //Gdy nie mam czym sie podzielic
        if(*count<=1)
                return 0;


        *count=(*count)/2;

        int *values=(int*)malloc(sizeof(int)*(*count));

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
        printf("getHalf: COUNT %d I %d\n",*count,i);
        return values;
}

//Return new head
Item* tasksToItems(int* data,int count){
        Item* head=(Item*)malloc(sizeof(Item));
	head->next=0;
	head->val=data;

	int a=1;
	for(a=1;a<count;a++){
		Item* item=(Item*)malloc(sizeof(Item));
		item->next=0;
		item->val=(data+numbersCount*a);
		head=listAddItemOnBegin(item, head);
	}

        return head;
}







int main( int argc, char **argv ){
        int i=0;
        //----------Char to Number-------------
        numbersCount=argc-1;
        numbers		=(int*)malloc(sizeof(int)*(argc-1));
	int *firstTask	=(int*)malloc(sizeof(int)*(argc-1));

        for(i=0;i<argc-1;i++){
                numbers[i]=atoi(argv[i+1]);
		firstTask[i]=-1;
		numbersSum+=numbers[i];
        }

        printf("Ciag: ");
        for(i=0;i<numbersCount;i++)
                printf("%d,",numbers[i]);

	//Stworz pierwsze zadanie
	Item *fitem=(Item*)malloc(sizeof(Item));
	fitem->next=0;
	fitem->val=firstTask;
	head=listAddItemOnBegin(fitem, head);
	printf("\n SUM: %d COUNT %d\n",numbersSum,numbersCount);
	if(numbersCount%2!=0){
		printf("DEBIL! jest bledny ciag!\n");
		return 0;
	}
	//------------------------------------

	//If not empty
	Item *item=0;
	while(head!=0){		
//		printf("List count %d \n",listCount(head));
		//wez pierwsze zadanie
		item=listTakeAt(&head,0);
		//przetestuj czy jest prawdziwe (nie przekroczono limitow) lub czy nie jest rozwiazaniem
		int n=result(item->val);
//		printf("LL %d\n",n);
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
		}
		else if(n==1){
			//Podziel zadania na pol
			int count=0;
			printf("%d, %d\n",count,listCount(head));
			int*a=getHalfTasks(&head,&count);
			printf("%d, %d\n",count,listCount(head));
			int zzz=0;
			int zzz2=0;
			for(zzz=0;zzz<count;zzz++){
				printf("Zadanie:");
				for(zzz2=0;zzz2<numbersCount;zzz2++){
					printf("%d,",a[zzz*count+zzz2]);
				}
				printf("\n");
			}

//			printf("%d, %d\n",count,listCount(head));
//			printf("TaskToItems");
//			Item*head2=tasksToItems(a,count);
//			printf("%d\n \n",listCount(head2));
			

			int z=0;
			printf("Wynik: ");
			for(z=0;z<numbersCount;z++){
				if(item->val[z]==1)
	                                printf("%d,",numbers[z]);
                        }
			printf("\n");
		}

		//Wyczysc po zadaniu
		free( item->val);
		free( item );
	}
	printf("SYF\n");


	return 0;
}

