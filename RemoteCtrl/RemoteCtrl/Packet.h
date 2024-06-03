#pragma once
#include "pch.h"
#include "framework.h"

#pragma pack(push)  // 保存当前字节对齐的状态
#pragma pack(1)	// 强制取消字节对齐，改为连续存放
class CPacket {
public:
	CPacket() :sHead(0), nLength(0), sCmd(0), sSUm(0) {}
	// 打包构造
	CPacket(WORD sCmd, const BYTE* pData, size_t nSize) {
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
	// 解包构造函数
	CPacket(const BYTE* pData, size_t& nSize) {
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
	~CPacket() {}
	int Size() { return nLength + 6; } // 数据包的大小
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
	WORD sHead;				// 包头，固定位，0xFEFF
	DWORD nLength;			// 包长度，从控制命令开始，到和校验结束
	WORD sCmd;				// 控制命令
	std::string strData;	// 包数据
	WORD sSUm;				// 和校验
	std::string strOut;		// 整个包的数据
};
#pragma pack(pop)	// 还原字节对齐

typedef struct MouseEvent {
	MouseEvent() {
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction;	// 点击、移动、双击
	WORD nButton;	// 左键、右键、中键
	POINT ptXY;		// 坐标
}MOUSEEV, * PMOUSEEV;

typedef struct file_info {
	file_info() {
		isInvalid = false;
		isDirectory = -1;
		hasNext = true;
		memset(szFileName, 0, sizeof(szFileName));
	}
	bool isInvalid;         // 是否有效
	int isDirectory;       // 是否为目录
	bool hasNext;           // 是否还有后续
	char szFileName[256];   // 文件名
}FILEINFO, * PFILEINFO;