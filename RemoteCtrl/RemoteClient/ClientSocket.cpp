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

// 打包构造函数
CPacket::CPacket(WORD sCmd, const BYTE* pData, size_t nSize)
{
	{
		this->sHead = 0xFEFF;
		this->nLength = (DWORD)(nSize + 4);	// 数据长度+sCmd长度(2)+sSum长度(2)
		this->sCmd = sCmd;
		if (nSize > 0) {
			this->strData.resize(nSize);
			memcpy((void*)strData.c_str(), pData, nSize);
		}
		else {
			this->strData.clear();
		}

		this->sSUm = 0;
		for (size_t j = 0; j < strData.size(); j++) { // 进行和校验
			this->sSUm += BYTE(strData[j]) & 0xFF;
		}
	}
}

CPacket::CPacket(const CPacket& pack)
{
	sHead = pack.sHead;
	nLength = pack.nLength;
	sCmd = pack.sCmd;
	strData = pack.strData;
	sSUm = pack.sSUm;
}

CPacket& CPacket::operator=(const CPacket& pack) {
	if (this != &pack) {
		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strData = pack.strData;
		sSUm = pack.sSUm;
	}
	return *this;
}
// 解包构造函数
CPacket::CPacket(const BYTE* pData, size_t& nSize)
{
	size_t i = 0;	// i 始终指向已读到数据最新的位置
	for (; i < nSize; i++) {
		if (*(WORD*)(pData + i) == 0xFEFF) {
			sHead = *(WORD*)(pData + i); // 找到包头
			i += 2;	// 防止只有包头FEFF，占两个字节，但没有数据
			break;
		}
	}
	if (i + 4 + 2 + 2 > nSize) {// 包数据可能不全，或包头未全部接收到，解析失败  +nLength +sCmd +sSum
		nSize = 0;
		return;
	}

	nLength = *(DWORD*)(pData + i);
	i += 4;
	if (nLength + i > nSize) { // 包没有完全接收到，比如只收到一半，解析失败
		nSize = 0;
		return;
	}

	sCmd = *(WORD*)(pData + i);
	i += 2;
	if (nLength > 4) {
		strData.resize(nLength - 2 - 2); // 减去sCmd和sSum的长度
		memcpy((void*)strData.c_str(), pData + i, nLength - 4); //c_str()把string 对象转换成c中的字符串样式
		i += nLength - 4;
	}

	sSUm = *(WORD*)(pData + i);
	i += 2;
	WORD sum = 0;
	for (size_t j = 0; j < strData.size(); j++) { // 进行和校验
		sum += BYTE(strData[j]) & 0xFF;
	}
	if (sum == sSUm) {
		nSize = i; //  head(2字节）nLength(4字节)data...
		return;
	}
	nSize = 0;
}

const char* CPacket::Data(std::string& strOut) const
{
	strOut.resize(nLength + 6);
	BYTE* pData = (BYTE*)strOut.c_str();
	*(WORD*)pData = sHead;
	pData += 2;
	*(DWORD*)pData = nLength;
	pData += 4;
	*(WORD*)pData = sCmd;
	pData += 2;
	memcpy(pData, strData.c_str(), strData.size());
	pData += strData.size();
	*(WORD*)pData = sSUm;
	return strOut.c_str();
}


bool CClientSocket::SendPacket(HWND hWnd, const CPacket& pack, bool isAutoClosed, WPARAM wParam)
{
	UINT nMode = isAutoClosed ? CSM_AUTOCLOSE : 0;
	std::string strOut;
	pack.Data(strOut);
	PACKET_DATA* pData = new PACKET_DATA(strOut.c_str(), strOut.size(), nMode, wParam);
	bool ret = PostThreadMessage(m_nThreadID, WM_SEND_PACK, (WPARAM)pData, (LPARAM)hWnd); // 只是把消息放到对应的线程消息队列中
	if (ret == false) {
		delete pData;
	}
	return ret;
}

CClientSocket::CClientSocket(const CClientSocket& ss) {
	m_hThread = INVALID_HANDLE_VALUE;
	m_bAutoClosed = ss.m_bAutoClosed;
	m_sock = ss.m_sock;
	m_nIP = ss.m_nIP;
	m_nPort = ss.m_nPort;
	for (auto it = ss.m_mapFunc.begin(); it != ss.m_mapFunc.end(); it++) {
		m_mapFunc.insert(std::pair<UINT, MSGFUNC>(it->first, it->second));
	}
}
CClientSocket::CClientSocket()
	:m_nIP(INADDR_ANY), m_nPort(0), m_sock(INVALID_SOCKET), m_bAutoClosed(true), m_hThread(INVALID_HANDLE_VALUE) 
{
	if (InitSockEnv() == false) {
		MessageBox(NULL, _T("无法初始化套接字"), _T("初始化错误！"), MB_OK | MB_ICONERROR); // _T用来保证编码兼容性
		exit(0);
	}
	m_eventInvoke = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, &CClientSocket::threadEntry, this, 0, &m_nThreadID);
	if (WaitForSingleObject(m_eventInvoke, 100) == WAIT_TIMEOUT) {
		TRACE("网络消息处理线程启动失败\r\n");
	}
	CloseHandle(m_eventInvoke);
	m_buffer.resize(BUFFER_SIZE);
	memset(m_buffer.data(), 0, BUFFER_SIZE);

	struct {
		UINT message;
		MSGFUNC func;
	} funcs[] = {
		{WM_SEND_PACK, &CClientSocket::SendPack},
		{0,NULL}
	};
	for (int i = 0; funcs[i].message != 0; i++) {
		if (m_mapFunc.insert(std::pair<UINT, MSGFUNC>(funcs[i].message, funcs[i].func)).second == false) {
			TRACE("插入失败，消息值：%d 函数值：%08X, 序号：%d\r\n", funcs[i].message, funcs[i].func, i);
		}
	}
}

bool CClientSocket::InitSocket() 
{
	if (m_sock != INVALID_SOCKET) CloseSocket();
	m_sock = socket(PF_INET, SOCK_STREAM, 0);
	if (m_sock == -1) return false;
	sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(m_nIP);
	serv_addr.sin_port = htons(m_nPort);

	if (serv_addr.sin_addr.s_addr == INADDR_NONE) {
		AfxMessageBox("指定IP不存在");
		return false;
	}
	int ret = connect(m_sock, (sockaddr*)&serv_addr, sizeof(serv_addr));
	if (ret == -1) {
		AfxMessageBox("连接失败");
		TRACE("连接失败,%d,%s\r\n", WSAGetLastError(), GetErrInfo(WSAGetLastError()).c_str());
		return false;
	}
	TRACE("InitSocket成功\r\n");
	return true;
}

int CClientSocket::DealCommand() 
{
	if (m_sock == -1) return -1;
	char* buffer = m_buffer.data();
	static size_t index = 0;
	while (true) {
		size_t len = recv(m_sock, buffer + index, (int)(BUFFER_SIZE - index), 0);
		if (((int)len <= 0) && ((int)index <= 0)) return -1; // len是size_t，recv返回-1时会变成大于0的数，所以要强转为int判断
		TRACE("index = %d, len=%d\r\n", index, len);
		index += len;
		len = index;
		m_packet = CPacket((BYTE*)buffer, len);
		TRACE("command = %d\r\n", m_packet.sCmd);
		if (len > 0) {
			TRACE("index = %d, len=%d\r\n", index, len);
			memmove(buffer, buffer + len, index - len);
			index -= len;

			return m_packet.sCmd;
		}
	}
	delete[] buffer;
	return -1;
}

unsigned CClientSocket::threadEntry(void* arg)
{
	CClientSocket* thiz = (CClientSocket*)arg;
	thiz->threadFunc();
	_endthreadex(0);
	return 0;
}

void CClientSocket::threadFunc()
{
	SetEvent(m_eventInvoke);
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0)){ // 只有用GetMessage获得消息后，线程才能起效
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		TRACE("Get message %08X\r\n", msg.message);
		if (m_mapFunc.find(msg.message) != m_mapFunc.end()) {
			(this->*m_mapFunc[msg.message])(msg.message, msg.wParam, msg.lParam);
		}
	}
}

bool CClientSocket::InitSockEnv()
{
	WSADATA data;
	if (WSAStartup(MAKEWORD(1, 1), &data) != 0) {
		return false;
	}
	else {
		m_sock = socket(PF_INET, SOCK_STREAM, 0);
		return true;
	}
}

bool CClientSocket::Send(const CPacket& pack)
{
	TRACE("m_sock = %d\r\n", m_sock);
	if (m_sock == -1) return false;
	std::string strOut;
	pack.Data(strOut);
	return send(m_sock, strOut.c_str(), (int)strOut.size(), 0) > 0;
}

void CClientSocket::SendPack(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	PACKET_DATA data = *(PACKET_DATA*)wParam;
	delete (PACKET_DATA*)wParam; // 防止内存泄露
	HWND hWnd = (HWND)lParam;
	size_t nTemp = data.strData.size();
	CPacket current((BYTE*)data.strData.c_str(), nTemp);
	if (InitSocket() == true) {
		int ret = send(m_sock, (char*)data.strData.c_str(), (int)data.strData.size(), 0);
		if (ret > 0) {
			size_t index = 0; // index 记录数据尾部位置，相当于长度
			std::string strBuffer;
			strBuffer.resize(BUFFER_SIZE);
			char* pBuffer = (char*)strBuffer.c_str();
			while (m_sock != INVALID_SOCKET) {
				int length = recv(m_sock, pBuffer + index, (int)(BUFFER_SIZE - index), 0);
				if ((length>0) || (index>0)) {
					index += (size_t)length;
					size_t nLen = index;
					CPacket pack((BYTE*)pBuffer, nLen);
					if (nLen > 0) {
						::SendMessage(hWnd, WM_SEND_PACK_ACK, (WPARAM)new CPacket(pack), data.wParam);
						if (data.nMode & CSM_AUTOCLOSE) {
							CloseSocket();
							return;
						}
						index -= nLen;
						memmove(pBuffer, pBuffer + nLen, index);
					}	
				}
				else { // 对方关闭套接字或网络异常
					CloseSocket();
					::SendMessage(hWnd, WM_SEND_PACK_ACK, (WPARAM)new CPacket(current.sCmd, NULL, 0), 1);
				}
			}	
		}
		else {
			CloseSocket();
			::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, -1); 
		}
	}
	else {
		::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, -2);
	}
	
}
