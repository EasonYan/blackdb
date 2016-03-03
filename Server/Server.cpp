#include "stdafx.h"
#include <assert.h>
#include<iostream>
#include <WinSock2.h>
#include <Windows.h>
#include "Server.h"
using namespace std;

#define ERROR(msg, code) do {                                                         \
  fprintf(stderr, "%s: [%s: %s]\n", msg, uv_err_name((code)), uv_strerror((code)));   \
  assert(0);                                                                          \
} while(0);

void alloc_cb(uv_handle_t *handle, size_t size, uv_buf_t *buf);
void on_read(uv_stream_t *tcp_client, ssize_t nread, const uv_buf_t *buf);
void del_reply(uv_write_t *req, int status);

//globall variable
int total_read;
int total_written;
Server *server = new Server();
/*
命令
*/
struct blackCommand blackCommandTable[] = {
	{ "get", getCommand, 2, 0 },
	{ "set", setCommand, 3, 0 },
	{ "del", delCommand, 2, 0 },
	{ "select", selectCommand, 2, 0 },
	{ "showall", show_allCommand, 1, 0 },
	{ "save", rdbSaveCommand, 2, 0 },
	{ "load",rdbLoadCommand,2,0 }
};
uv_loop_t *loop;
//解析客户端传来的命令
void rdbLoadCommand(Client *client)
{
	char tempfile[256];
	//路径加载
	sprintf_s(tempfile, "%s%s", file_route, client->argv[1]);
	FILE *fp = fopen(tempfile,"r");
	if (fp == NULL)
	{
		addReplyString(client, "file open fail");
	}
	else
	{
		cout << "file open success" << endl;
		if (rdbLoad(fp))
		{
			addReplyString(client, "load data success!");
			fclose(fp);
		}
		else
		{
			addReplyString(client, "load data fail,please retry!");
		}
	}
	
}
void rdbSaveCommand(Client *client)
{
	if (rdbSave(client->argv[1]))
	{
		addReplyString(client, "save data success!");
	}
	else
	{
		addReplyString(client, "sava data fail,pleasse retry!");
	}
	
	//回送
}
void show_allCommand(Client *client)
{
	dictIterator *iter = getDictIterator(client->database->db_dict);
	dictEntry *entry = dictNext(iter);
	sds res = sdsnew("all keys :\n");
	int i = 1;
	while (entry != NULL)
	{
		//是否可能会有内存泄露？
		sds key = sdsnew((sds)entry->key);
		sds key2 = sdsnew(key);
		sprintf(key2, "%d):%s\n", i, key);
		res = sdscat(res, key2);
		entry = dictNext(iter);
		i++;
		sdsfree(key);
	}
	addReplyString(client, res);
	dictReleaseIterator(iter);
}
void selectCommand(Client *client)
{
	int db_num;
	if (client->argc == 2)
	{
		//把字符串转化为int
		db_num = atoi(client->argv[1]);
		if (db_num >= DB_DEFAULT_NUM || db_num<0)
		{
			addReplyString(client, "database index error");
		}
		else
		{
			client->db_id = db_num;
			client->database = &server->database[client->db_id];
			char reply[50];
			sprintf_s(reply,50, "switch to database:%d", db_num);
			addReplyString(client, reply);
		}
	}
	else
	{
		addReplyString(client, "argument illegal");
	}
}

void setCommand(Client *client)
{
	cout << "this is a set command" << endl;
	sds key = sdsnew(client->argv[1]);
	sds val = sdsnew(client->argv[2]);
	dictAdd(client->database->db_dict, key, val);
	//->w_req = (write_req_t*)malloc(sizeof(write_req_t));
	char reply[50] = "set done";
	addReplyString(client, reply);
}

void delCommand(Client *client)
{
	cout << "this is a del command" << endl;
	sds key = sdsnew(client->argv[1]);
	client->w_req = (write_req_t*)malloc(sizeof(write_req_t));

	int res = dictDelete(client->database->db_dict, key);
	if (res)
	{
		addReplyString(client, "delete ok");
		//client->w_req->buf = uv_buf_init("delete ok", 10);
	}
	else
	{
		addReplyString(client, "no suck key");
		//client->w_req->buf = uv_buf_init("no suck key", 16);
	}
}

void getCommand(Client *client)
{
	cout << "this is a get command" << endl;
	client->w_req = (write_req_t*)malloc(sizeof(write_req_t));
	sds key = sdsnew(client->argv[1]);
	char *result = (char*)dictFecthValue(client->database->db_dict, key);
	if (result == NULL)
	{
		cout << "get error" << endl;
		addReplyString(client, "NULL");
		//  client->w_req->buf = uv_buf_init("NULL", 5);
		//uv_write(&client->w_req->req, (uv_stream_t*)client->uv_client, &client->w_req->buf, 1/*nbufs*/, NULL);
		return;
	}
	char *reply = (char*)malloc(strlen(result) + 1);
	strncpy(reply, result, strlen(result) + 1);
	cout << reply << endl;
	addReplyString(client, reply);
	//  client->w_req->buf = uv_buf_init(reply, strlen(result) + 1);
	//  uv_write(&client->w_req->req, (uv_stream_t*)client->uv_client, &client->w_req->buf, 1/*nbufs*/, NULL);
}
void connection_cb(uv_stream_t *server, int status)
{
	//服务器每次有新的连接，触发此回调事件
	if (status) ERROR("async connect", status);
	uv_tcp_t *tcp_client = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
	uv_tcp_init(loop, tcp_client);
	//闭包？
	int r = uv_accept(server, (uv_stream_t*)tcp_client);
	if (r)
	{
		fprintf(stderr, "errror accepting connection %d", r);
		uv_close((uv_handle_t*)tcp_client, NULL);
	}
	else
	{
		//read_cb 为callback函数指针
		cout << "new connection" << endl;
		//为新连接绑定事件，回调函数是CreateClient
		//每次server 的监听事件有相应，就新建一个Client客户端
		//
		Client *client = CreateClient(tcp_client);
		//携带用户数据
		client->uv_client->data = client;

		uv_read_start((uv_stream_t*)client->uv_client, alloc_cb, on_read);
	}
}
void alloc_cb(uv_handle_t *handle, size_t size, uv_buf_t *buf) {
	*buf = uv_buf_init((char*)malloc(size), size);

	assert(buf->base != NULL);
}
int processInputBuffer(Client *client)
{
	blackCommand *cmd;
	cmd = lookupCommandbyName(client->argv[0]);
	if (cmd&&cmd->arity == client->argc)
	{
		cmd->proc(client);
	}
	else
	{
		addReplyString(client, "command no exist or illegal arugument!");
	}
	return 1;

}
//将命令读入为三元组的格式
int readQueryFromClient(Client *client, char *cmd_str)
{
	//将命令行读入server 里的argc,argv，格式正确就返回1，否则返回0
	int argc = 0;
	int len = strlen(cmd_str);
	int i = 0;
	int lastindex = 0;
	char *tmp = cmd_str;
	bool flag = true;
	bool commandtype = false;
	//简单解析成三元组，存到argc，和argv中，以空格为分界线,
	//遍历到结束符那
	for (i = 0; i <= len; i++)
	{
		if (cmd_str[i] == ' ' || cmd_str[i] == '\0')
		{
			client->argv[argc] = (char*)malloc(i - lastindex + 1);
			strncpy(client->argv[argc], tmp, i - lastindex);
			client->argv[argc][i - lastindex] = '\0';
			tmp = cmd_str + i + 1;
			argc++;
			lastindex = i + 1;
			//第三个分组必须是以" 为开头和结尾，否则为错误
			if (argc == 2)
			{
				//第三个参数不扫描
				break;
			}
		}
	}
	if (argc == 2 && i < len)
	{
		//说明有第三个函数
		argc++;
		if (cmd_str[i + 1] != '"' || cmd_str[len - 1] != '"')
		{
			flag = false;
		}
		client->argv[argc - 1] = (char*)malloc(len - i);
		strncpy(client->argv[argc - 1], tmp, len - i);
		client->argv[argc - 1][len - i - 1] = '\0';
	}

	for (i = 0; i < argc; i++)
	{
		cout << client->argv[i] << endl;
	}
	if (!flag)
		cout << "command illegal" << endl;
	client->argc = argc;
	if (flag)
		return 1;
	return 0;
}
Client *CreateClient(uv_tcp_t *client)
{
	Client * cli = new Client();
	//申请3元祖的空间
	cli->argv = (char**)malloc(sizeof(char*) * 3);
	memset(cli->argv, 0, sizeof(char*) * 3);
	cli->db_id = 0;
	cli->database = &server->database[cli->db_id];
	cli->uv_client = client;
	//cli->w_req = (write_req_t*)malloc(sizeof(write_req_t));
	cli->argc = 0;
	return cli;
}

void addReplyString(Client *client, char *reply)
{
	client->w_req = (write_req_t*)malloc(sizeof(write_req_t));
	client->w_req->buf = uv_buf_init(reply, strlen(reply) + 1);
	client->w_req->req.data = client;
	uv_write(&client->w_req->req, (uv_stream_t*)client->uv_client, &client->w_req->buf, 1/*nbufs*/, NULL);
	//write_req_t *wr = new write_req_t();
	//wr->buf = uv_buf_init(reply, strlen(reply));
	//uv_write(&wr->req, (uv_stream_t*)client->uv_client, &wr->buf, 1/*nbufs*/, del_reply);
}

//每个新建的客户端socket监听，有数据就回调on_read
void on_read(uv_stream_t *tcp_client, ssize_t nread, const uv_buf_t *buf)
{
	//新连接的数据回调函数
	cout << "on_read call" << endl;
	Client *client = (Client*)tcp_client->data;
	char reply[20];
	if (nread == UV_EOF)
	{
		// XXX: this is actually a bug, since client writes could back up (see ./test-client.sh) and therefore
		// some chunks to be written be left in the pipe.
		// Closing in that case results in: [ECANCELED: operation canceled]
		// Not closing here avoids that error and a few more chunks get written.
		// But even then not ALL of them make it through and things get stuck.
		uv_close((uv_handle_t*)tcp_client, NULL);
		fprintf(stderr, "closed client connection\n");
		fprintf(stderr, "Total read:    %d\n", total_read);
		fprintf(stderr, "Total written: %d\n", total_written);
		total_read = total_written = 0;
	}
	else if (nread > 0)
	{
		//socket里不为空，进行读事件
		fprintf(stderr, "%ld bytes read\n", nread);
		total_read += nread;
		int  argc = 3;
		write_req_t *wr = (write_req_t*)malloc(sizeof(write_req_t));
		if (!readQueryFromClient(client, buf->base))
		{
			addReplyString(client, "command error");
		}
		else
		{
			cout << "readquery yes" << endl;
			processInputBuffer(client);
			//strcpy(reply, "command yes");
			//wr->buf = uv_buf_init(reply, 20);
			//uv_write(&wr->req, tcp_client, &wr->buf, 1/*nbufs*/, NULL);
			//client->w_req->buf = uv_buf_init(reply, 20);
			//uv_write(&client->w_req->req, (uv_stream_t*)client->uv_client, &client->w_req->buf, 1/*nbufs*/, write_cb);
		}
	}
	if (nread == 0)
	{
		free(buf->base);
	}
}
//删除命令回复的，暂时没有用到
void del_reply(uv_write_t *req, int status)
{
	cout << "call write cb" << endl;
	write_req_t* wr;
	wr = (write_req_t*)req;
	int written = wr->buf.len;
	if (status) ERROR("async write", status);
	assert(wr->req.type == UV_WRITE);
	total_written += written;
	cout << "del_reply:" << wr->buf.base << endl;
	/* Free the read/write buffer and the request */
	//delete wr;
	//这里delete wr->buf.base会报指针错误，先挑过
	//free(wr);
}
//命令产生，将commandtable 加到字典中
void populateCommandTable()
{
	int numcommands = sizeof(blackCommandTable) / sizeof(struct blackCommand);
	int i;
	for (i = 0; i < numcommands; i++)
	{
		blackCommand *command = blackCommandTable + i;
		dictAdd(server->command, sdsnew(command->name), command);
	}
}
//根据name 寻找命令，返回结构体
blackCommand *lookupCommand(sds name)
{
	return (blackCommand*)dictFecthValue(server->command, name);
}
blackCommand *lookupCommandbyName(char *name)
{
	sds key = sdsnew(name);
	struct blackCommand *cmd;
	cmd = (blackCommand*)dictFecthValue(server->command, key);
	sdsfree(key);
	return cmd;
}

void initServer()
{
	//按照定义，所有命令
	//默认16个database
	server->database = new blackdb[DB_DEFAULT_NUM];
	for (int i = 0; i < DB_DEFAULT_NUM; i++)
	{
		server->database[i].db_dict = dictCreate(NULL, NULL);
		server->database[i].dbnum = i;
		server->database[i].currentkeylen = 0;
	}
	server->command = dictCreate(NULL, NULL);
	//生成命令表
	populateCommandTable();
}

//根据传入的str，保存其长度和内容，长度的大小一律用4个字节来简化
int rdbSaveString(FILE *fp, sds str)
{
	int len = sdslen(str);
	fwrite(&len, 4, 1, fp);
	fwrite(str, len, 1, fp);
	return 1;
}
//save 命令的执行函数
int rdbSave(char *filename)
{
	FILE *fp;
	char tmpfile[256];
	sprintf_s(tmpfile, 256, "%stemp-%s.rdb",file_route, filename);
	fp = fopen(tmpfile, "w+");
	//若是文件存在就会将原来的覆盖./rdb/文件目录下
	if (fp == NULL)
	{
		printf("file open error");
		return 0;
	}
	blackdb *db;
	int dbnum = 0, i;
	//遍历server端所有的数据库
	for (i = 0; i < DB_DEFAULT_NUM; i++)
	{
		db = &server->database[i];
		dict *d = db->db_dict;
		//当前的数据库为0就跳过
		if (dictSize(d) == 0)
			continue;
		//不空就迭代整个键值空间
		//把当前的数据库值写入先
		dictIterator *iter = getDictIterator(d);
		//写入数据库标识。RDB_OPCODE_SELECTDB （254）
		unsigned char selectdb = RDB_OPCODE_SELECTDB;
		//1个字节
		fwrite(&selectdb, 1, 1, fp);
		//写入几号数据库
		fwrite(&i, 4, 1, fp);

		dictEntry *entry = dictNext(iter);
		while (entry)
		{
			//写入String type，虽然只实现了sting type，方便以后要改可以扩展
			//这里简化了，写入一个key就插入一个string type，由于val也是string，所以也插入一个stirng type
			//好处是处理简单，坏处是rdb文件会变大
			unsigned char type = RDB_STRING_TYPE;
			fwrite(&type, 1, 1, fp);
			sds key = (sds)entry->key;
			rdbSaveString(fp, key);
			//fwrite(&type, 1, 1, fp);
			sds val = (sds)entry->value;
			rdbSaveString(fp, val);
			entry = dictNext(iter);
		}
		dictReleaseIterator(iter);
	}
	//所有数据库写入完毕后，写入EOF
	unsigned char eof = RDB_OPCODE_EOF;
	fwrite(&eof, 1, 1, fp);
	//缓冲区内容写入磁盘
	fflush(fp);
	fclose(fp);
	return 1;
}
//load 命令执行函数。
int rdbLoad(FILE *fp)
{	
	//需要把现有的数据库都清空。
	int dbnum = 0, i;
	int len;
	blackdb *db;
	dict *dict;
	for (i = 0; i < DB_DEFAULT_NUM; i++)
	{
		db = &server->database[i];
		dict = db->db_dict;
		if (dictSize(db->db_dict) == 0)
			continue;
		//清空db中的键空间
		dictClear(dict);
	}
	//进行文件的载入
	unsigned char type;
	while (1)
	{
		cout << "enter while" << endl;
		//第一个字节读入，type，用来判断接下来读入的数据的类型
		fread(&type, 1, 1, fp);
		//要是读入EOF，自定义的255，代表文件已经结束
		if (type == RDB_OPCODE_EOF)
			break;
		else if (type == RDB_OPCODE_SELECTDB)
		{
			//读入的是一个数据库,读入4个字节的int，看是几号数据库
			fread(&dbnum, 4, 1, fp);
			if (dbnum >= DB_DEFAULT_NUM)
			{
				printf("db_num out of boundary");
				return 0;
			}
			db = &server->database[dbnum];
			continue;
		}
		else if (type == RDB_STRING_TYPE)
		{
			//读入的string,由于简单的设置，这里会连续读入一个key val pair
			fread(&len, 4, 1, fp);
			sds key = sdsnewlen(NULL, len);
			//读入key
			fread(key, len, 1, fp);
			//读入val 的长度
			fread(&len, 4, 1, fp);
			sds val = sdsnewlen(NULL, len);
			//读入val
			fread(val, len, 1, fp);
			dictAdd(db->db_dict, key, val);
		}
	}
	return 1;
}

int main()
{
	//初始化服务器数据
	loop = uv_default_loop();
	uv_tcp_t tcpserver;
	initServer();
	uv_tcp_init(loop, &tcpserver);
	struct sockaddr_in bind_addr;
	int r = uv_ip4_addr("0.0.0.0", PORT, &bind_addr);
	assert(!r);
	//绑定
	uv_tcp_bind(&tcpserver, (struct sockaddr*)&bind_addr, 0);
	//监听
	r = uv_listen((uv_stream_t*)&tcpserver, 128 /*backlog*/, connection_cb);
	if (r)
		ERROR("listen error", r)
		fprintf(stderr, "Listening on localhost:7000\n");
	uv_run(loop, UV_RUN_DEFAULT);
	return 0;
}
