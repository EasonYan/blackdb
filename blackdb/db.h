#pragma once
#include "dict.h"
#include "sds.h"

class blackdb
{
public:
	//���ݿ���ռ�
	dict *db_dict;
	//���ݿ����
	int dbnum;
	int currentkeylen;
	blackdb();
	~blackdb();
};

blackdb *CreateDb();
void InitDb(blackdb *d);
//blackobject *dbcontainKey(blackdb* db, blackobject *key);
//void setKey(blackdb *db, blackobject *key, blackobject *val);
//void dbRewrite(blackdb *db, blackobject *key, blackobject *val);
//void setCommand(blackdb *db, void *key, void *val);
//void *getCommand(blackdb *db, char *key);