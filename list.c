#include<stdlib.h>
#include<stdio.h>


struct ListEl {
   int* val;
   struct ListEl * next;
};
typedef struct ListEl Item;

//return head
Item* listAddItemOnEnd(Item *item, Item*head){
	//jesli lista pusta
	if(head==0)
		return item;

	//jesli sa elementy znajdz ostatni
	Item*h=head;
	while(head->next!=0){
		head=head->next;
	}
	item->next=0;
	head->next=item;
	return h;
}

//return head
Item* listAddItemOnBegin(Item *item, Item*head){
	item->next=head;     
        return item;
}

//return item, and set new head
Item* listTakeAt(Item **head,int i){
//	printf("L %d %d\n", *head,head);
	if(*head==0)
		return 0;

	if(i==0){
		Item *ret=*head;
		(*head)=((*head)->next);
		return ret;
	}

	Item*ret=0;
	Item*cur=0;
	while(i>0 && (*head) !=0 && (*head)->next!=0){
		cur=*head;
		i--;
	}
	if(cur==0)
		return 0;

	ret=cur->next;
	if(cur->next!=0)
		cur->next=cur->next->next;
	else
		cur=0;

	ret->next=0;
	return ret;
}

int listCount(Item*head){
	int r=0;
	while(head!=0){
		r++;
		head=head->next;
	}
	return r;
}


