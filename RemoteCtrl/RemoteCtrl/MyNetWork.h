#pragma once
#include "MySocket.h"
#include "MyThread.h"

class ENetWork
{
};

typedef int (*AcceptFunc)(void* arg, MySocket& client);
typedef int (*RecvFunc)(void* arg, const MyBuffer& buffer);
typedef int (*SendFunc)(void* arg, const MYSOCKET& client, int ret);
typedef int (*RecvFromFunc)(void* arg, const MyBuffer& buffer, MySockAddrIn& addr);
typedef int (*SendToFunc)(void* arg, const MySockAddrIn& addr, int ret);

class EServerParameter
{
public:
	EServerParameter(
		const std::string& ip = "0.0.0.0",
		short port = 9527,
		MYTYPE type = MYTYPE::MyTypeTCP,
		AcceptFunc acceptf = NULL,
		RecvFunc recvf = NULL,
		SendFunc sendf = NULL,
		RecvFromFunc recvfromf = NULL,
		SendToFunc sendtof = NULL
	);
	// 输入
	EServerParameter& operator<<(AcceptFunc func) {
		m_accept = func; 
		return *this; 
	}
	EServerParameter& operator<<(RecvFunc func) { 
		m_recv = func; 
		return *this; 
	}
	EServerParameter& operator<<(SendFunc func) {
		m_send = func; 
		return *this;
	}
	EServerParameter& operator<<(RecvFromFunc func) {
		m_recvfrom = func; 
		return *this;
	}
	EServerParameter& operator<<(SendToFunc func) {
		m_sendto = func; 
		return *this;
	}
	EServerParameter& operator<<(const std::string& ip) {
		m_ip = ip; 
		return *this;
	}
	EServerParameter& operator<<(short port) {
		m_port = port; 
		return *this;
	}
	EServerParameter& operator<<(MYTYPE type) {
		m_type = type; 
		return *this;
	}
	// 输出
	EServerParameter& operator>>(AcceptFunc& func) {
		func = m_accept; 
		return *this;
	}
	EServerParameter& operator>>(RecvFunc& func) {
		func = m_recv;
		return *this;
	}
	EServerParameter& operator>>(SendFunc& func) {
		func = m_send;
		return *this;
	}
	EServerParameter& operator>>(RecvFromFunc& func) {
		func = m_recvfrom;
		return *this;
	}
	EServerParameter& operator>>(SendToFunc& func) {
		func = m_sendto;
		return *this;
	}
	EServerParameter& operator>>(std::string& ip) {
		ip = m_ip;
		return *this;
	}
	EServerParameter& operator>>(short& port) {
		port = m_port;
		return *this;
	}
	EServerParameter& operator>>(MYTYPE& type) {
		type = m_type;
		return *this;
	}
	//复制构造函数，等于号重载
	EServerParameter(const EServerParameter& param);
	EServerParameter& operator=(const EServerParameter& param);
	std::string m_ip;
	short m_port;
	MYTYPE m_type;
	AcceptFunc m_accept;
	RecvFunc m_recv;
	SendFunc m_send;
	RecvFromFunc m_recvfrom;
	SendToFunc m_sendto;
};

class ESever : public ThreadFuncBase
{
public:
	ESever(const EServerParameter& param);
	~ESever();
	int Invoke(void* arg);
	int Send(MYSOCKET& client, const MyBuffer& buffer);
	int Sendto(MySockAddrIn& addr, const MyBuffer& buffer);
	int Stop();
private:
	int threadFunc();
	int threadTCPFunc() { return 0; }
	int threadUDPFunc();
private:
	EServerParameter m_params;
	void* m_args;
	MyThread m_thread;
	MYSOCKET m_sock;
	std::atomic<bool> m_stop;
};