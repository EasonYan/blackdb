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
	//���粻����rehash״̬���򷵻ش���
	//N��ת��
	//����ht[0]�е�used==0,˵���Ѿ�rehash���ˣ��ر�rehash��־��ht[1]�ŵ�ht[0]��
	//rehashidx ��0��ʼ��ÿ�ν���Ӧ��slot�����ݣ�һ��һ���Ķ�Ӧrehash��
	if (!dictIsRehashing(d))
		return 0;
	while (n--)
	{
		//ÿ��ִ��N��rehash��ÿһ��rehashһ��slot��
		//����ht[0]���Ѿ�ȫ��rehash����
		if (d->ht[0].used == 0)
		{
			//ֻɾ��tableָ���ָ�����飬��û�о���ɾ��key valueֵ
			delete d->ht[0].table;
			d->ht[0] = d->ht[1];
			//�ر�rehashidx
			d->rehashidx = -1;
			//ht[1].table��NULL��
			_dictTableReset(&d->ht[1]);
			return 0;
		}
		else
		{
			dictEntry *entry, *nextEntry;
			unsigned int hash;
			int idx;
			//ht[0]�пյ�slot��
			while (d->ht[0].table[d->rehashidx] == NULL)
				d->rehashidx++;
			entry = d->ht[0].table[d->rehashidx];
			//�Ѵ�slot�����г�ͻ��һ��һ��ȡ����Ȼ��rehash��ht[1]��
			while (entry)
			{
				nextEntry = entry->next;
				hash = dictGenHashFunction((char*)entry->key, sdslen((sds)entry->key));
				idx = hash & d->ht[1].mask;
				//���뵽ht[1]��Ӧ��slot��
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

	// �����ڹر� rehash �������� rehash ��ʱ�����
	if (dictIsRehashing(d)) return DICT_ERROR;

	// �����ñ��ʽӽ� 1��1 ����Ҫ�����ٽڵ�����
	minimal = d->ht[0].used;
	if (minimal < DICT_HT_INITIAL_SIZE)
		minimal = DICT_HT_INITIAL_SIZE;
	// �����ֵ�Ĵ�С
	// T = O(N)
	return dictExpand(d, minimal);
}
int dictIsRehashing(dict *d)
{
	//rehashing �򷵻�1�����򷵻�0
	return d->rehashidx != -1;
}
// dictCreate ����OK
dict *dictCreate(dictType *type, void *privdata)
{
	dict *d = new dict();
	_initDict(d,type,privdata);
	return d;
}
void _initDict(dict *d, dictType *type, void *privdata)
{
	//����OK
	_dictTableReset(&d->ht[0]);
	_dictTableReset(&d->ht[1]);
	d->type = type;
	d->privdata = privdata;
	d->rehashidx = -1;
	dictExpand(d, DICT_HT_INITIAL_SIZE);
}
int dictExpand(dict *d,unsigned long size)
{
	//����OK
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
		// ��� 0 �Ź�ϣ��ǿգ���ô����һ�� rehash ��
		// �����¹�ϣ������Ϊ 1 �Ź�ϣ��
		// �����ֵ�� rehash ��ʶ�򿪣��ó�����Կ�ʼ���ֵ���� rehash
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
	// ���Ѵ��ڣ����ʧ��
	if (!entry)
		return 0;
	else
		entry->value = val;
	// ��ӳɹ�
	return 1;
}
dictEntry *dictAddRaw(dict *d, void *key)
{
	//����OK
	//��ӳɹ�����1��ʧ�ܷ���0
	if (dictIsRehashing(d))
	{
		//����hash
		dictStepRehash(d);
	}
	_dictExpandIfNeeded(d);
	int index;
	dictEntry *entry;
	dictTable *ht;
	index = _dictKeyIndex(d, (char*)key);
	//��key���������ֵ��У����½�һ��dictentry
	if (index == -1)
	{ 
		return NULL;
	}
	else
	{
		//��Ϊ-1,��key�����ֵ���
		entry = new dictEntry();
		entry->key = key;
		ht = dictIsRehashing(d) ? &d->ht[1] : &d->ht[0];
		//��������ͷ
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
				//ɾ���ڵ�
				if (preEn)
					preEn->next = dictEn->next;
				else
					d->ht[table].table[idx] = dictEn->next;
				delete dictEn;
				//����Ӧ��Ϊdelete key ,delete val
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
	1.�ж��Ƿ��Ѿ���rehash��״̬������ֱ�ӷ���OK
	2.�ж�used/size �ı�ֵ�Ƿ����dict_resize_ratio,���ǣ������dictExpand
	3.resize�Ĵ�СΪĿǰ��used��������
	��rehashidx����Ϊ0�ʹ���Ŀǰ������rehash״̬
	*/
	if (dictIsRehashing(d))
		return DICT_OK;
	if (d->ht[0].used > d->ht[0].size && (d->ht[0].used / d->ht[0].size > dict_resize_ratio))
		dictExpand(d, d->ht[0].used * 2);
	return DICT_OK;
}
int dictSdsKeyCompare(const void *key1, const void *key2)
{
	//����OK
	int len1, len2;
	len1 = sdslen((sds)key1);
	len2 = sdslen((sds)key2);
	if (len1 != len2)
		return 0;
	return memcmp(key1, key2, len1) == 0;
	//��ȷ���1������ȷ���0
}
int getdictKeyIndex(dict *d, void *key)
{
	unsigned int hash;
	//����sds����
	//�õ��ַ���key��hashֵ
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
	//����OK
	/*��key����tablez�д��ڣ��򷵻������±�ֵ�������ڣ��򷵻�-1*/
	unsigned int hash, table, idx;
	dictEntry *de;
	//����sds����
	//�õ��ַ���key��hashֵ
	hash = dictGenHashFunction((unsigned char*)key, sdslen((char*)key));
	//��ȡ����dict���λ�á�
	for (table = 0; table <= 1; table++)
	{
		//�����key��dicttable���λ��
		idx = hash & d->ht[table].mask;
		//�ж��Ƿ����ظ���
		de = d->ht[table].table[idx];
		while (de)
		{
			if (dictSdsKeyCompare(key, de->key))
				return -1;
			//�����Աȳ�ͻ�������һ��keyֵ
			de = de->next; 
		}
		//
		if (!dictIsRehashing(d)) break;
	}
	return idx;  //�������ڵ�����ֵ 
}

int dictClear(dict *dict)
{
	_dictClear(dict, &dict->ht[0]);
	_dictClear(dict, &dict->ht[1]);
	return 1;
}
int _dictClear(dict *d, dictTable *dt)
{
	//����ͨ��
	unsigned long i;
	dictEntry *dictEn, *nextEn;
	//������ϣ��һ��һ��ɾ��
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
	//����ͨ��
	_dictClear(d,&d->ht[0]);
	_dictClear(d,&d->ht[1]);
	delete d;
}
dictEntry *dictFind(dict *d, void *key)
{
	//����OK
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
	//���е�����ʱ��˵��������ϣ��û�ҵ�
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
����������
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
			//index �����˵�ǰhashtable�Ĵ�С�����Ƿ���rehashing
			if (iter->index >= (signed)iter->d->ht[iter->table].size)
			{
				if (dictIsRehashing(iter->d) && iter->table == 0)
				{
					//��0�Ź�ϣ��1�ű�ͬʱindex����Ϊ0
					iter->table++;
					iter->index = 0;
				}
				else
				{
					//������ǵ�������
					break;
				}
			}
			//�ѵ�ǰ��iter��entry ָ����һ��entry����
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
	//���Ҳ���������NULL
	return NULL;
}

void dictReleaseIterator(dictIterator *iter)
{
	delete iter;
}

