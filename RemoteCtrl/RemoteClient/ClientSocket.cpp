#include "pch.h"
#include "ClientSocket.h"

CClientSocket* CClientSocket::mInstance = NULL; // ��̬��Ա�������������������ʼ��

CClientSocket::CHelper CClientSocket::mHelper; // ����ù��죬��˽�еģ������ط����ò��ˣ���֤Ψһ

// ��Ϊ�����ж��cpp�ļ�������ClientSocket.h������ʵ�ַ���cpp�ļ���ͷ�ļ�ֻ�Ŷ��壬����������ͻ
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