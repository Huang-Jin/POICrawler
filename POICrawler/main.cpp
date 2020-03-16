#include <iostream>
#include <fstream>
#include <sstream>
//#include <iomanip>
//#include <WinSock2.h>
#include <string>
#include <list>
#include <memory>

#include "Crawler.h"
#include "mysqlHelper.h"

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
std::ofstream real_file("POI_real.txt", std::ios::app);
int g_iBegin = 0;
int g_iHtml = 0;

HttpsClient g_client;

bool GetPoiData(string & strResource)
{
	cout << "Try to connect with " << strResource << "\r\n";
	shared_ptr<char> html(g_client.get(g_strHost, strResource), free);
	if (html.get() == NULL) return false;

	stPOI stPoi;
	const char *pBuf = NULL;

	pBuf = strstr(html.get(), "<title>");
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

	return true;
}

bool ParseUrl(const string & url, string & host, string & src)
{
	const char * pos_str = strstr(url.c_str(), "http://");
	if (pos_str == NULL)
	{
		pos_str = url.c_str();
	}
	else
	{
		pos_str = pos_str + strlen("http://");
	}

	char host_c[MAXBYTE] = { 0 };
	char resource_c[MAXBYTE] = { 0 };

	sscanf_s(pos_str, "%[^/]%s", host_c, MAXBYTE, resource_c, MAXBYTE);

	host = host_c;
	if (strlen(resource_c) != 0)
	{
		src = resource_c;
	}
	else
	{
		src = "/";
	}

	return true;
}

bool ParseHtml()
{
	cout << "Try to connect with " << g_strResource << endl;

	shared_ptr<char> html(g_client.get(g_strHost, g_strResource), free);
	if (html.get() == NULL) return false;

	const char *buf = html.get();
	while (buf)
	{
		buf = strstr(buf, "<tr>");
		if (buf == NULL)
		{
			break;
		}
		buf = strstr(buf, "<td>");	// match "<td>"
		if (buf == NULL)
		{
			break;
		}

		stHtml html;
		sscanf_s(buf, "%*[^\"]\"%[^\"]", html.h_address, Len);	// get the url between " ".
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
	MySQLParam param;
	MySQLHelper sqlHelper(&param);
	sqlHelper.connect();

	char * sql = "create table test (name varchar(20), province varchar(20), city varchar(10), district varchar(15), address varchar(50), type varchar(30), LatLon varchar(100));";
	sqlHelper.query(sql);

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
	system("pause");
	return 0;
}