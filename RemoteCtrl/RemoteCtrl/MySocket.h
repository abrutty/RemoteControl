#pragma once
#include <WinSock2.h>
#include <memory>
enum class MYTYPE {
	MyTypeTCP=1,
	MyTypeUDP
};

class MySockAddrIn
{
public:
	MySockAddrIn() {
		memset(&m_addr, 0, sizeof(m_addr));
		m_ip = -1;
	}
	MySockAddrIn(sockaddr_in addr) {
		memcpy(&m_addr, &addr, sizeof(addr));
		m_ip = inet_ntoa(m_addr.sin_addr);
		m_port = ntohs(m_addr.sin_port);
	}
	MySockAddrIn(UINT nIP, short nPort) {
		m_addr.sin_family = AF_INET;
		m_addr.sin_port = htons(nPort);
		m_addr.sin_addr.s_addr = htonl(nIP);
		m_ip = inet_ntoa(m_addr.sin_addr);
		m_port = nPort;
	}
	MySockAddrIn(const std::string& strIP, short nPort) {
		m_ip = strIP;
		m_port = nPort;
		m_addr.sin_family = AF_INET;
		m_addr.sin_port = htons(nPort);
		m_addr.sin_addr.s_addr = inet_addr(strIP.c_str());
	}
	MySockAddrIn(const MySockAddrIn& addr) {
		memcpy(&m_addr, &addr.m_addr, sizeof(m_addr));
		m_ip = addr.m_ip;
		m_port = addr.m_port;
	}
	MySockAddrIn& operator=(const MySockAddrIn& addr) {
		if (this != &addr) {
			memcpy(&m_addr, &addr.m_addr, sizeof(m_addr));
			m_ip = addr.m_ip;
			m_port = addr.m_port;
		}
		return *this;
	}
	operator sockaddr* () const {
		return (sockaddr*)&m_addr;
	}
	operator void* () const {
		return (void*)&m_addr;
	}
	void update() {
		m_ip = inet_ntoa(m_addr.sin_addr);
		m_port = ntohs(m_addr.sin_port);
	}
	std::string GetIP() const { return m_ip; }
	short GetPort() const { return m_port; }
	inline int size() const { return sizeof(sockaddr_in); }

private:
	sockaddr_in m_addr;
	std::string m_ip;
	short m_port;
};

class MyBuffer :public std::string
{
public:
	MyBuffer(const char* str) {
		resize(strlen(str));
		memcpy((void*)c_str(), str, size());
	}
	MyBuffer(size_t size = 0) :std::string() {
		if (size > 0) {
			resize(size);
			memset(*this, 0, this->size());
		}
	}
	MyBuffer(void* buffer, size_t size) :std::string() {
		resize(size);
		memcpy((void*)c_str(), buffer, size);
	}
	/*MyBuffer& operator=(const char* str) {
		std::string::operator=(str);
		return *this;
	}*/
	~MyBuffer() {
		std::string::~basic_string();
	}
	void Update(void* buffer, size_t size) {
		resize(size);
		memcpy((void*)c_str(), buffer, size);
	}
	operator char* () const { return (char*)c_str(); }
	operator const char* ()  const { return c_str(); }
	operator BYTE* () const { return (BYTE*)c_str(); }
	operator void* ()const { return (void*)c_str(); }
};

class MySocket
{
public:
	MySocket(MYTYPE nType = MYTYPE::MyTypeTCP, int nProtocol = 0) {
		m_socket = socket(PF_INET, (int)nType, nProtocol);
		m_type = nType;
		m_protocol = nProtocol;
	}
	MySocket(const MySocket& sock) {
		m_socket = socket(PF_INET, (int)sock.m_type, sock.m_protocol);
		m_type = sock.m_type;
		m_protocol = sock.m_protocol;
		m_addr = sock.m_addr;
	}
	~MySocket() {
		close();
	}
	MySocket& operator=(const MySocket& sock) {
		if (this != &sock) {
			m_socket = socket(PF_INET, (int)sock.m_type, sock.m_protocol);
			m_type = sock.m_type;
			m_protocol = sock.m_protocol;
			m_addr = sock.m_addr;
		}
		return *this;
	}
	operator SOCKET() const { return m_socket; }
	operator SOCKET() { return m_socket; }
	bool operator==(SOCKET sock) const { return m_socket == sock; }
	int listen(int backlog = 5) {
		if (m_type != MYTYPE::MyTypeTCP) return -1;
		return ::listen(m_socket, backlog);
	}
	int bind(const std::string& ip, short port) {
		m_addr = MySockAddrIn(ip, port);
		return ::bind(m_socket, m_addr, m_addr.size());
	}
	int accept(){}
	int connect(const std::string& ip,short port){}
	int send(const MyBuffer& buffer) { return ::send(m_socket, buffer, (int)buffer.size(), 0); }
	int recv(MyBuffer& buffer) { return ::recv(m_socket, buffer, (int)buffer.size(), 0); }
	int sendto(const MyBuffer& buffer, const MySockAddrIn& to) {
		return ::sendto(m_socket, buffer, (int)buffer.size(), 0, to, (int)to.size());
	}
	int recvfrom(MyBuffer& buffer, MySockAddrIn& from) {
		int len = from.size();
		int ret = ::recvfrom(m_socket, buffer, (int)buffer.size(), 0, from, &len);
		if (ret > 0) from.update();
		return ret;
	}
	void close() {
		if (m_socket != INVALID_SOCKET) {
			closesocket(m_socket);
			m_socket = INVALID_SOCKET;
		}
	}
private:
	SOCKET m_socket;
	MYTYPE m_type;
	int m_protocol;
	MySockAddrIn m_addr;
};

typedef std::shared_ptr<MySocket> MYSOCKET;

