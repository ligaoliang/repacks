#ifndef _DEALSOCKET_H
#define _DEALSOCKET_H

class CDealSocket  
{
public:
	CDealSocket();
	virtual ~CDealSocket();
	static SOCKET GetConnect(CString host ,int port);
};

extern CDealSocket g_dealSocket;
#endif
