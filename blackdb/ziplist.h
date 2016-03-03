/*
ziplist的实现
ziplist主要结构如下
zlbytes     zltail     zlremain     zllen      entry    zlend
uint32_t   uint32_t    uint32_t    uint16_t               255
*/
#pragma once
#ifndef IOSTREAM
#include <stdint.h>
#define IOSTREAM
#include <iostream>
#include <assert.h>
#endif
#define ZIP_END 255
#define ZIP_BIGLEN 254

#define ZIP_STR_MASK 0xc0
#define ZIP_INT_MASK 0x30

#define ZIP_INT_IMM_MASK 0x0f
#define ZIP_INT_IMM_MIN 0xf1    /* 11110001 */
#define ZIP_INT_IMM_MAX 0xfd    /* 11111101 */
#define ZIP_INT_IMM_VAL(v) (v & ZIP_INT_IMM_MASK)

//大端小端的判断(BYTE_ORDER == LITTLE_ENDIAN)

#if 0
#define memrev16ifbe(p)
#define memrev32ifbe(p)
#define memrev64ifbe(p)
#define intrev16ifbe(v) (v)
#define intrev32ifbe(v) (v)
#define intrev64ifbe(v) (v)
#else
#define memrev16ifbe(p) memrev16(p)
#define memrev32ifbe(p) memrev32(p)
#define memrev64ifbe(p) memrev64(p)
#define intrev16ifbe(v) intrev16(v)
#define intrev32ifbe(v) intrev32(v)
#define intrev64ifbe(v) intrev64(v)
#endif
//每个初始化压缩list的初始大小
#define ZIP_INIT_SIZE 512
//每次扩容要扩出的多余空间
#define ZIP_RESIZE_RATE 0.2

//encoding 的编码
#define ZIP_INT_16B (0xc0 | 0<<4)
#define ZIP_INT_32B (0xc0 | 1<<4)
#define ZIP_INT_64B (0xc0 | 2<<4)
#define ZIP_INT_8B 0xfe

#define ZIP_STR_06B (0x00)
#define ZIP_STR_14B (0x40)
#define ZIP_STR_32B (0x80)
//ZIPLIST_HEADER_SIZE，头部大小，不包括标志结束符 14字节
#define ZIPLIST_HEADER_SIZE (sizeof(uint32_t)*3 + sizeof(uint16_t))
typedef unsigned char* ziplist;
typedef class zlentry
{
public:
	uint32_t prerawlen, prevrawlensize;
	uint32_t currentlen, currentlensize;
	uint32_t headersize;
	unsigned char encode;
	unsigned char *p;
}zlentry;


//ZIPLIST_BYTES: 压缩列表的真实长度，包括特殊标识的停止符号，大小为字节
#define ZIPLIST_BYTES(zl)       (*((uint32_t*)(zl)))
//头


// 返回指向 ziplist 最后一个节点（的起始位置）的指针
#define ZIPLIST_ENTRY_TAIL(zl)  ((zl)+ZIPLIST_TAIL_OFFSET(zl))
#define ZIPLIST_ENTRY_HEAD(zl)  ((zl)+ZIPLIST_HEADER_SIZE)

// 返回指向 ziplist 末端 ZIP_END （的起始位置）的指针
#define ZIPLIST_ENTRY_END(zl)   ((zl)+ZIPLIST_BYTES(zl)-1)

//ZIP_LIST_TAIL_OFFSET:压缩列表的尾部距离列表头的长度，按字节来算
#define ZIPLIST_TAIL_OFFSET(zl)		(*((uint32_t*)(zl) + 1))

//ZIPLIST_REMAIN:压缩列表中剩余的空闲空间大小
#define	ZIPLIST_REMAIN(zl)		(*((uint32_t*)(zl) + 2))

//ZIPLIST_NODE_LENGTH：压缩列表中一共有多少个压缩节点，转化为uint16_t指针。指向其内容
//先转化为uint16_t,再加上6，即距离压缩列表的12字节处
#define ZIPLIST_NODE_LENGTH(zl)			 (*((uint16_t*)(zl)+sizeof(uint32_t) * 3 / 2))
										

/*
* 从 ptr 中取出节点值的编码类型，并将它保存到 encoding 变量中。
*
*/
#define ZIP_ENTRY_ENCODING(ptr, encoding) do {  \
    (encoding) = (ptr[0]); \
    if ((encoding) < ZIP_STR_MASK) (encoding) &= ZIP_STR_MASK; \
} while(0);

//节点数增加

#define ZIPLIST_INCR_LENGTH(zl,incr) { \
    if (ZIPLIST_NODE_LENGTH(zl) < UINT16_MAX) \
        ZIPLIST_NODE_LENGTH(zl) = (ZIPLIST_NODE_LENGTH(zl))+incr; \
}


/*
根据传入的指针，返回encode编码的字节数，返回值为1，2，5,同时设置content的长度
*/

#define ZIP_DECODE_LENGTH(ptr, encoding, lensize, len) do {                    \
                                                                               \
    /* 取出值的编码类型 */                                                     \
    ZIP_ENTRY_ENCODING((ptr), (encoding));                        \
                                                                               \
    /* 字符串编码 */                                                           \
    if ((encoding) < ZIP_STR_MASK) {                                           \
        if ((encoding) == ZIP_STR_06B) {                                       \
            (lensize) = 1;                                                     \
            (len) = (ptr)[0] & 0x3f;                                           \
		        } else if ((encoding) == ZIP_STR_14B) {                        \
            (lensize) = 2;                                                     \
            (len) = (((ptr)[0] & 0x3f) << 8) | (ptr)[1];                       \
        } else if (encoding == ZIP_STR_32B) {                                  \
            (lensize) = 5;                                                     \
            (len) = ((ptr)[1] << 24) |                                         \
                    ((ptr)[2] << 16) |                                         \
                    ((ptr)[3] <<  8) |                                         \
                    ((ptr)[4]);                                                \
        } else {                                                               \
            assert(NULL);                                                      \
        }                                                                      \
			                                                                    \
    /* 整数编码 */                                                             \
	    } else {                                                               \
        (lensize) = 1;                                                         \
        (len) = zipIntSize(encoding);                                          \
	    }                                                                      \
} while(0);

//新建一个压缩列表，并初始化内容。(调试通过)
ziplist CreateZiplist();

/*
给定指针和长度大小，返回编码len大小的需要多少个字节
传入指针为空，则同时把内容写入p。
*/
static unsigned int zipPrevEncodeLength(unsigned char *p, unsigned int len);

/*
编码规则API(根据编码规则进行解码和编码，主要针对prev_entry_Length,和encoding)
prev_entry_length:
*/
//给定一个节点的指针，获取编码上一节点的长度，所需的字节数
size_t GetPrevEntryLenSize(unsigned char *p);

/*
给定一个节点的指针，获取上一个节点的长度。
*/
size_t GetPrevEntryLen(unsigned char *p);

/*
获取当前节点的编码类型，字符串返回1，整数返回0
*/
int Zip_Is_Str(unsigned char encoding);

/*
根据encoding来得到整数的大小，为1，2，4，8个字节
*/
static unsigned int zipIntSize(unsigned char encoding);

/*
根据传入的rawlen来设置encode的内容，包括encoding 的长度可能为1，2，5，字节
p指向的是encode的位置，而不是节点的开头
*/
static void zipSetCurrentEncode(unsigned char *p, unsigned char encoding, unsigned int rawlen);

/*
通过传入len长度，判断encode的编码长度，可能值为1，2，5
*/
int GetCurrentEncodeLen(unsigned char encoding,unsigned int rawlen);

/*
返回当前节点占用的总字节数
*/
static unsigned int GetCurrentAllLen(unsigned char *p);
/*
扩展压缩链表的长度
*/
ziplist ziplistResize(ziplist zp, unsigned int len);

ziplist ziplistPush(unsigned char *zl, unsigned char *s, unsigned int slen, int direction);
ziplist ziplistInsert(ziplist zp, unsigned char *p, unsigned char *s, unsigned int len);
ziplist ziplistNext(ziplist zp, unsigned char *p);

ziplist ziplistPrev(ziplist zl, unsigned char *p);
ziplist ziplistLen(ziplist zp);
ziplist ziplistIndex(unsigned char *zl, int index);

unsigned int ziplistGet(unsigned char *p, unsigned char **sstr, unsigned int *slen, long long *sval);
int stringtoll(const char *p, size_t slen, long long *value);
zlentry zipGetEntry(ziplist zl);

ziplist ziplistDelete(unsigned char *zl, unsigned char **p);