# blackdb
#目录结构
1.Client kv 存储cmd客户端
2.Server kv 存储客户端
3.blackdb 具体的功能实现与数据结构
1.实现了类redis的简单KV内存数据库。
具体功能：
1：set get showall save dump delete
2：实现简单持久化与对象序列化
3:实现内存数据dump到磁盘上进行数据保存。
