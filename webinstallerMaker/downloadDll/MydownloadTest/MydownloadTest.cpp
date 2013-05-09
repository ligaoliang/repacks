// MydownloadTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <atlstr.h>

int _tmain(int argc, _TCHAR* argv[])
{

	CString URL,host,path,filename;
	URL = L"http://download.firefox.com.cn/releases/webins2.0/full/firefox.exe";
	//
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
	return 0;
}

