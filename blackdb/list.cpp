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
	//�����ڴ�ʧ��
	if (node == NULL)
		return NULL;
	//����ֵ���
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
	//ɾ�����
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
	//ע��ָ����Ϊ�β�ʱ���ǻ�����һ���ֲ�ָ������β�P��
	//������_p =  p��������p�ϵĲ�����ʵ���ǲ��� _p�����Բ��ı�p��ֵ��
	//���п��ܸı�pָ��ĵ�ַ��ֵ
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
	//��getheadȡ����valueʱ��Ӧ����NULLָ�롣������NULL->value
	return listnode->next;
}

listNode *getlistPrev(listNode *listnode)
{
	return listnode->prev;
}