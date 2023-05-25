#ifndef _LINKLIST_H
#define _LINKLIST_H


#define ADD_TAIL        1
#define ADD_BEFORE      2

typedef struct list_head {
	struct list_head *next, *prev;
}LinkList,*PLinkList;


//initial to Bidirectional cyclic linked list
void list_init(PLinkList head)
{
    head->next = head;
    head->prev = head;
}

//Tail insertion method
void list_add_tail(PLinkList new, PLinkList this)
{
    new->next = this->next;
    new->prev = this;
    this->next->prev = new;
    this->next = new;
}
//Head insertion method
void list_add_before(PLinkList new, PLinkList this)
{
    new->next = this;
    new->prev = this->prev;
    this->prev->next = new;
    this->prev = new;
}

void list_add(PLinkList head, PLinkList new, int flag)
{
    PLinkList tail = head;
    while(tail->next != head)
    {
        tail = tail->next;
    }
    if(flag == ADD_TAIL)
    {
        list_add_tail(new, tail);
    }
    else if(flag == ADD_BEFORE)
    {
        list_add_before(new,tail);
    }
}

void list_delete_node(PLinkList prev, PLinkList next, PLinkList node)
{
    prev->next = next;
    next->prev = prev;
    //page_free_addr(node);
}

int list_empty(PLinkList head)
{
    if(head->next == head && head->prev == head)
    {
        return 1;
    }
    return 0;
}
#endif