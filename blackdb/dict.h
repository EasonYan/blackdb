#pragma once
#ifndef IOSTREAM
#include <stdint.h>
#define IOSTREAM
#include<iostream>
#endif
#define DICT_ERROR 0
#define DICT_OK 1
#define DICT_HT_INITIAL_SIZE 4
#define dict_resize_ratio 3
static uint32_t dict_hash_function_seed = 5381;

typedef class dictEntry
{
public:
	dictEntry();
	~dictEntry();
	void *key;
	void *value;
	//还可以存对应的这个key的hash值，这样在rehash的时候，就不用再算一次，直接跟mask &一下就可以了
	//第二次改版时考虑
	dictEntry *next;
}dictEntry;

typedef class dictTable
{
public:
	dictTable();
	~dictTable();
	dictEntry **table;
	unsigned long size;
	unsigned long mask;
	unsigned long used;
}dictTable;
typedef class dictType
{
	unsigned int(*hashFunction)(const void *key);
	void *(keyDup)(void *privdata, const void *key);
	void *(*valDup)(void *privdata, const void *obj);
	int (*keyCompare)(void *privdata, const void *key1, const void *key2);
	void(*valDestructor)(void *privdata, void *obj);
}dictType;
typedef class dict
{
public:
	dictTable ht[2];
	dictType *type;
	void *privdata;
	int rehashidx;
}dict;

typedef class dictIterator
{
public:
	dict *d;
	int table, index, safe;
	dictEntry *current_entry, *next_entry;
}dictIterator;
/*
迭代器的几个操作
*/
dictIterator *getDictIterator(dict *d);
dictEntry *dictNext(dictIterator *iter);
void dictReleaseIterator(dictIterator *iter);

//dict resize ，缩小化dict，其实也是用到了dictExpand()
int dictResize(dict *d);
int dictStepRehash(dict *d);
int dictRehash(dict *d, int n);
int dictExpand(dict *d,unsigned long size);
unsigned int dictIntHashFunction(unsigned int key);
unsigned int dictGenHashFunction(const void *key, int len);
int _dictExpandIfNeeded(dict *d);
dict *dictCreate(dictType *type, void *privdata);
int dictIsRehashing(dict *d);
void _initDict(dict *d, dictType *type, void *privdata);
void _dictTableReset(dictTable *dt);
int dictAdd(dict *d, void *key, void *value);
int dictDelete(dict *, void *key);
int dictClear(dict *dict);
int _dictClear(dict *d,dictTable *dt);
dictEntry *dictAddRaw(dict *d, void *key);
void dictRelease(dict *d);
dictEntry *dictFind(dict *d, void *key);
void *dictFecthValue(dict *d, void *key);
int _dictKeyIndex(dict *d, void *key);
int dictSdsKeyCompare(const void *key1, const void *key2);
void * _dictGetVal(dictEntry *de);
int getdictKeyIndex(dict *d, void *key);
static unsigned long _dictNextPower(unsigned long size);
int dictRewrite(dict *d, void *key, void *val);
int dictSize(dict *d);