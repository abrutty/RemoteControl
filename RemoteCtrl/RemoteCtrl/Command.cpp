#include "pch.h"
#include "Command.h"

CCommand::CCommand() :thread_id(0) {
	struct {
		int nCmd;
		CMDFUNC func;
	} data[] = {
		{1, &CCommand::MakeDriverInfo}, // 查看磁盘分区
		{2, &CCommand::MakeDirectoryInfo}, // 查看指定目录下的文件
		{3, &CCommand::RunFile}, // 打开文件
		{4, &CCommand::DownloadFile}, // 下载文件
		{5, &CCommand::MouseEvent},
		{6, &CCommand::SendScreen}, // 发送屏幕
		{7, &CCommand::LockMachine}, // 锁机
		{8, &CCommand::UnlockMachine}, // 解锁
		{9, &CCommand::DeleteLocalFile}, // 删除文件
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