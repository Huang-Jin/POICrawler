#pragma once
#include<mysql.h>

#pragma comment(lib, "libmysql.lib")

struct MySQLParam {
	const char * _host = "127.0.0.1"; //����MySQL������
	const char * _user = "root"; //����MySQL���û���
	const char * _passwd = "644152350+"; //MySQL����������
	const char * _db = "POIData"; //Ҫ���ӵ����ݿ���
	unsigned int _port = 3306; //MySQL�������Ķ˿ڣ�Ĭ����3306
	const char *unix_socket = NULL; //unix_socket��unix�µģ���windows�¾Ͱ�������ΪNULL
	unsigned long client_flag = 0; //һ��Ϊ0;
};

class MySQLHelper {
	MySQLParam * _pParam;
	MYSQL _mysql;
	MYSQL_RES * _pResult; //��������
	MYSQL_ROW _row; //�洢������е�һ��

public:
	MySQLHelper(MySQLParam * param) {
		_pParam = param;
	}

	~MySQLHelper() {
		mysql_free_result(_pResult);
		mysql_close(&_mysql);
	}

	bool connect();
	MYSQL_RES * query(const char * sql);
};

bool MySQLHelper::connect() {
	mysql_init(&_mysql);
	if (!mysql_real_connect(&_mysql, _pParam->_host, _pParam->_user, _pParam->_passwd, 
		_pParam->_db, _pParam->_port, _pParam->unix_socket, _pParam->client_flag))
	{
		cout << mysql_error(&_mysql) << endl;
		mysql_close(&_mysql);
		return false;
	}

	if (!mysql_set_character_set(&_mysql, "utf8"))
	{
		printf("New client character set: %s\n",
			mysql_character_set_name(&_mysql));
	}
	else return false;

	return true;
}

MYSQL_RES * MySQLHelper::query(const char * sql) {
	_pResult = NULL;
	if (mysql_query(&_mysql, sql) == 0) {
		_pResult = mysql_store_result(&_mysql);
	}
	else {
		cout << mysql_error(&_mysql) << endl;
	}
	return _pResult;
}
