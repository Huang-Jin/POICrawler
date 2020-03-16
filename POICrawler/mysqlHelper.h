#pragma once
#include<mysql.h>

#pragma comment(lib, "libmysql.lib")

struct MySQLParam {
	const char * _host = "127.0.0.1"; //本地MySQL服务器
	const char * _user = "root"; //连接MySQL的用户名
	const char * _passwd = "644152350+"; //MySQL服务器密码
	const char * _db = "poi_data"; //要连接的数据库名
	unsigned int _port = 3306; //MySQL服务器的端口，默认是3306
	const char *unix_socket = NULL; //unix_socket是unix下的，在windows下就把它设置为NULL
	unsigned long client_flag = 0; //一般为
};

class MySQLHelper {
	MySQLParam * _pParam;
	MYSQL _mysql, * _pSock;
	MYSQL_RES * _pResult; //保存结果集
	MYSQL_ROW _row; //存储结果集中的一行

public:
	MySQLHelper(MySQLParam * param) {
		_pParam = param;
	}

	~MySQLHelper() {
		mysql_free_result(_pResult);
		mysql_close(_pSock);
	}

	bool connect();
	MYSQL_RES * query(const char * sql);
};

bool MySQLHelper::connect() {
	mysql_init(&_mysql);
	_pSock = mysql_real_connect(&_mysql, _pParam->_host, _pParam->_user, _pParam->_passwd, _pParam->_db, _pParam->_port, _pParam->unix_socket, _pParam->client_flag);
	if (_pSock == NULL) //连接MySQL失败
	{
		cout << mysql_error(&_mysql) << endl;
		mysql_close(&_mysql);
		return false;
	}

	return true;
}

MYSQL_RES * MySQLHelper::query(const char * sql) {
	if (mysql_query(&_mysql, sql) == 0) {
		_pResult = mysql_store_result(&_mysql);
	}
	return _pResult;
}