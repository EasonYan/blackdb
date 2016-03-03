// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//
// TODO:  在此处引用程序需要的其他头文件


#include <uv.h>

#include "config.h"
#include "../blackdb/db.h"
#include "../blackdb/dict.h"
#include "../blackdb/list.h"

#define RDB_OPCODE_SELECTDB 254
#define RDB_OPCODE_EOF        255
#define RDB_STRING_TYPE 1
typedef struct {
	uv_write_t req;
	uv_buf_t buf;
} write_req_t;

typedef struct blackClient
{
public:
	uv_tcp_t *uv_client;
	int argc;
	char **argv;
	blackdb *database;
	//客户端的soket连接
	int db_id;
	write_req_t *w_req;
	char buf[BLACK_REPLY_CHUNK_BYTES];
}Client;

typedef void CommandProc(Client *c);

struct blackCommand 
{
	// 命令名字
	char *name;
	// 实现函数
	CommandProc *proc;
	// 参数个数
	int arity;
	// 字符串表示的 FLAG
	char *sflags; /* Flags as string representation, one char per flag. */
	int flags;    /* The actual flags, obtained from the 'sflags' field. */
};
Client *CreateClient(uv_tcp_t *client);

struct  Server
{
public:
	int argc;
	char **argv;
	int fd;
	int query_peak_len;
	dict *command;
	void *reply;
	uv_stream_t *tcp_server;
	blackdb *database;
	char *shortreply;
};
void initServer();

void rdbLoadCommand(Client *client);
void rdbSaveCommand(Client *client);
void setCommand(Client *client);
void delCommand(Client *client);
void getCommand(Client *client);
void selectCommand(Client *client);
void show_allCommand(Client *client);

blackCommand *lookupCommand(sds name);
blackCommand *lookupCommandbyName(char *name);
void addReplyString(Client *client,char *reply);

/*
以下是持久化的接口
*/
int rdbSave(char *filename);
int rdbSaveString(sds str);
int rdbLoad(FILE *fp);