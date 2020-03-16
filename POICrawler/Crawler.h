#pragma once

#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <WinSock2.h>
#include <string>
#include <list>
#include <memory>

#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "tools.h"

//#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "libssl.lib")
#pragma comment(lib, "libcrypto.lib")
#pragma comment(lib, "ws2_32.lib")

using namespace std;

#define HTPPS_PORT 443
#define HTTPS_MAXBUFFERSIZE 1024 * 4

class HttpsClient {
private:
	const int _port;
	const int _repeat;

	SOCKET _socket;
	SSL* _pSSLHandle;
	SSL_CTX *_pSSLContext;

	bool init();
	bool sslConnect();
	bool start(const string& host);
	bool close();
public:
	HttpsClient();
	~HttpsClient();

	char* get(const string& host, const string& urlSrc);
};


HttpsClient::HttpsClient() : _port(HTPPS_PORT), _repeat(3) {
	init();
}

HttpsClient::~HttpsClient() {
	if (_pSSLHandle) {
		SSL_shutdown(_pSSLHandle);
		SSL_free(_pSSLHandle);
		_pSSLHandle = NULL;
	}
	if (_pSSLContext) {
		SSL_CTX_free(_pSSLContext);
		_pSSLContext = NULL;
	}

	closesocket(_socket);
}

bool HttpsClient::init() {
	_pSSLHandle = NULL;
	_pSSLContext = NULL;

	ERR_load_BIO_strings();
	SSL_library_init();
	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();

	return true;
}

bool HttpsClient::sslConnect() {
	_pSSLContext = SSL_CTX_new(SSLv23_client_method());
	if (_pSSLContext == NULL)
	{
		ERR_print_errors_fp(stderr);
		return false;
	}
	// Create an SSL struct for the connection
	_pSSLHandle = SSL_new(_pSSLContext);
	if (_pSSLHandle == NULL)
	{
		ERR_print_errors_fp(stderr);
		return false;
	}
	// Connect the SSL struct to our connection
	if (!SSL_set_fd(_pSSLHandle, _socket))
	{
		ERR_print_errors_fp(stderr);
		return false;
	}
	// Initiate SSL handshake
	if (SSL_connect(_pSSLHandle) != 1)
	{
		ERR_print_errors_fp(stderr);
		return false;
	}

	//X509* x509 = SSL_get_peer_certificate(sslHandle);
	//X509_NAME* xname = X509_get_subject_name(x509);
	//printf("Connected with %s encryption/n", SSL_get_cipher(sslHandle));

	return true;
}

bool HttpsClient::start(const string& host) {

	WSADATA wsaData = { 0 };
	if (0 != WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		return false;
	}
	hostent *server_hostent = gethostbyname(host.c_str());
	sockaddr_in send_addr_in = { 0 };
	_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	send_addr_in.sin_family = AF_INET;
	send_addr_in.sin_port = htons(_port);
	memcpy(&send_addr_in.sin_addr, server_hostent->h_addr, 4);
	if (0 != connect(_socket, (sockaddr *)&send_addr_in, sizeof(send_addr_in)))
	{
		cout << "Connect to host failed\r\n";
		return false;
	}

	return sslConnect();
}

bool HttpsClient::close() {
	WSACleanup();
	_pSSLHandle = NULL;
	_pSSLContext = NULL;
	return true;
}

char* HttpsClient::get(const string& host, const string& urlSrc) {
	int i = 0;
	char* resBuf = NULL;

	do
	{
		if (this->start(host)) {
			cout << "WSA Start Failed!" << endl;
			continue;
		}

		if (resBuf != NULL) {
			free(resBuf);
			resBuf = NULL;
		}

		if (i >= _repeat) {
			cout << "Failed to connect.\r\n";
			return false;
		}

		if (i > 0) cout << "Repeat the " << i << "th time.\n";
		i++;

		string request_string;
		request_string = "GET " + urlSrc + " HTTP/1.1\r\n"
			+ "Host: " + host + "\r\n"
			+ "Content-Type: text/html; charset=utf-8\r\n"
			+ "Connection: close\r\n"
			+ "\r\n";

		//if (SOCKET_ERROR == send(g_send_socket, request_string.c_str(), request_string.length(), 0))
		//{
		//	cout << "Send Failed,Error Code: " << WSAGetLastError() << endl;
		//	continue;
		//}

		SSL_write(_pSSLHandle, request_string.c_str(), request_string.length());

		Byte* recv_buf_c = static_cast<Byte*>(malloc(HTTPS_MAXBUFFERSIZE));
		ZeroMemory(recv_buf_c, HTTPS_MAXBUFFERSIZE);
		int recv_size = 0, offset = 0, realloc_count = 0;

		while ((recv_size = SSL_read(_pSSLHandle, recv_buf_c + offset, HTTPS_MAXBUFFERSIZE - 1)) > 0)
		{
			offset += recv_size;
			if ((realloc_count + 1) * HTTPS_MAXBUFFERSIZE - offset< 100)
			{
				realloc_count++;
				recv_buf_c = static_cast<Byte *>(realloc(recv_buf_c, (realloc_count + 1) * HTTPS_MAXBUFFERSIZE));
			}
		}
		recv_buf_c[offset] = '\0';
		std::shared_ptr<Byte> recvPtr(recv_buf_c, free);

		uLong nzdata = 0;
		Byte* zdata = GetGzipByte(recv_buf_c, offset, nzdata);
		uLong ndata = 10 * nzdata;
		std::shared_ptr<Byte> bufPtr(static_cast<Byte*>(malloc(ndata)), free);

		if (false == GzipDecompress(zdata, nzdata, bufPtr.get(), &ndata)) {
			cout << "Error for gzip.\r\n";
			return NULL;
		}

		resBuf = Byte2Char(bufPtr.get(), ndata);
		this->close();
	} while (strstr(resBuf, "html") == NULL);

	return resBuf;
}