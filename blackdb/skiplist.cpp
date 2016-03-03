#include "skiplist.h"


skiplist::skiplist()
{
}
skiplist::~skiplist()
{
}
skiplistnode *sklistCreateNode(int level, double score, robj *obj)
{
	skiplistnode *sknode = (skiplistnode*) operator new(sizeof(*sknode) + level*sizeof(skilistLevel));
	sknode->obj = obj;
	sknode->score = score;
	return sknode;
}
skiplist * sklistCreate()
{
	skiplist *sl = new skiplist();
	//初始化
	sl->length = 0;
	sl->level = 0;
	sl->tail = NULL;
	sl->header = sklistCreateNode(SKIPLIST_MAXLEVEL, 0, NULL);
	for (int i = 0; i < SKIPLIST_MAXLEVEL - 1; i++)
	{
		sl->header->level[i].forward = NULL;
		sl->header->level[i].span = NULL;
	}
	sl->header->backward = NULL;
	return sl;
}
int getrandomLevel()
{
	int level = 1;
	srand((unsigned)time(NULL));
	while (rand() % 2)
		level++;
	return level < SKIPLIST_MAXLEVEL ? level : SKIPLIST_MAXLEVEL;

}
//字符串比较
int comparestring(void *str1, void *str2)
{
	//str1 < str2 则返回-1，大于则返回1，否则返回0
	return 0;
}
skiplistnode *sklistInsert(skiplist *sl, double score, robj *obj)
{
	skiplistnode *update[SKIPLIST_MAXLEVEL], *node;
	unsigned int rank[SKIPLIST_MAXLEVEL];
	int i, level,cmpresult;
	//更新update指针数组
	//2:生成随机插入层数
	for (i = sl->level - 1; i >= 0; i--)
	{
		node = sl->header->level[i].forward;
		cmpresult = comparestring(node->obj, obj);
		//node不为NULL 或者分数相同，obj 较小
		while (node && (node->score < score || (node->score < score&&cmpresult < 0)))
		{
			node = node->level[i].forward;
			rank[i] = i == (sl->level - 1) ? 0 : rank[i + 1];
		}
		//把每一层找到的的插入位置更新到update数组
		update[i] = node;
	}
	//假如生成的层数比当前的level层数大
	level = getrandomLevel();
	if (level > sl->level)
	{
		for (i = sl->level; i < level; i++)
		{
			update[i] = sl->header;
			update[i]->level[i].span = sl->length;
		}
		sl->level = level;
	}
	for (i = 0; i < level; i++)
	{
		node->level[i].forward = update[i]->level[i].forward;
		update[i]->level[i].forward = node;
		node->level[i].span = update[i]->level[i].span - (rank[0] - rank[i]);

		// 更新新节点插入之后，沿途节点的 span 值
		// 其中的 +1 计算的是新节点
		update[i]->level[i].span = (rank[0] - rank[i]) + 1;
	}
	for (i = level; i < sl->level; i++) {
		update[i]->level[i].span++;
	}
	node->backward = (update[0] == sl->header) ? NULL : update[0];
	if (node->level[0].forward)
		node->level[0].forward->backward = node;
	else
		sl->tail = node;

	// 跳跃表的节点计数增一
	sl->length++;
	return node;
}
void sklistDeleteNode(skiplist *sl, skiplistnode *sknode)
{
	//更新update数组
	skiplistnode *update[SKIPLIST_MAXLEVEL], *node = NULL;
	unsigned int rank[SKIPLIST_MAXLEVEL];
	int i, level, cmpresult;
	//更新update指针数组
	//2:生成随机插入层数
	for (i = sl->level - 1; i >= 0; i--)
	{
		node = sl->header->level[i].forward;
		cmpresult = comparestring(node->obj, sknode->obj);
		//node不为NULL 或者分数相同，obj 较小
		while (node && (node->score < sknode->score || (node->score < sknode->score&&cmpresult < 0)))
		{
			node = node->level[i].forward;
			rank[i] = i == (sl->level - 1) ? 0 : rank[i + 1];
		}
		//把每一层找到的的插入位置更新到update数组
		update[i] = node;
	}
	node = node->level[0].forward;
	if (node &&sknode->score == node->score&&comparestring(node->obj, sknode->obj) == 0)
	{
		//删除节点
		for (i = sl->level - 1; i >= 0; i--)
		{
			
			if (update[i]->level[i].forward == node) {
				update[i]->level[i].span += node->level[i].span - 1;
				update[i]->level[i].forward = node->level[i].forward;
			}
			else {
				update[i]->level[i].span -= 1;
			}
		}
		//forward指针不为空
		if (node->level[0].forward)
		{
			node->level[0].forward->backward = node->backward;
		}
		else
		{
			//若为空，则删除的是最后一个节点
			sl->tail = node->backward;
		}
		sl->length--;
		//层数是否要减1;
		while (sl->level > 1 && sl->header->level[sl->level - 1].forward == NULL)
			sl->level--;
	}
}

void sklistFree(skiplist *sl)
{
		skiplistnode *node = sl->header->level[0].forward, *next;
		// 释放表头
		delete sl->header;
		// 释放表中所有节点
		// T = O(N)
		while (node) {
			next = node->level[0].forward;
			delete node;
			node = next;
		}
		// 释放跳跃表结构
		delete sl;
}

