#include "stdafx.h"
#include "utils.h"
#include <process.h>
#include "global.h"
#include <winhttp.h>

bool g_bAutoConfig = false;

int my_atoi(TCHAR *p)
{
	int sign = 0;
	int v = 0;
	for (;;)
	{
		int c = *p++ - L'0';
		if (c < 0 || c > 9) break;
		v *= 10;
		v += c;
	}

	return v;
}

void LogAddonList(CString csFilePath,CString csFile)
{
	Log(csFilePath,csFile);
}
void Log(CString csFilePath,CString csFile)
{
	FILE * fp = _tfopen(csFilePath, _T("a"));
	fwrite(csFile.GetBuffer(), sizeof(TCHAR),csFile.GetLength(), fp);
	csFile.ReleaseBuffer();
	fclose(fp);
}


void parseURL(CString URL,CString &host,CString &path,CString &filename)
{
	URL.TrimLeft();
	URL.TrimRight();
	CString str=URL;
	CString strFind=_T("http://");
	int n=str.Find(strFind);
	if(n!=-1){
		str.Delete(0, n+strFind.GetLength());
	}

	n=str.Find('/');
	host = str.Left(n);
	n=URL.ReverseFind('/');
	path = URL.Left(n+1);
	filename = URL.Right(URL.GetLength()-n-1);

}

HANDLE RunCmd(CString csCMD,BOOL bShow)
{
	MSG msg;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory( &si, sizeof(si) );
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	if(bShow)
		si.wShowWindow = SW_SHOW;
	else
		si.wShowWindow = SW_HIDE;
	ZeroMemory( &pi, sizeof(pi) );

	// Start the child process. 
	if( !CreateProcess( NULL,   // No module name (use command line). 
		csCMD.GetBuffer(), // Command line. 
		NULL,             // Process handle not inheritable. 
		NULL,             // Thread handle not inheritable. 
		FALSE,            // Set handle inheritance to FALSE. 
		0,                // No creation flags. 
		NULL,             // Use parent's environment block. 
		NULL,             // Use parent's starting directory. 
		&si,              // Pointer to STARTUPINFO structure.
		&pi )             // Pointer to PROCESS_INFORMATION structure.
		) 
	{

	}
	csCMD.ReleaseBuffer();
	//WaitForSingleObject(pi.hProcess, 1000);
	return pi.hProcess;
}

void GetProxyConfig(CString& csProxy,int& nProxyPort)
{
	WINHTTP_CURRENT_USER_IE_PROXY_CONFIG pconfig;
	CString csLog;

	if( WinHttpGetIEProxyConfigForCurrentUser( &pconfig ) )
	{
		if(pconfig.fAutoDetect == 1)
		{
			g_bAutoConfig = true;
			csLog = _T("Current user proxy setting: automatically detection\r");
			Log(g_csTempDir + _T("log.txt"),csLog);
		}
		if ( pconfig.lpszAutoConfigUrl != NULL)
		{
			GlobalFree(pconfig.lpszAutoConfigUrl);
		}
		if ( pconfig.lpszProxyBypass != NULL)
			GlobalFree(pconfig.lpszProxyBypass);
		if ( pconfig.lpszProxy != NULL)
		{
			CString csTemp(pconfig.lpszProxy); 	
			csTemp.Trim();
			int start = csTemp.Find(L"http=");
			if (start != -1)
			{
				int end = csTemp.Find(L";",start);
				csTemp = csTemp.Right(csTemp.GetLength() - start-5);
				csTemp = csTemp.Left(end - start-5);
			}
			start = csTemp.Find(L":");
			csProxy = csTemp.Left(start);
			csTemp = csTemp.Right(csTemp.GetLength()-start-1);
			nProxyPort = _wtoi(csTemp);
			GlobalFree(pconfig.lpszProxyBypass);
			csLog.Format(_T("Proxy setting is:%s:%d\r"),g_csProxy,nProxyPort);

			Log(g_csTempDir + _T("log.txt"),csLog);
		}
	}
}