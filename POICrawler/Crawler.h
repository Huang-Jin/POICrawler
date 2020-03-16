#pragma once

#include <iostream>
#include <Windows.h>
#include <string>
#include <queue>
#include <regex>
#include <urlmon.h>

#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#pragma comment(lib, "urlmon.lib" )
#pragma comment(lib, "libssl.lib" )
#pragma comment(lib, "libcrypto.lib" )

#pragma comment(lib, "WS2_32")  // ���ӵ�WS2_32.lib

using namespace std;

char g_Host[MAX_PATH];
char g_Object[MAX_PATH];

SOCKET g_sock;
SSL *sslHandle;
SSL_CTX *sslContext;
BIO * bio;


//��ʼץȡ
void StartCatch(string startUrl);
//����URL
bool Analyse(string url);
//���ӷ�����
bool Connect();
//����SSl����
bool SSL_Connect();
//�õ�html
bool Gethtml(string& html);
//UTFתGBK
std::string UtfToGbk(const char* utf8);
//������ʽ
bool RegexIamage(string & html);

LPCWSTR stringToLPCWSTR(std::string orig);