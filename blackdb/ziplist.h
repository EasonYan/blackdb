/*
ziplist��ʵ��
ziplist��Ҫ�ṹ����
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

//���С�˵��ж�(BYTE_ORDER == LITTLE_ENDIAN)

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
//ÿ����ʼ��ѹ��list�ĳ�ʼ��С
#define ZIP_INIT_SIZE 512
//ÿ������Ҫ�����Ķ���ռ�
#define ZIP_RESIZE_RATE 0.2

//encoding �ı���
#define ZIP_INT_16B (0xc0 | 0<<4)
#define ZIP_INT_32B (0xc0 | 1<<4)
#define ZIP_INT_64B (0xc0 | 2<<4)
#define ZIP_INT_8B 0xfe

#define ZIP_STR_06B (0x00)
#define ZIP_STR_14B (0x40)
#define ZIP_STR_32B (0x80)
//ZIPLIST_HEADER_SIZE��ͷ����С����������־������ 14�ֽ�
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


//ZIPLIST_BYTES: ѹ���б����ʵ���ȣ����������ʶ��ֹͣ���ţ���СΪ�ֽ�
#define ZIPLIST_BYTES(zl)       (*((uint32_t*)(zl)))
//ͷ


// ����ָ�� ziplist ���һ���ڵ㣨����ʼλ�ã���ָ��
#define ZIPLIST_ENTRY_TAIL(zl)  ((zl)+ZIPLIST_TAIL_OFFSET(zl))
#define ZIPLIST_ENTRY_HEAD(zl)  ((zl)+ZIPLIST_HEADER_SIZE)

// ����ָ�� ziplist ĩ�� ZIP_END ������ʼλ�ã���ָ��
#define ZIPLIST_ENTRY_END(zl)   ((zl)+ZIPLIST_BYTES(zl)-1)

//ZIP_LIST_TAIL_OFFSET:ѹ���б��β�������б�ͷ�ĳ��ȣ����ֽ�����
#define ZIPLIST_TAIL_OFFSET(zl)		(*((uint32_t*)(zl) + 1))

//ZIPLIST_REMAIN:ѹ���б���ʣ��Ŀ��пռ��С
#define	ZIPLIST_REMAIN(zl)		(*((uint32_t*)(zl) + 2))

//ZIPLIST_NODE_LENGTH��ѹ���б���һ���ж��ٸ�ѹ���ڵ㣬ת��Ϊuint16_tָ�롣ָ��������
//��ת��Ϊuint16_t,�ټ���6��������ѹ���б��12�ֽڴ�
#define ZIPLIST_NODE_LENGTH(zl)			 (*((uint16_t*)(zl)+sizeof(uint32_t) * 3 / 2))
										

/*
* �� ptr ��ȡ���ڵ�ֵ�ı������ͣ����������浽 encoding �����С�
*
*/
#define ZIP_ENTRY_ENCODING(ptr, encoding) do {  \
    (encoding) = (ptr[0]); \
    if ((encoding) < ZIP_STR_MASK) (encoding) &= ZIP_STR_MASK; \
} while(0);

//�ڵ�������

#define ZIPLIST_INCR_LENGTH(zl,incr) { \
    if (ZIPLIST_NODE_LENGTH(zl) < UINT16_MAX) \
        ZIPLIST_NODE_LENGTH(zl) = (ZIPLIST_NODE_LENGTH(zl))+incr; \
}


/*
���ݴ����ָ�룬����encode������ֽ���������ֵΪ1��2��5,ͬʱ����content�ĳ���
*/

#define ZIP_DECODE_LENGTH(ptr, encoding, lensize, len) do {                    \
                                                                               \
    /* ȡ��ֵ�ı������� */                                                     \
    ZIP_ENTRY_ENCODING((ptr), (encoding));                        \
                                                                               \
    /* �ַ������� */                                                           \
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
    /* �������� */                                                             \
	    } else {                                                               \
        (lensize) = 1;                                                         \
        (len) = zipIntSize(encoding);                                          \
	    }                                                                      \
} while(0);

//�½�һ��ѹ���б�����ʼ�����ݡ�(����ͨ��)
ziplist CreateZiplist();

/*
����ָ��ͳ��ȴ�С�����ر���len��С����Ҫ���ٸ��ֽ�
����ָ��Ϊ�գ���ͬʱ������д��p��
*/
static unsigned int zipPrevEncodeLength(unsigned char *p, unsigned int len);

/*
�������API(���ݱ��������н���ͱ��룬��Ҫ���prev_entry_Length,��encoding)
prev_entry_length:
*/
//����һ���ڵ��ָ�룬��ȡ������һ�ڵ�ĳ��ȣ�������ֽ���
size_t GetPrevEntryLenSize(unsigned char *p);

/*
����һ���ڵ��ָ�룬��ȡ��һ���ڵ�ĳ��ȡ�
*/
size_t GetPrevEntryLen(unsigned char *p);

/*
��ȡ��ǰ�ڵ�ı������ͣ��ַ�������1����������0
*/
int Zip_Is_Str(unsigned char encoding);

/*
����encoding���õ������Ĵ�С��Ϊ1��2��4��8���ֽ�
*/
static unsigned int zipIntSize(unsigned char encoding);

/*
���ݴ����rawlen������encode�����ݣ�����encoding �ĳ��ȿ���Ϊ1��2��5���ֽ�
pָ�����encode��λ�ã������ǽڵ�Ŀ�ͷ
*/
static void zipSetCurrentEncode(unsigned char *p, unsigned char encoding, unsigned int rawlen);

/*
ͨ������len���ȣ��ж�encode�ı��볤�ȣ�����ֵΪ1��2��5
*/
int GetCurrentEncodeLen(unsigned char encoding,unsigned int rawlen);

/*
���ص�ǰ�ڵ�ռ�õ����ֽ���
*/
static unsigned int GetCurrentAllLen(unsigned char *p);
/*
��չѹ������ĳ���
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