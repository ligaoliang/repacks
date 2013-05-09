#include "stdafx.h"
#include "global.h"
#include <shlwapi.h>
#include "utils.h"

volatile bool g_bStopDownload = false;
volatile bool g_bErrorDuringDownload = false;

volatile unsigned long g_downloaded = 0;
volatile unsigned long g_initialSize = 0;
volatile unsigned long g_threadNumber = 0;
volatile long g_finishedThreadNumber = 0;
volatile long g_totalThreadNumber = 0;

CString g_csProxy = _T("");
//CString g_csProxy = _T("217.66.205.76");
int g_ProxyPort = 8080;
std::vector<struct threadManagerInfo*> g_threadManagers;
std::vector<HANDLE> g_hThreadManagers;

//当前工作路径
CString g_csCurrentDir = NULL;
CString g_csTempDir = NULL;
//TCHAR g_TempPath[MAX_PATH] = {0};
CString g_csResDir = NULL;


CString g_csResFilesUrl = _T("");
CRC32 crc32;
volatile HANDLE g_hMainDownloadThread = NULL;
volatile HANDLE g_hResouceDownloadThread = NULL;
volatile bool g_bDownloadBroken = false;
CString g_csAddonList= _T("[Addons]\rList=");



TCHAR * g_resUrl = NULL;
std::map<LONG_PTR, UiItemInfo> g_UiItemMap;
std::vector<defaultExtensionInfo> g_defaultExtensions;
std::vector<GroupInfo> g_groupsInfo;

//volatile long CHttpGet::m_nCount = 0;
//CHttpSect * CHttpGet::sectInfo = NULL;

volatile bool g_bResDownloaded = false; 

volatile bool g_bInner = false;
CString g_csCmd = _T("");

HWND g_hProcess = NULL;

CLock g_lock;
CMyFile myfile;

//Main package downloading threads handles
//HANDLE g_hThread[5] = {0};

LPSTR CStringToA(const CString& strData) 
{
    CString ret;
    char *ptr = NULL;
#ifdef _UNICODE
    LONG    len;
    len = WideCharToMultiByte(CP_ACP, 0, strData, -1, NULL, 0, NULL, NULL);
    ptr = new char [len+1];
	if (ptr)
	{
		memset(ptr,0,len + 1);
		WideCharToMultiByte(CP_ACP, 0, strData, -1, ptr, len + 1, NULL, NULL);
	}
#else
    ptr = new char [strData.GetAllocLength()+1];
    sprintf(ptr,_T("%s"),strData);
#endif
    return ptr;
}


unsigned __stdcall ThreadDownLoad(void* pParam)
{
	struct threadInfo *pthreadInfo = (struct threadInfo *)pParam;
	if(pthreadInfo->bState)
		return 0;
	//BOOL bError = FALSE;

	// 计算临时文件中已成功下载部分大小.
	DWORD occupy_size = 0;

	DWORD dwFileSize = myfile.GetDownloadedFileSizeByName(pthreadInfo->csDesFileName, &occupy_size);
    DWORD dwSectSize = pthreadInfo->dwEnd - pthreadInfo->dwStart;

	// 此段已下载完毕.
	if( dwFileSize >= dwSectSize )
	{
		//InterlockedIncrement(&g_finishedThreadNumber);
		pthreadInfo->bState = true;
		return 0;
	}

	BYTE * pStore = NULL;
	while (!pStore)
	{
		pStore = (BYTE*)::VirtualAlloc(NULL, PAGE_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE); 
	}

    HANDLE hFile = myfile.GetFilePointer(pthreadInfo->csDesFileName, occupy_size);
	if(hFile == INVALID_HANDLE_VALUE) 
	{
		//SetErrorDuringDownload();
		::VirtualFree(pStore, 0, MEM_RELEASE);
		return 0;
	}

	SOCKET hSocket;
	if(!g_csProxy.IsEmpty())
	{	
		hSocket = CHttpGet::ConnectHttpProxy(g_csProxy,g_ProxyPort);
	}
	else
	{
		hSocket = CHttpGet::ConnectHttpNonProxy(pthreadInfo->csHost,80);
	}

	if(hSocket == INVALID_SOCKET)
	{
		SetErrorDuringDownload();
		CloseHandle(hFile);
		::VirtualFree(pStore, 0, MEM_RELEASE);
		return 0;
	}

    // 设置下载范围.
	DWORD dwFileLength = 0;
	if (!CHttpGet::SendHttpHeader(hSocket,pthreadInfo->csHost,pthreadInfo->csPath,
		pthreadInfo->csFileName,dwFileLength,pthreadInfo->dwStart+dwFileSize))
	{
		g_bErrorDuringDownload = true;
		closesocket(hSocket);
		CloseHandle(hFile);
		::VirtualFree(pStore, 0, MEM_RELEASE);
		return 0;
	}

	int nLen; 
	DWORD nSumLen=0; 
	char szBuffer[1024];
	BYTE* p = pStore;

	while(!g_bStopDownload && !g_bErrorDuringDownload)
	{
		if(nSumLen>=dwSectSize-dwFileSize) break;
		nLen = recv(hSocket,szBuffer,sizeof(szBuffer),0);
		
		if (nLen == SOCKET_ERROR)
		{
			int err_code = WSAGetLastError();
			if (WSAEWOULDBLOCK != err_code)
			{
				SetErrorDuringDownload();
				break;
			}
			else
			{
				Sleep(300);
				continue;
			}
		}
		else if(nLen==0) break;

		nSumLen +=nLen;
		if (nSumLen>=dwSectSize-dwFileSize)
		{
			g_lock.Lock();
			g_downloaded += nLen - (nSumLen - (dwSectSize-dwFileSize));
			g_lock.Unlock();
		}
		else
		{
			g_lock.Lock();
			g_downloaded += nLen;
			g_lock.Unlock();
		}

		if ( (p + nLen) >= (pStore + PAGE_SIZE) )
		{
			memcpy(p, szBuffer, pStore + PAGE_SIZE - p);
			// CRC value
			//unsigned long crc_value = crc32.Calculate(pStore, WRITE_SIZE);
			//memcpy(pStore + WRITE_SIZE, &crc_value, sizeof(unsigned long));
			DWORD dwWritten;
			if (0 == WriteFile(hFile, pStore, PAGE_SIZE, &dwWritten, NULL))
			{
				SetErrorDuringDownload();
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

	if (!g_bStopDownload && !g_bErrorDuringDownload && p > pStore)
	{
		// CRC value
		//unsigned long crc_value = crc32.Calculate(pStore, p - pStore);
		//memcpy(p, &crc_value, sizeof(unsigned long));
		DWORD dwWritten;
		WriteFile(hFile, pStore, p - pStore, &dwWritten, NULL);

	}

	closesocket(hSocket); // 关闭套接字.
	CloseHandle(hFile);
	::VirtualFree(pStore, 0, MEM_RELEASE);
	//InterlockedIncrement(&g_finishedThreadNumber);
	pthreadInfo->bState = true;
	return 0;
}

void SetErrorDuringDownload()
{
	g_bErrorDuringDownload = true;
}


BOOL ParseXMLElements(const TCHAR * config_file_name)
{
	BOOL bRet = TRUE;
	// 
	DWORD dwNum = WideCharToMultiByte(CP_OEMCP,NULL,config_file_name,-1,NULL,0,NULL,FALSE);
	char * file_name = new char[dwNum];
	if (!file_name)
	{
		return FALSE;
	}
	
	WideCharToMultiByte(CP_OEMCP,NULL,config_file_name,-1,file_name,dwNum,NULL,FALSE);

	TiXmlDocument document(file_name);
	if(!document.LoadFile(TIXML_ENCODING_UTF8))
	{
		bRet = FALSE;
		goto cleanup;
	}

	const TiXmlElement* pRootElement = document.RootElement();
	if (NULL == pRootElement)
	{
		bRet = FALSE;
		goto cleanup;
	}

	//get the  resource download url
	GetElementValue(pRootElement, "resUrl", g_resUrl);
	if (NULL == g_resUrl)
	{
		bRet = FALSE;
		goto cleanup;
	}


	const TiXmlElement* pDefaultElement = pRootElement->FirstChildElement("default");
	
	//default extensions
	if (NULL == pDefaultElement)
	{
		bRet = FALSE;
		goto cleanup;
	}
	
	for(const TiXmlElement* pDefaultExtension = pDefaultElement->FirstChildElement();pDefaultExtension;pDefaultExtension = pDefaultExtension->NextSiblingElement())
	{
		defaultExtensionInfo defaultExtension;
		TCHAR * pDefault = NULL;
		if (!GetElementValue(pDefaultExtension, "file", defaultExtension.file))
		{
			bRet = FALSE;
			goto cleanup;
		}
		if (GetElementValue(pDefaultExtension, "unpacked", pDefault))
		{
			if (_tcsicmp(pDefault, _T("true")) == 0 || _tcsicmp(pDefault, _T("1")) == 0)
			{
				defaultExtension.unpacked = true;
			}		
		}
		g_defaultExtensions.push_back(defaultExtension);
	}
	

	const TiXmlNode* node = NULL;
	node = pRootElement->FirstChildElement()->NextSibling();

	// group
	for (node = node->NextSibling(); node; node = node->NextSibling())
	{	
		// groupInfo
		const TiXmlElement* pGroupInfo = node->FirstChildElement("groupInfo");
		if (NULL == pGroupInfo)
		{
			bRet = FALSE;
			goto cleanup;
		}
		
		TCHAR * pDefaultSelected = NULL;
		TCHAR * pDefaultUnpacked = NULL;
		TCHAR * pType =NULL;
		GroupInfo group;

		// group description
		if (!GetElementValue(pGroupInfo, "discription", group.description))
		{
			bRet = FALSE;
			goto cleanup;
		}
		// group picture 
		if (!GetElementValue(pGroupInfo, "picture", group.picture_url))
		{
			bRet = FALSE;
			goto cleanup;
		}
		// intro 1 
		if (!GetElementValue(pGroupInfo, "intro1", group.intro[0]))
		{
			bRet = FALSE;
			goto cleanup;
		}
		// intro 2
		if (!GetElementValue(pGroupInfo, "intro2", group.intro[1]))
		{
			bRet = FALSE;
			goto cleanup;
		}
		// intro 3
		if (!GetElementValue(pGroupInfo, "intro3", group.intro[2]))
		{
			bRet = FALSE;
			goto cleanup;
		}
		// intro 4
		if (!GetElementValue(pGroupInfo, "intro4", group.intro[3]))
		{
			bRet = FALSE;
			goto cleanup;
		}
		if (GetElementValue(pGroupInfo, "selected", pDefaultSelected))
		{
			if (_tcsicmp(pDefaultSelected, _T("true")) == 0 || _tcsicmp(pDefaultSelected, _T("1")) == 0)
			{
				group.checked = true;
			}
			//delete[] pDefaultSelected;
		}

		// extensions
		const TiXmlElement* pExtensions = node->FirstChildElement("extensions");
		if (NULL == pExtensions)
		{
			bRet = FALSE;
			goto cleanup;
		}

		for (const TiXmlElement* extension = pExtensions->FirstChildElement(); extension; extension = extension->NextSiblingElement())
		{	
			ExtensionInfo extInfo;
			
			// extension name
			if (!GetElementValue(extension, "name", extInfo.name))
			{
				bRet = FALSE;
				goto cleanup;
			}
			// extension file
			if (!GetElementValue(extension, "file", extInfo.file))
			{
				bRet = FALSE;
				goto cleanup;
			}
			// extension picture
			if (!GetElementValue(extension, "picture", extInfo.picture_url))
			{
				bRet = FALSE;
				goto cleanup;
			}
			// extension intro1
			if (!GetElementValue(extension, "intro1", extInfo.intro[0]))
			{
				bRet = FALSE;
				goto cleanup;
			}
			// extension intro2
			if (!GetElementValue(extension, "intro2", extInfo.intro[1]))
			{
				bRet = FALSE;
				goto cleanup;
			}
			// extension intro3
			if (!GetElementValue(extension, "intro3", extInfo.intro[2]))
			{
				bRet = FALSE;
				goto cleanup;
			}
			// extension intro4
			if (!GetElementValue(extension, "intro4", extInfo.intro[3]))
			{
				bRet = FALSE;
				goto cleanup;
			}

			if (GetElementValue(extension, "selected", pDefaultSelected))
			{
				if (_tcsicmp(pDefaultSelected, _T("true")) == 0 || _tcsicmp(pDefaultSelected, _T("1")) == 0)
				{
					extInfo.checked = true;
				}
				delete[] pDefaultSelected;
			}
			if (GetElementValue(extension, "unpacked", pDefaultUnpacked))
			{
				if (_tcsicmp(pDefaultUnpacked, _T("true")) == 0 || _tcsicmp(pDefaultUnpacked, _T("1")) == 0)
				{
					extInfo.unpacked = true;
				}
				delete[] pDefaultUnpacked;
			}
			if (!GetElementValue(extension, "url", extInfo.url))
			{
				bRet = FALSE;
				goto cleanup;
			}
			if (!GetElementValue(extension, "cmd", extInfo.cmd))
			{
				bRet = FALSE;
				goto cleanup;
			}
			if (GetElementValue(extension, "type", pType))
			{
				extInfo.type = _wtoi(pType);
			}
			if (!GetElementValue(extension, "id", extInfo.id))
			{
				bRet = FALSE;
				goto cleanup;
			}

			group.extensions.push_back(extInfo);
		}
	
		g_groupsInfo.push_back(group);
	}

cleanup:
	delete[] file_name;
	return bRet;
}
BOOL GetElementValue(const TiXmlElement* node, const char * value, TCHAR *& ret)
{
	BOOL bRet = TRUE;
	const TiXmlElement* pElement = node->FirstChildElement(value);
	if (NULL == pElement)
	{
		bRet = FALSE;
		goto cleanup;
	}

	if (pElement->FirstChild())
	{
		const char * pAttr = pElement->FirstChild()->Value();
		if (pAttr == NULL)
		{
			bRet = FALSE;
			goto cleanup;
		}

		int nMinSize = MultiByteToWideChar(CP_UTF8, 0, pAttr, -1, NULL, 0);
		TCHAR * pWideChar = new TCHAR[nMinSize];
		if (NULL == pWideChar)
		{
			bRet = FALSE;
			goto cleanup;		
		}

		MultiByteToWideChar(CP_UTF8, 0, pAttr, -1, pWideChar, nMinSize);  
		ret = pWideChar;
	}
	else
	{
		ret = NULL;
	}

cleanup:
	return bRet;
}

int GetTotalThreadsNumber()
{
	int nTotalThreadsNumber = 0;
	std::vector<struct threadManagerInfo*>::iterator it;
	for(it = g_threadManagers.begin();it!= g_threadManagers.end();it++)
		nTotalThreadsNumber += (*it)->threadNum;
	return nTotalThreadsNumber;
}

unsigned long GetTotalFileSize()
{
	unsigned long unSize = 0;
	std::vector<struct threadManagerInfo*>::iterator it;
	for(it = g_threadManagers.begin();it!= g_threadManagers.end();it++)
		unSize += (*it)->totalSize;
	return unSize;
}

int GetFinishedThreadsNumber(struct threadManagerInfo* pThreadManagerInfo)
{
	int nNum = 0;
	struct threadInfo* pThreadInfo = pThreadManagerInfo->pThreadInfo;
	if (pThreadInfo != NULL)
	{
		for(int i =0;i< pThreadManagerInfo->threadNum; i++)
		{
			if(pThreadInfo->bState)
				nNum += 1;
		}
	}
	
	return nNum;
}