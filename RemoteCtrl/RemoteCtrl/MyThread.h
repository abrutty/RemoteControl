#pragma once
#include "pch.h"
#include <atomic>
#include <vector>
#include <mutex>

class ThreadFuncBase{};
typedef int (ThreadFuncBase::* FUNCTYPE)();

class ThreadWorker {
public:
	ThreadWorker():thiz(NULL),func(NULL){}
	ThreadWorker(void* obj, FUNCTYPE f):thiz((ThreadFuncBase*)obj), func(f){}
	ThreadWorker(const ThreadWorker& worker) {
		thiz = worker.thiz;
		func = worker.func;
	}
	ThreadWorker& operator=(const ThreadWorker& worker) {
		if (this != &worker) {
			thiz = worker.thiz;
			func = worker.func;
		}
		return *this;
	}
	//ThreadWorker& operator=(ThreadWorker&& worker) = delete;
	int operator()() {
		if (IsValid()) {
			return (thiz->*func)();
		}
		return -1;
	}
	bool IsValid() const{
		return (thiz != NULL) && (func != NULL);
	}
private:
	ThreadFuncBase* thiz;
	FUNCTYPE func;
};

class MyThread
{
public:
	MyThread() {
		m_hThread = NULL;
		m_bStatus = false;
	}
	~MyThread() {
		Stop();
	}
	// true 表示成功，false 表示失败
	bool Start() {
		m_bStatus = true;
		m_hThread = (HANDLE)_beginthread(&MyThread::ThreadEntry, 0, this);
		if (!IsValid()) {
			m_bStatus = false;
		}
		return m_bStatus;
	}
	// 返回true 表示有效，返回false 表示线程异常或已终止
	bool IsValid() {
		if (m_hThread == NULL || (m_hThread == INVALID_HANDLE_VALUE)) return false;
		return WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT;
	}
	bool Stop() {
		if (m_bStatus == false) return true;
		m_bStatus = false;
		bool ret = WaitForSingleObject(m_hThread, INFINITE) == WAIT_OBJECT_0;
		/*if (ret == WAIT_TIMEOUT) {
			TerminateThread(m_hThread, -1);
		}*/
		UpdateWorker();
		return ret;
	}
	void UpdateWorker(const ::ThreadWorker& worker = ::ThreadWorker()) {
		if ((m_worker.load()) != NULL && (m_worker.load()!=&worker)) {
			::ThreadWorker* pWorker = m_worker.load();
			m_worker.store(NULL);
			delete pWorker;
		}
		if (m_worker.load() == &worker) return;
		if (!worker.IsValid()) {
			m_worker.store(NULL);
			return;
		}
		m_worker.store(new ::ThreadWorker(worker));
	}
	// true 表示空闲， false 表示已经分配了工作
	bool IsIdle() {
		if (m_worker.load() == NULL) return true;
		return !m_worker.load()->IsValid();
	}
private:
	void ThreadWorker() {
		while (m_bStatus) {
			if (m_worker == NULL) {
				Sleep(1);
				continue;
			}
			::ThreadWorker worker = *m_worker.load();
			if (worker.IsValid()) {
				int ret = worker();
				if (ret != 0) {
					CString str;
					str.Format(_T("thread found warning code %d\r\n"), ret);
					OutputDebugString(str);
				}
				if (ret < 0) {
					::ThreadWorker* pWorker = m_worker.load();
					m_worker.store(NULL);
					delete pWorker;
				}
			}
			else {
				Sleep(1);
			}
		}
	}
	static void ThreadEntry(void* arg) {
		MyThread* thiz = (MyThread*)arg;
		if (thiz) {
			thiz->ThreadWorker();
		}
		_endthread();
	}
private:
	HANDLE m_hThread;
	bool m_bStatus; // false 表示线程将要关闭， true 表示线程正在运行
	std::atomic<::ThreadWorker*> m_worker;
};

class MyThreadPool
{
public:
	MyThreadPool(size_t size) {
		m_threads.resize(size);
		for (size_t i = 0; i < size; i++) {
			m_threads[i] = new MyThread();
		}
	}
	MyThreadPool(){}
	~MyThreadPool() {
		Stop();
		for (size_t i = 0; i < m_threads.size(); i++)
		{
			MyThread* pThread = m_threads[i];
			m_threads[i] = NULL;
			delete pThread;
		}
		m_threads.clear();
	}
	bool Invoke() {
		bool ret = true;
		for (size_t i = 0; i < m_threads.size(); i++) {
			if (m_threads[i]->Start() == false) {
				ret = false;
				break;
			}
		}
		if (ret == false) {
			for (size_t i = 0; i < m_threads.size(); i++) {
				m_threads[i]->Stop();
			}
		}
		return ret;
	}
	void Stop() {
		for (size_t i = 0; i < m_threads.size(); i++) {
			m_threads[i]->Stop();
		}
	}
	// 返回-1 表示分配失败，所有线程都在忙； 大于等于0表示分配第n个线程
	int DispatchWorker(const ThreadWorker& worker) {
		int index = -1;
		m_lock.lock();
		for (int i = 0; i < m_threads.size(); i++) {
			if (m_threads[i] != NULL && m_threads[i]->IsIdle()) {
				m_threads[i]->UpdateWorker(worker);
				index = i;
				break;
			}
		}
		m_lock.unlock();
		return index;
	}
	bool CheckThreadValid(size_t index) {
		if (index < m_threads.size()) {
			return m_threads[index]->IsValid();
		}
		return false;
	}
private:
	std::mutex m_lock;
	std::vector<MyThread*> m_threads;
};