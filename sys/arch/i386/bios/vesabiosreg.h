/* $NetBSD: vesabiosreg.h,v 1.1.2.2 2002/07/16 08:29:07 gehenna Exp $ */

struct modeinfoblock {
	/* Mandatory information for all VBE revisions */
	u_int16_t ModeAttributes;
	u_int8_t WinAAttributes, WinBAttributes;
	u_int16_t WinGranularity, WinSize, WinASegment, WinBSegment;
	u_int32_t WinFuncPtr;
	u_int16_t BytesPerScanLine;
	/* Mandatory information for VBE 1.2 and above */
	u_int16_t XResolution, YResolution;
	u_int8_t XCharSize, YCharSize, NumberOfPlanes, BitsPerPixel;
	u_int8_t NumberOfBanks, MemoryModel, BankSize, NumberOfImagePages;
	u_int8_t Reserved1;
	/* Direct Color fields
	   (required for direct/6 and YUV/7 memory models) */
	u_int8_t RedMaskSize, RedFieldPosition;
	u_int8_t GreenMaskSize, GreenFieldPosition;
	u_int8_t BlueMaskSize, BlueFieldPosition;
	u_int8_t RsvdMaskSize, RsvdFieldPosition;
	u_int8_t DirectColorModeInfo;
	/* Mandatory information for VBE 2.0 and above */
	u_int32_t PhysBasePtr;
	u_int32_t OffScreenMemOffset;
	u_int16_t OffScreenMemSize;
	u_int8_t Reserved2[206];
} __attribute__ ((packed));
