#include "stdafx.h"
#include <shlwapi.h>
#include "functions.h"
#include "Types.h"
#include "HttpGet.h"
#include "utils.h"

unsigned __stdcall DownloadResFilesThread(void* pParam)
{
	CString csURL = (TCHAR*)pParam;
	while ((!g_bStopDownload) && (!g_bErrorDuringDownload ) && (!InternetDownloadFile(csURL,g_csTempDir)));
	CString str = g_csCurrentDir + _T("7za.exe x ") + g_csTempDir + _T("res.7z -o")  + g_csResDir;
	HANDLE handle = RunCmd(str,FALSE);
	WaitForSingleObject(handle,INFINITE);
	g_bResDownloaded = true;
	ParseXMLElements(g_csResDir + _T("extensions.xml"));
	return 0;
}

BOOL InternetDownloadFile(CString& csURL, CString& csOutPath)
{
	CMyFile file;
	SOCKET hSocket;

	CString csHostAddr,csHttpAddr,csHttpFilename;
	parseURL(csURL, csHostAddr, csHttpAddr, csHttpFilename);

	BOOL bRet = FALSE;
	CString csDesFileName = csOutPath + csHttpFilename;

	//如果文件存在并且不能删除，则跳过下载
	if (PathFileExists(csDesFileName))
	{
		if (!DeleteFile(csDesFileName))
			return TRUE;
	}

	HANDLE hFile = file.GetFilePointer(csDesFileName, 0);
	if(hFile == INVALID_HANDLE_VALUE) 
	{
		return FALSE;
	}

	if(g_csProxy.IsEmpty())
		hSocket = CHttpGet::ConnectHttpNonProxy(csHostAddr, 80);
	else
		hSocket = CHttpGet::ConnectHttpProxy(g_csProxy,g_ProxyPort);
	if(hSocket == INVALID_SOCKET)
	{
		CloseHandle(hFile);
		g_bErrorDuringDownload = true;
		return FALSE;
	}

	BYTE * pStore = new BYTE[PAGE_SIZE];
	if (!pStore)
	{
		closesocket(hSocket);
		CloseHandle(hFile);
		g_bErrorDuringDownload = true;
		return FALSE;
	}

	// 设置下载范围.
	DWORD dwFileLength = 0;
	if (!CHttpGet::SendHttpHeader(hSocket, csHostAddr, csHttpAddr, csHttpFilename, dwFileLength, 0))
	{
		g_bErrorDuringDownload = true;
		delete []pStore;
		closesocket(hSocket);
		CloseHandle(hFile);
		return FALSE;
	}

	int nLen; 
	char szBuffer[1024];
	BYTE * p = pStore;

	while(!g_bStopDownload && !g_bErrorDuringDownload)
	{
		nLen = recv(hSocket, szBuffer, sizeof(szBuffer), 0);	
		if (nLen == SOCKET_ERROR)
		{
			if (WSAEWOULDBLOCK != WSAGetLastError())
			{
				g_bErrorDuringDownload = true;
				bRet = FALSE;
				break;
			}
			else
			{
				Sleep(100);
				continue;
			}
		}
		else if(nLen == 0)
		{
			bRet = TRUE;
			break;
		}

		if ( (p + nLen) > (pStore + PAGE_SIZE) )
		{
			memcpy(p, szBuffer, pStore + PAGE_SIZE - p);

			DWORD dwWritten;
			if (0 == WriteFile(hFile, pStore, PAGE_SIZE, &dwWritten, NULL))
			{
				g_bErrorDuringDownload = true;
				break;
			}

			memcpy(pStore, szBuffer + (pStore + PAGE_SIZE - p), (p + nLen) - (pStore + PAGE_SIZE));
			p = (p + nLen) - PAGE_SIZE; 
		}
		else
		{
			memcpy(p, szBuffer, nLen);
			p += nLen;
		}
	}
	if (!g_bErrorDuringDownload && !g_bStopDownload)
	{
		if (p > pStore)
		{
			DWORD dwWritten;
			WriteFile(hFile, pStore, p - pStore, &dwWritten, NULL);
		}
	}	

	delete[] pStore;
	CloseHandle(hFile);
	closesocket(hSocket); // 关闭套接字.

	return bRet;
}

void download(int threadNum, CString url, CString filename)
{
	unsigned int threadID = 0;
	struct threadManagerInfo* pThreadManager = new threadManagerInfo;
	pThreadManager->threadNum = threadNum;
	pThreadManager->url = url;
	pThreadManager->filename = filename;
	g_threadManagers.push_back(pThreadManager);
	HANDLE handle = (HANDLE)_beginthreadex(NULL, 0, &MultiThreadsDownLoadManager, (void*)(pThreadManager), 0, &threadID);
	g_hThreadManagers.push_back(handle);
}

UINT __stdcall MultiThreadsDownLoadManager(void* pParam)
{
	struct threadManagerInfo* pThreadManager = (struct threadManagerInfo*)pParam;
	pThreadManager->pThreadsHandls = new HANDLE[pThreadManager->threadNum];
	if (pThreadManager->pThreadsHandls != NULL)
		ZeroMemory(pThreadManager->pThreadsHandls,sizeof(HANDLE) * pThreadManager->threadNum);
	
	parseURL(pThreadManager->url,pThreadManager->host,pThreadManager->path,pThreadManager->filename);
	Downloading(g_csProxy,g_ProxyPort,pThreadManager);

	while (!g_bStopDownload && !g_bErrorDuringDownload)
	{
		DWORD dw = WaitForMultipleObjects(pThreadManager->threadNum, pThreadManager->pThreadsHandls, TRUE, 0);
		if (dw >= WAIT_OBJECT_0 && dw < WAIT_OBJECT_0 + pThreadManager->threadNum)
		{
			//有某个文件块未下载完，返回错误，应由最上层调用者重新下载。
			if(pThreadManager->threadNum  != GetFinishedThreadsNumber(pThreadManager))
			{
				if (!g_bStopDownload && !g_bErrorDuringDownload) 
				{
					Downloading(g_csProxy,g_ProxyPort,pThreadManager);
					continue;
				}
				else
				{
					g_bErrorDuringDownload = TRUE;
				}
			}
			else
			{
				/*
				BOOL bSucessed = TRUE;
				for(int i=0;i<pThreadManager->threadNum;i++)
				{
					//if (!CHttpGet::FileVerify(&(pThreadManager->pThreadInfo[i])))
					{
						if (!g_bStopDownload && !g_bErrorDuringDownload)
						{
							pThreadManager->pThreadInfo[i].bState = false;
							Downloading(g_csProxy,g_ProxyPort,pThreadManager);
							bSucessed = FALSE;
						}
					}
				}
				if(!bSucessed)
					continue;
				*/
			}

			break;
		}
		else if ( (dw >= WAIT_ABANDONED_0 && dw < WAIT_ABANDONED_0 + pThreadManager->threadNum) || dw == WAIT_FAILED)
		{
			break;
		}
	}

	if (g_bStopDownload || g_bErrorDuringDownload)
	{
		if(pThreadManager->bInitiated)
			WaitForMultipleObjects(pThreadManager->threadNum, pThreadManager->pThreadsHandls, TRUE, INFINITE);
	}
/*
	if (pThreadManager != NULL)
	{
		if(pThreadManager->pThreadInfo != NULL)
		{
			delete []pThreadManager->pThreadInfo;
			pThreadManager->pThreadInfo = NULL;
		}
		delete pThreadManager;
		pThreadManager = NULL;
	}
	*/
	return true;
}

bool Downloading(CString strProxy,int nProxyPort,struct threadManagerInfo* pInfo)
{

	if(!strProxy.IsEmpty()){
		if(!CHttpGet::HttpDownLoadProxy(pInfo))
			return false;
	}
	else{
		if(!CHttpGet::HttpDownLoadNonProxy(pInfo))
			return false;
	}
	
	return true;
}

void downloadThirdPartyFiles()
{
	std::vector<GroupInfo>::iterator it;
	it = g_groupsInfo.begin();
	while (it != g_groupsInfo.end())
	{
		std::vector<ExtensionInfo>::iterator it1 = (*it).extensions.begin();
		while (it1 != (*it).extensions.end())
		{
			if ((*it1).checked)
			{
				if((*it1).type == 1)
				{
					if((*it1).url != NULL)
						download(1,(*it1).url,(*it1).file);
				}
			}
			++it1;
		}

		++it;
	}	
}
