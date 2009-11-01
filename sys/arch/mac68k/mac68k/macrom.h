/*	$NetBSD: macrom.h,v 1.19 2009/11/01 01:51:35 snj Exp $	*/

/*-
 * Copyright (C) 1994	Bradley A. Grantham
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <sys/types.h>
#include <machine/frame.h>


/* Low-memory Globals */
extern void *		ROMBase;	/* Base address of ROM space */
extern void *		ADBBase;	/* Base address of ADB scratch */
extern void *		ADBYMM; 	/* Base address of yet more ADB mem */
extern void *		ADBState;	/* Base address of ADB scratch ? */
extern void	(*JADBProc)(void); /* ADBReInit pre/post processing */
extern void	(*Lvl1DT[8])(void); /* VIA1 interrupt table by bit */
extern void	(*Lvl2DT[8])(void); /* VIA2 interrupt table by bit */
extern void	(*jADBOp)(void);	/* low memory pointer to ADBOp */
extern void	(*jUnimplTrap)(void);	/* Unimplemented trap */
	/* loglob(KbdLast, 0x218)	* addr of last kbd to send */
	/* loglob(KbdType, 0x21E)	* type of last kbd to send */
extern void	(*JKybdTask)(void); /* Keyboard task ptr? */
extern u_char		CPUFlag;	/* Type of CPU in this machine */
extern void	(*MacJmp)(void);	/* ??? */
extern u_long		Lo3Bytes;	/* 0x00ffffff */
extern u_long		MinusOne;	/* 0xffffffff */
extern u_short		MMU32Bit;	/* MMU mode; short? */
extern u_char		Scratch8[8];	/* 8-byte scratch */
extern u_char		Scratch20[20];	/* 20-byte scratch */
extern u_long		Ticks;		/* ticks since startup */
extern u_long		Time;		/* ticks since startup */
extern u_short		TimeDBRA;	/* DBRA's per milli */
extern u_short		ADBDelay;	/* DBRAs per ADB loop, / 8 */
extern u_char		ToolScratch[8]; /* Yet another 8-byte scratch area */
extern void *		VIA;		/* VIA1 base address */
extern void *		mrg_VIA2;	/* VIA2 base address */
extern void *		SCCRd;		/* SCC read base address */
extern u_char		FinderName[20]; /* FinderName - Pascal string */
extern void	(*jSwapMMU)(void); /* Pointer to MMU Swap routine */
extern void	(*jEgret)(void);	/* Pointer to MMU Swap routine */
extern u_int16_t	HwCfgFlags;	/* Hardware config flags */
extern u_int32_t	HwCfgFlags2;	/* more hardware config flags */
extern u_int32_t	HwCfgFlags3;	/* more hardware config flags */
extern u_int32_t	ADBReInit_JTBL;	/* pointer to patch table */
extern void	(*jClkNoMem)(void); /* pointer to ClkNoMem */
extern u_char		SysParam[20];	/* Place where PRam data gets stored */
extern void *		ExpandMem;	/* pointer to Expanded Memory used by */
					/*  newer ADB routines (since LCIII) */
extern u_int16_t	VBLQueue;	/* Vertical blanking Queue, unused ? */
extern void *		VBLQueue_head;	/* Vertical blanking Queue, head */
extern void *		VBLQueue_tail;	/* Vertical blanking Queue, tail */
extern void *		jDTInstall;	/* Deferred task mgr trap handler */

extern u_int32_t	**InitEgretJTVec; /* pointer to a jump table for */
					  /* InitEgret on AV machines */

	/* Types */

typedef void *Ptr;
typedef void **Handle;

/* ADB Manager */
typedef struct {
	Ptr siServiceRtPtr;
	Ptr siDataAreaAddr;
} ADBSetInfoBlock;
typedef struct {
	unsigned char	devType;
	unsigned char	origADBAddr;
	Ptr		dbServiceRtPtr;
	Ptr		dbDataAreaAddr;
} ADBDataBlock;


	/* Trap Flesh; these functions are C, not Pascal */

/* trap tests */
int	MyOwnTrap(void);
void	KnownRTS(void);

#ifdef MRG_ADB
/*
 * These functions are defined in adb_direct.c if we are not using
 * the MRG method of accessing the ADB/PRAM/RTC.
 */
/* ADB Manager */
int	SetADBInfo(ADBSetInfoBlock *, int);
int	CountADBs(void);
int	GetIndADB(ADBDataBlock *, int);
int	GetADBInfo(ADBDataBlock *, int);
void	ADBReInit(void);
	/* note different order of parameters */
int	ADBOp(Ptr, Ptr, Ptr, short);
void	ADBAlternateInit(void);
#endif

/* Memory Manager */
Ptr	NewPtr(int);
int	DisposPtr(Ptr);
int	GetPtrSize(Ptr);
int	SetPtrSize(Ptr, int);

/* Resource Manager */
Handle	GetResource(u_int, short);
short	ResError(void);
short	mrg_CountResources(u_int32_t);
short	Count_Resources(u_int32_t);
void *	*mrg_GetIndResource(u_int16_t, u_int32_t);


	/* Mac ROM Glue globals for BSD kernel */
extern void *mrg_romadbintr;
extern void *mrg_ADBIntrPtr;
extern u_char mrg_GetResource[];	/* type is almost a lie; 
					call it an array of bytes of code */
extern u_char mrg_ResError[];


	/* MacOS Error Codes */
#define noErr		0
#define memFullErr	-108
#define memWZErr	-111
#define resNotFound	-192


	/* Dump instruction trace */
void	dumptrace(void);


	/* Stuff for configuring ROM Glue */
typedef struct rsrc_s {
	u_int16_t unknown[4];	/* ???? */
	u_int32_t next;		/* pointer to next resoure in list */
	u_int32_t body;		/* pointer to resource body? */
	u_int32_t name;		/* resource name */
	u_int16_t index;	/* ???? */
} rsrc_t;

typedef struct romvec_s {
	const char *romident; 	/* just to print it out */
	void *adbintr;	/* where is ADB interrupt */
	void *pmintr; 	/* where is ADB/PM interrupt, on machines */
				/*  that have it */
	void *adb130intr;	/* ADBBase[0x130] interrupt; don't know */
				/*  what it is, but it's important.  Don't */
				/*  you love reverse engineering? */
	void *CountADBs;
	void *GetIndADB;
	void *GetADBInfo;
	void *SetADBInfo;
	void *ADBReInit;
	void *ADBOp;
	void *PMgrOp; 	/* On machines that have it */
	void *WriteParam;
	void *SetDateTime;
	void *InitUtil;
	void *ReadXPRam;
	void *WriteXPRam;
	void *jClkNoMem;
	void *ADBAlternateInit;	/* more fundamental than ABDReInit */
	void *Egret;
	void *InitEgret;	/* Set up Buffer for Egret routines */
	void *ADBReInit_JTBL;
	void *ROMResourceMap; /* Address of first Resource in linked list */
	void *FixDiv;
	void *FixMul;
} romvec_t;

/*
 * Function prototypes.
 */

/* macrom.c */
void	mrg_setvectors(romvec_t *);
int	mrg_romready(void);
void *	*Get_Ind_Resource(u_int32_t, u_int16_t);
void	mrg_initadbintr(void);
long	mrg_adbintr(void);
long	mrg_pmintr(void);
void	mrg_fixupROMBase(void *, void *);
int	mrg_Delay(void);
void	mrg_DTInstall(void);
void	mrg_execute_deferred(void);
void	mrg_VBLQueue(void);
void	mrg_init_stub_1(void);
void	mrg_init_stub_2(void);
void	mrg_1sec_timer_tick(void);
void	mrg_lvl1dtpanic(void);
void	mrg_lvl2dtpanic(void);
void	mrg_jadbprocpanic(void);
void	mrg_jswapmmupanic(void);
void	mrg_jkybdtaskpanic(void);
void	mrg_notrap(void);
int	myowntrap(void);
int	mrg_NewPtr(void);
int	mrg_DisposPtr(void);
int	mrg_GetPtrSize(void);
int	mrg_SetPtrSize(void);
int	mrg_PostEvent(void);
int	mrg_SetTrapAddress(void);
void	mrg_StripAddress(void);
void	mrg_aline_super(struct frame *);
void	mrg_init(void);
void	mrg_FixDiv(void);
void	mrg_FixMul(void);

/* machdep.c */
int	mach_cputype(void);

/* Tracing aids */

/* trace all instructions, not just flow changes. */
#define tron() \
	__asm("movw %%sr, %%d0 ; orw #0x8000, %%d0 ; movw %%d0, %%sr" : : : "d0")
#define troff() \
	__asm("movw %%sr, %%d0 ; andw #0x3fff, %%d0 ; movw %%d0, %%sr" : : : "d0")
