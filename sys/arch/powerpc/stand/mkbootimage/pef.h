/*	$NetBSD: pef.h,v 1.2.18.1 2014/08/20 00:03:20 tls Exp $	*/

/*-
 * Copyright (C) 1995-1997 Gary Thomas (gdt@linuxppc.org)
 * All rights reserved.
 *
 * Structure of a PEF format file
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
 *      This product includes software developed by Gary Thomas.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

struct FileHeader
{
	uint32_t magic;
	uint32_t fileTypeID;
	uint32_t archID;
	uint32_t versionNumber;
	uint32_t dateTimeStamp;
	uint32_t definVersion;
	uint32_t implVersion;
	uint32_t currentVersion;
	uint16_t numSections;
	uint16_t loadableSections;
	uint32_t memoryAddress;
};

#define PEF_MAGIC 0x4A6F7921  /* Joy! */
#define PEF_FILE  0x70656666  /* peff */
#define PEF_PPC   0x70777063  /* pwpc */

struct SectionHeader
{
	uint32_t sectionName;
	uint32_t sectionAddress;
	uint32_t execSize;
	uint32_t initSize;
	uint32_t rawSize;
	uint32_t fileOffset;
	uint8_t regionKind;
	uint8_t shareKind;
	uint8_t alignment;
	uint8_t _reserved;
};

#define CodeSection	0
#define DataSection	1
#define PIDataSection	2
#define ConstantSection	3
#define LoaderSection	4

#define NeverShare	0
#define ContextShare	1
#define TeamShare	2
#define TaskShare	3
#define GlobalShare	4

struct LoaderHeader
{
	uint32_t entryPointSection;
	uint32_t entryPointOffset;
	uint32_t initPointSection;
	uint32_t initPointOffset;
	uint32_t termPointSection;
	uint32_t termPointOffset;
	uint32_t numImportFiles;
	uint32_t numImportSyms;
	uint32_t numSections;
	uint32_t relocationsOffset;
	uint32_t stringsOffset;
	uint32_t hashSlotTable;
	uint32_t hashSlotTableSize;
	uint32_t numExportSyms;
};
