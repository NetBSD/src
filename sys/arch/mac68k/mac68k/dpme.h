/*-
 * Copyright (C) 1993	Allen K. Briggs, Chris P. Caputo,
 *			Michael L. Finch, Bradley A. Grantham, and
 *			Lawrence A. Kesteloot
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Alice Group.
 * 4. The names of the Alice Group or any of its members may not be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE ALICE GROUP ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE ALICE GROUP BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/* parts Copyright (c) 1988 Apple Computer */

/*
	ALICE 5/19/92 BG

	Handy dandy header info.
	This is just disk partitioning info from Inside Mac V
*/


struct partmapentry{
	unsigned short pmSig;
	unsigned short pmSigPad;
	unsigned long pmMapBlkCnt;
	unsigned long pmPyPartStart;
	unsigned long pmPartBlkCnt;
	unsigned char pmPartName[32];
	unsigned char pmPartType[32];
	unsigned long pmLgDataStart;
	unsigned long pmDataCnt;
	unsigned long pmPartStatus;
	unsigned long pmLgBootStart;
	unsigned long pmBootSize;
	unsigned long pmBootLoad;
	unsigned long pmBootLoad2;
	unsigned long pmBootEntry;
	unsigned long pmBootEntry2;
	unsigned long pmBootCksum;
	char pmProcessor[16];
	unsigned char pmBootArgs[128];
	unsigned char blockpadding[248];
};

#define DPME_MAGIC	0x504d


struct altblkmap{
	int	abmSize;
	int	abmEntries;
	unsigned long	abmOffset;
};


struct blockzeroblock{
	unsigned long bzbMagic;
	unsigned char bzbCluster;
	unsigned char bzbType;
	unsigned short bzbBadBlockInode;
	unsigned short bzbFlags;
	unsigned short bzbReserved;
	unsigned long bzbCreationTime;
	unsigned long bzbMountTime;
	unsigned long bzbUMountTime;
	struct altblkmap bzbABM;
};

#define BZB_MAGIC	0xABADBABE
#define BZB_TYPEFS	1
#define BZB_TYPESWAP	3
#define BZB_ROOTFS	0x8000
#define BZB_USRFS	0x4000

/* MF */
#define PART_UNIX_TYPE	"APPLE_UNIX_SVR2"
#define PART_MAC_TYPE	"APPLE_HFS"
#define PART_SCRATCH	"APPLE_SCRATCH"

/* AKB */
#define FS_HFS		9
#define FS_SCRATCH	10
