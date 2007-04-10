/* $NetBSD: vesabiosreg.h,v 1.5.14.1 2007/04/10 13:23:00 ad Exp $ */

/*
 * Written by M. Drochner
 * Public domain.
 */

#ifndef _ARCH_I386_BIOS_VESABIOSREG_H
#define _ARCH_I386_BIOS_VESABIOSREG_H

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
	uint32_t OffScreenMemOffset;	/* reserved in VBE 3.0 and above */
	uint16_t OffScreenMemSize;	/* reserved in VBE 3.0 and above */

	/* Mandatory information for VBE 3.0 and above */
	uint16_t LinBytesPerScanLine;
	uint8_t BnkNumberOfImagePages;
	uint8_t LinNumberOfImagePages;
	uint8_t LinRedMaskSize, LinRedFieldPosition;
	uint8_t LinGreenMaskSize, LinGreenFieldPosition;
	uint8_t LinBlueMaskSize, LinBlueFieldPosition;
	uint8_t LinRsvdMaskSize, LinRsvdFieldPosition;
	uint32_t MaxPixelClock;
	uint8_t Reserved4[189];
} __attribute__ ((packed));

struct paletteentry {
	uint8_t Blue;
	uint8_t Green;
	uint8_t Red;
	uint8_t Alignment;
} __attribute__ ((packed));

#endif /* !_ARCH_I386_BIOS_VESABIOSREG_H */
