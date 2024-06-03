#pragma once
#include "pch.h"
#include "framework.h"

#pragma pack(push)  // ���浱ǰ�ֽڶ����״̬
#pragma pack(1)	// ǿ��ȡ���ֽڶ��룬��Ϊ�������
class CPacket {
public:
	CPacket() :sHead(0), nLength(0), sCmd(0), sSUm(0) {}
	// �������
	CPacket(WORD sCmd, const BYTE* pData, size_t nSize) {
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
	CPacket(const CPacket& pack) {
		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strData = pack.strData;
		sSUm = pack.sSUm;
	}
	CPacket& operator=(const CPacket& pack) {
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
	CPacket(const BYTE* pData, size_t& nSize) {
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
	~CPacket() {}
	int Size() { return nLength + 6; } // ���ݰ��Ĵ�С
	const char* Data() {
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

public:
	WORD sHead;				// ��ͷ���̶�λ��0xFEFF
	DWORD nLength;			// �����ȣ��ӿ������ʼ������У�����
	WORD sCmd;				// ��������
	std::string strData;	// ������
	WORD sSUm;				// ��У��
	std::string strOut;		// ������������
};
#pragma pack(pop)	// ��ԭ�ֽڶ���

typedef struct MouseEvent {
	MouseEvent() {
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction;	// ������ƶ���˫��
	WORD nButton;	// ������Ҽ����м�
	POINT ptXY;		// ����
}MOUSEEV, * PMOUSEEV;

typedef struct file_info {
	file_info() {
		isInvalid = false;
		isDirectory = -1;
		hasNext = true;
		memset(szFileName, 0, sizeof(szFileName));
	}
	bool isInvalid;         // �Ƿ���Ч
	int isDirectory;       // �Ƿ�ΪĿ¼
	bool hasNext;           // �Ƿ��к���
	char szFileName[256];   // �ļ���
}FILEINFO, * PFILEINFO;