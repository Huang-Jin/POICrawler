# POICrawler

## 2020/02/28
- 大三科研立项时写的，数据是存成了txt，因为后续的处理软件可以直接读。
- 具体的思路也是用Socket、TCP协议来模拟http连接，然后获得html之后进行字符串处理。
- 之前的网址是http的，最近他们搞成https了，所以要加上openSSl来做https版本的。
主要就是给原来的socket套上一个SSLHandle，然后connect。读写方式发生了小小的改变（包装）。
- 不过目前调试有点问题，read到的数据是乱码的，猜测可能是这个网站加入了反爬虫？有可能和cookie有关，还没测试。应该无关。

## 2020/02/29
- 经过利用firefox浏览器查看，感觉可能是加密算法的问题，这个网站用的TLS1.0，解密不成功。
- 测试了一下Python的urllib，整体来说很正常，出现的一个问题是获得的html字节流是gzip编码的，需要解码。
- 真相大白，需要gzip解码。C++获取的data里出现了gzip开头，0x1f 0x8b 0x08。
利用zlib库来做gzip的解码，主要问题有
- 首先莫名其妙的lib库无法链接，翻阅readme和百度搜索，原因在于需要在VS的宏定义里加上ZLIB_WINAPI。
- zlib解码代码是直接抄的。大致看了一下，应该是先设置好输入输出Byte\*，存到stream里，然后用inflate函数一步步解码。
- Byte\*与char\*没法直接static_cast。重写了一个转换代码。
- 原来准备转成gbk的，代码都找好了，不过后来觉得文件存的utf-8更好，反正不影响匹配。
- 另外对于malloc/free创建的空间，可以用shared_ptr来存储，方便释放。
- std::shared_ptr<Byte> bufPtr(static_cast<Byte*>(malloc(ndata)), free);

## 问题
1. WSAStart之后，用过一次就不能继续用了，需要关闭再开始才行。

