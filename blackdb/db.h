#pragma once
#include "dict.h"
#include "sds.h"

class blackdb
{
public:
	//数据库键空间
	dict *db_dict;
	//数据库号码
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