#include "pch.h"
#include "MyNetWork.h"

ESever::ESever(const EServerParameter& param):m_stop(false), m_args(NULL)
{
	m_params = param;
	m_thread.UpdateWorker(ThreadWorker(this, (FUNCTYPE)&ESever::threadFunc));
}

ESever::~ESever()
{
	Stop();
}

int ESever::Invoke(void* arg)
{
	m_sock.reset(new MySocket(m_params.m_type));
	if (*m_sock == INVALID_SOCKET) {
		printf("%s(%d):%s error\r\n", __FILE__, __LINE__, __FUNCTION__);
		return -1;
	}
	if (m_params.m_type == MYTYPE::MyTypeTCP) {
		if (m_sock->listen() == -1) return -2;
	}
	MySockAddrIn client;
	if (m_sock->bind(m_params.m_ip, m_params.m_port) == -1) {
		printf("%s(%d):%s bind error\r\n", __FILE__, __LINE__, __FUNCTION__);
		return -3;
	}
	if (m_thread.Start() == false) return -4;
	m_args = arg;
	return 0;
}

int ESever::Send(MYSOCKET& client, const MyBuffer& buffer)
{
	int ret = m_sock->send(buffer);
	if (m_params.m_send) m_params.m_send(m_args, client, ret);
	return ret;
}

int ESever::Sendto(MySockAddrIn& addr, const MyBuffer& buffer)
{
	int ret = m_sock->sendto(buffer, addr);
	if (m_params.m_sendto) m_params.m_sendto(m_args, addr, ret);
	return ret;
}

int ESever::Stop()
{
	if (m_stop == false) {
		m_sock->close();
		m_stop = true;
		m_thread.Stop();
	}
	return 0;
}

int ESever::threadFunc()
{
	if (m_params.m_type == MYTYPE::MyTypeTCP) {
		return threadTCPFunc();
	}
	else {
		return threadUDPFunc();
	}
	
}

int ESever::threadUDPFunc()
{
	MyBuffer buf(1024 * 256);
	MySockAddrIn client;
	int ret = 0;
	while (!m_stop) {
		ret = m_sock->recvfrom(buf, client);
		//printf("%s(%d):%s 服务端收到 %d\r\n", __FILE__, __LINE__, __FUNCTION__, ret);
		if (ret > 0) {
			client.update();
			if (m_params.m_recvfrom != NULL) {
				m_params.m_recvfrom(m_args, buf, client);
			}
			/*if (lstclients.size() <= 0) {
				lstclients.push_back(client);
				printf("%s(%d):%s IP=%s port=%d\r\n", __FILE__, __LINE__, __FUNCTION__, client.GetIP().c_str(), client.GetPort());
				ret = sock->sendto(buf, client);
				printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
			}
			else {
				buf.Update((void*)&lstclients.front(), lstclients.front().size());
				ret = sock->sendto(buf, client);
				printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
			}*/
			//CTool::Dump((BYTE*)buf, ret);	
		}
		else {
			printf("%s(%d):%s error\r\n", __FILE__, __LINE__, __FUNCTION__);
			break;
		}
	}
	if (m_stop == false) m_stop = true;
	m_sock->close();
	printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
	return 0;
}

EServerParameter::EServerParameter(
	const std::string& ip, 
	short port, 
	MYTYPE type, 
	AcceptFunc acceptf, 
	RecvFunc recvf,
	SendFunc sendf, 
	RecvFromFunc recvfromf, 
	SendToFunc sendtof)
{
	m_ip = ip;
	m_port = port;
	m_type = type;
	m_accept = acceptf;
	m_recv = recvf;
	m_send = sendf;
	m_recvfrom = recvfromf;
	m_sendto = sendtof;
}


EServerParameter::EServerParameter(const EServerParameter& param)
{
	m_ip = param.m_ip;
	m_port = param.m_port;
	m_type = param.m_type;
	m_accept = param.m_accept;
	m_recv = param.m_recv;
	m_send = param.m_send;
	m_recvfrom = param.m_recvfrom;
	m_sendto = param.m_sendto;
}

EServerParameter& EServerParameter::operator=(const EServerParameter& param)
{
	if (this != &param) {
		m_ip = param.m_ip;
		m_port = param.m_port;
		m_type = param.m_type;
		m_accept = param.m_accept;
		m_recv = param.m_recv;
		m_send = param.m_send;
		m_recvfrom = param.m_recvfrom;
		m_sendto = param.m_sendto;
	}
	return *this;
}
