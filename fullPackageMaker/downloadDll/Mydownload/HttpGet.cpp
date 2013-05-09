#include "stdafx.h"
#include "HttpGet.h"

//---------------------------------------------------------------------------
CHttpGet::CHttpGet()
{
}

//---------------------------------------------------------------------------
CHttpGet::~CHttpGet()
{
}

//---------------------------------------------------------------------------

BOOL CHttpGet::HttpDownLoadProxy(struct threadManagerInfo* pThreadInfo)
{
	// �����ļ�ͷ�������ļ���С.
	DWORD dwFileLength = 0;
	if( !pThreadInfo->bInitiated )
	{
		SOCKET hSocket;
		hSocket = ConnectHttpProxy(g_csProxy,g_ProxyPort);
		if(hSocket == INVALID_SOCKET) 
		{
			g_bErrorDuringDownload = true;
			return FALSE;
		}
		if( !SendHttpHeader(hSocket,pThreadInfo->host,pThreadInfo->path,pThreadInfo->filename,dwFileLength,0))
		{
			g_bErrorDuringDownload = true;
			closesocket(hSocket);
			return FALSE;
		}
 		closesocket(hSocket);
	}
   return HttpDownLoad(dwFileLength,pThreadInfo);
}

//---------------------------------------------------------------------------
BOOL CHttpGet::HttpDownLoadNonProxy(struct threadManagerInfo* pThreadInfo)
{
	// �����ļ�ͷ�������ļ���С.
	DWORD dwFileLength = 0;
	if(!pThreadInfo->bInitiated)
	{
		int nHostPort = 80;
		SOCKET hSocket;
		hSocket=ConnectHttpNonProxy(pThreadInfo->host,nHostPort);
		if(hSocket == INVALID_SOCKET) 
		{
			g_bErrorDuringDownload = true;
			return FALSE;
		}
		if( !SendHttpHeader(hSocket,pThreadInfo->host,pThreadInfo->path,pThreadInfo->filename,dwFileLength,0))
		{
			g_bErrorDuringDownload = false;
			closesocket(hSocket);
			return FALSE;
		}
 		closesocket(hSocket);
	}
	return HttpDownLoad(dwFileLength,pThreadInfo);
}

//---------------------------------------------------------------------------
BOOL CHttpGet::HttpDownLoad(DWORD dwFileLength,struct threadManagerInfo* pThreadManagerInfo)
{
	if(!pThreadManagerInfo->bInitiated)
	{
		DWORD nSize= dwFileLength/pThreadManagerInfo->threadNum;           // ����ָ�εĴ�С.
		if (pThreadManagerInfo->pThreadInfo == NULL)
			pThreadManagerInfo->pThreadInfo = new struct threadInfo[pThreadManagerInfo->threadNum];
		
		for(int i = 0; i < pThreadManagerInfo->threadNum; i++)
		{
			pThreadManagerInfo->pThreadInfo[i].csHost = pThreadManagerInfo->host;
			pThreadManagerInfo->pThreadInfo[i].csPath = pThreadManagerInfo->path;
			pThreadManagerInfo->pThreadInfo[i].csFileName = pThreadManagerInfo->filename;
			// ������ʱ�ļ���.
			CString strTempFileName;
			//if(pThreadManagerInfo->threadNum >1)
			{
				strTempFileName.Format(_T("%s_%d"),pThreadManagerInfo->filename, i);
				pThreadManagerInfo->pThreadInfo[i].csDesFileName = g_csTempDir + strTempFileName; // ���غ���ļ���.
			}
			//else
			//{
				//pThreadManagerInfo->pThreadInfo[i].csDesFileName = g_csTempDir + pThreadManagerInfo->filename;
			//}

			//DWORD nFileSize= myfile.GetDownloadedFileSizeByName(strTempFileName);
			//pThreadInfo->initialSize += nFileSize;

			//pThreadManagerInfo->pThreadInfo[i].index = i;
			if(i<pThreadManagerInfo->threadNum-1)
			{
				pThreadManagerInfo->pThreadInfo[i].dwStart = i * nSize;            // �ָ����ʼλ��.
				pThreadManagerInfo->pThreadInfo[i].dwEnd = (i+1) * nSize;          // �ָ����ֹλ��.
			}
			else
			{
				pThreadManagerInfo->pThreadInfo[i].dwStart = i * nSize;            // �ָ����ʼλ��.
				pThreadManagerInfo->pThreadInfo[i].dwEnd = dwFileLength;        // �ָ����ֹλ��.
			}
		}
		pThreadManagerInfo->totalSize = dwFileLength;
		//pThreadManagerInfo->bInitiated = true;
	}
	ZeroMemory(pThreadManagerInfo->pThreadsHandls, sizeof(HANDLE*) * pThreadManagerInfo->threadNum);
	for(int i=0; i < pThreadManagerInfo->threadNum; ++i)
	{
		unsigned threadID;
		while (pThreadManagerInfo->pThreadsHandls[i] == 0)
			pThreadManagerInfo->pThreadsHandls[i] = (HANDLE)_beginthreadex(NULL, 0, &ThreadDownLoad, (void*)(&pThreadManagerInfo->pThreadInfo[i]), 0, &threadID);
	}
	pThreadManagerInfo->bInitiated = true;
    return TRUE;
}


//---------------------------------------------------------------------------
BOOL CHttpGet::FileCombine(std::vector<struct threadManagerInfo *> threadManagers)
{	
	std::vector<struct threadManagerInfo*>::iterator it;
	for(it = g_threadManagers.begin();it!= g_threadManagers.end();it++)
	{
		//if((*it)->threadNum ==1)
			//continue;

		FILE *fpwrite;
		FILE *fpread;
		if((fpwrite = _tfopen(g_csTempDir+(*it)->filename, _T("wb")))==NULL)
			return FALSE;

		struct threadInfo* pThreadInfo = (*it)->pThreadInfo;

		for (int i =0; i<(*it)->threadNum; i++)
		{
			
			DWORD nPos = pThreadInfo[i].dwStart;
			if((fpread = _tfopen(pThreadInfo[i].csDesFileName, _T("rb")))==NULL)
				return FALSE;
			// �����ļ�дָ����ʼλ��.
			fseek(fpwrite,nPos,SEEK_SET);
	
			BYTE buffer[PAGE_SIZE];
			size_t sz;

			while((sz = fread(buffer, 1, PAGE_SIZE, fpread)) > 0)
			{
				/*
				unsigned long crc_value;
				memcpy(&crc_value, buffer + (sz - sizeof(unsigned long)), sizeof(unsigned long));
				unsigned long verify = crc32.Calculate(buffer, sz - sizeof(unsigned long));
				if (crc_value != verify)
				{
					fclose(fpread);
					return FALSE;
				}

				fwrite(buffer, 1, sz - sizeof(unsigned long), fpwrite);
				nPos += sz - sizeof(unsigned long);
				*/
				fwrite(buffer, 1, sz, fpwrite);
				nPos += sz ;

				if(nPos >= pThreadInfo[i].dwEnd)
					break;
			}
			fclose(fpread);
		}
		fclose(fpwrite);
	}
	return TRUE;
}
/*
BOOL CHttpGet::FileVerify(struct threadInfo *pInfo)
{	

	FILE *fpread;
	
	// ���ļ�.
	if((fpread = _tfopen(pInfo->csDesFileName, _T("rb")))==NULL)
		return FALSE;

	DWORD nPos=pInfo->dwStart;
		
	BYTE buffer[PAGE_SIZE];
	size_t sz;
	// ���ļ�����д�뵽���ļ�.		
	while((sz = fread(buffer, 1, PAGE_SIZE, fpread)) > 0)
	{
		unsigned long crc_value;
		memcpy(&crc_value, buffer + (sz - sizeof(unsigned long)), sizeof(unsigned long));
		unsigned long verify = crc32.Calculate(buffer, sz - sizeof(unsigned long));
		if (crc_value != verify)
		{
			fclose(fpread);
			return FALSE;
		}

		nPos += sz - sizeof(unsigned long);
		if(nPos >= pInfo->dwEnd) break;
	}
	
	fclose(fpread);

	return TRUE;
}
*/
BOOL CHttpGet::SendHttpHeader(SOCKET hSocket,CString strHostAddr,
				CString strHttpAddr,CString strHttpFilename,DWORD& dwFileLength,DWORD nPos, DWORD nEnd)
{
//	USES_CONVERSION;

	// ��������. 
	char sTemp[1024] = {0};
	char cTmpBuffer[1024] = {0};

	std::string strSend;

	int index = strHttpAddr.Find(_T("http://dl_dir.qq.com"));
	if(index != -1)
	{	
		strHttpAddr.Delete(index,20);
		//strHttpAddr += "QzoneMusicInstall.exe";
	}
	// Line1: �����·��,�汾.
	char * pHttpAddr = CStringToA(strHttpAddr);
	char * pHttpFileName = CStringToA(strHttpFilename);
	char * pHostAddr = CStringToA(strHostAddr);
	if (pHttpAddr == NULL || pHttpFileName == NULL || pHostAddr == NULL)
	{
		delete[] pHttpAddr;
		delete[] pHttpFileName;
		delete[] pHostAddr;
		return FALSE;
	}

	sprintf(sTemp, "GET %s%s HTTP/1.1\r\n", pHttpAddr, pHttpFileName);
	strSend += sTemp;

	// Line2:����.
	sprintf(sTemp, "Host: %s\r\n", pHostAddr);
	strSend += sTemp;

	// Line3:���յ���������.
	sprintf(sTemp, "%s\r\n", "Accept: */*");
	strSend += sTemp;
	
	// Line4:�ο���ַ.
    sprintf(sTemp, "Referer: %s\r\n",pHttpAddr); 
	strSend += sTemp;

	delete[] pHttpAddr;
	delete[] pHttpFileName;
	delete[] pHostAddr;
		
	// Line5:���������.
	sprintf(sTemp, "%s\r\n", "User-Agent: Mozilla/4.0 (compatible; MSIE 5.0; Windows NT; DigExt; DTS Agent;)");
	strSend += sTemp;

	// ����. Range ��Ҫ���ص����ݷ�Χ������������Ҫ.
	if (nEnd <= 0)
	{
		sprintf(sTemp, "Range: bytes=%d-\r\n",nPos);
	}
	else
	{
		sprintf(sTemp, "Range: bytes=%d-%d\r\n",nPos, nEnd - 1);
	}
	strSend += sTemp;
	
	// LastLine: ����.
	sprintf(sTemp, "\r\n");
	strSend += sTemp;
	
	if(!SocketSend(hSocket, strSend.c_str(), strSend.length())) 
		return FALSE;

	// ȡ��httpͷ.
	int i=GetHttpHeader(hSocket,cTmpBuffer);
	if(!i)
	{
		TRACE("��ȡHTTPͷ����!\n");
		return FALSE;
	}
	
	// ���ȡ�õ�httpͷ����404�����������ʾ���ӳ�����.
	if(strstr(cTmpBuffer, "404") != NULL) 
	{
		dwFileLength = 0;
		TRACE("%s\n", cTmpBuffer);
		return FALSE;
	}

	// �õ��������ļ��Ĵ�С.
	dwFileLength=GetFileLength(cTmpBuffer);

	return TRUE;
}

SOCKET CHttpGet::ConnectHttpProxy(CString strProxyAddr,int nPort)
{	
	//char sTemp[1024] = {0};
	//char sTemp[1024] = "CONNECT 10.241.5.214:3128 HTTP/1.0\r\n\r\n";
	//char cTmpBuffer[1024] = {0};
	SOCKET hSocket = CDealSocket::GetConnect(strProxyAddr,nPort);

    if(hSocket == INVALID_SOCKET)
    {
		TRACE("����http������ʧ�ܣ�\n");
		return INVALID_SOCKET;
    }

//	char cProxy[1024];
//	wcstombs(cProxy,strProxyAddr.GetBuffer(),1024);
//	strProxyAddr.ReleaseBuffer();

/*
	// ����CONNCET�������������������ںʹ��������Ӵ����������
	// ��ַ�Ͷ˿ڷ���strProxyAddr,nPort ����.
	sprintf(sTemp, "CONNECT %s:%d HTTP/1.1\r\nUser-Agent:Mozilla/4.0 (compatible; MSIE 5.0; Windows NT; DigExt; DTS Agent;)\r\n\r\n",cProxy,nPort);
	//sprintf(sTemp, "CONNECT %s:%d HTTP/1.0\r\n\r\n",cProxy,nPort);
	
	if(!SocketSend(hSocket,sTemp))
	{
		TRACE("���Ӵ���ʧ��\n");
		closesocket(hSocket);
		return INVALID_SOCKET;
	}

	// ȡ�ô�����Ӧ��������Ӵ���ɹ��������������
	// ����"200 Connection established".
	int nLen=GetHttpHeader(hSocket,cTmpBuffer);
	if(!nLen)
	{
		TRACE("��ȡHTTPͷ����!\n");
		closesocket(hSocket);
		return INVALID_SOCKET;
	}

	if(strstr(cTmpBuffer, "HTTP/1.0 200 OK") == NULL)
	{
		TRACE("���Ӵ���ʧ��\n");
		closesocket(hSocket);
		return INVALID_SOCKET;
	}
	*/

	return hSocket; 
}
//---------------------------------------------------------------------------
SOCKET CHttpGet::ConnectHttpNonProxy(CString strHostAddr,int nPort)
{
	SOCKET hSocket=CDealSocket::GetConnect(strHostAddr,nPort);
	if(hSocket == INVALID_SOCKET)
		return INVALID_SOCKET;
    
	return hSocket;
}

DWORD CHttpGet:: GetFileLength(char *httpHeader)
{
	char strFind[] = {"Content-Length:"};

	char * p = strstr(httpHeader, strFind);
	if (p != NULL)
	{
		p += sizeof(strFind);

		char * p1 = strstr(p, "\r\n");		
		if(p1 != NULL)
		{
			*p1 = '\0';
		}
	}
	else
		return 0;

	return atoi(p);
}

DWORD CHttpGet::GetHttpHeader(SOCKET sckDest,char *str)
{
	BOOL bResponsed=FALSE;
	DWORD nResponseHeaderSize;
	
	if(!bResponsed)
	{
		char c = 0;
		int nIndex = 0;
		BOOL bEndResponse = FALSE;
		while(!bEndResponse && !g_bStopDownload && !g_bErrorDuringDownload)
		{
			int nLen = recv(sckDest,&c,1,0);
			if (nLen == SOCKET_ERROR)
			{
				int err_code = WSAGetLastError();
				if (WSAEWOULDBLOCK != err_code)
				{
					TRACE("Read error!\n");
					SetErrorDuringDownload();
					g_bDownloadBroken = true;
					return 0;
				}
				else
				{
					continue;
				}
			}
			else if(nLen==0) 
			{
				return 0;
			}

			str[nIndex++] = c;
			if(nIndex >= 4)
			{
				if( str[nIndex - 4] == '\r' && 
					str[nIndex - 3] == '\n' && 
					str[nIndex - 2] == '\r' && 
					str[nIndex - 1] == '\n')
					bEndResponse = TRUE;
			}
		}

		str[nIndex]=0;
		nResponseHeaderSize = nIndex;
		bResponsed = TRUE;
	}
	
	return nResponseHeaderSize;
}

BOOL CHttpGet::SocketSend(SOCKET sckDest,const char* szHttp,int uiLen)
{

	int iLen = (uiLen < 0) ? strlen(szHttp) : uiLen;
	
	int nSend = 0;
	while (nSend < iLen 
		 && !g_bStopDownload 
		 && !g_bErrorDuringDownload)
	{
		int n = send(sckDest, szHttp + nSend, iLen - nSend, 0);
		if(n == SOCKET_ERROR)
		{
			if (WSAEWOULDBLOCK == WSAGetLastError())
			{
			}
			else
			{
				TRACE("��������ʧ��!\n");
				SetErrorDuringDownload();
				return FALSE;
			}
		}
		else
		{
			nSend += n;
		}

		Sleep(100);
	}

	return TRUE;
}
