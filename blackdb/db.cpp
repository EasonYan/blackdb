#include "db.h"



blackdb::blackdb()
{
}


blackdb::~blackdb()
{
}

blackdb *CreateDb()
{
	blackdb *db = new blackdb();
	db->db_dict = dictCreate(NULL,NULL);
	db->dbnum = 0;
	return db;
}

//
//blackobject *dbcontainKey(blackdb* db, blackobject *key)
//{
//	//db中包含key，则返回dictEntry,否则返回NULL
//	dictEntry *de = dictFind(db->db_dict, key->ptr);
//	if (de)
//	{
//		blackobject *obj = (blackobject*)de->value;
//		return obj;
//	}
//	else
//		return NULL;
//}
//
//void dbAdd(blackdb *db, void *key, void *val)
//{
//	sds key1 = sdsnew((char *)key); 
//	dictAdd(db->db_dict, key1, val);
//}
//void dbRewrite(blackdb *db, blackobject *key, blackobject *val)
//{
//	dictRewrite(db->db_dict, key->ptr, val);
//}
//
//void setKey(blackdb *db, blackobject *key, blackobject *val)
//{
//	if (dbcontainKey(db, key) == NULL)
//	{//db_dict不含键，则写入
//		dbAdd(db, key, val);
//	}
//	else
//	{
//		dbRewrite(db, key, val);
//	}
//}
//void setCommand(blackdb *db,void *key,void *val)
//{
//	//必要检查
//	setKey(db, (blackobject *)key, (blackobject *)val);
//
//}
//void *getCommand(blackdb *db,char *key)
//{
//	dictEntry *entry = dictFind(db->db_dict,sdsnew(key));
//	return entry->value;
//}