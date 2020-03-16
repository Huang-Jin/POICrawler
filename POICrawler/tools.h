#pragma once
#include <zlib.h>

#pragma comment(lib, "zlibwapi.lib")

using namespace std;

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

