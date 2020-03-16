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

#include <zlib.h>

//#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "libssl.lib")
#pragma comment(lib, "libcrypto.lib")
#pragma comment(lib, "zlibwapi.lib")
#pragma comment(lib, "ws2_32.lib")

#define Len 100
#define PORT 443

using std::cout;
using std::endl;
using std::string;

struct stPOI
{
	char h_name[Len];
	char h_prov[Len];
	char h_city[Len];
	char h_area[Len];
	char h_adress[Len];
	char h_type[Len];
	char h_eastnorth[Len];
};

struct stHtml
{
	char h_address[Len];
};

typedef std::list<struct stHtml> stHtmlList;
stHtmlList list_Html;

const string strResHead = "/poi/amap/district/420102/";
string g_strHost = "";
string g_strResource = "";
std::stringstream strStream;
string strHtml = "";
string g_place[Len] = { "" };
SOCKET g_send_socket;
std::ofstream real_file("POI_real.txt", std::ios::app);
int g_iBegin = 0;
int g_iHtml = 0;

SSL_CTX *sslContext;
SSL *sslHandle;

char* Byte2Char(Byte* data, uLong ndata) {
	char* res = static_cast<char*>(malloc(ndata));

	for (uLong i = 0; i < ndata; i++) {
		res[i] = data[i];
	}

	return res;
}

char* Utf8ToGbk(const char *src_str)
{
	int len = MultiByteToWideChar(CP_UTF8, 0, src_str, -1, NULL, 0);
	wchar_t* wszGBK = new wchar_t[len + 1];
	memset(wszGBK, 0, len * 2 + 2);
	MultiByteToWideChar(CP_UTF8, 0, src_str, -1, wszGBK, len);
	len = WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, NULL, 0, NULL, NULL);
	char* szGBK = new char[len + 1];
	memset(szGBK, 0, len + 1);
	WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, szGBK, len, NULL, NULL);
	if (wszGBK) delete[] wszGBK;
	return szGBK;
}

bool GzipDecompress(Byte *zdata, uLong nzdata, Byte *data, uLong *ndata) {
	int err = 0;
	z_stream d_stream = { 0 }; /* decompression stream */
	static char dummy_head[2] =
	{
		0x8 + 0x7 * 0x10,
		(((0x8 + 0x7 * 0x10) * 0x100 + 30) / 31 * 31) & 0xFF,
	};

	d_stream.zalloc = (alloc_func)0;
	d_stream.zfree = (free_func)0;
	d_stream.opaque = (voidpf)0;
	d_stream.next_in = zdata;
	d_stream.avail_in = 0;
	d_stream.next_out = data;

	if (inflateInit2(&d_stream, 47) != Z_OK) return false;

	while (d_stream.total_out < *ndata && d_stream.total_in < nzdata)
	{
		d_stream.avail_in = d_stream.avail_out = 1; /* force small buffers */
		if ((err = inflate(&d_stream, Z_NO_FLUSH)) == Z_STREAM_END) break;
		if (err != Z_OK)
		{
			if (err == Z_DATA_ERROR)
			{
				d_stream.next_in = (Bytef*)dummy_head;
				d_stream.avail_in = sizeof(dummy_head);
				if ((err = inflate(&d_stream, Z_NO_FLUSH)) != Z_OK)
				{
					return false;
				}
			}
			else return false;
		}
	}
	
	if (inflateEnd(&d_stream) != Z_OK) return false;
	*ndata = d_stream.total_out;

	return true;
}

Byte* GetGzipByte(Byte* zdata, uLong nzdata, uLong& ndata) {
	for (uLong i = 4; i < nzdata; i++) {
		if (zdata[i - 4] == '\r' &&  zdata[i - 3] == '\n' && 
			zdata[i - 2] == '\r' && zdata[i - 1] == '\n') {
			ndata = nzdata - i;
			return zdata + i;
		}
	}

	return NULL;
}

bool SSL_Connect() {
	ERR_load_BIO_strings();

	// Init SSl library，Load algorithms，load error info.
	SSL_library_init();
	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();

	// New context saying we are a client, and using SSL 2 or 3
	sslContext = SSL_CTX_new(SSLv23_client_method());
	if (sslContext == NULL)
	{
		ERR_print_errors_fp(stderr);
		return false;
	}
	// Create an SSL struct for the connection
	sslHandle = SSL_new(sslContext);
	if (sslHandle == NULL)
	{
		ERR_print_errors_fp(stderr);
		return false;
	}
	// Connect the SSL struct to our connection
	if (!SSL_set_fd(sslHandle, g_send_socket))
	{
		ERR_print_errors_fp(stderr);
		return false;
	}
	// Initiate SSL handshake
	if (SSL_connect(sslHandle) != 1)
	{
		ERR_print_errors_fp(stderr);
		return false;
	}

	//X509* x509 = SSL_get_peer_certificate(sslHandle);
	//X509_NAME* xname = X509_get_subject_name(x509);
	//printf("Connected with %s encryption/n", SSL_get_cipher(sslHandle));

	return true;
}

bool WSAStart()
{
	WSADATA wsaData = { 0 };
	if (0 != WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		return false;
	}
	hostent *server_hostent = gethostbyname(g_strHost.c_str());
	sockaddr_in send_addr_in = { 0 };
	g_send_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	send_addr_in.sin_family = AF_INET;
	send_addr_in.sin_port = htons(PORT);
	memcpy(&send_addr_in.sin_addr, server_hostent->h_addr, 4);
	if (0 != connect(g_send_socket, (sockaddr *)&send_addr_in, sizeof(send_addr_in)))
	{
		cout << "没有连接成功\r\n";
		return false;
	}

	return SSL_Connect();
}

char* GetHtml(string & strResource)
{
	const int times = 10;
	const int MAXBUFFERSIZE = 1024 * 4;
	int iCount = 0;
	char* resBuf = NULL;

	do
	{
		if (!WSAStart())
		{
			cout << "WSA Start Failed!" << endl;
			continue;
		}

		iCount++;
		if (resBuf != NULL)
		{
			free(resBuf);
			resBuf = NULL;
		}
		if (iCount >= times)
		{
			cout << "Failed to connect.\r\n";
			return false;
		}
		if(iCount > 1) cout << "The " << iCount << "th connection\r\n";

		string request_string;
		request_string = "GET " + strResource + " HTTP/1.1\r\n"
			+ "Host: " + g_strHost + "\r\n"
			+ "Content-Type: text/html; charset=utf-8\r\n"
			+ "Connection: close\r\n"
			+ "\r\n";

		//if (SOCKET_ERROR == send(g_send_socket, request_string.c_str(), request_string.length(), 0))
		//{
		//	cout << "Send Failed,Error Code: " << WSAGetLastError() << endl;
		//	continue;
		//}
		SSL_write(sslHandle, request_string.c_str(), request_string.length());

		Byte* recv_buf_c = static_cast<Byte*>(malloc(MAXBUFFERSIZE));
		ZeroMemory(recv_buf_c, MAXBUFFERSIZE);
		int recv_size = 0, offset = 0, realloc_count = 0;

		while ((recv_size = SSL_read(sslHandle, recv_buf_c + offset, MAXBUFFERSIZE - 1)) > 0)
		{
			offset += recv_size;
			if ((realloc_count + 1) * MAXBUFFERSIZE - offset< 100)
			{
				realloc_count++;
				recv_buf_c = static_cast<Byte *>(realloc(recv_buf_c, (realloc_count + 1) * MAXBUFFERSIZE));
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
		WSACleanup();
		//resBuf = Utf8ToGbk(rData);
	} while (strstr(resBuf, "html") == NULL);

	return resBuf;
}

bool GetPoiData(string & strResource)
{
	cout << "Try to connect with " << strResource << "\r\n";
	char* html = GetHtml(strResource);

	if (html == NULL) return false;

	stPOI stPoi;
	const char *pBuf = NULL;

	pBuf = strstr(html, "<title>");
	if (pBuf != NULL)
	{
		sscanf_s(pBuf, "%*[^>]>%[^-]", stPoi.h_name, Len);
		pBuf = strstr(pBuf, "text-muted");
		sscanf_s(pBuf, "%*[^>]>%*[^>]>%*[^>]>%[^<]", stPoi.h_prov, Len);
		pBuf += sizeof("text-muted");
		pBuf = strstr(pBuf, "text-muted");
		sscanf_s(pBuf, "%*[^>]>%*[^>]>%*[^>]>%[^<]", stPoi.h_city, Len);
		pBuf += sizeof("text-muted");
		pBuf = strstr(pBuf, "text-muted");
		sscanf_s(pBuf, "%*[^>]>%*[^>]>%*[^>]>%[^<]", stPoi.h_area, Len);
		pBuf += sizeof("text-muted");
		pBuf = strstr(pBuf, "text-muted");
		sscanf_s(pBuf, "%*[^>]>%*[^>]>%[^<]", stPoi.h_adress, Len);
		pBuf += sizeof("text-muted");
		pBuf = strstr(pBuf, "text-muted");
		pBuf += sizeof("text-muted");
		pBuf = strstr(pBuf, "text-muted");
		sscanf_s(pBuf, "%*[^>]>%*[^>]>%[^<]", stPoi.h_type, Len);
		pBuf += sizeof("text-muted");
		pBuf = strstr(pBuf, "text-muted");
		sscanf_s(pBuf, "%*[^>]>%*[^>]>%[^<]", stPoi.h_eastnorth, Len);
	}

	real_file << stPoi.h_name << "\t"
		<< stPoi.h_prov << "\t"
		<< stPoi.h_city << "\t"
		<< stPoi.h_area << "\t"
		<< stPoi.h_type << "\t"
		<< stPoi.h_eastnorth << endl;
	cout << "Wrote the data.\r\n";

	free(html);
	return true;
}

bool ParseUrl(const string &url_string, string &host_string, string &resource_string)
{
	const char * pos_str = strstr(url_string.c_str(), "http://");
	if (pos_str == NULL)
	{
		pos_str = url_string.c_str();
	}
	else
	{
		pos_str = pos_str + strlen("http://");
	}

	char host_c[MAXBYTE] = { 0 };
	char resource_c[MAXBYTE] = { 0 };

	sscanf_s(pos_str, "%[^/]%s", host_c, MAXBYTE, resource_c, MAXBYTE);

	host_string = host_c;
	if (strlen(resource_c) != 0)
	{
		resource_string = resource_c;
	}
	else
	{
		resource_string = "/";
	}

	return true;
}

bool ParseHtml()
{
	if (!WSAStart())
	{
		cout << "WSA Start Failed!" << endl;
		return false;
	}

	cout << "Try to connect with " << g_strResource << endl;
	std::shared_ptr<char> html(GetHtml(g_strResource), free);

	if (html.get() == NULL)
	{
		return false;
	}

	const char *szBuf2 = html.get();
	while (szBuf2)
	{
		szBuf2 = strstr(szBuf2, "<tr>");
		if (szBuf2 == NULL)
		{
			break;
		}
		szBuf2 = strstr(szBuf2, "<td>");	// match "<td>"
		if (szBuf2 == NULL)
		{
			break;
		}

		stHtml html;
		sscanf_s(szBuf2, "%*[^\"]\"%[^\"]", html.h_address, Len);	// get the url between " ".
		list_Html.push_back(html);
	}

	strStream.clear();
	strStream << ++g_iHtml;
	strStream >> strHtml;
	g_strResource = strResHead + strHtml + ".html";

	WSACleanup();
	return true;
}

int main()
{
	g_strHost = "www.poi86.com";

	std::ifstream save_file("Save.txt");
	save_file >> strHtml >> g_iBegin;
	save_file.close();
	g_iHtml = atoi(strHtml.c_str());

	if (g_iHtml <= 0)
	{
		char cTem;
		cout << "The starting point MAY be broken. First time for \"Y/y\", or check the starting point in \"Save.txt\"!\n";
		std::cin >> cTem;
		if (cTem == 'y' || cTem == 'Y')
		{
			g_iHtml = 1;
			strHtml = "1";
		}
		else
		{
			std::cout << "Error for starting point.\r\n";
			system("pause");
			return -1;
		}
	}

	if (g_iBegin < 0 || g_iBegin >= 50)
	{
		cout << "The starting point MUST be broken!\r\n";
		system("pause");
		return -1;
	}

	g_strResource = strResHead + strHtml + ".html";
	
	while (ParseHtml())
	{
		int iCount = 0;
		cout << g_iHtml - 1 << "." << endl;
		for (auto it = list_Html.begin(); it != list_Html.end(); it++)
		{
			++iCount;
			if (g_iBegin)
			{
				g_iBegin--;
				continue;
			}

			cout << iCount << ".\t";
			string strBuf = it->h_address;
			GetPoiData(strBuf);

			if (iCount >= 44)
			{
				std::ofstream save_file("Save.txt");
				save_file << g_iHtml << " " << 0;
				save_file.close();
			}
			else
			{
				std::ofstream save_file("Save.txt");
				save_file << g_iHtml - 1 << " " << iCount + 1;
				save_file.close();
			}
		}
		list_Html.clear();
	}

	real_file.close();
	SSL_shutdown(sslHandle);
	SSL_free(sslHandle);
	SSL_CTX_free(sslContext);
	closesocket(g_send_socket);
	system("pause");
	return 0;
}