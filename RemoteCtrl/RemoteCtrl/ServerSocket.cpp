#include "pch.h"
#include "ServerSocket.h"

//CServerSocket server;
CServerSocket* CServerSocket::mInstance = NULL; // 静态成员变量类内声明，类外初始化

CServerSocket::CHelper CServerSocket::mHelper; // 会调用构造，是私有的，其他地方调用不了，保证唯一
//CServerSocket* pserver = CServerSocket::getInstance();
