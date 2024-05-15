#pragma once
#include "Windows.h"
#include <string>
#include <atlimage.h>

class CTool
{
public:
	static void Dump(BYTE* pData, size_t nSize) {
		std::string strOut;
		for (size_t i = 0; i < nSize; i++) {
			char buf[8] = "";
			if (i > 0 && (i % 16 == 0)) strOut += "\n";
			snprintf(buf, sizeof(buf), "%02X ", pData[i] & 0xFF); // 02X输出域宽为2，右对齐，不足的用字符0替代
			strOut += buf;
		}
		strOut += "\n";
		OutputDebugStringA(strOut.c_str());
	}
	static int Bytes2Image(CImage& image, const std::string& strBuffer) {
		BYTE* pData = (BYTE*)strBuffer.c_str();
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);
		if (hMem == nullptr) {
			TRACE("内存不足\r\n");
			Sleep(1);
			return -1;
		}
		IStream* pStream = nullptr;
		HRESULT hRet = CreateStreamOnHGlobal(hMem, true, &pStream);
		if (hRet == S_OK) {
			ULONG length = 0;
			pStream->Write(pData, strBuffer.size(), &length);
			LARGE_INTEGER bg = { 0 };
			pStream->Seek(bg, STREAM_SEEK_SET, nullptr);
			if ((HBITMAP)image != nullptr) image.Destroy();
			image.Load(pStream);
		}
		return hRet;
	}
};