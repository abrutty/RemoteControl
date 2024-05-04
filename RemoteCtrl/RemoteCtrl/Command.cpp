#include "pch.h"
#include "Command.h"

CCommand::CCommand() :thread_id(0) {
	struct {
		int nCmd;
		CMDFUNC func;
	} data[] = {
		{1, &CCommand::MakeDriverInfo}, // �鿴���̷���
		{2, &CCommand::MakeDirectoryInfo}, // �鿴ָ��Ŀ¼�µ��ļ�
		{3, &CCommand::RunFile}, // ���ļ�
		{4, &CCommand::DownloadFile}, // �����ļ�
		{5, &CCommand::MouseEvent},
		{6, &CCommand::SendScreen}, // ������Ļ
		{7, &CCommand::LockMachine}, // ����
		{8, &CCommand::UnlockMachine}, // ����
		{9, &CCommand::DeleteLocalFile}, // ɾ���ļ�
		{1981, &CCommand::TestConnect},
		{-1,nullptr},
	};
	for (int i = 0; data[i].nCmd != -1; i++) {
		m_mapFunction.insert(std::pair<int, CMDFUNC>(data[i].nCmd, data[i].func));
	}
}

int CCommand::ExcuteCommand(int nCmd, std::list<CPacket>& lstPacket, CPacket& inPacket) {
	std::map<int, CMDFUNC>::iterator it = m_mapFunction.find(nCmd);
	if (it == m_mapFunction.end()) {
		return -1;
	}
	return (this->*it->second)(lstPacket, inPacket);
}