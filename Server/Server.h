// stdafx.h : ��׼ϵͳ�����ļ��İ����ļ���
// ���Ǿ���ʹ�õ��������ĵ�
// �ض�����Ŀ�İ����ļ�
//
// TODO:  �ڴ˴����ó�����Ҫ������ͷ�ļ�


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
	//�ͻ��˵�soket����
	int db_id;
	write_req_t *w_req;
	char buf[BLACK_REPLY_CHUNK_BYTES];
}Client;

typedef void CommandProc(Client *c);

struct blackCommand 
{
	// ��������
	char *name;
	// ʵ�ֺ���
	CommandProc *proc;
	// ��������
	int arity;
	// �ַ�����ʾ�� FLAG
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
�����ǳ־û��Ľӿ�
*/
int rdbSave(char *filename);
int rdbSaveString(sds str);
int rdbLoad(FILE *fp);