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

// ������캯��
CPacket::CPacket(WORD sCmd, const BYTE* pData, size_t nSize)
{
	{
		this->sHead = 0xFEFF;
		this->nLength = (DWORD)(nSize + 4);	// ���ݳ���+sCmd����(2)+sSum����(2)
		this->sCmd = sCmd;
		if (nSize > 0) {
			this->strData.resize(nSize);
			memcpy((void*)strData.c_str(), pData, nSize);
		}
		else {
			this->strData.clear();
		}

		this->sSUm = 0;
		for (size_t j = 0; j < strData.size(); j++) { // ���к�У��
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
// ������캯��
CPacket::CPacket(const BYTE* pData, size_t& nSize)
{
	size_t i = 0;	// i ʼ��ָ���Ѷ����������µ�λ��
	for (; i < nSize; i++) {
		if (*(WORD*)(pData + i) == 0xFEFF) {
			sHead = *(WORD*)(pData + i); // �ҵ���ͷ
			i += 2;	// ��ֹֻ�а�ͷFEFF��ռ�����ֽڣ���û������
			break;
		}
	}
	if (i + 4 + 2 + 2 > nSize) {// �����ݿ��ܲ�ȫ�����ͷδȫ�����յ�������ʧ��  +nLength +sCmd +sSum
		nSize = 0;
		return;
	}

	nLength = *(DWORD*)(pData + i);
	i += 4;
	if (nLength + i > nSize) { // ��û����ȫ���յ�������ֻ�յ�һ�룬����ʧ��
		nSize = 0;
		return;
	}

	sCmd = *(WORD*)(pData + i);
	i += 2;
	if (nLength > 4) {
		strData.resize(nLength - 2 - 2); // ��ȥsCmd��sSum�ĳ���
		memcpy((void*)strData.c_str(), pData + i, nLength - 4); //c_str()��string ����ת����c�е��ַ�����ʽ
		i += nLength - 4;
	}

	sSUm = *(WORD*)(pData + i);
	i += 2;
	WORD sum = 0;
	for (size_t j = 0; j < strData.size(); j++) { // ���к�У��
		sum += BYTE(strData[j]) & 0xFF;
	}
	if (sum == sSUm) {
		nSize = i; //  head(2�ֽڣ�nLength(4�ֽ�)data...
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
	bool ret = PostThreadMessage(m_nThreadID, WM_SEND_PACK, (WPARAM)pData, (LPARAM)hWnd); // ֻ�ǰ���Ϣ�ŵ���Ӧ���߳���Ϣ������
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
		MessageBox(NULL, _T("�޷���ʼ���׽���"), _T("��ʼ������"), MB_OK | MB_ICONERROR); // _T������֤���������
		exit(0);
	}
	m_eventInvoke = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, &CClientSocket::threadEntry, this, 0, &m_nThreadID);
	if (WaitForSingleObject(m_eventInvoke, 100) == WAIT_TIMEOUT) {
		TRACE("������Ϣ�����߳�����ʧ��\r\n");
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
			TRACE("����ʧ�ܣ���Ϣֵ��%d ����ֵ��%08X, ��ţ�%d\r\n", funcs[i].message, funcs[i].func, i);
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
		AfxMessageBox("ָ��IP������");
		return false;
	}
	int ret = connect(m_sock, (sockaddr*)&serv_addr, sizeof(serv_addr));
	if (ret == -1) {
		AfxMessageBox("����ʧ��");
		TRACE("����ʧ��,%d,%s\r\n", WSAGetLastError(), GetErrInfo(WSAGetLastError()).c_str());
		return false;
	}
	TRACE("InitSocket�ɹ�\r\n");
	return true;
}

int CClientSocket::DealCommand() 
{
	if (m_sock == -1) return -1;
	char* buffer = m_buffer.data();
	static size_t index = 0;
	while (true) {
		size_t len = recv(m_sock, buffer + index, (int)(BUFFER_SIZE - index), 0);
		if (((int)len <= 0) && ((int)index <= 0)) return -1; // len��size_t��recv����-1ʱ���ɴ���0����������ҪǿתΪint�ж�
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
	while (::GetMessage(&msg, NULL, 0, 0)){ // ֻ����GetMessage�����Ϣ���̲߳�����Ч
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
	delete (PACKET_DATA*)wParam; // ��ֹ�ڴ�й¶
	HWND hWnd = (HWND)lParam;
	size_t nTemp = data.strData.size();
	CPacket current((BYTE*)data.strData.c_str(), nTemp);
	if (InitSocket() == true) {
		int ret = send(m_sock, (char*)data.strData.c_str(), (int)data.strData.size(), 0);
		if (ret > 0) {
			size_t index = 0; // index ��¼����β��λ�ã��൱�ڳ���
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
				else { // �Է��ر��׽��ֻ������쳣
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
