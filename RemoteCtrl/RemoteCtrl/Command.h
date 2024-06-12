#pragma once
#include <map>
#include "atlimage.h"
#include "ServerSocket.h"
#include "LockDialog.h"
#include "resource.h"
#include <direct.h>
#include <stdio.h>
#include <io.h>
#include <list>
#include "Tool.h"
#include "Packet.h"
class CCommand
{
public:
	CCommand();
	~CCommand() {}
	int ExcuteCommand(int nCmd, std::list<CPacket>& lstPacket, CPacket& inPacket);
	static void RunCommand(void* arg, int status, std::list<CPacket>& lstPacket, CPacket& inPacket) {
		CCommand* thiz = (CCommand*)arg;
		if (status > 0) {
			int ret = thiz->ExcuteCommand(status, lstPacket, inPacket);
			if (ret != 0) {
				TRACE("ִ������ʧ�ܣ�%d, ret=%d\r\n", status, ret);
			}
		}
		else {
			MessageBox(nullptr, _T("�޷������û����Զ�����"), _T("�����û�ʧ��"), MB_OK | MB_ICONERROR);
		}
	}
protected:
	typedef int(CCommand::* CMDFUNC)(std::list<CPacket>& lstPacket, CPacket& inPacket); // ��Ա����ָ��
	std::map<int, CMDFUNC> m_mapFunction; // ������ŵ����ܵ�ӳ��
	CLockDialog dlg;
	unsigned thread_id;
protected:
	// �鿴���ش��̷�����������Ϣ��������ݰ�
	int MakeDriverInfo(std::list<CPacket>& lstPacket, CPacket& inPacket);
	// ��ȡĿ¼��Ϣ�����
	int MakeDirectoryInfo(std::list<CPacket>& lstPacket, CPacket& inPacket);
	// ���ļ�
	int RunFile(std::list<CPacket>& lstPacket, CPacket& inPacket);
	// �����ļ�
	int DownloadFile(std::list<CPacket>& lstPacket, CPacket& inPacket);
	// ������
	int MouseEvent(std::list<CPacket>& lstPacket, CPacket& inPacket);
	// ������Ļ��ͼ
	int SendScreen(std::list<CPacket>& lstPacket, CPacket& inPacket);


	// ���������̣߳��������߳��о�һֱ������Ϣѭ�����ղ�����������Ϣ
	static unsigned __stdcall ThreadLockDlg(void* arg) {
		CCommand* thiz = (CCommand*)arg;
		thiz->ThreadLockDlgMain();
		_endthreadex(0);
		return 0;
	}
	// ����ʵ�������ĺ���
	void ThreadLockDlgMain();
	// ����������
	int LockMachine(std::list<CPacket>& lstPacket, CPacket& inPacket);
	// ����������
	int UnlockMachine(std::list<CPacket>& lstPacket, CPacket& inPacket) {
		PostThreadMessage(thread_id, WM_KEYDOWN, 0x1B, 0); // ��Ϣ���Ƹ����̣߳������Ǿ��
		lstPacket.emplace_back(CPacket(8, nullptr, 0));
		TRACE("%s(%d):%d\r\n", __FUNCTION__, __LINE__, thread_id);
		return 0;
	}
	// ���Ӳ���
	int TestConnect(std::list<CPacket>& lstPacket, CPacket& inPacket) {
		lstPacket.emplace_back(CPacket(1981, nullptr, 0));
		return 0;
	}
	// ɾ���ļ�
	int DeleteLocalFile(std::list<CPacket>& lstPacket, CPacket& inPacket);
};

