#include "stdafx.h"
#include "global2.h"
CDealSocket g_dealsocket;
/*
LPSTR CStringToA(const CString& strData) 
{
    CString ret;
    char *ptr = NULL;
#ifdef _UNICODE
    LONG    len;
    len = WideCharToMultiByte(CP_ACP, 0, strData, -1, NULL, 0, NULL, NULL);
    ptr = new char [len+1];
	if (ptr)
	{
		memset(ptr,0,len + 1);
		WideCharToMultiByte(CP_ACP, 0, strData, -1, ptr, len + 1, NULL, NULL);
	}
#else
    ptr = new char [strData.GetAllocLength()+1];
    sprintf(ptr,_T("%s"),strData);
#endif
    return ptr;
}*/