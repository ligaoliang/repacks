#ifndef _CONST_H
#define _CONST_H

//定义当前DLL名称
#define DLLNAME "Mydownload.dll"


const unsigned int PAGE_SIZE = 4096;
//const unsigned int DOWNLOAD_THREADS = 5;
const unsigned int WRITE_SIZE = PAGE_SIZE - sizeof(unsigned long);

//ERROR CODE
#define ERR_WRITE_FILE		0x11
#define ERR_DOWNLOAD		0x12
#define ERR_SUCCESS			0x0

//Addons-selecte Page resources IDs
#define PICTURE_HOLDER      0x4B0
#define FIRST_TEXT_HOLDER   0x4B1
#define SECOND_TEXT_HOLDER  0x4B2
#define THIRD_TEXT_HOLDER   0x4B3
#define FOURTH_TEXT_HOLDER  0x4B4
#define WAIT_PROGRESS_BAR   0x4B5
#define WAIT_TEXT           0x4B6

//
#define MAX_RECV_LEN           100   // 每次接收最大字符串长度.
#define MAX_PENDING_CONNECTS   4     // 等待队列的长度.


#define MAX_FILE_NAME_LEN   0x200


#endif