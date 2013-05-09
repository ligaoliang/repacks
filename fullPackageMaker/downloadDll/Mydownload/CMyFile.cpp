#include "stdafx.h"
#include "MyFile.h"
#include "global.h"

CMyFile::CMyFile()
{
}

CMyFile::~CMyFile()
{
}

BOOL CMyFile::FileExists(LPCTSTR lpszFileName)
{
	DWORD dwAttributes = GetFileAttributes(lpszFileName);
    if (dwAttributes == 0xFFFFFFFF)
        return FALSE;

	if ((dwAttributes & FILE_ATTRIBUTE_DIRECTORY)  ==	FILE_ATTRIBUTE_DIRECTORY)
	{
			return FALSE;
	}
	else
	{
		return TRUE;
	}
}

HANDLE CMyFile::GetFilePointer(LPCTSTR lpszFileName, DWORD pos)
{
    HANDLE hFile;
	hFile = ::CreateFile(lpszFileName,    //创建文件的名称。
							GENERIC_WRITE,          // 写文件。
							FILE_SHARE_READ, 
							NULL,                  // 缺省安全属性。
							OPEN_ALWAYS,          
							FILE_ATTRIBUTE_NORMAL, // 一般的文件。         
							NULL);                // 模板文件为空。

	if (hFile != INVALID_HANDLE_VALUE)
	{
		SetFilePointer(hFile, pos, NULL, FILE_BEGIN);
	}
	return hFile;
}

//---------------------------------------------------------------------------
DWORD CMyFile::GetDownloadedFileSizeByName(LPCTSTR lpszFileName, DWORD *occupySize)
{
	if(!FileExists(lpszFileName)) return 0;
	struct _stat ST; 
	// 获取文件长度.
	_tstat(lpszFileName, &ST);
	//UINT nFilesize=ST.st_size;
	return ST.st_size;
	/*
	size_t checked_size = 0;
	size_t occupy_size = 0;
	BYTE buffer[PAGE_SIZE] = {0};

	FILE * fp = _tfopen(lpszFileName, _T("rb"));
	if (fp == NULL) return 0;
	
	size_t sz;
	while ((sz = fread(buffer, 1, PAGE_SIZE, fp)) > 0)
	{
		
		unsigned long crc_value;
		memcpy(&crc_value, buffer + (sz - sizeof(unsigned long)), sizeof(unsigned long));
		unsigned long verify = crc32.Calculate(buffer, sz - sizeof(unsigned long));
		if (crc_value != verify)
		{
			fclose(fp);
			if (occupySize) *occupySize = occupy_size;
			return checked_size;
		}

		checked_size += (sz - sizeof(unsigned long));
		occupy_size += sz;
	}

	fclose(fp);
	if (occupySize) *occupySize = occupy_size;

	return checked_size;
	*/
}


void CMyFile::RoundFile(LPCTSTR lpszFileName, DWORD size)
{
	HANDLE hFile = CreateFile(lpszFileName, GENERIC_WRITE,
                     0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile)
	{
		SetFilePointer(hFile, size, NULL, FILE_BEGIN);
		SetEndOfFile(hFile);
	}
}

