#pragma once
#ifndef IOSTREAM
#include <stdint.h>
#define IOSTREAM
#include<iostream>
#include <time.h> 
#endif

#define SKIPLIST_MAXLEVEL 32
typedef char * robj;
typedef class skiplistnode* node;

class skilistLevel
{
public:
	node forward;
	unsigned int span;
};
typedef class skiplistnode
{
public:
	robj *obj;
	//后面扩展为对象系统
	skiplistnode *backward; 
	double score;
	class skilistLevel level[];
}sikplistnode;

typedef class skiplist
{
public:
	skiplist();
	~skiplist();
	sikplistnode *header, *tail;
	unsigned long length;
	int level;
}skiplist;

int getrandomLevel();
skiplist * sklistCreate();
void sklistFree(skiplist *sl);
skiplistnode *sklistInsert(skiplist *sl,double score,robj *obj);
skiplistnode *sklistCreateNode(skiplist *sl,double score,robj *obj);
void sklistDeleteNode(skiplist *sl, skiplistnode *node);
int sklistDelete(skiplist *sl, double score, robj *obj);