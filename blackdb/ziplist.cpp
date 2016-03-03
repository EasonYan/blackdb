#include "ziplist.h"
ziplist CreateZiplist()
{
	//����ͨ��
	//�����ڴ棬����ʼ����Ĭ���ô�˵Ĵ洢��ʽ
	ziplist zl = (ziplist)malloc(ZIP_INIT_SIZE);
	if (!zl)
	{
		//�ڴ����ʧ��
		fprintf(stderr, "malloc: Out of memory trying to allocate %d bytes\n",
		ZIP_INIT_SIZE);
		abort();
	}
	//��ʼ���ڴ�
	//��ʼ��zlbtyes,zltail
	ZIPLIST_TAIL_OFFSET(zl) = ZIPLIST_HEADER_SIZE-1;
	ZIPLIST_REMAIN(zl) = ZIP_INIT_SIZE - ZIPLIST_HEADER_SIZE - 1;
	ZIPLIST_BYTES(zl) = ZIPLIST_HEADER_SIZE + 1;
	ZIPLIST_NODE_LENGTH(zl) = 0;
	zl[ZIPLIST_HEADER_SIZE - 1] = ZIP_END;
	return zl;
}

/*
����һ���ڵ�ָ�룬���ر�����һ���ڵ�ĳ��ȣ�����Ҫ���ֽ���
����һ���ֽ�С��254���򷵻�1�����򣬷���5.
*/
size_t GetPrevEntryLenSize(unsigned char *p)
{
	if (p[0] < ZIP_BIGLEN)
	{
		return 1;
	}
	else
	{
		return 5;
	}
}

/*
����һ���ڵ�ָ�룬������һ���ڵ�ĳ��ȡ������prevlensize��Ϊ1����Ϊ5
��ȡ����ĳ���,��Ϊ5ʱ������4���ֽڵ����ݵ�prerawlen�У�����
*/
size_t GetPrevEntryLen(unsigned char *p)
{
	//��ȡprev_entry_length�Ĵ�С��Ϊ1����Ϊ5
	size_t prevlensize = GetPrevEntryLenSize(p);
	size_t prevrawlen = 0;
	if (prevlensize == 1)
	{
		prevrawlen = p[0];
	}
	else if (prevlensize ==5)
	{
		memcpy(&prevrawlen, p + 1, 4);
	}
	return prevrawlen;

}

/*
��ȡ��ǰ�ڵ�ı������ͣ��ַ�������1����������0
*/
int Zip_Is_Str(unsigned char encoding)
{
	unsigned char enc = encoding;
	return (encoding&ZIP_STR_MASK) < ZIP_STR_MASK ? 1 : 0;
}

static unsigned int zipIntSize(unsigned char encoding) {

	switch (encoding) {
	case ZIP_INT_8B:  return 1;
	case ZIP_INT_16B: return 2;
	case ZIP_INT_32B: return 4;
	case ZIP_INT_64B: return 8;
	default: return 0; /* 4 bit immediate */
	}
	assert(NULL);
	return 0;
}

/*
����ָ��ͳ��ȴ�С�����ر���len��С����Ҫ���ٸ��ֽ�
����ָ�벿��Ϊ�գ���ͬʱ������д��p��
*/
static unsigned int zipPrevEncodeLength(unsigned char *p, unsigned int len) {

	// �����ر��� len ������ֽ�������1��5���ֽ�
	if (p == NULL) {
		return (len < ZIP_BIGLEN) ? 1 : sizeof(len) + 1;

		// д�벢���ر��� len ������ֽ�����
	}
	else {
		// 1 �ֽ�
		if (len < ZIP_BIGLEN) {
			p[0] = len;
			return 1;
			// 5 �ֽ�
		}
		else {
			// ��� 5 �ֽڳ��ȱ�ʶ
			p[0] = ZIP_BIGLEN;
			// д�����
			memcpy(p + 1, &len, sizeof(len));
			// ����б�Ҫ�Ļ������д�С��ת��
			// ���ر��볤��
			return 1 + sizeof(len);
		}
	}
}

zlentry zipGetEntry(ziplist zl)
{
	//��ȡprevrawlensize
	size_t prevrawlensize, prevrawlen;
	prevrawlensize = GetPrevEntryLenSize(zl);
	//��ȡ��һ�ڵ����ʵ����
	prevrawlen = GetPrevEntryLen(zl);

	zlentry entry;
	entry.prevrawlensize = prevrawlensize;
	entry.prerawlen = prevrawlen;
	ZIP_DECODE_LENGTH(zl, entry.encode, entry.currentlen, entry.currentlensize);
	entry.headersize = entry.prevrawlensize + entry.currentlensize;
	entry.p = zl;
	return entry;
}

int stringtoll(const char *p, size_t slen, long long *value)
{
	//1.�����ţ�0�����ȣ����
	//����ͨ��
	size_t plen = 0; int negative = 0;
	long long v;
	if (plen == slen)
		return 0;
	if (slen == 1 && p[0] == '0')
	{
		if (value != NULL) *value = 0;
		return 1;
	}
	if (p[0] == '-')
	{
		negative = 1;
		plen++;
		p++;
		if (plen == slen) //ֻ��һ�����ţ����ܱ���Ϊ����
			return 0;
	}

	if (p[0]>'0'&&p[0] <= '9')
	{
		v = p[0] - '0';
		p++; plen++;
	}
	else if (p[0] == '0'&&slen == 1)
	{
		*value = 0;
		return 1;
	}
	else
	{
		return 0;
	}
	while (plen < slen && p[0] >= '0' && p[0] <= '9')
	{
		v *= 10;
		if (v > LLONG_MAX) /* Overflow. */
			return 0;
		v += p[0] - '0';

		if (v > LLONG_MAX) /* Overflow. */
			return 0;
		p++; plen++;
	}

	if (plen < slen)
		return 0;

	if (negative)
	{
		if (-v < (LLONG_MIN))   //С����С
			return 0;
		if (value != NULL) *value = -v;
	}
	else
	{
		if (v > LLONG_MAX) /* Overflow.��������ֵ*/
			return 0;
		if (value != NULL) *value = v;
	}
	return 1;
}
/*
ͨ������len���ȣ��ж�encode�ı��볤�ȣ�����ֵΪ1��2��5
*/
int GetCurrentEncodeLen(unsigned char encoding, unsigned int rawlen)
{
	if (Zip_Is_Str(encoding))  //�������ַ����Ļ�
	{
		if (rawlen < 0x3f)
		{
			return 1;
		}
		else if (rawlen <= 0x3fff)
		{
			return 2;
		}
		else
		{
			return 5;
		}
	}
	else
	{//����������������encode����ֻ��ռ1λ
		return 1;
	}
}

/*
//���ݴ����rawlen������encode�����ݣ�����encoding �ĳ��ȿ���Ϊ1��2��5���ֽ�
pָ�����encode��λ�ã������ǽڵ�Ŀ�ͷ
*/
static void zipSetCurrentEncode(unsigned char *p, unsigned char encoding, unsigned int rawlen) {
	unsigned char len = 1, buf[5];
	// �����ַ���
	
	if (Zip_Is_Str(encoding)) 
	{
		if (rawlen <= 0x3f)
		{
			//rawlenС�ڵ���63��encode����Ϊ1���ֽ�
			buf[0] = ZIP_STR_06B | rawlen;
		}
		else if (rawlen <= 0x3fff) 
		{
			//rawlenС��16383�ֽڣ�encode����Ϊ2�ֽڣ�ͬʱ��Ҫ����buf[0]��buf[1]
			len += 1;
			buf[0] = ZIP_STR_14B | ((rawlen >> 8) & 0x3f);
			buf[1] = rawlen & 0xff;
		}
		else 
		{
			len += 4;
			buf[0] = ZIP_STR_32B;
			buf[1] = (rawlen >> 24) & 0xff;
			buf[2] = (rawlen >> 16) & 0xff;
			buf[3] = (rawlen >> 8) & 0xff;
			buf[4] = rawlen & 0xff;
		}
		// ��������
	}/*Zip_Is_Str(encoding)*/
	else 
	{
		buf[0] = encoding;
	}
	// �������ĳ���д�� p 
	memcpy(p, buf, len);
	// ���ر���������ֽ���
}

static unsigned int GetCurrentAllLen(unsigned char *p)
{
	unsigned int prevlensize, encodesize, len;
	unsigned char encoding;

	// ȡ������ǰ�ýڵ�ĳ���������ֽ���
	// T = O(1)	
	prevlensize = GetPrevEntryLenSize(p);
	encoding = p[prevlensize];
	// ȡ����ǰ�ڵ�ֵ�ı������ͣ�����ڵ�ֵ����������ֽ������Լ��ڵ�ֵ�ĳ���
	// T = O(1)
	ZIP_DECODE_LENGTH(p, encoding, encodesize, len);
	// ����ڵ�ռ�õ��ֽ����ܺ�
	return prevlensize + encodesize + len;
}
/*
�� encoding ָ���ı��뷽ʽ����ȡ������ָ�� p �е�����ֵ��
*/
static int64_t zipLoadInteger(unsigned char *p, unsigned char encoding) {
	int16_t i16;
	int32_t i32;
	int64_t i64, ret = 0;
	if (encoding == ZIP_INT_8B) {
		ret = ((int8_t*)p)[0];
	}
	else if (encoding == ZIP_INT_16B) {
		memcpy(&i16, p, sizeof(i16));
		ret = i16;
	}
	else if (encoding == ZIP_INT_32B) {
		memcpy(&i32, p, sizeof(i32));
		ret = i32;
	}
	else if (encoding == ZIP_INT_64B) {
		memcpy(&i64, p, sizeof(i64));
		ret = i64;
	}
	else if (encoding >= ZIP_INT_IMM_MIN && encoding <= ZIP_INT_IMM_MAX) {
		ret = (encoding & ZIP_INT_IMM_MASK) - 1;
	}
	else
	{
		assert(NULL);
	}
	return ret;
}


static int zipTryIntegerEncoding(unsigned char *entry, unsigned int entrylen, long long *v, unsigned char *encoding) {
	long long value;
	if (entrylen >= 32 || entrylen == 0) return 0;
	if (stringtoll((char*)entry, entrylen, &value)) {
		// ת���ɹ����Դ�С�����˳�����ʺ�ֵ value �ı��뷽ʽ
		//������һ��8BIT��������64λ����
		if (value >= INT8_MIN && value <= INT8_MAX) {
			*encoding = ZIP_INT_8B;
		}
		else if (value >= INT16_MIN && value <= INT16_MAX) {
			*encoding = ZIP_INT_16B;
		}
		else if (value >= INT32_MIN && value <= INT32_MAX) {
			*encoding = ZIP_INT_32B;
		}
		else {
			*encoding = ZIP_INT_64B;
		}
		// ��¼ֵ��ָ��
		*v = value;
		// ����ת���ɹ���ʶ
		return 1;
	}
	// ת��ʧ��
	return 0;
}

static int zipPrevLenByteDiff(unsigned char *p, unsigned int len) {
	unsigned int prevlensize;

	// ȡ������ԭ����ǰ�ýڵ㳤��������ֽ���
	// T = O(1)
	prevlensize = GetPrevEntryLenSize(p);

	// ������� len ������ֽ�����Ȼ����м�������
	// T = O(1)
	return zipPrevEncodeLength(NULL, len) - prevlensize;
}
/*
��չѹ�������С�����ж�ʣ�µĿռ��Ƿ��㹻����������չ����չ��ΪZIP_RESIZE_RATE   --(0.2)
*/
ziplist ziplistResize(ziplist zp, unsigned int len)
{
	size_t newlen = 0;
	int addlen;
	addlen = len - ZIPLIST_BYTES(zp);
	size_t remain = ZIPLIST_REMAIN(zp);
	if (remain < addlen)
	{
		//ʣ��ռ䲻����������
		newlen = len * (1 + ZIP_RESIZE_RATE);
		ziplist old = zp;
		ziplist newptr = (ziplist)realloc(zp,len);
		if (!newptr)
		{
			//�ڴ����ʧ��
			fprintf(stderr, "realloc: Out of memory trying to allocate %d bytes\n",
				len);
			abort();
		}
		remain = newlen - len;
		ZIPLIST_BYTES(zp) = len;
		ZIPLIST_REMAIN(newptr) = remain;
		return newptr;
	}
	else
	{
		//������
		ZIPLIST_REMAIN(zp) = ZIPLIST_REMAIN(zp) -addlen;
		ZIPLIST_BYTES(zp) = len;
		return zp;
	}
}

static void zipSaveInteger(unsigned char *p, int64_t value, unsigned char encoding) {
	int16_t i16;
	int32_t i32;
	int64_t i64;
	if (encoding == ZIP_INT_8B) {
		((int8_t*)p)[0] = (int8_t)value;
	}
	else if (encoding == ZIP_INT_16B) {
		i16 = value;
		memcpy(p, &i16, sizeof(i16));
	}
	else if (encoding == ZIP_INT_32B) {
		i32 = value;
		memcpy(p, &i32, sizeof(i32));
	}
	else if (encoding == ZIP_INT_64B) {
		i64 = value;
		memcpy(p, &i64, sizeof(i64));
	}
	else {
		assert(NULL);
	}
}

static void zipPrevEncodeLengthForceLarge(unsigned char *p, unsigned int len) {

	if (p == NULL) return;

	// ���� 5 �ֽڳ��ȱ�ʶ
	p[0] = ZIP_BIGLEN;

	// д�� len
	memcpy(p + 1, &len, sizeof(len));
}

static unsigned char *ziplistCascadeUpdate(unsigned char *zl, unsigned char *p) {
	size_t curlen = ZIPLIST_BYTES(zl), rawlen, rawlensize;
	size_t offset, noffset, extra;
	unsigned char *np;
	zlentry cur, next;
	while (p[0] != ZIP_END)
	{
		// �� p ��ָ��Ľڵ����Ϣ���浽 cur �ṹ��
		cur = zipGetEntry(p);
		// ��ǰ�ڵ�ĳ��� d          
		rawlen = cur.headersize + cur.currentlen;
		// ������뵱ǰ�ڵ�ĳ���������ֽ���
		rawlensize = zipPrevEncodeLength(NULL, rawlen);
		// ����Ѿ�û�к����ռ���Ҫ�����ˣ�����
		if (p[rawlen] == ZIP_END)
			break;
		// ȡ�������ڵ����Ϣ�����浽 next �ṹ��
		next = zipGetEntry(p + rawlen);
		if (next.prerawlen == rawlen)
			break;
		if (next.prevrawlensize < rawlensize)
		{
			// ��ʾ next �ռ�Ĵ�С�����Ա��� cur �ĳ���
			offset = p - zl;
			// ������Ҫ���ӵĽڵ�����
			extra = rawlensize - next.prevrawlensize;
			zl = ziplistResize(zl, curlen + extra);
			// ��ԭָ�� p
			p = zl + offset;
			// ��¼��һ�ڵ��ƫ����
			np = p + rawlen;
			noffset = np - zl;
			if ((zl + ZIPLIST_TAIL_OFFSET(zl)) != np)
			{
				ZIPLIST_TAIL_OFFSET(zl) =
					ZIPLIST_TAIL_OFFSET(zl) + extra;
			}
			memmove(np + rawlensize,
				np + next.prevrawlensize,
				curlen - noffset - next.prevrawlensize - 1);
			// ���µ�ǰһ�ڵ㳤��ֵ������µ� next �ڵ�� header
			// T = O(1)
			zipPrevEncodeLength(np, rawlen);
			// �ƶ�ָ�룬���������¸��ڵ�
			p += rawlen;
			curlen += extra;
		}
		else
		{
			if (next.prevrawlensize > rawlensize)
			{
				// ִ�е����˵�� next �ڵ����ǰ�ýڵ�� header �ռ��� 5 �ֽ�
				// ������ rawlen ֻ��Ҫ 1 �ֽ�
				// ���ǳ��򲻻�� next ������С��
				// ��������ֻ�� rawlen д�� 5 �ֽڵ� header �о����ˡ�
				// T = O(1)
				zipPrevEncodeLengthForceLarge(p + rawlen, rawlen);
			}
			else {
				// ���е����
				// ˵�� cur �ڵ�ĳ������ÿ��Ա��뵽 next �ڵ�� header ��
				// T = O(1)
				zipPrevEncodeLength(p + rawlen, rawlen);
			}
			break;
		}
	}

	return zl;
}

ziplist ziplistInsert(ziplist zl, unsigned char *p, unsigned char *s, unsigned int len)
{
	//current Ϊ��ǰѹ������ʵ�ʳ���
	size_t currentlen = ZIPLIST_BYTES(zl);
	size_t prevlen = 0, reqlen, offset = 0;
	int nextdiff = 0;
	unsigned char encoding = 0;
	long long value = 987654321;
	zlentry entry, tail;
	//1.�������ȡǰ�ýڵ��ʵ�ʳ��ȣ�Ȼ��ͨ�����ʵ�ʳ����������prev_encode_length���ֽ���
	//2.Ȼ����Ҫ����������len�����Լ�encode ���ֽڴ�С��
	//3.�Լ���һ�ڵ�Ҫ���������ڵ��prev_length���ֽڴ�С�仯��nextdiff

	if (p[0] != ZIP_END)
	{//�������λ�ò��Ǳ�β
		//��ǰ���нڵ�
		entry = zipGetEntry(p);
		prevlen = entry.prerawlen;;
	}
	else
	{//�������λ��Ϊ��β
		//���ȡ���һ���ڵ�λ�ã������ж�ǰ���Ƿ��нڵ㡣
		unsigned char *ptail = ZIPLIST_ENTRY_TAIL(zl);
		if (ptail[0] != ZIP_END)
		{
			// ��β�ڵ㲻Ϊ�գ���������ڵ�ռ�õ��ֽ�����С
			prevlen = GetCurrentAllLen(ptail);
		}
		else
		{
			prevlen = 0;
		}
		//����ptailΪZIP_END,���б�Ϊ�ա�,prevlen=0.
	}
	//�����������Ƿ����ת��������
	if (zipTryIntegerEncoding(s, len, &value, &encoding))
	{
		//ת���ɹ�����ı�encoding�����ݣ�ʹ��߶�λΪ11�����򱣳ָ߶�λΪ00
		//�������ת���ɹ����������ڵ�s��������encoding�Ѿ����£�������encoding�õ���������
		reqlen = zipIntSize(encoding);
	}
	else
	{
		//����ת��Ϊ�������������Ľڵ�Ĭ��Ϊ�ֽڣ�����Ϊ�������len.
		reqlen = len;
	}
	// ������ýڵ����ڱ���pre_length���ֽ�����
	nextdiff = (p[0] != ZIP_END) ? zipPrevLenByteDiff(p, reqlen) : 0;
	//��ǰ�ڵ��ʵ�ʳ��ȼ��ϱ���ǰһ�ڵ㳤��������ֽ���
	reqlen += zipPrevEncodeLength(NULL, prevlen);
	// �ܽڵ����encode��������Ҫ���ֽ���
	// T = O(1)
	reqlen += GetCurrentEncodeLen(encoding, len);
	offset = p - zl;
	//��Ϊ���ݣ�����ѹ������ָ����ܻ�ı�
	zl = ziplistResize(zl, currentlen + reqlen + nextdiff);
	p = zl + offset;
	//����ƶ�

	if (p[0] != ZIP_END) 
	{
		// ��Ԫ��֮���нڵ㣬��Ϊ��Ԫ�صļ��룬��Ҫ����Щԭ�нڵ���е���
		// �ƶ�����Ԫ�أ�Ϊ��Ԫ�صĲ���ռ��ڳ�λ��
		//memmove ���memcpy���������ڴ���н���ʱ��memmove�ܱ�֤��ȫ������
		//��ʱ��û���и��£������漰��������
		memmove(p + reqlen, p - nextdiff, currentlen - offset - 1 + nextdiff);
		// ���½ڵ�ĳ��ȱ��������ýڵ�
		zipPrevEncodeLength(p + reqlen, reqlen);

		// ���µ����β��ƫ���������½ڵ�ĳ���Ҳ����
		ZIPLIST_TAIL_OFFSET(zl) =ZIPLIST_TAIL_OFFSET(zl) + reqlen;

		// ����½ڵ�ĺ����ж���һ���ڵ�
		// ��ô������Ҫ�� nextdiff ��¼���ֽ���Ҳ���㵽��βƫ������
		// ���������ñ�βƫ������ȷ�����β�ڵ�

		tail = zipGetEntry(p + reqlen);
		if (p[reqlen + tail.headersize + tail.currentlen] != ZIP_END)
		{
			ZIPLIST_TAIL_OFFSET(zl) =ZIPLIST_TAIL_OFFSET(zl) + nextdiff;
		}
	}
	else 
	{
		/* This element will be the new tail. */
		// ��Ԫ�����µı�β�ڵ�
		*((uint32_t*)(zl)+1) = p - zl;
	}

	if (nextdiff != 0) {
		offset = p - zl;
		zl = ziplistCascadeUpdate(zl, p + reqlen);
		p = zl + offset;
	}
	// һ�и㶨����ǰ�ýڵ�ĳ���д���½ڵ�� header
	p += zipPrevEncodeLength(p, prevlen);
	p += GetCurrentEncodeLen(encoding, len);
	//���õ�ǰ��encding��ֵ
	zipSetCurrentEncode(p, encoding, len);
	//����ֵ�ַ���������ֵ
	if (Zip_Is_Str(encoding)) 
	{
		memcpy(p, s, len);
	}
	else {
		zipSaveInteger(p, value, encoding);
	}
	ZIPLIST_INCR_LENGTH(zl, 1);
	return zl;

}

ziplist ziplistPush(unsigned char *zl, unsigned char *s, unsigned int slen, int direction) 
{
	// ���� where ������ֵ��������ֵ���뵽��ͷ���Ǳ�β
	unsigned char *p;
	p = (direction == 0) ? ZIPLIST_ENTRY_HEAD(zl) : ZIPLIST_ENTRY_END(zl);
	return ziplistInsert(zl, p, s, slen);
}

//����zp,�Լ��ڵ㿪ʼp,������һ���ڵ��λ��
ziplist ziplistNext(ziplist zp, unsigned char *p)
{
	if (p[0] == ZIP_END) {
		return NULL;
	}
	// ָ���һ�ڵ�
	p += GetCurrentAllLen(p);
	if (p[0] == ZIP_END) {
		// p �Ѿ��Ǳ�β�ڵ㣬û�к��ýڵ�
		return NULL;
	}
	return p;
}

ziplist ziplistPrev(ziplist zl, unsigned char *p)
{
	zlentry entry;
	if (p[0] == ZIP_END) 
	{
		p = ZIPLIST_ENTRY_TAIL(zl);
		// β�˽ڵ�Ҳָ���б�ĩ�ˣ���ô�б�Ϊ��
		return (p[0] == ZIP_END) ? NULL : p;
	}
	else if (p == ZIPLIST_ENTRY_HEAD(zl))
	{
		return NULL;
		// �Ȳ��Ǳ�ͷҲ���Ǳ�β���ӱ�β���ͷ�ƶ�ָ��
	}
	else 
	{
		entry = zipGetEntry(p);
		// �ƶ�ָ�룬ָ��ǰһ���ڵ�
		return p - entry.prerawlen;
	}
}

ziplist ziplistIndex(unsigned char *zl, int index)
{
	unsigned char *p;

	zlentry entry;

	// ����������
	if (index < 0)
	{
		// ������ת��Ϊ����
		index = (-index) - 1;
		// ��λ����β�ڵ�
		p = ZIPLIST_ENTRY_TAIL(zl);
		if (p[0] != ZIP_END) 
		{

			// �ӱ�β���ͷ����
			entry = zipGetEntry(p);
			// T = O(N)
			while (entry.prerawlen > 0 && index--) 
			{
				p -= entry.prerawlen;
				// T = O(1)
				entry = zipGetEntry(p);
			}
		}
	}
	else 
	{
		p = ZIPLIST_ENTRY_HEAD(zl);
		while (p[0] != ZIP_END && index>0) 
		{
			p += GetCurrentAllLen(p);
		}
	}

	return (p[0] == ZIP_END || index > 0) ? NULL : p;
}

unsigned int ziplistGet(unsigned char *p, unsigned char **sstr, unsigned int *slen, long long *sval) {

	zlentry entry;
	if (p == NULL || p[0] == ZIP_END) return 0;
	if (sstr) *sstr = NULL;

	// ȡ�� p ��ָ��Ľڵ�ĸ�����Ϣ�������浽�ṹ entry ��
	// T = O(1)
	entry = zipGetEntry(p);

	// �ڵ��ֵΪ�ַ��������ַ������ȱ��浽 *slen ���ַ������浽 *sstr
	// T = O(1)
	if (Zip_Is_Str(entry.encode)) 
	{
		//�ַ���
		if (sstr) {
			*slen = entry.currentlen;
			*sstr = p + entry.headersize;
		}

	}
	else 
	{
		//����
		if (sval) 
		{
			*sval = zipLoadInteger(p + entry.headersize, entry.encode);
		}
	}

	return 1;
}

unsigned char *ziplistFind(unsigned char *p, unsigned char *vstr, unsigned int vlen, unsigned int skip) {
	int skipcnt = 0;
	unsigned char vencoding = 0;
	long long vll = 0;

	// ֻҪδ�����б�ĩ�ˣ���һֱ����
	// T = O(N^2)
	while (p[0] != ZIP_END) {
		unsigned int prevlensize, encoding, lensize, len;
		unsigned char *q;

		prevlensize = GetPrevEntryLenSize(p);
		ZIP_DECODE_LENGTH(p + prevlensize, encoding, lensize, len);
		q = p + prevlensize + lensize;

		if (skipcnt == 0) {

			/* Compare current entry with specified entry */
			// �Ա��ַ���ֵ
			// T = O(N)
			if (Zip_Is_Str(encoding)) {
				if (len == vlen && memcmp(q, vstr, vlen) == 0) {
					return p;
				}
			}
			else
			{
				// ��Ϊ����ֵ�п��ܱ������ˣ�
				// ���Ե���һ�ν���ֵ�Ա�ʱ�������Դ���ֵ���н���
				// ����������ֻ�����һ��
				if (vencoding == 0) {
					if (!zipTryIntegerEncoding(vstr, vlen, &vll, &vencoding)) {
						vencoding = UCHAR_MAX;
					}
					/* Must be non-zero by now */
					assert(vencoding);
				}
				// �Ա�����ֵ
				if (vencoding != UCHAR_MAX) {
					// T = O(1)
					long long ll = zipLoadInteger(q, encoding);
					if (ll == vll) {
						return p;
					}
				}
			}

			/* Reset skip count */
			skipcnt = skip;
		}
		else {
			/* Skip entry */
			skipcnt--;
		}
		/* Move to next entry */
		// ����ָ�룬ָ����ýڵ�
		p = q + len;
	}
	// û���ҵ�ָ���Ľڵ�
	return NULL;
}

static unsigned char *__ziplistDelete(unsigned char *zl, unsigned char *p, unsigned int num) {
	unsigned int i, totlen;
	int deleted = 0;
	size_t offset;
	int nextdiff = 0;
	zlentry first, tail;

	// ���㱻ɾ���ڵ��ܹ�ռ�õ��ڴ��ֽ���
	// �Լ���ɾ���ڵ���ܸ���
	first = zipGetEntry(p);
	for (i = 0; p[0] != ZIP_END && i < num; i++)
	{
		p += GetCurrentAllLen(p);
		deleted++;
	}
	// totlen �����б�ɾ���ڵ��ܹ�ռ�õ��ڴ��ֽ���
	totlen = p - first.p;
	if (totlen > 0)
	{
		if (p[0] != ZIP_END)
		{
			// �������ɲ����µ�ǰ�ýڵ㣬������Ҫ�����¾�ǰ�ýڵ�֮����ֽ�����
			nextdiff = zipPrevLenByteDiff(p, first.prerawlen);
			// �������Ҫ�Ļ�����ָ�� p ���� nextdiff �ֽڣ�Ϊ�� header �ճ��ռ�
			p -= nextdiff;
			// �� first ��ǰ�ýڵ�ĳ��ȱ����� p ��
			zipPrevEncodeLength(p, first.prerawlen);
			// ���µ����β��ƫ����
			ZIPLIST_TAIL_OFFSET(zl) = ZIPLIST_TAIL_OFFSET(zl) - totlen;
			// �����ɾ���ڵ�֮���ж���һ���ڵ�
			// ��ô������Ҫ�� nextdiff ��¼���ֽ���Ҳ���㵽��βƫ������
			tail = zipGetEntry(p);
			if (p[tail.headersize + tail.currentlen] != ZIP_END) {
				ZIPLIST_TAIL_OFFSET(zl) = ZIPLIST_TAIL_OFFSET(zl) + nextdiff;
			}
			// �ӱ�β���ͷ�ƶ����ݣ����Ǳ�ɾ���ڵ������
			memmove(first.p, p, ZIPLIST_BYTES(zl) - (p - zl) - 1);
		}
		else
		{
			// ִ�������ʾ��ɾ���ڵ�֮���Ѿ�û�������ڵ���
			/* The entire tail was deleted. No need to move memory. */
			// T = O(1)
			ZIPLIST_TAIL_OFFSET(zl) = (first.p - zl) - first.prerawlen;
		}

		// ��С������ ziplist �ĳ���
		offset = first.p - zl;
		zl = ziplistResize(zl, ZIPLIST_BYTES(zl) - totlen + nextdiff);
		ZIPLIST_INCR_LENGTH(zl, -deleted);
		p = zl + offset;
		// ��� p ֮������нڵ��Ƿ���� ziplist �ı���Ҫ��
		if (nextdiff != 0)
			zl = ziplistCascadeUpdate(zl, p);
	}
	return zl;
}

ziplist ziplistDelete(unsigned char *zl, unsigned char **p) {

	// ��Ϊ __ziplistDelete ʱ��� zl �����ڴ��ط���
	// ���ڴ�������ܻ�ı� zl ���ڴ��ַ
	// ����������Ҫ��¼���� *p ��ƫ����
	// ������ɾ���ڵ�֮��Ϳ���ͨ��ƫ�������� *p ��ԭ����ȷ��λ��
	size_t offset = *p - zl;
	zl = __ziplistDelete(zl, *p, 1);
	*p = zl + offset;

	return zl;
}

