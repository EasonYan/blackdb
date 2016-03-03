#ifndef IOSTREAM
#define IOSTREAM
#include<iostream>
#endif
#pragma once
typedef class listNode
{
public:
	listNode *next;
	listNode *prev;
	void *value;
	listNode();
	~listNode();

}listNode;
typedef class list
{
public:
	listNode *head;
	listNode *tail;
	unsigned long len;
	list();
	~list();
}list;
list *creatList();
list *addlistNodeHead(list *list, void *value);
list *addlistNodeTail(list *list, void *value);
list *dellistNode(list *list,listNode *listnode);
void deletelist(list *list);
unsigned long listLength(list *list);
listNode *getlistHead(list *list);
listNode *getlistTail(list *list);
listNode *getlistNext(listNode *listnode);
listNode *getlistPrev(listNode *listnode);


