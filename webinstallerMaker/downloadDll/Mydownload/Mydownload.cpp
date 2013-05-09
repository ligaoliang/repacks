// Mydownload.cpp : 定义 DLL 应用程序的入口点。

#include "stdafx.h"
#include "MyDownload.h"
#include <sys/stat.h>   // 状态显示头文件.
#include "ExDll/exdll.h"
#include <process.h>
#include <list>
#include <set>
#include <map>
#include <vector>
#include <algorithm>
#include <numeric>
#include <functional>
#include <assert.h>
#include "tinyxml/tinyXML.h"
#include <mstcpip.h>
#include "CRC32.h"
#include "global.h"
#include "HttpGet.h"
#include "Const.h"
#include "functions.h"
#include "utils.h"
#ifdef _MANAGED
#pragma managed(pop)
#endif


HWND        g_hwndProgressBar;
HWND        g_hwndStatic;
static int  g_cancelled;
static WNDPROC lpWndProcOld = NULL;

static UINT uMsgCreate;

HWND childwnd;
HWND hwndL;
HWND hwndB;
HWND hLabel = NULL;

static CFont * gFont = NULL;
using namespace std;


MYDOWNLOAD_API void fnInitial(HWND parent,
							  int string_size,
							  TCHAR *variables,
							  stack_t **stacktop,
							  extra_parameters *extra)
{	
	TCHAR dir[MAX_PATH];
	GetModuleFileName(NULL,dir,256);
	CString csDir(dir);
	int index = csDir.Find(_T("setup.exe"));
	if (-1 != index)
		g_csCurrentDir = csDir.Left(index);

	GetTempPath(MAX_PATH, dir);
	g_csTempDir = dir;
	g_csResDir = g_csTempDir + _T("resTemp\\");

	GetProxyConfig(g_csProxy,g_ProxyPort);
}

MYDOWNLOAD_API void fnDownloadResFiles(HWND parent,
										int    string_size,
										TCHAR   *variables,
										stack_t **stacktop)
{
	EXDLL_INIT();
	TCHAR url[1024];
	popstring(url);

	g_csResFilesUrl = url;
	unsigned threadID;
	g_hResouceDownloadThread = NULL;
	while (g_hResouceDownloadThread == NULL)
		g_hResouceDownloadThread = (HANDLE)_beginthreadex(NULL, 0, &DownloadResFilesThread, (void*)g_csResFilesUrl.GetBuffer(), 0, &threadID);
	g_csResFilesUrl.ReleaseBuffer();
}
MYDOWNLOAD_API bool fnDownload(HWND parent,
							   int    string_size,
							   TCHAR   *variables,
							   stack_t **stacktop)
{
	EXDLL_INIT();

	TCHAR buf[128];
	int threadNum;
	TCHAR url[1024];
	TCHAR filename[128];

	popstring(url);
	lstrcpyn(buf,url,15);
	if (!lstrcmpi(buf, L"/THREADNUMBER="))
	{
		threadNum = my_atoi(url+14);
	}
	popstring(url);
	popstring(filename);

	//资源下载线程同步
	if(g_hResouceDownloadThread != NULL)
		WaitForSingleObject(g_hResouceDownloadThread,INFINITE);
	download(threadNum,url,filename);

	return true;
}

MYDOWNLOAD_API void fndownloadThirdPartyFiles(HWND parent,
										 int string_size,
										 TCHAR *variables,
										 stack_t **stacktop,
										 extra_parameters *extra)
{
	downloadThirdPartyFiles();
}
MYDOWNLOAD_API void fnMyDownloadProgress(HWND parent,
									int string_size,
									TCHAR *variables,
									stack_t **stacktop,
									extra_parameters *extra)
{
	downloadThirdPartyFiles();
	EXDLL_INIT();
	MSG msg;

	if( parent != NULL && 
		(childwnd = FindWindowEx(parent, NULL, _T("#32770"), NULL)) != NULL )
	{
		hLabel = FindWindowEx(childwnd, NULL, _T("Static"), NULL);
		g_hProcess = FindWindowEx(childwnd, NULL, _T("msctls_progress32"), NULL);		
        PostMessage(g_hProcess, PBM_SETRANGE, 0, MAKELPARAM(0, 100));

		// still downloading configurable xml? Wait here.
		if (!g_bResDownloaded)
		{
			::SetWindowLongPtr(g_hProcess, GWL_STYLE,(::GetWindowLongPtr(g_hProcess, GWL_STYLE) | PBS_MARQUEE) );
			::PostMessage(g_hProcess, PBM_SETMARQUEE, 1, 50);	
			::SendMessage(hLabel,WM_SETTEXT, NULL,(LPARAM)_T("正在下載設定檔，請稍候..."));
			//::SendMessage(hLabel,WM_SETTEXT, NULL,(LPARAM)_T("Downloading configure file, please wait..."));

			while (!g_bResDownloaded
				&& !g_bStopDownload)
			{
				while (PeekMessage(&msg, parent,  0, 0, PM_REMOVE))
				{
					if (msg.message == WM_QUIT) 
					{ 
						g_bStopDownload = true;
						break;
					}

					if(!TranslateMessage(&msg))
						DispatchMessage(&msg);
				}
			}
				::SetWindowLongPtr(g_hProcess, GWL_STYLE,(::GetWindowLongPtr(g_hProcess, GWL_STYLE) & ~PBS_MARQUEE) );
		}
	

		::SendMessage(hLabel,WM_SETTEXT, NULL,(LPARAM)_T("正在下載安裝檔..."));
		//::SendMessage(hLabel,WM_SETTEXT, NULL,(LPARAM)_T("Downloading..."));
		InvalidateRect(childwnd, NULL, TRUE);

/*		int nTotalThreadsNum = GetTotalThreadsNumber();
		HANDLE* wait_for_threads = new HANDLE[nTotalThreadsNum];
		ZeroMemory(wait_for_threads,sizeof(HANDLE)* nTotalThreadsNum);
		
		int count = 0;
		
		std::vector<struct threadManagerInfo*>::iterator it;
		while(1)
		{
			it = g_threadManagers.begin();
			while(it!= g_threadManagers.end())
			{
				if(!((*it)->bInitiated))
				{
					Sleep(100);
					it = g_threadManagers.begin();
					continue;
				}
				it++;
			}
			break;
		}
		for(it = g_threadManagers.begin();it!= g_threadManagers.end();it++)
		{
			for( int i =0;i<(*it)->threadNum;i++)
			{
				wait_for_threads[count++] = (*it)->pThreadsHandls[i];
			}
		}
*/
		int size = g_hThreadManagers.size();
		HANDLE* wait_for_threads = new HANDLE[size];
		ZeroMemory(wait_for_threads,sizeof(HANDLE)* size);
		int count = 0;
		vector<HANDLE>::iterator it;
		for(it = g_hThreadManagers.begin();it != g_hThreadManagers.end(); it++)
		{
			wait_for_threads[count++] = *it;
		}


		DWORD dwStart = GetTickCount();
		DWORD dwStart_1 = dwStart;
		unsigned long old_downloaded = 0;
		unsigned long downloaded_baseline = 0;
		unsigned long g_totalSize = GetTotalFileSize();

		while (!g_bStopDownload)
		{
			
			g_totalSize = GetTotalFileSize();
			DWORD dw = WaitForMultipleObjects(size, wait_for_threads, TRUE, 0);
			if (dw >= WAIT_OBJECT_0 && dw < (WAIT_OBJECT_0 + count))
			{
				break;
			}
			else if ( (dw >= WAIT_ABANDONED_0 && dw < (WAIT_ABANDONED_0 + count)) || dw == WAIT_FAILED)
			{
				dw = GetLastError();
				break;
			}
	
			DWORD dwNow = GetTickCount();
			if (g_totalSize > 0 && dwNow - dwStart > 500)
			{
				unsigned long main_downloaded = 0;

				//g_lock.Lock();
				main_downloaded = g_downloaded;
				//g_lock.Unlock();

				unsigned long finished = g_initialSize + main_downloaded;
				//if(finished >= g_totalSize)
				
				PostMessage(g_hProcess, PBM_SETPOS, finished * 100.0 / (g_totalSize), 0);
		
				// update download data rate and remaining time
				if (downloaded_baseline > 0)
				{
					double avg_down_rate = (finished - downloaded_baseline) / (double)(dwNow - dwStart_1);
					double down_rate = (finished - old_downloaded) / (double)(dwNow - dwStart);
					if (avg_down_rate > 0.1)
					{
						int time_to_end = ((g_totalSize - finished) / avg_down_rate) / 1000.0 + 0.5;
						TCHAR prompt[256];

						int hour = time_to_end / 3600;
						time_to_end -= (3600 * hour);
						int minute = time_to_end / 60;
						time_to_end -= (60 * minute);
						int second = time_to_end;

						if (hour > 0)
						{
							_stprintf(prompt, _T("正在下載安裝檔，下載速度%.2fKB/秒，預計完成時間%d小時%d分%d秒"), down_rate, hour, minute, second);
							//_stprintf(prompt, _T("Downloading installer, downloading speed %.2fKB/s, it will be finished in %dh%dmin%ds"), down_rate, hour, minute, second);

						}
						else if (minute > 0)
						{
							_stprintf(prompt, _T("正在下載安裝檔，下載速度%.2fKB/秒，預計完成時間%d分%d秒"), down_rate, minute, second);
							//_stprintf(prompt, _T("Downloading installer, downloading speed %.2fKB/s, it will be finished in %dmin%ds"), down_rate, minute, second);
						}
						else
						{
							_stprintf(prompt, _T("正在下載安裝檔，下載速度%.2fKB/秒，預計完成時間%d秒"), down_rate, second);
							//_stprintf(prompt, _T("Downloading installer, downloading speed %.2fKB/s, it will be finished in %ds"), down_rate, second);
						}
						::SendMessage(hLabel,WM_SETTEXT, NULL,(LPARAM)prompt);
					}
					::Sleep(1000);
				}
				else
				{
					downloaded_baseline = finished;
				}
				dwStart = dwNow;
				old_downloaded = finished;
			}

			while (!g_bStopDownload && PeekMessage(&msg, parent,  0, 0, PM_REMOVE)) 
			{ 
				if (msg.message == WM_QUIT) 
				{ 
					g_bStopDownload = true;
					break;
				}

				if(!TranslateMessage(&msg))
					DispatchMessage(&msg);
			}

		}
/*
		while(1)
		{
			it = g_threadManagers.begin();
			while(it!= g_threadManagers.end())
			{
				if(!((*it)->bInitiated))
				{
					Sleep(100);
					it = g_threadManagers.begin();
					continue;
				}
				it++;
			}
			break;
		}
		for(it = g_threadManagers.begin();it!= g_threadManagers.end();it++)
		{
			for( int i =0;i<(*it)->threadNum;i++)
			{
				wait_for_threads[count++] = (*it)->pThreadsHandls[i];
			}
		}

		WaitForMultipleObjects(g_totalThreadNumber, wait_for_threads, TRUE, INFINITE);
*/

		// error happens during downloading?
		if (g_bErrorDuringDownload)
		{
			setuservariable(INST_R0, _T("error"));
			delete []wait_for_threads;
			return;
			//goto cleanup;
		}
		WaitForMultipleObjects(size, wait_for_threads, TRUE, INFINITE);

		if (!g_bStopDownload)
		//{
		//	WaitForMultipleObjects(size, wait_for_threads, TRUE, INFINITE);
		//}
		//else
		{
			FILE *fpwrite = NULL;

			if (g_hProcess)
			{
				PostMessage(g_hProcess, PBM_SETPOS, 100, 0);
				// calvin
				::SendMessage(hLabel,WM_SETTEXT, NULL,(LPARAM)_T("正在校驗下載檔的完整性..."));
				//::SendMessage(hLabel,WM_SETTEXT, NULL,(LPARAM)_T("Checking the integrity of downloaded files..."));

				::SetWindowLongPtr(g_hProcess, GWL_STYLE,(::GetWindowLongPtr(g_hProcess, GWL_STYLE) | PBS_MARQUEE) );
				::SendMessage(g_hProcess, PBM_SETMARQUEE, 1, 50);
			}
			if (!CHttpGet::FileCombine(g_threadManagers))
			{
				setuservariable(INST_R0, _T("error"));			
			}
		}
	}
}

MYDOWNLOAD_API bool fnCancelDownload(HWND parent,
              int    string_size,
              TCHAR   *variables,
              stack_t **stacktop)
{
	EXDLL_INIT();
	g_bStopDownload = true;
	/*
	int size = g_hThreadManagers.size();
	HANDLE* wait_for_threads = new HANDLE[size];
	ZeroMemory(wait_for_threads,sizeof(HANDLE)* size);
	int count = 0;
	vector<HANDLE>::iterator it;
	for(it = g_hThreadManagers.begin();it != g_hThreadManagers.end(); it++)
	{
		wait_for_threads[count++] = *it;
	}
	WaitForMultipleObjects(size, wait_for_threads, TRUE, INFINITE);
	std::vector<struct threadManagerInfo*>::iterator it2 = g_threadManagers.begin();
	while(it2 != g_threadManagers.end())	
	{
		struct threadManagerInfo* pThreadManager = *it2;
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
	}
	delete []wait_for_threads;
	*/
/*
	HANDLE wait_for_threads[DOWNLOAD_THREADS + 2] = {NULL};
	int count = 0;
	for (int i = 0; i < DOWNLOAD_THREADS; ++i) 
	{
		//if (g_hThread[i] != NULL)
		{
			//wait_for_threads[count++] = g_hThread[i];
		}
	}
	if (g_hMainDownloadThread != NULL)
	{
		wait_for_threads[count++] = g_hMainDownloadThread;
	}
	if (g_hConfigurableDownloadThread != NULL)
	{
		wait_for_threads[count++] = g_hConfigurableDownloadThread;
	}

	DWORD dw = WaitForMultipleObjects(count, wait_for_threads, TRUE, INFINITE);
	*/
	return true;
}

MYDOWNLOAD_API void fnExecuteCmd(HWND parent,
								 int string_size,
								 TCHAR *variables,
								 stack_t **stacktop,
								 extra_parameters *extra)
{
	EXDLL_INIT();
	
	MSG msg;

	TCHAR * cmd = (TCHAR*)GlobalAlloc(GPTR, string_size * sizeof(TCHAR));
	if (popstring(cmd)) goto cleanup;

	if( parent != NULL && 
		(childwnd = FindWindowEx(parent, NULL, _T("#32770"), NULL)) != NULL )
	{
		STARTUPINFO si;
		PROCESS_INFORMATION pi;

		ZeroMemory( &si, sizeof(si) );
		si.cb = sizeof(si);
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE;
		ZeroMemory( &pi, sizeof(pi) );
		
		//if(g_bInner)
			//cmd = g_csCmd.GetBuffer();

		// Start the child process. 
		if( !CreateProcess( NULL,   // No module name (use command line). 
			cmd, // Command line. 
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
			g_csCmd.ReleaseBuffer();
			goto cleanup;
		}

		g_csCmd.ReleaseBuffer();
		DWORD dwStart = GetTickCount();		
	
		while (1)
		{
			DWORD dw = WaitForSingleObject(pi.hProcess, 0);
			if (dw == WAIT_OBJECT_0 || dw == WAIT_ABANDONED_0)
			{
				break;
			}

			DWORD dwNow = GetTickCount();
			if (dwNow - dwStart > 200)
			{
//				PostMessage(hProcess, PBM_SETPOS, finished * 100.0 / totalSize, 0);
				dwStart = dwNow;
			}

			while (PeekMessage(&msg, parent,  0, 0, PM_REMOVE)) 
			{ 
				if (msg.message == WM_QUIT) 
				{ 
					continue;
				}

				if(!TranslateMessage(&msg))
					DispatchMessage(&msg);
			}
		}
	}

cleanup:
	GlobalFree(cmd);
}

HWND hwndPictureHolder = NULL;
HWND hwndLabel1 = NULL;
HWND hwndLabel2 = NULL;
HWND hwndLabel3 = NULL;
HWND hwndLabel4 = NULL;
HWND hwndConfigurableDownloadProgressBar = NULL;
HWND hwndConfigurableDownloadText = NULL;

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) 
{
//	TCHAR buf[256] = {0}; 
//	if (GetClassName(hwnd, buf, 255) > 0 
//		&& _tcsicmp(buf, _T("static")) == 0)
	{
		LONG_PTR i = GetWindowLongPtr(hwnd, GWLP_ID);
		switch (i)
		{
		case PICTURE_HOLDER:
			hwndPictureHolder = hwnd;
			break;
		case FIRST_TEXT_HOLDER:
			hwndLabel1 = hwnd;
			break;
		case SECOND_TEXT_HOLDER:
			hwndLabel2 = hwnd;
			break;
		case THIRD_TEXT_HOLDER:
			hwndLabel3 = hwnd;
			break;
		case FOURTH_TEXT_HOLDER:
			hwndLabel4 = hwnd;
			break;		
		case WAIT_PROGRESS_BAR:
			hwndConfigurableDownloadProgressBar = hwnd;
			break;
		case WAIT_TEXT:
			hwndConfigurableDownloadText = hwnd;
			break;
		}
	}

	return TRUE;
}

void ShowPicture(HWND hwnd, const TCHAR* filename)
{
	CStatic * pStatic = (CStatic*)(CWnd::FromHandle(hwnd));

	HBITMAP bmp; 
	bmp = (HBITMAP)::LoadImage((HINSTANCE)GetWindowLongPtr(childwnd, GWLP_HINSTANCE), filename, 
			IMAGE_BITMAP,0,0,    
			LR_CREATEDIBSECTION  
			|  LR_DEFAULTSIZE  
			|  LR_LOADFROMFILE  
			|  LR_DEFAULTCOLOR); 
	pStatic->SetBitmap(bmp);

}

bool IsGroupChecked(std::map<LONG_PTR, UiItemInfo>::iterator& const itItem, 
								std::map<LONG_PTR, UiItemInfo>::iterator& itLeader)
{
	
	size_t checked_count = 0;
	std::map<LONG_PTR, UiItemInfo>::iterator itStart = itItem;
	std::map<LONG_PTR, UiItemInfo>::iterator itEnd = itItem;

	while (itStart != g_UiItemMap.begin() && !((*itStart).second.bIsGroupLeader))
		--itStart;
	while (itEnd != g_UiItemMap.end() && !((*itEnd).second.bIsGroupLeader))
		++itEnd;

	if ((*itStart).second.bIsGroupLeader)
	{
		itLeader = itStart;
		++itStart;
	}

	std::map<LONG_PTR, UiItemInfo>::iterator itTemp = itStart;
	while (itStart != itEnd)
	{
		if (*((*itStart).second.pbChecked))
			++checked_count;
	
		++itStart;
	}

	TRACE(_T("%d\n"), distance(itStart, itEnd));
	if (distance(itTemp, itEnd) == checked_count)
	{
		return true;
	}

	return false;
}

LRESULT MyMsgProc(HWND hwnd,UINT message, WPARAM wParam, LPARAM lParam)
{
	CString str;
	bool bFound = false;
	LONG_PTR old_proc = NULL;
	LONG_PTR id = GetWindowLongPtr(hwnd, GWLP_ID);

	std::map<LONG_PTR, UiItemInfo>::iterator it = g_UiItemMap.find(id);
	if (it != g_UiItemMap.end())
	{
		old_proc = (*it).second.old_msg_proc;
		bFound = true;
	}

	switch(message)
	{
	case WM_LBUTTONUP:
		if (bFound)
		{			
			ShowPicture(hwndPictureHolder, (*it).second.picture_local_name);

			SetWindowText(hwndLabel1, (*it).second.intros[0]);
			SetWindowText(hwndLabel2, (*it).second.intros[1]);
			SetWindowText(hwndLabel3, (*it).second.intros[2]);
			SetWindowText(hwndLabel4, (*it).second.intros[3]);

			CButton * pButton = (CButton*)(CWnd::FromHandle(hwnd));
			int state = pButton->GetCheck();
			if (state == BST_UNCHECKED)
			{
				*((*it).second.pbChecked) = true;
				state = BST_CHECKED;
			}
			else
			{
				*((*it).second.pbChecked) = false;
				state = BST_UNCHECKED;
			}

			if ((*it).second.bIsGroupLeader)
			{			
				// set the group to checked/unchecked
				++it;
				while (it != g_UiItemMap.end() && !((*it).second.bIsGroupLeader))
				{
					((CButton*)((*it).second.pWnd))->SetCheck(state);
					*((*it).second.pbChecked) = (state == BST_CHECKED) ? true : false;

					LONG_PTR idCtl = GetWindowLongPtr(((*it).second.pWnd)->GetSafeHwnd(), GWLP_ID);
					if (state == BST_CHECKED)
					{
						//DownloadXPI(idCtl, (*it).second.size, (*it).second.download_url);					
					}
					else
					{
						//CancelXPIDownload(idCtl);
					}

					++it;
				}
			}
			else
			{
				std::map<LONG_PTR, UiItemInfo>::iterator itLeader;
				if (state == BST_CHECKED)
				{
					//DownloadXPI(id, (*it).second.size, (*it).second.download_url);
					if (IsGroupChecked(it, itLeader))
					{
						// check the group leader
						((CButton*)((*itLeader).second.pWnd))->SetCheck(BST_CHECKED);
					}
				}
				else
				{
					//CancelXPIDownload(id);
					if (!IsGroupChecked(it, itLeader))
					{
						// check the group leader
						((CButton*)((*itLeader).second.pWnd))->SetCheck(BST_UNCHECKED);
					}
				}
			}
		}
		break;
	}

	//调用控件原来的消息处理函数
	if (bFound)
		return CallWindowProc((WNDPROC)old_proc,hwnd,message,wParam,lParam);
	else 
		return DefWindowProc(hwnd, message, wParam, lParam);
		
	return NULL;
}

void CreateControl(ControlType type, 
				   const TCHAR* title, 
				   const CRect & rect, 
				   UINT id, 
				   const TCHAR * picture_url, 
				   TCHAR* const* intros,
				   bool bIsGroupLeader,
				   bool &bChecked,
				   bool &bUnpacked,
				   const TCHAR * file
				   )
{

	CStatic * pStatic = NULL;
	CButton * checkb = NULL;
	HWND hwndButton = NULL;
	//CString strHostAddr;
	//CString strHttpAddr;
	//CString strHttpFilename;
	CString strFileName;
	TCHAR tempPath[256] = {0};
	GetTempPath(sizeof(tempPath)/sizeof(TCHAR) - 1, tempPath);

	//static CFont * font = NULL;
	CFont * font = gFont;
	switch (type)
	{
	case Static:
		pStatic = new CStatic();
		pStatic->Create(title, 
					WS_VISIBLE | WS_CHILDWINDOW | SS_LEFT, 
					rect, 
					CWnd::FromHandle(childwnd), 
					id);

		pStatic->SetFont(font);	
		break;
	case Checkbox:
		
		hwndButton = CreateWindow( 
			_T("BUTTON"),   // predefined class 
			title,       // button text 
			BS_AUTOCHECKBOX | WS_VISIBLE | WS_CHILDWINDOW | WS_CLIPSIBLINGS | WS_TABSTOP | BS_TOP | BS_BOTTOM | BS_MULTILINE,  // styles 
			// Size and position values are given explicitly, because 
			// the CW_USEDEFAULT constant gives zero values for buttons. 
			rect.left,         // starting x position 
			rect.top,         // starting y position 
			rect.Width(),        // button width 
			rect.Height(),        // button height 
			childwnd,       // parent window 父窗口
			(HMENU)id,       // id 
			(HINSTANCE) GetWindowLongPtr(childwnd, GWLP_HINSTANCE), 
			NULL);      // pointer not needed
		//DWORD dw =  GetLastError();

		::SetWindowLongPtr(hwndButton, GWL_EXSTYLE,(::GetWindowLongPtr(hwndButton, GWL_EXSTYLE) | WS_EX_LEFT | WS_EX_LTRREADING));
		checkb = (CButton*)(CWnd::FromHandle(hwndButton));
		checkb->SetFont(font);
		if (bChecked)
		{
			checkb->SetCheck(BST_CHECKED);
		}
		LONG_PTR proc = SetWindowLongPtr(hwndButton, GWLP_WNDPROC, (LONG_PTR)MyMsgProc);

		//ParseURL(CString(picture_url), strHostAddr, strHttpAddr, strHttpFilename);
		//strFileName = tempPath;
		strFileName = g_csResDir + CString(picture_url);

		UiItemInfo item;
		item.picture_local_name = _tcsdup((LPCTSTR)strFileName);
		item.old_msg_proc = proc;
		item.bIsGroupLeader = bIsGroupLeader;
		item.pWnd = checkb;
		item.intros = intros;
		item.pbChecked = &bChecked;
		item.pbUnpacked = &bUnpacked;
		item.pFile = file;
		//item.size = extension_size;
		//item.download_url = download_url;

		g_UiItemMap.insert(make_pair<LONG_PTR, UiItemInfo>(id, item));
		break;
	}
	
}


#define SHOW(hwnd) \
	if (hwnd != NULL) \
		::SetWindowLongPtr(hwnd, GWL_STYLE,(::GetWindowLongPtr(hwnd, GWL_STYLE) | WS_VISIBLE) );
#define HIDE(hwnd) \
	if (hwnd != NULL) \
		::SetWindowLongPtr(hwnd, GWL_STYLE,(::GetWindowLongPtr(hwnd, GWL_STYLE) & ~WS_VISIBLE) );

MYDOWNLOAD_API bool fnShowConfigurableUI(HWND parent,
              int    string_size,
              TCHAR   *variables,
              stack_t **stacktop)
{
	EXDLL_INIT();

	MSG msg;
	gFont = CWnd::FromHandle(parent)->GetFont();
	if (!g_bStopDownload)
	{
		// prompt "waiting..." on UI
		if( parent != NULL && 
			(childwnd = FindWindowEx(parent, NULL, _T("#32770"), NULL)) != NULL )
		{
			EnumChildWindows(childwnd, (WNDENUMPROC)EnumWindowsProc,(LPARAM)0); 
			
			if (!g_bResDownloaded)
			{
				if (hwndConfigurableDownloadProgressBar != NULL)
				{
					::SetWindowLongPtr(hwndConfigurableDownloadProgressBar, GWL_STYLE,(::GetWindowLongPtr(hwndConfigurableDownloadProgressBar, GWL_STYLE) | PBS_MARQUEE | WS_VISIBLE) );
					::PostMessage(hwndConfigurableDownloadProgressBar, PBM_SETMARQUEE, 1, 50);			
				}

				SHOW(hwndConfigurableDownloadText);
				InvalidateRect(childwnd, NULL, TRUE);
			}
		}

// calvin
//		DWORD t1 = GetTickCount();
		while (!g_bResDownloaded
			&& !g_bStopDownload && !g_bErrorDuringDownload)
		{
			while (PeekMessage(&msg, parent,  0, 0, PM_REMOVE))
			{
				if (msg.message == WM_QUIT) 
				{ 
					g_bStopDownload = true;
					break;
				}

				if(!TranslateMessage(&msg))
					DispatchMessage(&msg);
			}
		}

		if (g_bErrorDuringDownload)
		{
			setuservariable(INST_R0, _T("error"));
			return false;
		}
		//CString str = g_csCurrentDir + _T("7za.exe x ") + g_csTempDir + _T("res.7z -o")  + g_csResDir;
		//DecompressCmd(str);
		//DWORD dw = ::GetLastError();
		
		HIDE(hwndConfigurableDownloadProgressBar);
		HIDE(hwndConfigurableDownloadText);
		InvalidateRect(childwnd, NULL, TRUE);

		SHOW(hwndPictureHolder);
		SHOW(hwndLabel1);
		SHOW(hwndLabel2);
		SHOW(hwndLabel3);
		SHOW(hwndLabel4);

		g_UiItemMap.clear();

		// create 
		if (childwnd != NULL)
		{
			UINT id = 4000;
			CRect rect(5,-22,150,15);
			std::vector<GroupInfo>::iterator it = g_groupsInfo.begin();
			while (it != g_groupsInfo.end())
			{	
				rect.left = 5;
				rect.right = 150;
				rect.top += 22;
				rect.bottom = rect.top + 15;
				bool bNull = false;
				CreateControl(Checkbox, 
							(*it).description,
							rect,
							id++,
							(*it).picture_url,
							(*it).intro,
							true,
							(*it).checked,
							bNull,
							NULL);

				std::vector<ExtensionInfo>::iterator it1 = (*it).extensions.begin();
				while (it1 != (*it).extensions.end())
				{
					size_t j = it1 - (*it).extensions.begin();
					if (j % 2 == 0)
					{
						// left column
						rect.top += ((j == 0) ? 22 : 20);
						rect.bottom = rect.top + 15;
						rect.left += ((j == 0) ? 20 : -130);
					}
					else
					{
						// right column
						rect.left += 130;
					}
					rect.right = rect.left + 110;
					CreateControl(Checkbox, 
								(*it1).name,
								rect,
								id++,
								(*it1).picture_url,
								(*it1).intro,
								false,
								(*it1).checked,
								(*it1).unpacked,
								(*it1).file);
					++it1;
				}

				++it;
			}

			g_lock.Lock();
			//DownloadDefaultExtensionsWithoutLock();
			g_lock.Unlock();
		}

		// show default picture	
		std::map<LONG_PTR, UiItemInfo>::iterator mapit = g_UiItemMap.find(4000);
		if (mapit != g_UiItemMap.end())
		{
			ShowPicture(hwndPictureHolder, (*mapit).second.picture_local_name);
			SetWindowText(hwndLabel1, (*mapit).second.intros[0]);
			SetWindowText(hwndLabel2, (*mapit).second.intros[1]);
			SetWindowText(hwndLabel3, (*mapit).second.intros[2]);
			SetWindowText(hwndLabel4, (*mapit).second.intros[3]);
		}

		InvalidateRect(childwnd, NULL, TRUE);
		// enable next button
		EnableWindow(GetDlgItem(parent, 1), 1);
	}

	return true;
}


MYDOWNLOAD_API void fnMoveExtensionFiles(HWND parent,
								 int string_size,
								 TCHAR *variables,
								 stack_t **stacktop,
								 extra_parameters *extra)
{

	EXDLL_INIT();
	CString csXpiFile, csDestFile, strFileName;
	//TCHAR tempPath[256] = {0};
	//GetTempPath(sizeof(tempPath)/sizeof(TCHAR) - 1, tempPath);

	//csXpiFile = tempPath;
	//csDestFile = tempPath;
	csXpiFile = g_csTempDir + _T("MozillaFirefox\\core\\distribution\\myextensions\\");
	csDestFile = g_csTempDir + _T("MozillaFirefox\\core\\distribution\\extensions\\");
	
	
	std::vector<defaultExtensionInfo>::iterator defaultIt;
	std::vector<GroupInfo>::iterator it;
	

	//TCHAR * folder = (TCHAR*)GlobalAlloc(GPTR, string_size * sizeof(TCHAR));
	//if (popstring(folder)) return;//goto cleanup;

	defaultIt = g_defaultExtensions.begin();
	while (defaultIt != g_defaultExtensions.end())
	{
		if((*defaultIt).unpacked)
		{
			CString str = g_csCurrentDir + _T("7za.exe x ") + csXpiFile +defaultIt->file +_T(".xpi -o") + csDestFile + defaultIt->file ;
			HANDLE handle = RunCmd(str,FALSE);
			WaitForSingleObject(handle,INFINITE);
			DWORD dw = ::GetLastError();
		}
		else
		{
			CopyFile(csXpiFile + defaultIt->file +_T(".xpi"), csDestFile+defaultIt->file+_T(".xpi"),FALSE);
		}
		++defaultIt;
	}
	
	it = g_groupsInfo.begin();
	while (it != g_groupsInfo.end())
	{
		std::vector<ExtensionInfo>::iterator it1 = (*it).extensions.begin();
		while (it1 != (*it).extensions.end())
		{
			if ((*it1).checked)
			{
				if((*it1).type == 2)
				{
					if((*it1).unpacked)
					{
						CString str = g_csCurrentDir + _T("7za.exe x ") + csXpiFile +it1->file +_T(".xpi -o") + csDestFile + it1->file ;
						HANDLE handle = RunCmd(str,FALSE);
						WaitForSingleObject(handle,INFINITE);
						DWORD dw = ::GetLastError();
					}
					else
					{
						CopyFile(csXpiFile + it1->file +_T(".xpi"), csDestFile+it1->file+_T(".xpi"),FALSE);
					}
				}
				if((*it1).type == 1)
				{
					CString str = g_csTempDir + (*it1).cmd;
					HANDLE handle = RunCmd(str,TRUE);
					WaitForSingleObject(handle,INFINITE);
				}

				g_csAddonList += (*it1).id;
				g_csAddonList += _T(";");
			}
			else
			{
				if(it1->type == 2)
					DeleteFile(csXpiFile+ it1->file +_T(".xpi"));
			}
			++it1;
		}
		++it;
	}	
	CString csAddonList = g_csTempDir;
	csAddonList += _T("addonlist.ini");
	LogAddonList(csAddonList,g_csAddonList);
	//GlobalFree(folder);
//cleanup:
	return;
	
}



