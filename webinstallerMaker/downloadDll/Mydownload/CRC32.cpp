#include "StdAfx.h"
#include "CRC32.h"

#define CRC32_POLY 0x04c11db7     

unsigned long CRC32::Calculate(BYTE *buf, int len)
{
	BYTE *p;
	unsigned long crc;

	crc = 0xffffffff;       /* preload shift register, per CRC-32 spec */
	for (p = buf; len > 0; ++p, --len)
		crc = (crc << 8) ^ crc32_table[(crc >> 24) ^ *p];
	return ~crc;            /* transmit complement, per CRC-32 spec */
}
void CRC32::Init()
{
	int i, j;
	u_long c;

	for (i = 0; i < 256; ++i) 
	{
		for (c = i << 24, j = 8; j > 0; --j)
			c = c & 0x80000000 ? (c << 1) ^ CRC32_POLY : (c << 1);
			crc32_table[i] = c;
     }
}

