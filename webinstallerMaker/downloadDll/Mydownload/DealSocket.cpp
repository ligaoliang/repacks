#include "stdafx.h"
#include "DealSocket.h"
#include "global.h"
#include <mstcpip.h>
#include <stdlib.h>
#include "utils.h"

CDealSocket g_dealSocket;

CDealSocket::CDealSocket()
{
	// �׽��ֳ�ʼ��.
	WORD wVersionRequested = MAKEWORD(2,2);
	WSADATA wsaData;
	
	// ��ʼ��WinSock.
	if (int a = WSAStartup(wVersionRequested, &wsaData)!=0)
	{
		TRACE("WSAStartup\n");
		return;
	}

	// ��� WinSock �汾.
	if (wsaData.wVersion != wVersionRequested)
	{
		TRACE("WinSock version not supported\n");
		WSACleanup();
		return;
	}
}

//---------------------------------------------------------------------------
CDealSocket::~CDealSocket()
{
	// �ͷ�WinSock.
	WSACleanup();
}

//---------------------------------------------------------------------------
SOCKET CDealSocket::GetConnect(CString host ,int port)
{
    SOCKET hSocket;
	
	struct addrinfo hints, *res=NULL;
	memset(&hints,0,sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	Log(g_csTempDir + _T("log.txt"),_T("Createing socket\r"));
	CString csLog;
	int errorCode;
	// ����һ���󶨵���������TCP/IP�׽���.
	if ((hSocket = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
	{
		errorCode = WSAGetLastError();	
		//TRACE("Allocating socket failed. Error: %d\n",WSAGetLastError ());
		csLog.Format(_T("Allocating socket failed. Error: %d\r"),errorCode);
		Log(g_csTempDir + _T("log.txt"),csLog);
		return INVALID_SOCKET;
	}
	
	//if(!g_bAutoConfig)
	{

		int nReturn;
/*		u_long ul=1;
		nReturn = ioctlsocket(hSocket, FIONBIO, &ul);
		int val = 1;
		nReturn = setsockopt(hSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&val, sizeof(int));
		val = 1;
		nReturn = setsockopt(hSocket, SOL_SOCKET, SO_DONTLINGER, (char*)&val, sizeof(int));
		val = 1;
		nReturn = setsockopt(hSocket, SOL_SOCKET, SO_KEEPALIVE, (char*)&val,sizeof(val));
	*/
	tcp_keepalive alive_in                = {0};
	tcp_keepalive alive_out                = {0};
	alive_in.keepalivetime                = 5000;
	alive_in.keepaliveinterval        = 1000;
	alive_in.onoff                    = TRUE;
	unsigned long ulBytesReturn = 0;
	nReturn = WSAIoctl(hSocket, SIO_KEEPALIVE_VALS, &alive_in, sizeof(alive_in),
				&alive_out, sizeof(alive_out), &ulBytesReturn, NULL, NULL);
	
	}
	
	SOCKADDR_IN saServer;          // �������׽��ֵ�ַ.
	PHOSTENT phostent = NULL;	   // ָ��HOSTENT�ṹָ��

	// ʹ��TCP/IPЭ��.
	saServer.sin_family = AF_INET;
	
	
	// ��ȡ��������ص���Ϣ.
	/*
	char cHost[1024];
	wcstombs(cHost,host.GetBuffer(),1024);
	host.ReleaseBuffer();
	
	if((errorCode = getaddrinfo(cHost,"http",&hints, &res) )!= 0)
	{
		csLog.Format(_T("Get hostaddressinfo failed. Error: %d\r"),errorCode);
		Log(g_csTempDir + _T("log.txt"),csLog);
		g_bErrorDuringDownload = true;
		if(res != NULL)
			freeaddrinfo(res);
		return INVALID_SOCKET;
	}

	*/
	// ��ȡ��������ص���Ϣ.
	char * pHost = CStringToA(host);
	if (pHost == NULL)
	{
		TRACE("Memory allocation error.\n");
		closesocket (hSocket);
		return INVALID_SOCKET;
	}

	if ((phostent = gethostbyname (pHost)) == NULL) 
	{
		delete[] pHost;
		TRACE("Unable to get the host name. Error: %d\n",WSAGetLastError ());
		closesocket (hSocket);
		return INVALID_SOCKET;
	}
	delete[] pHost;

	// ���׽���IP��ַ��ֵ.
	memcpy ((char *)&(saServer.sin_addr), 
		phostent->h_addr, 
		phostent->h_length);

	// �趨�׽��ֶ˿ں�.
	saServer.sin_port =htons (port); 


	// ���������������׽�������.
	while (!g_bStopDownload 
			&& !g_bErrorDuringDownload)
	{
		if (//connect(hSocket,res->ai_addr, res->ai_addrlen) != 0
			connect(hSocket,(PSOCKADDR) &saServer, sizeof (saServer)) != 0) 
		{
			errorCode = WSAGetLastError();
			if ( ( errorCode == WSAEWOULDBLOCK ) ||
			   ( errorCode == WSAEALREADY ) )
			{
				Sleep(100);
				continue;
			}
			else if ( errorCode == WSAEISCONN)
			{
				if(res != NULL)
					freeaddrinfo(res);
				return hSocket;
			}
			else
			{
				csLog.Format(_T("Connect Failed. Error code is:%d\r"),errorCode);
				Log(g_csTempDir + _T("log.txt"),csLog);
				closesocket (hSocket);
				break;
			}
		}
	}
	if(res != NULL)
		freeaddrinfo(res);

	return INVALID_SOCKET;
}

