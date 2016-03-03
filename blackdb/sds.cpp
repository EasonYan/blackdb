#include "sds.h"
#include<iostream>

sdsdr::sdsdr()
{
}
sdsdr::~sdsdr()
{
}

sds sdsnew(const char *init)
{
	size_t initlen = (init == NULL) ? 0 : strlen(init);
	return sdsnewlen(init, initlen);
}

sds sdsnewlen(const char *init, size_t size)
{
	class sdsdr *sh;
	sh = (sdsdr*)malloc(size + 1 + sizeof(sdsdr));
	if (sh == NULL)
		return NULL;
	if (init== NULL)  //无初始化内容
	{
		memset(sh, 0, sizeof(sh)+1+size);
	}
	sh->len = size;
	sh->free = 0;
	if (size && init)
		memcpy(sh->buf, init, size);
	sh->buf[size] = '\0';
	return (char*)sh->buf;
}

sds sdsempty()
{
	return sdsnewlen("",0);
}
void sdsfree(sds s)
{
	if (s == NULL)
		return;
	class sdsdr *sh = (sdsdr*)(s - sizeof(sdsdr));
	delete sh;
	sh = NULL;
}
void sdsclear(sds s)
{
	class sdsdr *sh = (sdsdr*)(s - sizeof(sdsdr));
	sh->len = 0;
	sh->free += sh->len;
	sh->buf[0] = '\0';
}
sds sdsMakeroom(sds s, size_t addlen)
{
	//sds大小扩展
	class sdsdr * sh = (sdsdr*)(s - sizeof(sdsdr));
	// s 目前的空余空间已经足够，无须再进行扩展，直接返回

	size_t newlen ,free;
	free = sdsavail(s);
	newlen = sdslen(s) + addlen;
	if(free>= addlen)
		return s;
	//小于SDS_MAX_PREALLOC，则翻倍，否则在原来基础上增加。
	if (newlen < SDS_MAX_PREALLOC)
	{
		newlen *= 2;
	}
	else
	{
		newlen += SDS_MAX_PREALLOC;
	}
	//更新sds
	class sdsdr *newsh = (sdsdr*)realloc(sh, newlen + 1 + sizeof(sdsdr));
	if (newsh == NULL)
		return NULL;
	newsh->free = newlen - sdslen(s);
	return newsh->buf;
}
sds sdscat(sds s, const char *c)
{
	size_t addlen = strlen(c);
	size_t curlen = sdslen(s);
	s = sdsMakeroom(s,addlen);
	if (s == NULL)
		return NULL;
	class sdsdr * sh = (sdsdr*)(s - sizeof(sdsdr));
	memcpy(s + curlen, c, addlen);
	sh->buf[curlen + addlen] = '\0';
	sh->free -= addlen;
	sh->len = curlen + addlen;
	return sh->buf;
}

