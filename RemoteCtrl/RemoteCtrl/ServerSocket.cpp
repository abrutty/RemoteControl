#include "pch.h"
#include "ServerSocket.h"

//CServerSocket server;
CServerSocket* CServerSocket::mInstance = NULL; // ��̬��Ա�������������������ʼ��

CServerSocket::CHelper CServerSocket::mHelper; // ����ù��죬��˽�еģ������ط����ò��ˣ���֤Ψһ
//CServerSocket* pserver = CServerSocket::getInstance();
