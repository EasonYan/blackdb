#include "ziplist.h"
ziplist CreateZiplist()
{
	//调试通过
	//申请内存，并初始化，默认用大端的存储方式
	ziplist zl = (ziplist)malloc(ZIP_INIT_SIZE);
	if (!zl)
	{
		//内存分配失败
		fprintf(stderr, "malloc: Out of memory trying to allocate %d bytes\n",
		ZIP_INIT_SIZE);
		abort();
	}
	//初始化内存
	//初始化zlbtyes,zltail
	ZIPLIST_TAIL_OFFSET(zl) = ZIPLIST_HEADER_SIZE-1;
	ZIPLIST_REMAIN(zl) = ZIP_INIT_SIZE - ZIPLIST_HEADER_SIZE - 1;
	ZIPLIST_BYTES(zl) = ZIPLIST_HEADER_SIZE + 1;
	ZIPLIST_NODE_LENGTH(zl) = 0;
	zl[ZIPLIST_HEADER_SIZE - 1] = ZIP_END;
	return zl;
}

/*
给定一个节点指针，返回编码上一个节点的长度，所需要的字节数
若第一个字节小于254，则返回1，否则，返回5.
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
给定一个节点指针，返回上一个节点的长度。需根据prevlensize是为1还是为5
读取具体的长度,当为5时，拷贝4个字节的内容到prerawlen中，返回
*/
size_t GetPrevEntryLen(unsigned char *p)
{
	//获取prev_entry_length的大小，为1或者为5
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
获取当前节点的编码类型，字符串返回1，整数返回0
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
给定指针和长度大小，返回编码len大小的需要多少个字节
传入指针部不为空，则同时把内容写入p。
*/
static unsigned int zipPrevEncodeLength(unsigned char *p, unsigned int len) {

	// 仅返回编码 len 所需的字节数量，1或5个字节
	if (p == NULL) {
		return (len < ZIP_BIGLEN) ? 1 : sizeof(len) + 1;

		// 写入并返回编码 len 所需的字节数量
	}
	else {
		// 1 字节
		if (len < ZIP_BIGLEN) {
			p[0] = len;
			return 1;
			// 5 字节
		}
		else {
			// 添加 5 字节长度标识
			p[0] = ZIP_BIGLEN;
			// 写入编码
			memcpy(p + 1, &len, sizeof(len));
			// 如果有必要的话，进行大小端转换
			// 返回编码长度
			return 1 + sizeof(len);
		}
	}
}

zlentry zipGetEntry(ziplist zl)
{
	//获取prevrawlensize
	size_t prevrawlensize, prevrawlen;
	prevrawlensize = GetPrevEntryLenSize(zl);
	//获取上一节点的真实长度
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
	//1.正负号，0，长度，溢出
	//调试通过
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
		if (plen == slen) //只有一个负号，则不能编码为整数
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
		if (-v < (LLONG_MIN))   //小于最小
			return 0;
		if (value != NULL) *value = -v;
	}
	else
	{
		if (v > LLONG_MAX) /* Overflow.大于最大的值*/
			return 0;
		if (value != NULL) *value = v;
	}
	return 1;
}
/*
通过传入len长度，判断encode的编码长度，可能值为1，2，5
*/
int GetCurrentEncodeLen(unsigned char encoding, unsigned int rawlen)
{
	if (Zip_Is_Str(encoding))  //假如是字符串的话
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
	{//否则若是整数，则encode编码只需占1位
		return 1;
	}
}

/*
//根据传入的rawlen来设置encode的内容，包括encoding 的长度可能为1，2，5，字节
p指向的是encode的位置，而不是节点的开头
*/
static void zipSetCurrentEncode(unsigned char *p, unsigned char encoding, unsigned int rawlen) {
	unsigned char len = 1, buf[5];
	// 编码字符串
	
	if (Zip_Is_Str(encoding)) 
	{
		if (rawlen <= 0x3f)
		{
			//rawlen小于等于63，encode设置为1个字节
			buf[0] = ZIP_STR_06B | rawlen;
		}
		else if (rawlen <= 0x3fff) 
		{
			//rawlen小于16383字节，encode设置为2字节，同时，要设置buf[0]和buf[1]
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
		// 编码整数
	}/*Zip_Is_Str(encoding)*/
	else 
	{
		buf[0] = encoding;
	}
	// 将编码后的长度写入 p 
	memcpy(p, buf, len);
	// 返回编码所需的字节数
}

static unsigned int GetCurrentAllLen(unsigned char *p)
{
	unsigned int prevlensize, encodesize, len;
	unsigned char encoding;

	// 取出编码前置节点的长度所需的字节数
	// T = O(1)	
	prevlensize = GetPrevEntryLenSize(p);
	encoding = p[prevlensize];
	// 取出当前节点值的编码类型，编码节点值长度所需的字节数，以及节点值的长度
	// T = O(1)
	ZIP_DECODE_LENGTH(p, encoding, encodesize, len);
	// 计算节点占用的字节数总和
	return prevlensize + encodesize + len;
}
/*
以 encoding 指定的编码方式，读取并返回指针 p 中的整数值。
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
		// 转换成功，以从小到大的顺序检查适合值 value 的编码方式
		//整数从一个8BIT的整数到64位整数
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
		// 记录值到指针
		*v = value;
		// 返回转换成功标识
		return 1;
	}
	// 转换失败
	return 0;
}

static int zipPrevLenByteDiff(unsigned char *p, unsigned int len) {
	unsigned int prevlensize;

	// 取出编码原来的前置节点长度所需的字节数
	// T = O(1)
	prevlensize = GetPrevEntryLenSize(p);

	// 计算编码 len 所需的字节数，然后进行减法运算
	// T = O(1)
	return zipPrevEncodeLength(NULL, len) - prevlensize;
}
/*
扩展压缩链表大小，先判断剩下的空间是否足够，不够就扩展，扩展率为ZIP_RESIZE_RATE   --(0.2)
*/
ziplist ziplistResize(ziplist zp, unsigned int len)
{
	size_t newlen = 0;
	int addlen;
	addlen = len - ZIPLIST_BYTES(zp);
	size_t remain = ZIPLIST_REMAIN(zp);
	if (remain < addlen)
	{
		//剩余空间不够，需扩容
		newlen = len * (1 + ZIP_RESIZE_RATE);
		ziplist old = zp;
		ziplist newptr = (ziplist)realloc(zp,len);
		if (!newptr)
		{
			//内存分配失败
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
		//不扩容
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

	// 设置 5 字节长度标识
	p[0] = ZIP_BIGLEN;

	// 写入 len
	memcpy(p + 1, &len, sizeof(len));
}

static unsigned char *ziplistCascadeUpdate(unsigned char *zl, unsigned char *p) {
	size_t curlen = ZIPLIST_BYTES(zl), rawlen, rawlensize;
	size_t offset, noffset, extra;
	unsigned char *np;
	zlentry cur, next;
	while (p[0] != ZIP_END)
	{
		// 将 p 所指向的节点的信息保存到 cur 结构中
		cur = zipGetEntry(p);
		// 当前节点的长度 d          
		rawlen = cur.headersize + cur.currentlen;
		// 计算编码当前节点的长度所需的字节数
		rawlensize = zipPrevEncodeLength(NULL, rawlen);
		// 如果已经没有后续空间需要更新了，跳出
		if (p[rawlen] == ZIP_END)
			break;
		// 取出后续节点的信息，保存到 next 结构中
		next = zipGetEntry(p + rawlen);
		if (next.prerawlen == rawlen)
			break;
		if (next.prevrawlensize < rawlensize)
		{
			// 表示 next 空间的大小不足以编码 cur 的长度
			offset = p - zl;
			// 计算需要增加的节点数量
			extra = rawlensize - next.prevrawlensize;
			zl = ziplistResize(zl, curlen + extra);
			// 还原指针 p
			p = zl + offset;
			// 记录下一节点的偏移量
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
			// 将新的前一节点长度值编码进新的 next 节点的 header
			// T = O(1)
			zipPrevEncodeLength(np, rawlen);
			// 移动指针，继续处理下个节点
			p += rawlen;
			curlen += extra;
		}
		else
		{
			if (next.prevrawlensize > rawlensize)
			{
				// 执行到这里，说明 next 节点编码前置节点的 header 空间有 5 字节
				// 而编码 rawlen 只需要 1 字节
				// 但是程序不会对 next 进行缩小，
				// 所以这里只将 rawlen 写入 5 字节的 header 中就算了。
				// T = O(1)
				zipPrevEncodeLengthForceLarge(p + rawlen, rawlen);
			}
			else {
				// 运行到这里，
				// 说明 cur 节点的长度正好可以编码到 next 节点的 header 中
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
	//current 为当前压缩链表实际长度
	size_t currentlen = ZIPLIST_BYTES(zl);
	size_t prevlen = 0, reqlen, offset = 0;
	int nextdiff = 0;
	unsigned char encoding = 0;
	long long value = 987654321;
	zlentry entry, tail;
	//1.首先需获取前置节点的实际长度，然后通过这个实际长度算出编码prev_encode_length的字节数
	//2.然后需要算出自身编码len，即自己encode 的字节大小。
	//3.以及后一节点要编码新增节点的prev_length的字节大小变化。nextdiff

	if (p[0] != ZIP_END)
	{//假如插入位置不是表尾
		//则前面有节点
		entry = zipGetEntry(p);
		prevlen = entry.prerawlen;;
	}
	else
	{//假如插入位置为表尾
		//则获取最后一个节点位置，用来判断前面是否有节点。
		unsigned char *ptail = ZIPLIST_ENTRY_TAIL(zl);
		if (ptail[0] != ZIP_END)
		{
			// 表尾节点不为空，返回这个节点占用的字节数大小
			prevlen = GetCurrentAllLen(ptail);
		}
		else
		{
			prevlen = 0;
		}
		//否则ptail为ZIP_END,则列表为空。,prevlen=0.
	}
	//看传进来的是否可以转换成整数
	if (zipTryIntegerEncoding(s, len, &value, &encoding))
	{
		//转换成功，会改变encoding的内容，使其高二位为11，否则保持高二位为00
		//假如可以转换成功，则新增节点s是整数，encoding已经更新，可以由encoding得到整数长度
		reqlen = zipIntSize(encoding);
	}
	else
	{
		//不能转换为整数，则新增的节点默认为字节，长度为传入参数len.
		reqlen = len;
	}
	// 计算后置节点用于编码pre_length的字节数差
	nextdiff = (p[0] != ZIP_END) ? zipPrevLenByteDiff(p, reqlen) : 0;
	//当前节点的实际长度加上编码前一节点长度所需的字节数
	reqlen += zipPrevEncodeLength(NULL, prevlen);
	// 总节点加上encode编码所需要的字节数
	// T = O(1)
	reqlen += GetCurrentEncodeLen(encoding, len);
	offset = p - zl;
	//因为扩容，所以压缩链表指针可能会改变
	zl = ziplistResize(zl, currentlen + reqlen + nextdiff);
	p = zl + offset;
	//向后移动

	if (p[0] != ZIP_END) 
	{
		// 新元素之后还有节点，因为新元素的加入，需要对这些原有节点进行调整
		// 移动现有元素，为新元素的插入空间腾出位置
		//memmove 相比memcpy，当两块内存间有交叉时，memmove能保证安全不出错
		//这时还没进行更新，可能涉及连锁更新
		memmove(p + reqlen, p - nextdiff, currentlen - offset - 1 + nextdiff);
		// 将新节点的长度编码至后置节点
		zipPrevEncodeLength(p + reqlen, reqlen);

		// 更新到达表尾的偏移量，将新节点的长度也算上
		ZIPLIST_TAIL_OFFSET(zl) =ZIPLIST_TAIL_OFFSET(zl) + reqlen;

		// 如果新节点的后面有多于一个节点
		// 那么程序需要将 nextdiff 记录的字节数也计算到表尾偏移量中
		// 这样才能让表尾偏移量正确对齐表尾节点

		tail = zipGetEntry(p + reqlen);
		if (p[reqlen + tail.headersize + tail.currentlen] != ZIP_END)
		{
			ZIPLIST_TAIL_OFFSET(zl) =ZIPLIST_TAIL_OFFSET(zl) + nextdiff;
		}
	}
	else 
	{
		/* This element will be the new tail. */
		// 新元素是新的表尾节点
		*((uint32_t*)(zl)+1) = p - zl;
	}

	if (nextdiff != 0) {
		offset = p - zl;
		zl = ziplistCascadeUpdate(zl, p + reqlen);
		p = zl + offset;
	}
	// 一切搞定，将前置节点的长度写入新节点的 header
	p += zipPrevEncodeLength(p, prevlen);
	p += GetCurrentEncodeLen(encoding, len);
	//设置当前的encding的值
	zipSetCurrentEncode(p, encoding, len);
	//保存值字符串和整数值
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
	// 根据 where 参数的值，决定将值推入到表头还是表尾
	unsigned char *p;
	p = (direction == 0) ? ZIPLIST_ENTRY_HEAD(zl) : ZIPLIST_ENTRY_END(zl);
	return ziplistInsert(zl, p, s, slen);
}

//给定zp,以及节点开始p,返回下一个节点的位置
ziplist ziplistNext(ziplist zp, unsigned char *p)
{
	if (p[0] == ZIP_END) {
		return NULL;
	}
	// 指向后一节点
	p += GetCurrentAllLen(p);
	if (p[0] == ZIP_END) {
		// p 已经是表尾节点，没有后置节点
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
		// 尾端节点也指向列表末端，那么列表为空
		return (p[0] == ZIP_END) ? NULL : p;
	}
	else if (p == ZIPLIST_ENTRY_HEAD(zl))
	{
		return NULL;
		// 既不是表头也不是表尾，从表尾向表头移动指针
	}
	else 
	{
		entry = zipGetEntry(p);
		// 移动指针，指向前一个节点
		return p - entry.prerawlen;
	}
}

ziplist ziplistIndex(unsigned char *zl, int index)
{
	unsigned char *p;

	zlentry entry;

	// 处理负数索引
	if (index < 0)
	{
		// 将索引转换为正数
		index = (-index) - 1;
		// 定位到表尾节点
		p = ZIPLIST_ENTRY_TAIL(zl);
		if (p[0] != ZIP_END) 
		{

			// 从表尾向表头遍历
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

	// 取出 p 所指向的节点的各项信息，并保存到结构 entry 中
	// T = O(1)
	entry = zipGetEntry(p);

	// 节点的值为字符串，将字符串长度保存到 *slen ，字符串保存到 *sstr
	// T = O(1)
	if (Zip_Is_Str(entry.encode)) 
	{
		//字符串
		if (sstr) {
			*slen = entry.currentlen;
			*sstr = p + entry.headersize;
		}

	}
	else 
	{
		//整数
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

	// 只要未到达列表末端，就一直迭代
	// T = O(N^2)
	while (p[0] != ZIP_END) {
		unsigned int prevlensize, encoding, lensize, len;
		unsigned char *q;

		prevlensize = GetPrevEntryLenSize(p);
		ZIP_DECODE_LENGTH(p + prevlensize, encoding, lensize, len);
		q = p + prevlensize + lensize;

		if (skipcnt == 0) {

			/* Compare current entry with specified entry */
			// 对比字符串值
			// T = O(N)
			if (Zip_Is_Str(encoding)) {
				if (len == vlen && memcmp(q, vstr, vlen) == 0) {
					return p;
				}
			}
			else
			{
				// 因为传入值有可能被编码了，
				// 所以当第一次进行值对比时，程序会对传入值进行解码
				// 这个解码操作只会进行一次
				if (vencoding == 0) {
					if (!zipTryIntegerEncoding(vstr, vlen, &vll, &vencoding)) {
						vencoding = UCHAR_MAX;
					}
					/* Must be non-zero by now */
					assert(vencoding);
				}
				// 对比整数值
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
		// 后移指针，指向后置节点
		p = q + len;
	}
	// 没有找到指定的节点
	return NULL;
}

static unsigned char *__ziplistDelete(unsigned char *zl, unsigned char *p, unsigned int num) {
	unsigned int i, totlen;
	int deleted = 0;
	size_t offset;
	int nextdiff = 0;
	zlentry first, tail;

	// 计算被删除节点总共占用的内存字节数
	// 以及被删除节点的总个数
	first = zipGetEntry(p);
	for (i = 0; p[0] != ZIP_END && i < num; i++)
	{
		p += GetCurrentAllLen(p);
		deleted++;
	}
	// totlen 是所有被删除节点总共占用的内存字节数
	totlen = p - first.p;
	if (totlen > 0)
	{
		if (p[0] != ZIP_END)
		{
			// 可能容纳不了新的前置节点，所以需要计算新旧前置节点之间的字节数差
			nextdiff = zipPrevLenByteDiff(p, first.prerawlen);
			// 如果有需要的话，将指针 p 后退 nextdiff 字节，为新 header 空出空间
			p -= nextdiff;
			// 将 first 的前置节点的长度编码至 p 中
			zipPrevEncodeLength(p, first.prerawlen);
			// 更新到达表尾的偏移量
			ZIPLIST_TAIL_OFFSET(zl) = ZIPLIST_TAIL_OFFSET(zl) - totlen;
			// 如果被删除节点之后，有多于一个节点
			// 那么程序需要将 nextdiff 记录的字节数也计算到表尾偏移量中
			tail = zipGetEntry(p);
			if (p[tail.headersize + tail.currentlen] != ZIP_END) {
				ZIPLIST_TAIL_OFFSET(zl) = ZIPLIST_TAIL_OFFSET(zl) + nextdiff;
			}
			// 从表尾向表头移动数据，覆盖被删除节点的数据
			memmove(first.p, p, ZIPLIST_BYTES(zl) - (p - zl) - 1);
		}
		else
		{
			// 执行这里，表示被删除节点之后已经没有其他节点了
			/* The entire tail was deleted. No need to move memory. */
			// T = O(1)
			ZIPLIST_TAIL_OFFSET(zl) = (first.p - zl) - first.prerawlen;
		}

		// 缩小并更新 ziplist 的长度
		offset = first.p - zl;
		zl = ziplistResize(zl, ZIPLIST_BYTES(zl) - totlen + nextdiff);
		ZIPLIST_INCR_LENGTH(zl, -deleted);
		p = zl + offset;
		// 检查 p 之后的所有节点是否符合 ziplist 的编码要求
		if (nextdiff != 0)
			zl = ziplistCascadeUpdate(zl, p);
	}
	return zl;
}

ziplist ziplistDelete(unsigned char *zl, unsigned char **p) {

	// 因为 __ziplistDelete 时会对 zl 进行内存重分配
	// 而内存充分配可能会改变 zl 的内存地址
	// 所以这里需要记录到达 *p 的偏移量
	// 这样在删除节点之后就可以通过偏移量来将 *p 还原到正确的位置
	size_t offset = *p - zl;
	zl = __ziplistDelete(zl, *p, 1);
	*p = zl + offset;

	return zl;
}

