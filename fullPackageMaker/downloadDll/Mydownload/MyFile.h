#ifndef MYFILE_H
#define MYFILE_H
class CMyFile  
{
public:
	CMyFile();
	virtual ~CMyFile();
public:
	BOOL FileExists(LPCTSTR lpszFileName);
	HANDLE GetFilePointer(LPCTSTR lpszFileName, DWORD pos);
	DWORD GetDownloadedFileSizeByName(LPCTSTR lpszFileName, DWORD *occupySize = NULL);
	void RoundFile(LPCTSTR lpszFileName, DWORD size);
};
#endif