/* $NetBSD: vesabiosreg.h,v 1.2 2005/12/26 19:23:59 perry Exp $ */

struct modeinfoblock {
	/* Mandatory information for all VBE revisions */
	uint16_t ModeAttributes;
	uint8_t WinAAttributes, WinBAttributes;
	uint16_t WinGranularity, WinSize, WinASegment, WinBSegment;
	uint32_t WinFuncPtr;
	uint16_t BytesPerScanLine;
	/* Mandatory information for VBE 1.2 and above */
	uint16_t XResolution, YResolution;
	uint8_t XCharSize, YCharSize, NumberOfPlanes, BitsPerPixel;
	uint8_t NumberOfBanks, MemoryModel, BankSize, NumberOfImagePages;
	uint8_t Reserved1;
	/* Direct Color fields
	   (required for direct/6 and YUV/7 memory models) */
	uint8_t RedMaskSize, RedFieldPosition;
	uint8_t GreenMaskSize, GreenFieldPosition;
	uint8_t BlueMaskSize, BlueFieldPosition;
	uint8_t RsvdMaskSize, RsvdFieldPosition;
	uint8_t DirectColorModeInfo;
	/* Mandatory information for VBE 2.0 and above */
	uint32_t PhysBasePtr;
	uint32_t OffScreenMemOffset;
	uint16_t OffScreenMemSize;
	uint8_t Reserved2[206];
} __attribute__ ((packed));
