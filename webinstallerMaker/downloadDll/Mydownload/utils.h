#ifndef _UTILS_H
#define _UTILS_H

extern bool g_bAutoConfig;
int my_atoi(TCHAR *p);
void Log(CString csFilePath,CString csFile);
void LogAddonList(CString csFilePath,CString csFile);

//url = http://download.firefox.com.cn/releases/webins2.0/full/firefox.exe
//host = download.firefox.com
//path = http://download.firefox.com.cn/releases/webins2.0/full/
//filename = firefox.exe
void parseURL(CString URL,CString &host,CString &path,CString &filename);
HANDLE RunCmd(CString csCMD,BOOL bShow);
void  GetProxyConfig(CString& csProxy,int& nProxyPort);
#endif