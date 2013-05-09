#ifndef HTTPGET_H
#define HTTPGET_H
#include "Const.h"
#include "MyFile.h"
#include "global.h"
#include "Types.h"
#include <process.h>
#include <string>
class  CHttpGet  
{
public:
	CHttpGet();
	virtual ~CHttpGet();

	//struct CHttpSect *sectInfo;
	//volatile long m_nCount;

	static BOOL SendHttpHeader(SOCKET hSocket,CString strHostAddr,
				CString strHttpAddr,CString strHttpFilename,DWORD& dwFileLength, DWORD nPos, DWORD nEnd = 0);
	static SOCKET ConnectHttpProxy(CString strProxyAddr,int nPort);
	static SOCKET ConnectHttpNonProxy(CString strHostAddr,int nPort);


	static BOOL FileCombine(std::vector<struct threadManagerInfo *>);
	//static BOOL FileVerify(struct threadInfo *pInfo);

	static BOOL HttpDownLoadProxy(struct threadManagerInfo* pThreadInfo);
	static BOOL HttpDownLoadNonProxy(struct threadManagerInfo* pThreadInfo);
	static BOOL HttpDownLoad(DWORD dwFileLength,struct threadManagerInfo* pThreadInfo);

private:
	static DWORD GetHttpHeader(SOCKET sckDest,char *str);
	static DWORD GetFileLength(char *httpHeader);
	static BOOL SocketSend(SOCKET sckDest,const char* szHttp, int uiLen=-1);
};
#endif