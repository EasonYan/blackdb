#pragma once
#define SDS_MAX_PREALLOC (1024*1024)
class sdsdr
{
public:
	int len;
	int free;
	char buf[];
	sdsdr();
	~sdsdr();
};
typedef char * sds;
static inline size_t sdslen(const sds s)
{
	class sdsdr *sh = (sdsdr*)(s - (sizeof(class sdsdr)));
	return sh->len;
}

static inline size_t sdsavail(const sds s)
{
	class sdsdr *sh = (sdsdr*)(s - (sizeof(class sdsdr)));
	return sh->free;
}
sds sdsnew(const char *init);
sds sdsnewlen(const char *init, size_t size);
sds sdsempty(void);
void sdsfree(sds s);
//sds sdsavail(const void *init);
void sdsclear(sds s);
sds sdscat(sds s, const char *c);
sds sdscopy(sds s, const char *c);
sds sdsMakeroom(sds s, size_t addlen);

