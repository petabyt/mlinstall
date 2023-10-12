// Stolen from Magic Lantern source
// https://raw.githubusercontent.com/reticulatedpines/magiclantern_simplified/dev/contrib/make-bootable/exfat_sum.c

#ifndef EXFAT_H
#define EXFAT_H

#define EXFAT_VBR_SIZE (512 * 11)

static unsigned int VBRChecksum(unsigned char octets[], long NumberOfBytes)
{
	unsigned int Checksum = 0;
	int Index;
	for (Index = 0; Index < NumberOfBytes; Index++) {
		if (Index != 106 && Index != 107 &&
		    Index != 112) // skip 'volume flags' and 'percent in use'
			Checksum =
				((Checksum << 31) | (Checksum >> 1)) + (unsigned int)octets[Index];
	}
	return Checksum;
}

static unsigned int endian_swap(unsigned int x)
{
	return (x << 24) | ((x >> 8) & 0x0000FF00) | ((x << 8) & 0x00FF0000) | (x >> 24);
}

#endif
