#ifndef _TYPES_H
#define _TYPES_H
#include <vector>

struct  threadInfo
{
	CString csHost;
	CString csPath;
	CString csFileName;
	CString csDesFileName;
	DWORD	dwStart;        // 分割的起始位置.
	DWORD	dwEnd;          // 分割的终点位置.
	bool	bState;			//
	//int		index;
	threadInfo()
	{
		csHost = L"";
		csPath = L"";
		csFileName = L"";
		csDesFileName = L"";
		dwStart = 0;
		dwEnd = 0;
		bState = false;
		//index = -1;
	}
};

struct threadManagerInfo{
	bool bInitiated;
	int threadNum;
	CString url;
	CString host;
	CString path;
	CString filename;
	HANDLE* pThreadsHandls;
	
	struct  threadInfo* pThreadInfo;
	//void* pHttpGet;
	volatile int finishedThreadNum;
	volatile bool bDownloadComplete;
	volatile unsigned long totalSize;
	volatile unsigned long downloaded;
	volatile unsigned long initialSize;

	threadManagerInfo()
	{
		bInitiated = false;
		threadNum = 0;
		url = L"";
		host = L"";
		path = L"";
		filename = L"";
		pThreadsHandls = NULL;
		pThreadInfo = NULL;
		finishedThreadNum = 0;
		//pHttpGet = NULL;
		bDownloadComplete = false;
		totalSize = 0;
		downloaded = 0;
		initialSize = 0;
	}
};

struct defaultExtensionInfo{
	TCHAR * file;
	bool	unpacked;

	defaultExtensionInfo()
	{
		file = NULL;
		unpacked = false;
	}
};

struct ExtensionInfo{
	TCHAR * name;
	TCHAR * file;
	TCHAR * picture_url;
	TCHAR * intro[4];
	bool    checked;
	bool	unpacked;
	TCHAR * id;
	TCHAR* url;
	TCHAR* cmd;
	int type;

	//size_t  extension_size;

	ExtensionInfo()
	{
		name = NULL;
		file = NULL;
		picture_url = NULL;
		intro[0] = intro[1] = intro[2] = intro[3] = NULL;
		checked = false;
		unpacked = false;
		id = NULL;
		url = NULL;
		cmd = NULL;
		type = 0;
		//extension_size = 0;
	}
};

struct GroupInfo{
	TCHAR * description;
	TCHAR * picture_url;
	TCHAR * intro[4];
	bool    checked;

	std::vector<ExtensionInfo> extensions;

	GroupInfo()
	{
		description = NULL;
		picture_url = NULL;
		intro[0] = intro[1] = intro[2] = intro[3] = NULL;
		checked = false;
	}
};

struct UiItemInfo{
	TCHAR * picture_local_name;
	LONG_PTR old_msg_proc;
	bool bIsGroupLeader;
	CWnd * pWnd;
	TCHAR * const * intros;
	bool   * pbChecked;
	bool   * pbUnpacked;
	const TCHAR * pFile;
	//size_t size;
	//const TCHAR * download_url;

	UiItemInfo()
	{
		picture_local_name = NULL;
		old_msg_proc = NULL;
		bIsGroupLeader = false;
		pWnd = NULL;
		intros = NULL;
		pbChecked = NULL;
		pbUnpacked = NULL;
		pFile = NULL;
		//size = 0;
		//download_url = NULL;
	}
};

#endif