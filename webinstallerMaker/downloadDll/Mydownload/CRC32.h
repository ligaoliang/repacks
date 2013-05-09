#pragma once
#ifndef CRC32_H
#define CRC32_H
class CRC32
{
public:
	CRC32() { Init(); }
	~CRC32(){}
	unsigned long Calculate(BYTE *buf, int len);
private:
	unsigned long crc32_table[256];
	void Init();
};
#endif