#include <iostream>
#include <fstream>
#include <sstream>
//#include <iomanip>
//#include <WinSock2.h>
#include <string>
#include <vector>
#include <memory>

#include "Crawler.h"
#include "mysqlHelper.h"

using namespace std;

struct POI
{
	char name[MAXBYTE];
	char prov[MAXBYTE];
	char city[MAXBYTE];
	char district[MAXBYTE];
	char address[MAXBYTE];
	char type[MAXBYTE];
	char latlon[MAXBYTE];
};

vector<string> g_htmls;
const string strUrlHead = "/poi/amap/district/420102/";
const string g_table_name = "JiangAnDistrict";
string g_strHost = "";
int g_index = 0;
int g_page = 0;

MySQLParam param;
MySQLHelper sqlHelper(&param);

HttpsClient g_client;

void GetLastIndex(int & page, int & index) {
	string sql = "select pageNum, pageIndex from " + g_table_name + " order by id desc limit 1;";
	MYSQL_RES * res = sqlHelper.query(sql.c_str());
	MYSQL_ROW row;
	if (res != NULL && (row = mysql_fetch_row(res)) != NULL) {
		page = atoi(row[0]);
		index = atoi(row[1]);
	}
	else {
		page = 0;
		index = 0;
	}
}

bool Write2MySQL(POI & poi) {
	string sql = "";
	sql += "insert into " + g_table_name + " values(id, '" + string(poi.name)
		+ "', '" + string(poi.prov)
		+ "', '" + string(poi.city)
		+ "', '" + string(poi.district)
		+ "', '" + string(poi.address)
		+ "', '" + string(poi.type)
		+ "', '" + string(poi.latlon)
		+ "', '" + to_string(g_page)
		+ "', '" + to_string(g_index + 1)
		+ "', NULL);";

	sqlHelper.query(sql.c_str());
	return true;
}

bool GetPoiData(string & strResource)
{
	cout << "Connect with " << strResource << "\r\n";
	shared_ptr<char> html(g_client.get(g_strHost, strResource), free);
	if (html.get() == NULL) return false;

	POI stPoi;
	const char *pBuf = NULL;

	pBuf = strstr(html.get(), "<title>");
	if (pBuf != NULL)
	{
		sscanf_s(pBuf, "%*[^>]>%[^-]", stPoi.name, MAXBYTE);
		pBuf = strstr(pBuf, "text-muted");
		sscanf_s(pBuf, "%*[^>]>%*[^>]>%*[^>]>%[^<]", stPoi.prov, MAXBYTE);
		pBuf += sizeof("text-muted");
		pBuf = strstr(pBuf, "text-muted");
		sscanf_s(pBuf, "%*[^>]>%*[^>]>%*[^>]>%[^<]", stPoi.city, MAXBYTE);
		pBuf += sizeof("text-muted");
		pBuf = strstr(pBuf, "text-muted");
		sscanf_s(pBuf, "%*[^>]>%*[^>]>%*[^>]>%[^<]", stPoi.district, MAXBYTE);
		pBuf += sizeof("text-muted");
		pBuf = strstr(pBuf, "text-muted");
		sscanf_s(pBuf, "%*[^>]>%*[^>]>%[^<]", stPoi.address, MAXBYTE);
		pBuf += sizeof("text-muted");
		pBuf = strstr(pBuf, "text-muted");
		pBuf += sizeof("text-muted");
		pBuf = strstr(pBuf, "text-muted");
		sscanf_s(pBuf, "%*[^>]>%*[^>]>%[^<]", stPoi.type, MAXBYTE);
		pBuf += sizeof("text-muted");
		pBuf = strstr(pBuf, "text-muted");
		sscanf_s(pBuf, "%*[^>]>%*[^>]>%[^<]", stPoi.latlon, MAXBYTE);
	}

	Write2MySQL(stPoi);
	return true;
}

bool ParseUrl(const string & url, string & host, string & src)
{
	const char * pos_str = strstr(url.c_str(), "http://");
	if (pos_str == NULL) {
		pos_str = url.c_str();
	}
	else {
		pos_str = pos_str + strlen("http://");
	}

	char host_c[MAXBYTE] = { 0 };
	char resource_c[MAXBYTE] = { 0 };

	sscanf_s(pos_str, "%[^/]%s", host_c, MAXBYTE, resource_c, MAXBYTE);

	host = host_c;
	if (strlen(resource_c) != 0) {
		src = resource_c;
	}
	else {
		src = "/";
	}

	return true;
}

bool ParseHtml()
{
	string url = strUrlHead + to_string(g_page) + ".html";
	shared_ptr<char> html(g_client.get(g_strHost, url), free);
	if (html.get() == NULL) return false;

	char temp[MAXBYTE] = { 0 };
	const char *buf = html.get();
	while (buf) {
		buf = strstr(buf, "<tr>");
		if (buf == NULL) {
			break;
		}
		buf = strstr(buf, "<td>");	// match "<td>"
		if (buf == NULL) {
			break;
		}

		sscanf_s(buf, "%*[^\"]\"%[^\"]", temp, MAXBYTE);	// get the url between " ".
		g_htmls.emplace_back(temp);
	}
	return true;
}

int main()
{
	sqlHelper.connect();

	string sql = "create table " + g_table_name + " (id bigint(20) AUTO_INCREMENT primary key, name varchar(255), province varchar(20), "
		+ "city varchar(10), district varchar(15), address varchar(255), type varchar(255), LatLon varchar(50), "
		+ "pageNum int(4), pageIndex int(4), "
		+ "update_time timestamp default current_timestamp on update current_timestamp"
		+ ") character set utf8;";
	sqlHelper.query(sql.c_str());

	g_strHost = "www.poi86.com";

	GetLastIndex(g_page, g_index);
	if (g_page <= 0) {
		cout << "There are no data in log, using default configuration.\n";
		g_page = 1;
		g_index = 0;
	}
	else {
		cout << "Start from page " << g_page << ", index " << g_index << ".\n";
	}

	while (ParseHtml())
	{
		cout << g_page << "." << endl;
		for (; g_index < g_htmls.size(); g_index++)
		{
			GetPoiData(g_htmls[g_index]);
		}
		g_page++;
		g_index = 0;
		g_htmls.clear();
	}

	system("pause");
	return 0;
}