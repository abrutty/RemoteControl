#pragma once
#include "pch.h"
#include <list>
#include <atomic>
#include "MyThread.h"

// 使用模板类最好不用cpp文件，都写在头文件里
template<class T>
class CQueue // 线程安全队列，用IOCP实现
{
public:
	enum {
		EQNone,
		EQPush,
		EQPop,
		EQSize,
		EQClear
	};
	typedef struct IocpParam {
		size_t nOperator; // 操作
		T Data; // 数据
		HANDLE hEvent; // pop 操作需要的
		IocpParam(int op, const T& data, HANDLE hEve=NULL) {
			nOperator = op;
			Data = data;
			hEvent = hEve;
		}
		IocpParam() {
			nOperator = EQNone;
		}
	}PPARAM; // Post Parameter 用于投递信息的结构体
	
public:
	CQueue() {
		m_lock = false;
		m_hCompeletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);
		m_hThread = INVALID_HANDLE_VALUE;
		if (m_hCompeletionPort != NULL) {
			m_hThread = (HANDLE)_beginthread(&CQueue<T>::threadEntry, 0, this);
		}
	}
	virtual ~CQueue() {
		if (m_lock == true) return;
		m_lock = true;
		PostQueuedCompletionStatus(m_hCompeletionPort, 0, NULL, NULL);
		WaitForSingleObject(m_hThread, INFINITE);
		if (m_hCompeletionPort != NULL) {
			HANDLE hTemp = m_hCompeletionPort;
			m_hCompeletionPort = NULL;
			CloseHandle(hTemp);
		}
	}
	bool PushBack(const T& data) {
		IocpParam* pParam = new IocpParam(EQPush, data);
		// 尽量减少m_lock==true和PostQueuedCompletionStatus代码之间的间隙
		if (m_lock == true) {// 等于true不能再插入了
			delete pParam;
			return false;
		}
		bool ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)pParam, NULL);
		if (ret == false) delete pParam;
		return ret;
	}
	virtual bool PopFront(T& data) {
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		IocpParam Param(EQPop, data, hEvent);
		if (m_lock == true) {
			if (hEvent) CloseHandle(hEvent);
			return false;
		}
		bool ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)&Param, NULL);
		if (ret == false) {
			CloseHandle(hEvent);
			return false;
		}
		ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;
		if (ret == true) data = Param.Data;
		return ret;
	}
	size_t Size() {
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		IocpParam Param(EQSize, T(), hEvent);
		if (m_lock == true) {
			if (hEvent) CloseHandle(hEvent);
			return -1;
		}
		bool ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)&Param, NULL);
		if (ret == false) {
			CloseHandle(hEvent);
			return -1;
		}
		ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;
		if (ret == true) return Param.nOperator;
		return -1;
	}
	bool Clear() {
		if (m_lock == true) return false; 
		IocpParam* pParam = new IocpParam(EQClear, T());
		bool ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)pParam, NULL);
		if (ret == false) delete pParam;
		return ret;
	}

protected:
	static void threadEntry(void* arg) {
		CQueue<T>* thiz = (CQueue<T>*)arg;
		thiz->threadMain();
		_endthread();
	}
	virtual void DealParam(PPARAM* pParam) {
		switch (pParam->nOperator) {
		case EQPush: 
			m_lstData.push_back(pParam->Data);
			delete pParam;
			break;
		case EQPop:
			if (m_lstData.size() > 0) {
				pParam->Data = m_lstData.front();
				m_lstData.pop_front();
			}
			if (pParam->hEvent != NULL) {
				SetEvent(pParam->hEvent);
			}
			break;
		case EQSize: 
			pParam->nOperator = m_lstData.size();
			if (pParam->hEvent != NULL) {
				SetEvent(pParam->hEvent);
			}
			break;
		case EQClear:
			m_lstData.clear();
			delete pParam;
			break;
		default:
			OutputDebugStringA("unknown operator\r\n");
			break;
		}
	}
	void threadMain() {
		DWORD dwTransferred = 0;
		ULONG_PTR CompletionKey = 0;
		OVERLAPPED* pOverlapped = NULL;
		PPARAM* pParam = NULL;
		while (GetQueuedCompletionStatus(m_hCompeletionPort, &dwTransferred, &CompletionKey, &pOverlapped, INFINITE)) {
			if (dwTransferred == 0 || CompletionKey == NULL) {
				printf("thread is prepared to exit\r\n");
				break;
			}
			pParam = (PPARAM*)CompletionKey;
			DealParam(pParam);
		}
		// Post 后可能还有残余数据
		while(GetQueuedCompletionStatus(m_hCompeletionPort, &dwTransferred, &CompletionKey, &pOverlapped, 0)) {
			if (dwTransferred == 0 || CompletionKey == NULL) {
				continue;
			}
			pParam = (PPARAM*)CompletionKey;
			DealParam(pParam);
		}
		HANDLE hTemp = m_hCompeletionPort;
		m_hCompeletionPort = NULL;
		CloseHandle(hTemp);
	}
	
protected:
	std::list<T> m_lstData;
	HANDLE m_hCompeletionPort;
	HANDLE m_hThread;
	std::atomic<bool> m_lock; // 队列正在析构
};

// windows 里可以访问的内存地址，一般都会占多位，相当于是高地址


template <class T>
class SendQueue :public CQueue<T>, public ThreadFuncBase
{
public:
	typedef int (ThreadFuncBase::* MYCALLBACK)(T& data);
	SendQueue(ThreadFuncBase* obj, MYCALLBACK callback)
		:CQueue<T>(), m_base(obj), m_callback(callback)
	{
		m_thread.Start();
		m_thread.UpdateWorker(::ThreadWorker(this, (FUNCTYPE)&SendQueue<T>::threadTick));
	}
	virtual ~SendQueue() {
		//m_thread.Stop();
		m_base = NULL;
		m_callback = NULL;
	}
protected:
	virtual bool PopFront(T& data) { return false; };
	bool PopFront()
	{
		typename CQueue<T>::IocpParam* Param = new typename CQueue<T>::IocpParam(CQueue<T>::EQPop, T());
		if (CQueue<T>::m_lock == true) {
			delete Param;
			return false;
		}
		bool ret = PostQueuedCompletionStatus(CQueue<T>::m_hCompeletionPort, sizeof(typename CQueue<T>::PPARAM), (ULONG_PTR)&Param, NULL);
		if (ret == false) {
			delete Param;
			return false;
		}
		return ret;
	}
	int threadTick() {
		if (CQueue<T>::m_lstData.size() > 0) {
			PopFront();
		}
		Sleep(1);
		return 0;
	}
	virtual void DealParam(typename CQueue<T>::PPARAM* pParam) {
		switch (pParam->nOperator)
		{
		case CQueue<T>::EQPush:
			CQueue<T>::m_lstData.push_back(pParam->Data);
			delete pParam;
			break;
		case CQueue<T>::EQPop:
			if (CQueue<T>::m_lstData.size() > 0) {
				pParam->Data = CQueue<T>::m_lstData.front();
				if ((m_base->*m_callback)(pParam->Data) == 0)
					CQueue<T>::m_lstData.pop_front();
			}
			delete pParam;
			break;
		case CQueue<T>::EQSize:
			pParam->nOperator = CQueue<T>::m_lstData.size();
			if (pParam->hEvent != NULL) {
				SetEvent(pParam->hEvent);
			}
			break;
		case CQueue<T>::EQClear:
			CQueue<T>::m_lstData.clear();
			delete pParam;
			break;
		default:
			OutputDebugStringA("unknown operator\r\n");
			break;
		}
	}
private:
	ThreadFuncBase* m_base;
	MYCALLBACK m_callback;
	MyThread m_thread;
};

typedef SendQueue<std::vector<char>>::MYCALLBACK SENDCALLBACK;

