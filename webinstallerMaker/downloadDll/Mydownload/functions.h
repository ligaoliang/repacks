#ifndef _FUNCTIOIS_H
#define _FUNCTIONS_H
#include "Types.h"

unsigned __stdcall DownloadResFilesThread(void* pParam);

BOOL InternetDownloadFile(CString& pURL, CString& tempPath);

void download(int threadNum, CString url, CString filename);
UINT __stdcall MultiThreadsDownLoadManager(void* pParam);
bool Downloading(CString strProxy,int nProxyPort,struct threadManagerInfo* pInfo);
void downloadThirdPartyFiles();

#endif
