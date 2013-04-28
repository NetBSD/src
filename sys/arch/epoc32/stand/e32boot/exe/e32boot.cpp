/*	$NetBSD: e32boot.cpp,v 1.1 2013/04/28 12:11:26 kiyohara Exp $	*/
/*
 * Copyright (c) 2012, 2013 KIYOHARA Takashi
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <e32base.h>
#include <e32cons.h>
#include <e32def.h>
#include <e32hal.h>
#include <e32svr.h>	/* XXXXX */
#include <w32std.h>

#include "e32boot.h"
#include "netbsd.h"
#include "../../../include/bootinfo.h"

CConsoleBase *console;
LOCAL_C NetBSD *LoadNetBSDL(void);
LOCAL_C struct btinfo_common *CreateBootInfo(TAny *);
LOCAL_C struct btinfo_common *FindBootInfoL(struct btinfo_common *, int);
TUint SummaryBootInfoMemory(struct btinfo_common *);
LOCAL_C void E32BootL(void);

struct memmap {
	TUint address;
	TUint size;	/* KB */
};
struct memmap series5_4m[] = {{ 0xc0000000, 512 }, { 0xc0100000, 512 },
			      { 0xc0400000, 512 }, { 0xc0500000, 512 },
			      { 0xc1000000, 512 }, { 0xc1100000, 512 },
			      { 0xc1400000, 512 }, { 0xc1500000, 512 }};
struct memmap series5_8m[] = {{ 0xc0000000, 512 }, { 0xc0100000, 512 },
			      { 0xc0400000, 512 }, { 0xc0500000, 512 },
			      { 0xc1000000, 512 }, { 0xc1100000, 512 },
			      { 0xc1400000, 512 }, { 0xc1500000, 512 },
			      { 0xd0000000, 512 }, { 0xd0100000, 512 },
			      { 0xd0400000, 512 }, { 0xd0500000, 512 },
			      { 0xd1000000, 512 }, { 0xd1100000, 512 },
			      { 0xd1400000, 512 }, { 0xd1500000, 512 }};
struct memmap revo[] = {{ 0xc0000000, 4096 }, { 0xc0800000, 4096 }};
struct memmap revopuls[] = {{ 0xc0000000, 4096 }, { 0xc0800000, 4096 },
			    { 0xd0000000, 4096 }, { 0xd0800000, 4096 }};
struct memmap series5mx_16m[] = {{ 0xc0000000, 8192 }, { 0xc1000000, 8192 }};
struct memmap series5mxpro_24m[] = {{ 0xc0000000, 8192 }, { 0xc1000000, 8192 },
				    { 0xd0000000, 4096 }, { 0xd0800000, 4096 }};
struct memmap series5mxpro_32m[] = {{ 0xc0000000, 8192 }, { 0xc1000000, 8192 },
				    { 0xd0000000, 8192 }, { 0xd1000000, 8192 }};
struct memmap series7_16m[] = {{ 0xc0000000, 16384 }};
struct memmap series7_32m[] = {{ 0xc0000000, 16384 }, { 0xc8000000, 16384 }};

struct {
	char *model;
	TInt width;
	TInt height;
	TUint memsize;
	struct memmap *memmaps;
} memmaps[] = {
	{ "SERIES5 R1",	640, 240,  4096, series5_4m },
	{ "SERIES5 R1",	640, 240,  8192, series5_8m },
	{ "SERIES5 R1",	640, 320,  4096, series5_4m },	/* Geofox One */
	{ "SERIES5 R1",	640, 320,  8192, series5_8m },	/* Geofox One */
//	{ "SERIES5 R1",	640, 320, 16384, one_16m },
	{ "SERIES5 R1",	320, 200,  4096, series5_4m },	/* Osaris */
//	{ "SERIES5 R1",	320, 200, 16384, osaris_16m },
	{ "SERIES5mx",	480, 160,  8192, revo },
	{ "SERIES5mx",	480, 160, 16384, revopuls },
	{ "SERIES5mx",	640, 240, 16384, series5mx_16m },
	{ "SERIES5mx",	640, 240, 24576, series5mxpro_24m },
	{ "SERIES5mx",	640, 240, 32768, series5mxpro_32m },
	{ "SERIES7",	800, 600, 16384, series7_16m },
	{ "SERIES7",	800, 600, 32768, series7_32m },
};

class E32BootLogicalChannel : public RLogicalChannel {
public:
	TInt DoCreate(const TDesC *aChan, TInt aUnit, const TDesC *aDriver,
							const TDesC8 *anInfo)
	{

		return RLogicalChannel::DoCreate(E32BootName, TVersion(0, 0, 0),
						aChan, aUnit, aDriver, anInfo);
	}

	TInt DoControl(TInt aFunction, TAny *a1)
	{

		return RLogicalChannel::DoControl(aFunction, a1);
	}

	TInt DoControl(TInt aFunction, TAny *a1, TAny *a2)
	{

		return RLogicalChannel::DoControl(aFunction, a1, a2);
	}
};


LOCAL_C void
E32BootL(void)
{
	E32BootLogicalChannel *E32BootChannel = new E32BootLogicalChannel;
	NetBSD *netbsd = NULL;
	TScreenInfoV01 screenInfo;
	TPckg<TScreenInfoV01> sI(screenInfo);
	TBuf<32> ldd;
	TInt err;
	TUint membytes;
	TAny *buf, *safeAddress;
	struct btinfo_common *bootinfo;
	struct btinfo_model *model;
	struct btinfo_video *video;

	console =
	    Console::NewL(E32BootName, TSize(KConsFullScreen, KConsFullScreen));

	buf = User::AllocL(ALIGN_SAFE_PAGE_SIZE);	/* bootinfo buffer */

	/* Put banner */
	console->Printf(_L("\n"));
	console->Printf(_L(">> %s, Revision %s\n"),
	    bootprog_name, bootprog_rev);

	UserSvr::ScreenInfo(sI);
	if (!screenInfo.iScreenAddressValid)
		User::Leave(KErrNotSupported);
	safeAddress = screenInfo.iScreenAddress;

	bootinfo = CreateBootInfo((TAny *)PAGE_ALIGN(buf));

	model = (struct btinfo_model *)FindBootInfoL(bootinfo, BTINFO_MODEL);
	console->Printf(_L(">> Model %s\n"), model->model);

	membytes = SummaryBootInfoMemory(bootinfo);
	console->Printf(_L(">> Memory %d k\n"), membytes / 1024);

	video = (struct btinfo_video *)FindBootInfoL(bootinfo, BTINFO_VIDEO);
	console->Printf(_L(">> Video %d x %d\n"), video->width, video->height);

	console->Printf(_L("\n"));

	TRAP(err, netbsd = LoadNetBSDL());
	if (err != KErrNone)
		User::Leave(err);
	else if (netbsd == NULL)
		return;
	console->Printf(_L("\nLoaded\n"));

	netbsd->ParseHeader();

	/* Load logical device(kernel part of e32boot). */
	if (_L(model->model).CompareF(_L("SERIES5 R1")) == 0)
		ldd = _L("e32boot-s5.ldd");
	else if (_L(model->model).CompareF(_L("SERIES5mx")) == 0)
		ldd = _L("e32boot-s5mx.ldd");
//	else if (_L(model->model).CompareF(_L("SERIES7")) == 0)
//		ldd = _L("e32boot-s7.ldd");	// not yet.
	else {
		console->Printf(_L("Not Supported machine\n"));
		console->Getch();
		User::Leave(KErrNotSupported);
	}
	err = User::LoadLogicalDevice(ldd);
	if (err != KErrNone && err != KErrAlreadyExists) {
		console->Printf(_L("LoadLogicalDevice failed: %d\n"), err);
		console->Getch();
		User::Leave(err);
	}
	/* Create channel to kernel part. */
	err = E32BootChannel->DoCreate(NULL, KNullUnit, NULL, NULL);
	if (err == KErrNone) {
		E32BootChannel->DoControl(KE32BootSetSafeAddress, safeAddress);
		E32BootChannel->DoControl(KE32BootBootNetBSD, netbsd, bootinfo);
	} else {
		console->Printf(_L("DoCreate failed: %d\n"), err);
		console->Getch();
	}

	User::FreeLogicalDevice(ldd);
	if (err != KErrNone)
		User::Leave(err);
}

GLDEF_C TInt E32Main(void)	/* main function called by E32 */
{

	__UHEAP_MARK;
	CTrapCleanup *cleanup = CTrapCleanup::New();

	TRAPD(error, E32BootL());
	__ASSERT_ALWAYS(!error, User::Panic(E32BootName, error));

	delete cleanup;
	__UHEAP_MARKEND;
	return 0;
}

LOCAL_C NetBSD *
LoadNetBSDL(void)
{
	NetBSD *netbsd = NULL;
	TBuf<KMaxCommandLine> input;
	TPtrC Default = _L("C:\\netbsd");
	TPtrC Prompt = _L("Boot: ");
	TInt pos, err;
	TBool retry;

	input.Zero();
	retry = false;
	console->Printf(Prompt);
	console->Printf(_L("["));
	console->Printf(Default);
	console->Printf(_L("]: "));
	console->SetPos(Prompt.Length() +
			_L("[").Length() +
			Default.Length() +
			_L("]: ").Length());
	pos = 0;
	while (1) {
		TChar gChar = console->Getch();
		switch (gChar) {
		case EKeyEscape:
			return NULL;

		case EKeyEnter:
			break;

		case EKeyBackspace:
			if (pos > 0) {
				pos--;
				input.Delete(pos, 1);
			}
			break;

		default:
			if (gChar.IsPrint()) {
				if (input.Length() < KMaxCommandLine) {
					TBuf<0x02> b;
					b.Append(gChar);
					input.Insert(pos++, b);
				}
			}
			break;
		}
		if (gChar == EKeyEnter) {
			if (input.Length() > 0) {
				TBufC<KMaxCommandLine> kernel =
				    TBufC<KMaxCommandLine>(input);

				TRAP(err, netbsd = NetBSD::New(kernel));
			} else {
				TRAP(err, netbsd = NetBSD::New(Default));
			}
			if (err == 0 && netbsd != NULL)
				break;
			console->Printf(_L("\nLoad failed: %d\n"), err);

			input.Zero();
			console->Printf(Prompt);
			pos = 0;
			retry = true;
		}
		TInt base = Prompt.Length();
		if (!retry)
			base += (_L("[").Length() + Default.Length() +
							_L("]: ").Length());
		console->SetPos(base + pos);
		console->ClearToEndOfLine();
		console->SetPos(base);
		console->Write(input);
		console->SetPos(base + pos);
	}

	return netbsd;
}

#define KB	* 1024

LOCAL_C struct btinfo_common *
CreateBootInfo(TAny *buf)
{
	TMachineInfoV1Buf MachInfo;
	TMemoryInfoV1Buf MemInfo;
	struct btinfo_common *bootinfo, *common;
	struct btinfo_model *model;
	struct btinfo_memory *memory;
	struct btinfo_video *video;
	struct memmap *memmap;
	TUint memsize;
	TUint i;

	UserHal::MachineInfo(MachInfo);
	UserHal::MemoryInfo(MemInfo);

	common = bootinfo = (struct btinfo_common *)buf;

	/* Set machine name to bootinfo. */
	common->len = sizeof(struct btinfo_model);
	common->type = BTINFO_MODEL;
	model = (struct btinfo_model *)common;
	Mem::Copy(model->model, &MachInfo().iMachineName[0],
	    sizeof(model->model));
	common = &(model + 1)->common;

	/* Set video width/height to bootinfo. */
	common->len = sizeof(struct btinfo_video);
	common->type = BTINFO_VIDEO;
	video = (struct btinfo_video *)common;
	video->width = MachInfo().iDisplaySizeInPixels.iWidth;
	video->height = MachInfo().iDisplaySizeInPixels.iHeight;
	common = &(video + 1)->common;

	/* Set memory size to bootinfo. */
	memsize = MemInfo().iTotalRamInBytes / 1024;
	for (i = 0; i < sizeof(memmaps) / sizeof(memmaps[0]); i++) {
		if (_L(memmaps[i].model).CompareF(_L(model->model)) == 0 &&
		    memmaps[i].width == video->width &&
		    memmaps[i].height == video->height &&
		    memmaps[i].memsize == memsize) {
			memmap = memmaps[i].memmaps;
			while (memsize > 0) {
				common->len = sizeof(struct btinfo_memory);
				common->type = BTINFO_MEMORY;
				memory = (struct btinfo_memory *)common;
				memory->address = memmap->address;
				memory->size = memmap->size KB;
				common = &(memory + 1)->common;
				memsize -= memmap->size;
				memmap++;
			}
			break;
		}
	}
	if (i == sizeof(memmaps) / sizeof(memmaps[0])) {
		common->len = sizeof(struct btinfo_memory);
		common->type = BTINFO_MEMORY;
		memory = (struct btinfo_memory *)common;
		memory->address = 0xc0000000;		/* default is here */
		memory->size = 4096 KB;			/* XXXXX */
		common = &(memory + 1)->common;
	}

	common->len = 0;
	common->type = BTINFO_NONE;

	/* Terminate bootinfo. */
	return bootinfo;
}

#undef KB

LOCAL_C struct btinfo_common *
FindBootInfoL(struct btinfo_common *bootinfo, int type)
{
	struct btinfo_common *entry;

	entry = bootinfo;
	while (entry->type != BTINFO_NONE) {
		if (entry->type == type)
			return entry;
		entry = (struct btinfo_common *)((int)entry + entry->len);
	}
	User::Leave(KErrNotFound);

	/* NOTREACHED */

	return NULL;
}

TUint
SummaryBootInfoMemory(struct btinfo_common *bootinfo)
{
	struct btinfo_common *entry;
	struct btinfo_memory *memory;
	TUint memsize = 0;

	entry = bootinfo;
	while (entry->type != BTINFO_NONE) {
		if (entry->type == BTINFO_MEMORY) {
			memory = (struct btinfo_memory *)entry;
			memsize += memory->size;
		}
		entry = (struct btinfo_common *)((int)entry + entry->len);
	}
	return memsize;
}
