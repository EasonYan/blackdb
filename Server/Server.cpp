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
����
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
//�����ͻ��˴���������
void rdbLoadCommand(Client *client)
{
	char tempfile[256];
	//·������
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
	
	//����
}
void show_allCommand(Client *client)
{
	dictIterator *iter = getDictIterator(client->database->db_dict);
	dictEntry *entry = dictNext(iter);
	sds res = sdsnew("all keys :\n");
	int i = 1;
	while (entry != NULL)
	{
		//�Ƿ���ܻ����ڴ�й¶��
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
		//���ַ���ת��Ϊint
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
	//������ÿ�����µ����ӣ������˻ص��¼�
	if (status) ERROR("async connect", status);
	uv_tcp_t *tcp_client = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
	uv_tcp_init(loop, tcp_client);
	//�հ���
	int r = uv_accept(server, (uv_stream_t*)tcp_client);
	if (r)
	{
		fprintf(stderr, "errror accepting connection %d", r);
		uv_close((uv_handle_t*)tcp_client, NULL);
	}
	else
	{
		//read_cb Ϊcallback����ָ��
		cout << "new connection" << endl;
		//Ϊ�����Ӱ��¼����ص�������CreateClient
		//ÿ��server �ļ����¼�����Ӧ�����½�һ��Client�ͻ���
		//
		Client *client = CreateClient(tcp_client);
		//Я���û�����
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
//���������Ϊ��Ԫ��ĸ�ʽ
int readQueryFromClient(Client *client, char *cmd_str)
{
	//�������ж���server ���argc,argv����ʽ��ȷ�ͷ���1�����򷵻�0
	int argc = 0;
	int len = strlen(cmd_str);
	int i = 0;
	int lastindex = 0;
	char *tmp = cmd_str;
	bool flag = true;
	bool commandtype = false;
	//�򵥽�������Ԫ�飬�浽argc����argv�У��Կո�Ϊ�ֽ���,
	//��������������
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
			//�����������������" Ϊ��ͷ�ͽ�β������Ϊ����
			if (argc == 2)
			{
				//������������ɨ��
				break;
			}
		}
	}
	if (argc == 2 && i < len)
	{
		//˵���е���������
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
	//����3Ԫ��Ŀռ�
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

//ÿ���½��Ŀͻ���socket�����������ݾͻص�on_read
void on_read(uv_stream_t *tcp_client, ssize_t nread, const uv_buf_t *buf)
{
	//�����ӵ����ݻص�����
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
		//socket�ﲻΪ�գ����ж��¼�
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
//ɾ������ظ��ģ���ʱû���õ�
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
	//����delete wr->buf.base�ᱨָ�����������
	//free(wr);
}
//�����������commandtable �ӵ��ֵ���
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
//����name Ѱ��������ؽṹ��
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
	//���ն��壬��������
	//Ĭ��16��database
	server->database = new blackdb[DB_DEFAULT_NUM];
	for (int i = 0; i < DB_DEFAULT_NUM; i++)
	{
		server->database[i].db_dict = dictCreate(NULL, NULL);
		server->database[i].dbnum = i;
		server->database[i].currentkeylen = 0;
	}
	server->command = dictCreate(NULL, NULL);
	//���������
	populateCommandTable();
}

//���ݴ����str�������䳤�Ⱥ����ݣ����ȵĴ�Сһ����4���ֽ�����
int rdbSaveString(FILE *fp, sds str)
{
	int len = sdslen(str);
	fwrite(&len, 4, 1, fp);
	fwrite(str, len, 1, fp);
	return 1;
}
//save �����ִ�к���
int rdbSave(char *filename)
{
	FILE *fp;
	char tmpfile[256];
	sprintf_s(tmpfile, 256, "%stemp-%s.rdb",file_route, filename);
	fp = fopen(tmpfile, "w+");
	//�����ļ����ھͻὫԭ���ĸ���./rdb/�ļ�Ŀ¼��
	if (fp == NULL)
	{
		printf("file open error");
		return 0;
	}
	blackdb *db;
	int dbnum = 0, i;
	//����server�����е����ݿ�
	for (i = 0; i < DB_DEFAULT_NUM; i++)
	{
		db = &server->database[i];
		dict *d = db->db_dict;
		//��ǰ�����ݿ�Ϊ0������
		if (dictSize(d) == 0)
			continue;
		//���վ͵���������ֵ�ռ�
		//�ѵ�ǰ�����ݿ�ֵд����
		dictIterator *iter = getDictIterator(d);
		//д�����ݿ��ʶ��RDB_OPCODE_SELECTDB ��254��
		unsigned char selectdb = RDB_OPCODE_SELECTDB;
		//1���ֽ�
		fwrite(&selectdb, 1, 1, fp);
		//д�뼸�����ݿ�
		fwrite(&i, 4, 1, fp);

		dictEntry *entry = dictNext(iter);
		while (entry)
		{
			//д��String type����Ȼֻʵ����sting type�������Ժ�Ҫ�Ŀ�����չ
			//������ˣ�д��һ��key�Ͳ���һ��string type������valҲ��string������Ҳ����һ��stirng type
			//�ô��Ǵ���򵥣�������rdb�ļ�����
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
	//�������ݿ�д����Ϻ�д��EOF
	unsigned char eof = RDB_OPCODE_EOF;
	fwrite(&eof, 1, 1, fp);
	//����������д�����
	fflush(fp);
	fclose(fp);
	return 1;
}
//load ����ִ�к�����
int rdbLoad(FILE *fp)
{	
	//��Ҫ�����е����ݿⶼ��ա�
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
		//���db�еļ��ռ�
		dictClear(dict);
	}
	//�����ļ�������
	unsigned char type;
	while (1)
	{
		cout << "enter while" << endl;
		//��һ���ֽڶ��룬type�������жϽ�������������ݵ�����
		fread(&type, 1, 1, fp);
		//Ҫ�Ƕ���EOF���Զ����255�������ļ��Ѿ�����
		if (type == RDB_OPCODE_EOF)
			break;
		else if (type == RDB_OPCODE_SELECTDB)
		{
			//�������һ�����ݿ�,����4���ֽڵ�int�����Ǽ������ݿ�
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
			//�����string,���ڼ򵥵����ã��������������һ��key val pair
			fread(&len, 4, 1, fp);
			sds key = sdsnewlen(NULL, len);
			//����key
			fread(key, len, 1, fp);
			//����val �ĳ���
			fread(&len, 4, 1, fp);
			sds val = sdsnewlen(NULL, len);
			//����val
			fread(val, len, 1, fp);
			dictAdd(db->db_dict, key, val);
		}
	}
	return 1;
}

int main()
{
	//��ʼ������������
	loop = uv_default_loop();
	uv_tcp_t tcpserver;
	initServer();
	uv_tcp_init(loop, &tcpserver);
	struct sockaddr_in bind_addr;
	int r = uv_ip4_addr("0.0.0.0", PORT, &bind_addr);
	assert(!r);
	//��
	uv_tcp_bind(&tcpserver, (struct sockaddr*)&bind_addr, 0);
	//����
	r = uv_listen((uv_stream_t*)&tcpserver, 128 /*backlog*/, connection_cb);
	if (r)
		ERROR("listen error", r)
		fprintf(stderr, "Listening on localhost:7000\n");
	uv_run(loop, UV_RUN_DEFAULT);
	return 0;
}
