#pragma once
#include "MyThread.h"
#include <map>
#include "CQueue.h"
#include <MSWSock.h>
#include "Tool.h"

enum MyOperator {
    MyNone,
    MyAccept,
    MyRecv,
    MySend,
    MyError
};
class MyServer;
class MyClient;
typedef std::shared_ptr<MyClient> PCLIENT;

class MyOverlapped : public ThreadFuncBase
{
public:
    OVERLAPPED m_overlapped;
    DWORD m_operator; // ����
    std::vector<char> m_buffer; // ������
    ThreadWorker m_worker; // ������
	MyServer* m_server; // ����������
	MyClient* m_client; // ��Ӧ�Ŀͻ���
	WSABUF m_wsabuffer;
	virtual ~MyOverlapped() {
		m_client = NULL;
	}
};
template<MyOperator> class AcceptOverlapped;
typedef AcceptOverlapped<MyAccept> ACCEPTOVERLAPPED;

template<MyOperator> class RecvOverlapped;
typedef RecvOverlapped<MyRecv> RECVOVERLAPPED;

template<MyOperator> class SendOverlapped;
typedef SendOverlapped<MySend> SENDOVERLAPPED;


class MyClient :public ThreadFuncBase
{
public:
	MyClient();
	~MyClient();
	void SetOverlapped(MyClient* ptr);
	operator SOCKET() {
		return m_sock;
	}
	operator PVOID() {
		return (PVOID)m_buffer.data();
	}
	operator LPOVERLAPPED();
	operator LPDWORD() {
		return &m_received;
	}
	LPWSABUF RecvWSABuffer();
	LPWSABUF SendWSABuffer();
	LPOVERLAPPED RecvOverlapped();
	LPOVERLAPPED SendOverlapped();
	DWORD& flags() { return m_flags; }
	sockaddr_in* getLocalAddr() { return &m_local_addr; }
	sockaddr_in* getRemoteAddr() { return &m_remote_addr; }
	size_t GetBufferSize() const { return m_buffer.size(); }
	int Recv();
	int Send(void* buffer, size_t nSize); // Send ���������������ݣ�ֻ�ǰ�������ӵ����Ͷ���
	int SendData(std::vector<char>& data);
private:
	SOCKET m_sock;
	DWORD m_received;
	DWORD m_flags;
	std::shared_ptr<ACCEPTOVERLAPPED> m_overlapped;
	std::shared_ptr<RECVOVERLAPPED> m_recv;
	std::shared_ptr<SENDOVERLAPPED> m_send;
	std::vector<char> m_buffer;
	size_t m_used; // �Ѿ�ʹ�õĻ�������С
	sockaddr_in m_local_addr;
	sockaddr_in m_remote_addr;
	bool m_isbusy;
	SendQueue<std::vector<char>> m_vecSend; // �������ݶ���
};

/*
* IOCP����֪ͨ��IOCP���׽��ֺ��׽���ͨ����AcceptEX��WSASend��WSARecv�¼�����IOCP������Щ�¼�
* IOCP�յ���Ӧ���¼��󣬾ͻỽ�Ѷ�Ӧ���ص��ṹ������GetQueuedCompletionStatus��ȥ����������ͻ����AceeptWorker,RecvWorker��
* �����������У��������������Ļ������У�������RecvWorker�Ⱥ���������Ҫȥ�������������
* 
* WSARecv,WSASend �����������Ľ��պͷ��ͣ����Ǹ���IOCP�������Ϣ��������첽����
*/

// AcceptEX �� WSARecv��WSASend��һ���ĵ����ڣ�Accept��ѿͻ��˵�socket���
template<MyOperator>
class AcceptOverlapped : public MyOverlapped
{
public:
	AcceptOverlapped();
	virtual ~AcceptOverlapped() {}
	int AcceptWorker();
};


template<MyOperator>
class RecvOverlapped : public MyOverlapped
{
public:
	RecvOverlapped();
	virtual ~RecvOverlapped() {}
	int RecvWorker() {
		int ret = m_client->Recv();
		return ret;
	}
	//PCLIENT m_client;
};


template<MyOperator>
class SendOverlapped : public MyOverlapped
{
public:
	SendOverlapped();
	virtual ~SendOverlapped() {}
	int SendWorker() {
		return -1;
	}
};


template<MyOperator>
class ErrorOverlapped : public MyOverlapped
{
public:
	ErrorOverlapped() :m_operator(MyError), m_worker(this, &ErrorOverlapped::ErrorWorker) {
		memset(&m_overlapped, 0, sizeof(m_overlapped));
		m_buffer.resize(1024);
	}
	virtual ~ErrorOverlapped() {}
	int ErrorWorker() {
		return -1;
	}
};
typedef ErrorOverlapped<MyError> ERROROVERLAPPED;


class MyServer : public ThreadFuncBase
{
public:
	MyServer(const std::string& ip="0.0.0.0", short port=9527) :m_pool(10) {
		m_hIOCP = INVALID_HANDLE_VALUE;
		m_sock = INVALID_SOCKET;
		m_addr.sin_family = AF_INET;
		m_addr.sin_port = htons(port);
		m_addr.sin_addr.s_addr = inet_addr(ip.c_str());
	}
	~MyServer();
	bool StartService();
	bool NewAccept();
	void BindNewSocket(SOCKET s, ULONG_PTR nKey);
private:
	void CreateSocket() {
		WSADATA data;
		if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
			return;
		}
		m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
		int opt = 1;
		setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
	}
	
	int threadIocp();
private:
	MyThreadPool m_pool;
	HANDLE m_hIOCP;
	SOCKET m_sock;
	sockaddr_in m_addr;
	std::map<SOCKET, MyClient*> m_client;
	//CQueue<MyClient> m_lstClient;
};

