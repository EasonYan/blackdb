#include "dict.h"
#include "sds.h"

dictEntry::dictEntry()
{
	key = NULL;
	value = NULL;
	next = NULL;
}
dictEntry::~dictEntry()
{}
dictTable::dictTable()
{
	table = NULL;
}
dictTable::~dictTable()
{}

/* Thomas Wang's 32 bit Mix Function */
unsigned int dictIntHashFunction(unsigned int key)
{
	key += ~(key << 15);
	key ^= (key >> 10);
	key += (key << 3);
	key ^= (key >> 6);
	key += ~(key << 11);
	key ^= (key >> 16);
	return key;
}
unsigned int dictGenHashFunction(const void *key, int len) {
	/* 'm' and 'r' are mixing constants generated offline.
	They're not really 'magic', they just happen to work well.  */
	uint32_t seed = dict_hash_function_seed;
	const uint32_t m = 0x5bd1e995;
	const int r = 24;
	/* Initialize the hash to a 'random' value */
	uint32_t h = seed ^ len;

	/* Mix 4 bytes at a time into the hash */
	const unsigned char *data = (const unsigned char *)key;
	while (len >= 4) {
		uint32_t k = *(uint32_t*)data;
		k *= m;
		k ^= k >> r;
		k *= m;

		h *= m;
		h ^= k;

		data += 4;
		len -= 4;
	}

	/* Handle the last few bytes of the input array  */
	switch (len) {
	case 3: h ^= data[2] << 16;
	case 2: h ^= data[1] << 8;
	case 1: h ^= data[0]; h *= m;
	};

	/* Do a few final mixes of the hash to ensure the last few
	* bytes are well-incorporated. */
	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;
	return (unsigned int)h;
}
int dictStepRehash(dict *d)
{
	return dictRehash(d, 100);
}
int dictRehash(dict *d, int n)
{
	//假如不处于rehash状态，则返回错误
	//N步转移
	//假如ht[0]中的used==0,说明已经rehash完了，关闭rehash标志，ht[1]放到ht[0]中
	//rehashidx 从0开始，每次将对应的slot的内容，一条一条的对应rehash到
	if (!dictIsRehashing(d))
		return 0;
	while (n--)
	{
		//每次执行N步rehash。每一步rehash一个slot。
		//假如ht[0]中已经全部rehash完了
		if (d->ht[0].used == 0)
		{
			//只删除table指向的指针数组，并没有具体删除key value值
			delete d->ht[0].table;
			d->ht[0] = d->ht[1];
			//关闭rehashidx
			d->rehashidx = -1;
			//ht[1].table置NULL；
			_dictTableReset(&d->ht[1]);
			return 0;
		}
		else
		{
			dictEntry *entry, *nextEntry;
			unsigned int hash;
			int idx;
			//ht[0]中空的slot。
			while (d->ht[0].table[d->rehashidx] == NULL)
				d->rehashidx++;
			entry = d->ht[0].table[d->rehashidx];
			//把此slot的所有冲突链一个一个取出，然后rehash到ht[1]中
			while (entry)
			{
				nextEntry = entry->next;
				hash = dictGenHashFunction((char*)entry->key, sdslen((sds)entry->key));
				idx = hash & d->ht[1].mask;
				//插入到ht[1]对应的slot中
				entry->next = d->ht[1].table[idx];
				d->ht[1].table[idx] = entry;
				d->ht[1].used++;
				d->ht[0].used--;
				entry = nextEntry;
			}//end while
			d->ht[0].table[d->rehashidx] = NULL;
			d->rehashidx++;
		}//end else
	}  //end while(n--)
	return 1;
}
static unsigned long _dictNextPower(unsigned long size)
{
	unsigned long i = DICT_HT_INITIAL_SIZE;

	if (size >= LONG_MAX) return LONG_MAX;
	while (1) {
		if (i >= size)
			return i;
		i *= 2;
	}
}
int dictResize(dict *d)
{
	int minimal;

	// 不能在关闭 rehash 或者正在 rehash 的时候调用
	if (dictIsRehashing(d)) return DICT_ERROR;

	// 计算让比率接近 1：1 所需要的最少节点数量
	minimal = d->ht[0].used;
	if (minimal < DICT_HT_INITIAL_SIZE)
		minimal = DICT_HT_INITIAL_SIZE;
	// 调整字典的大小
	// T = O(N)
	return dictExpand(d, minimal);
}
int dictIsRehashing(dict *d)
{
	//rehashing 则返回1，否则返回0
	return d->rehashidx != -1;
}
// dictCreate 调试OK
dict *dictCreate(dictType *type, void *privdata)
{
	dict *d = new dict();
	_initDict(d,type,privdata);
	return d;
}
void _initDict(dict *d, dictType *type, void *privdata)
{
	//调试OK
	_dictTableReset(&d->ht[0]);
	_dictTableReset(&d->ht[1]);
	d->type = type;
	d->privdata = privdata;
	d->rehashidx = -1;
	dictExpand(d, DICT_HT_INITIAL_SIZE);
}
int dictExpand(dict *d,unsigned long size)
{
	//调试OK
	unsigned long realsize = _dictNextPower(size);
	if (dictIsRehashing(d) || d->ht[0].used > size)
	{
		return DICT_ERROR;
	}
	dictTable dt;
	dt.size = realsize;
	dt.mask = realsize - 1;
	dt.used = 0;
	dictEntry **mem = new dictEntry*[realsize];
	memset(mem, 0, realsize*sizeof(dictEntry*));
	dt.table = mem;

	if (d->ht[0].table == NULL)
	{
		d->ht[0] = dt;
		return DICT_OK;
	}
	else
	{
		// 如果 0 号哈希表非空，那么这是一次 rehash ：
		// 程序将新哈希表设置为 1 号哈希表，
		// 并将字典的 rehash 标识打开，让程序可以开始对字典进行 rehash
		d->ht[1] = dt;
		d->rehashidx = 0;
		return DICT_OK;
	}

}
void _dictTableReset(dictTable *dt)
{
	dt->table = NULL;
	dt->size = 0;
	dt->mask = 0;
	dt->used = 0;
}
int dictAdd(dict *d,void *key,void *val)
{
	dictEntry *entry = dictAddRaw(d, key);
	// 键已存在，添加失败
	if (!entry)
		return 0;
	else
		entry->value = val;
	// 添加成功
	return 1;
}
dictEntry *dictAddRaw(dict *d, void *key)
{
	//调试OK
	//添加成功返回1，失败返回0
	if (dictIsRehashing(d))
	{
		//单步hash
		dictStepRehash(d);
	}
	_dictExpandIfNeeded(d);
	int index;
	dictEntry *entry;
	dictTable *ht;
	index = _dictKeyIndex(d, (char*)key);
	//此key不存在于字典中，则新建一个dictentry
	if (index == -1)
	{ 
		return NULL;
	}
	else
	{
		//不为-1,此key不在字典中
		entry = new dictEntry();
		entry->key = key;
		ht = dictIsRehashing(d) ? &d->ht[1] : &d->ht[0];
		//插入链表头
		entry->next = ht->table[index];
		ht->table[index] = entry;
		ht->used++;
		return entry;
	}
}

int dictDelete(dict *d, void *key)
{
	unsigned int hash, idx;
	dictEntry *dictEn, *preEn = NULL;
	int table;
	if (d->ht[0].size == 0)
		return DICT_ERROR;
	if (dictIsRehashing(d))
	{
		dictStepRehash(d);
	}
	hash = dictGenHashFunction((unsigned char*)key, sdslen((char*)key));
	for (table = 0; table <= 1; table++)
	{
		idx = hash & (d->ht[table].mask);
		dictEn = d->ht[table].table[idx];
		while (dictEn)
		{
			if (dictSdsKeyCompare(key, dictEn->key))
			{
				//删除节点
				if (preEn)
					preEn->next = dictEn->next;
				else
					d->ht[table].table[idx] = dictEn->next;
				delete dictEn;
				//这里应该为delete key ,delete val
				d->ht[table].used--;
				return DICT_OK;
			}
			else
			{
				preEn = dictEn;
				dictEn = dictEn->next;
			}
		}
		if (!dictIsRehashing(d)) break;
	}
	return DICT_ERROR;
}
int _dictExpandIfNeeded(dict *d)
{
	/*
	1.判断是否已经在rehash的状态，是则直接返回OK
	2.判断used/size 的比值是否大于dict_resize_ratio,若是，则调用dictExpand
	3.resize的大小为目前的used的两倍。
	将rehashidx设置为0就代表目前正处于rehash状态
	*/
	if (dictIsRehashing(d))
		return DICT_OK;
	if (d->ht[0].used > d->ht[0].size && (d->ht[0].used / d->ht[0].size > dict_resize_ratio))
		dictExpand(d, d->ht[0].used * 2);
	return DICT_OK;
}
int dictSdsKeyCompare(const void *key1, const void *key2)
{
	//调试OK
	int len1, len2;
	len1 = sdslen((sds)key1);
	len2 = sdslen((sds)key2);
	if (len1 != len2)
		return 0;
	return memcmp(key1, key2, len1) == 0;
	//相等返回1，不相等返回0
}
int getdictKeyIndex(dict *d, void *key)
{
	unsigned int hash;
	//先用sds尝试
	//得到字符串key的hash值
	hash = dictGenHashFunction((unsigned char*)key, sdslen((char*)key));
	if (dictIsRehashing(d))
	{
		return hash & (d->ht[1].mask);
	}
	else
	{
		return hash & d->ht[0].mask;
	}
}
int _dictKeyIndex(dict *d, void *key)
{
	//调试OK
	/*若key不在tablez中存在，则返回索引下标值，若存在，则返回-1*/
	unsigned int hash, table, idx;
	dictEntry *de;
	//先用sds尝试
	//得到字符串key的hash值
	hash = dictGenHashFunction((unsigned char*)key, sdslen((char*)key));
	//获取其在dict里的位置。
	for (table = 0; table <= 1; table++)
	{
		//算出在key在dicttable里的位置
		idx = hash & d->ht[table].mask;
		//判断是否有重复的
		de = d->ht[table].table[idx];
		while (de)
		{
			if (dictSdsKeyCompare(key, de->key))
				return -1;
			//继续对比冲突链表的下一个key值
			de = de->next; 
		}
		//
		if (!dictIsRehashing(d)) break;
	}
	return idx;  //返回所在的索引值 
}

int dictClear(dict *dict)
{
	_dictClear(dict, &dict->ht[0]);
	_dictClear(dict, &dict->ht[1]);
	return 1;
}
int _dictClear(dict *d, dictTable *dt)
{
	//调试通过
	unsigned long i;
	dictEntry *dictEn, *nextEn;
	//遍历哈希表，一个一个删除
	for (i = 0; i < dt->size;i++)
	{
		if (dt->table[i] == NULL)
			continue;
		dictEn = dt->table[i];
		while (dictEn)
		{
			nextEn = dictEn->next;
			sdsfree((sds)dictEn->key);
			sdsfree((sds)dictEn->value);
			delete dictEn;
			dt->used--;
			dictEn = nextEn;
		}
	}
	delete(dt->table);
	_dictTableReset(dt);
	return DICT_OK;
}
void dictRelease(dict *d)
{
	//调试通过
	_dictClear(d,&d->ht[0]);
	_dictClear(d,&d->ht[1]);
	delete d;
}
dictEntry *dictFind(dict *d, void *key)
{
	//调试OK
	if (dictIsRehashing(d))
	{
		dictStepRehash(d);
	}
	dictEntry *entry;
	unsigned int hash, idx, table;
	if (d->ht[0].size == 0)
		return NULL;
	if (dictIsRehashing(d))
	{
		dictStepRehash(d);
	}
	hash = dictGenHashFunction((unsigned char*)key, sdslen((char*)key));
	for (table = 0; table <= 1; table++)
	{
		idx = hash & (d->ht[table].mask);
		entry = d->ht[table].table[idx];
		while (entry)
		{
			if (dictSdsKeyCompare(key, entry->key))
				return entry;
			entry = entry->next;
		}
		if (!dictIsRehashing(d))
		{
			break;
		}
	}
	//进行到这里时，说明两个哈希表都没找到
	return NULL;
}
void *_dictGetVal(dictEntry *entry)
{
	return entry->value;
}
void *dictFecthValue(dict *d, void *key)
{
		dictEntry *he;

		// T = O(1)
		he = dictFind(d, key);
		return he ? _dictGetVal(he) : NULL;
}
int dictRewrite(dict *d, void *key, void *val)
{
	dictEntry *entry;
	if (dictAdd(d, key,val))
	{
		return 1;
	}
	else
	{
		entry = dictFind(d, key);
		entry->value = val;
		return 0;
	}
}
int dictSize(dict *d)
{
	return d->ht[0].used + d->ht[1].used;
}

/*
迭代器操作
*/
dictIterator *getDictIterator(dict *d)
{
	dictIterator *iter = new dictIterator();
	iter->d = d;
	iter->index = -1;
	iter->table = 0;
	iter->current_entry = NULL;
	iter->next_entry = NULL;
	return iter;
}
dictEntry *dictNext(dictIterator *iter)
{
	while (true)
	{
		if (iter->current_entry == NULL)
		{
			iter->index++;
			//index 超过了当前hashtable的大小，看是否在rehashing
			if (iter->index >= (signed)iter->d->ht[iter->table].size)
			{
				if (dictIsRehashing(iter->d) && iter->table == 0)
				{
					//由0号哈希到1号表，同时index设置为0
					iter->table++;
					iter->index = 0;
				}
				else
				{
					//否则就是迭代完了
					break;
				}
			}
			//把当前的iter的entry 指向下一个entry链表
			iter->current_entry = iter->d->ht[iter->table].table[iter->index];
		}//if (iter->current_entry == NULL)
		else
		{
			iter->current_entry = iter->next_entry;
		}
		if (iter->current_entry)
		{
			iter->next_entry = iter->current_entry->next;
			return iter->current_entry;
		}
	}
	//都找不到，返回NULL
	return NULL;
}

void dictReleaseIterator(dictIterator *iter)
{
	delete iter;
}

