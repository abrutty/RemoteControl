#include "pch.h"
#include "ClientSocket.h"

CClientSocket* CClientSocket::mInstance = NULL; // 静态成员变量类内声明，类外初始化

CClientSocket::CHelper CClientSocket::mHelper; // 会调用构造，是私有的，其他地方调用不了，保证唯一

// 因为其他有多个cpp文件包含了ClientSocket.h，所以实现放在cpp文件，头文件只放定义，否则会产生冲突
std::string GetErrInfo(int WSAErrCode) {
	std::string ret;
	LPVOID lpMsgBuf = nullptr;
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		nullptr,
		WSAErrCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, nullptr
	);
	ret = (char*)lpMsgBuf;
	LocalFree(lpMsgBuf);
	return ret;
}