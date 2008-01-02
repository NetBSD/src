/*	$NetBSD: pef.h,v 1.1.4.2 2008/01/02 21:49:16 bouyer Exp $	*/

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
   	unsigned long magic;
   	unsigned long fileTypeID;
   	unsigned long archID;
   	unsigned long versionNumber;
   	unsigned long dateTimeStamp;
   	unsigned long definVersion;
   	unsigned long implVersion;
   	unsigned long currentVersion;
   	unsigned short numSections;
   	unsigned short loadableSections;
   	unsigned long memoryAddress;
};

#define PEF_MAGIC 0x4A6F7921  /* Joy! */
#define PEF_FILE  0x70656666  /* peff */
#define PEF_PPC   0x70777063  /* pwpc */  

struct SectionHeader
{
   	unsigned long sectionName;
   	unsigned long sectionAddress;
   	unsigned long execSize;
   	unsigned long initSize;
   	unsigned long rawSize;
   	unsigned long fileOffset;
   	unsigned char regionKind;
   	unsigned char shareKind;
   	unsigned char alignment;
   	unsigned char _reserved;
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
   	unsigned long entryPointSection;
   	unsigned long entryPointOffset;
   	unsigned long initPointSection;
   	unsigned long initPointOffset;
   	unsigned long termPointSection;
   	unsigned long termPointOffset;
   	unsigned long numImportFiles;
   	unsigned long numImportSyms;
   	unsigned long numSections;
   	unsigned long relocationsOffset;
   	unsigned long stringsOffset;
   	unsigned long hashSlotTable;
   	unsigned long hashSlotTableSize;
   	unsigned long numExportSyms;
};
