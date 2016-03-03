#include "list.h"


list::list()
{
}

list::~list()
{
}
listNode::listNode()
{

}
listNode::~listNode()
{

}

list *creatList(void)
{
	class list *li;
	li = new list();
	li->head = NULL;
	li->tail = NULL;
	li->len = 0;
	return li;

}
list *addlistNodeHead(list *list, void *value)
{
	listNode *node;
	node = new listNode();
	if (node == NULL)
		return NULL;
	node->value = value;
	if (list->len == 0)
	{
		list->head = list->tail = node;
		node->prev = node->next = NULL;
	}
	else
	{
		list->head->prev = node;
		node->prev = NULL;
		node->next = list->head;
		list->head = node;
	}
	list->len++;
	return list;
}

list *addlistNodeTail(list *list, void *value)
{
	listNode *node;
	node = new listNode();
	//分配内存失败
	if (node == NULL)
		return NULL;
	//保存值结点
	node->next = node->prev = NULL;
	node->value = value;
	if (list->len == 0)
	{
		list->head = list->tail = node;
		node->prev = node->next = NULL;
	}
	else
	{
		list->tail->next = node;
		node->prev = list->tail;
		node->next = NULL;
		list->tail = node;
	}
	list->len++;
	return list;
}
list *dellistNode(list *list,listNode *listnode)
{
	if (listnode->prev)
		listnode->prev->next = listnode->next;
	else
		list->head = listnode->next;
	if (listnode->next)
		listnode->next->prev = listnode->prev;
	else
		list->tail = listnode->prev;
	//删除结点
	delete listnode;
	list->len--;
	return list;
}
unsigned long listLength(list *list)
{
	if (list)
		return list->len;
	else return 0;
}
void deletelist(list *list)
{
	//注意指针作为形参时，是会生成一个局部指针代替形参P的
	//如生成_p =  p；所有在p上的操作其实都是操作 _p，所以不改变p的值，
	//但有可能改变p指向的地址的值
	unsigned long len;
	class listNode *cur, *next;
	cur = list->head;
	len = list->len;
	while (len--)
	{
		next = cur->next;
		delete cur;
		cur = next;
	}
	delete list;
	list=NULL;
}
listNode *getlistHead(list *list)
{
	return list->head;
}

listNode *getlistTail(list *list)
{
	return list->tail;
}

listNode *getlistNext(listNode *listnode)
{
	//用gethead取其中value时，应考虑NULL指针。不可以NULL->value
	return listnode->next;
}

listNode *getlistPrev(listNode *listnode)
{
	return listnode->prev;
}