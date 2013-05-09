#ifndef GLOBAL_H
#define GLOBAL_H
#include "Types.h"
#include "CRC32.h"
#include "Lock.h"
#include "HttpGet.h"
#include "DealSocket.h"
#include "MyFile.h"
#include "tinyxml/tinyXML.h"
#include "tinyxml/tinystr.h"
#include <list>
#include <set>
#include <map>
#include <vector>
#include <algorithm>
#include <numeric>
#include <functional>
#include "Const.h"



extern volatile bool g_bStopDownload;
extern volatile bool g_bErrorDuringDownload;

//extern volatile unsigned long g_totalSize;
extern volatile unsigned long g_downloaded;
extern volatile unsigned long g_initialSize;
extern CString g_csProxy;
extern int g_ProxyPort;
extern std::vector<struct threadManagerInfo*> g_threadManagers;
extern std::vector<HANDLE> g_hThreadManagers;

extern volatile long g_finishedThreadNumber;
extern volatile long g_totalThreadNumber;


extern CString g_csCurrentDir;
extern CString g_csTempDir;
//extern TCHAR g_TempPath[MAX_PATH];
extern CString g_csResDir;




extern CString g_csResFilesUrl;
extern CRC32   crc32;
//extern volatile unsigned long * rdownloaded;
extern volatile HANDLE g_hMainDownloadThread;
extern volatile HANDLE g_hResouceDownloadThread;
extern CMyFile myfile;
extern CLock g_lock;
extern volatile bool g_bResDownloaded;  // configuration file, extension size and picture files are all downloaded
//extern volatile bool g_bConfigXMLDownloaded;    // only configuration file and extension size are downloaded

extern volatile bool g_bInner;
extern CString g_csCmd;
extern CString g_csAddonList;

//Downloading process bar handle,it's NULL before reaching the downloading page
extern HWND g_hProcess;

extern volatile bool g_bDownloadBroken;

extern TCHAR * g_resUrl;


extern std::map<LONG_PTR, UiItemInfo> g_UiItemMap;

extern std::vector<GroupInfo> g_groupsInfo;

extern std::vector<defaultExtensionInfo> g_defaultExtensions;

//void ParseURL(CString URL,CString &host,CString &path,CString &filename);

LPSTR CStringToA(const CString& strData);
unsigned __stdcall ThreadDownLoad(void* pParam);
void SetErrorDuringDownload();
//unsigned __stdcall ThreadDownLoadPicture(void* pParam);
//unsigned __stdcall ThreadDownloadPictureHelper(void* pParam);


BOOL ParseXMLElements(const TCHAR * config_file_name);
BOOL GetElementValue(const TiXmlElement* node, const char * value, TCHAR *& ret);
void LogAddonList(CString csFilePath,CString csFile);

int GetTotalThreadsNumber();
unsigned long GetTotalFileSize();
int GetFinishedThreadsNumber(struct threadManagerInfo* pThreadManagerInfo);

#endif