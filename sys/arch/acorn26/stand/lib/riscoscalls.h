/*	$NetBSD: riscoscalls.h,v 1.1 2002/03/24 15:47:27 bjh21 Exp $	*/

/*-
 * Copyright (c) 2001 Ben Harris
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
 * 3. The name of the author may not be used to endorse or promote products
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

/*
 * This file defines the interface for the veneers over RISC OS SWIs used
 * by libsa.  Only those SWIs actually needed by libsa are included.  In
 * general, the interface is based on the one provided by OsLib, though
 * this implementation is independent.
 */

#define OS_WriteC		0x000000
#define XOS_WriteC		0x020000
#define OS_NewLine		0x000003
#define XOS_NewLine		0x020003
#define OS_ReadC		0x000004
#define XOS_ReadC		0x020004
#define OS_Byte			0x000006
#define XOS_Byte		0x020006
#define OS_Args			0x000009
#define XOS_Args		0x020009
#define OS_GBPB			0x00000c
#define XOS_GBPB		0x02000c
#define OS_Find			0x00000d
#define XOS_Find		0x02000d
#define OS_GetEnv		0x000010
#define XOS_GetEnv		0x020010
#define OS_Exit			0x000011
#define XOS_Exit		0x020011
#define OS_IntOff		0x000014
#define XOS_IntOff		0x020014
#define OS_EnterOS		0x000016
#define XOS_EnterOS		0x020016
#define OS_Module		0x00001e
#define XOS_Module		0x02001e
#define OS_FSControl		0x000029
#define XOS_FSControl		0x020029
#define OS_ServiceCall		0x000030
#define XOS_ServiceCall		0x020030
#define OS_ReadVduVariables	0x000031
#define XOS_ReadVduVariables	0x020031
#define OS_SWINumberFromString	0x000039
#define XOS_SWINumberFromString	0x020039
#define OS_ReadMemMapInfo	0x000051
#define XOS_ReadMemMapInfo	0x020051
#define OS_ReadMemMapEntries	0x000052
#define XOS_ReadMemMapEntries	0x020052

#define Cache_Control		0x000280
#define XCache_Control		0x020280

#ifndef __ASSEMBLER__
typedef struct os_error {
	u_int32_t errnum;
	char errmess[252];
} os_error;

/* Errors talking to the console may as well be fatal. */

extern void os_writec(int);
extern void os_new_line(void);
extern int os_readc(void);
#endif

/* OS_Byte */

#define osbyte_OUTPUT_CURSOR_POSITION	165

#ifndef __ASSEMBLER__
void os_byte(int, int, int, int *, int *);
#endif

/* OS_Args */

#define OSArgs_ReadPtr		0
#define OSArgs_SetPtr		1
#define OSArgs_ReadExt		2
#define OSArgs_SetExt		3
#define OSArgs_ReadAllocation	4
#define OSArgs_ReadEOFStatus	5

#ifndef __ASSEMBLER__
extern os_error *xosargs_read(int, int, int *);
extern os_error *xosargs_set(int, int, int);

#define xosargs_read_ptr(f, vp)	xosargs_read(OSArgs_ReadPtr, (f), (vp))
#define xosargs_set_ptr(f, v)	xosargs_set(OSArgs_SetPtr, (f), (v))
#define xosargs_read_ext(f, vp)	xosargs_read(OSArgs_ReadExt, (f), (vp))
#define xosargs_set_ext(f, v)	xosargs_set(OSArgs_SetExt, (f), (v))
#define xosargs_read_allocation(f, vp) \
				xosargs_read(OSArgs_ReadAllocation, (f), (vp))
#define xosargs_read_eof_status(f, vp) \
				xosargs_read(OSArgs_ReadEOFStatus, (f), (vp))
#endif

/* OS_GBPB */

#define OSGBPB_WriteAt		1
#define OSGBPB_Write		2
#define OSGBPB_ReadAt		3
#define OSGBPB_Read		4

#ifndef __ASSEMBLER__
extern os_error *xosgbpb_write(int, char const *, int, int *);
extern os_error *xosgbpb_read(int, char *, int, int *);
#endif

/* OS_Find */

#define OSFind_Close	0x00
#define OSFind_Openin	0x40
#define OSFind_Openout	0x80
#define OSFind_Openup	0xc0

#define osfind_PATH		0x01
#define osfind_PATH_VAR		0x02
#define osfind_NO_PATH		0x03
#define osfind_ERROR_IF_DIR	0x04
#define osfind_ERROR_IF_ABSENT	0x08

#ifndef __ASSEMBLER__
extern os_error *xosfind_close(int);
extern os_error *xosfind_open(int, char const *, char const *, int *);

extern char *os_get_env(caddr_t *, void **);

extern void os_exit(os_error const *, int) __attribute__((noreturn));

extern void os_int_off(void);

extern void os_enter_os(void);
#endif

#define OSFSControl_Shutdown	23

#ifndef __ASSEMBLER__
extern os_error *xosfscontrol_shutdown(void);
#endif

#define Service_PreReset	0x45

#ifndef __ASSEMBLER__
extern void service_pre_reset(void);
#endif

#define os_MODEVAR_LOG2_BPP		9
#define os_MODEVAR_XWIND_LIMIT		11
#define os_MODEVAR_YWIND_LIMIT		12
#define os_VDUVAR_DISPLAY_START		149
#define os_VDUVAR_TOTAL_SCREEN_SIZE	150
#define os_VDUVAR_TCHAR_SPACEY		170

#ifndef __ASSEMBLER__
extern void os_read_vdu_variables(const int *, int *);

extern os_error *xos_swi_number_from_string(char const *, int *);

extern void os_read_mem_map_info(int *, int *);

struct os_mem_map_request {
	int page_no;
	caddr_t map;
	int access;
#define os_AREA_ACCESS_READ_WRITE	0
#define os_AREA_ACCESS_READ_ONLY	1
#define os_AREA_ACCESS_NONE		3
};

extern void os_read_mem_map_entries(struct os_mem_map_request *);

extern os_error xcache_control(u_int, u_int, u_int *);

struct filecore_disc {
	u_int8_t	log2secsize;
	u_int8_t	secspertrack;
	u_int8_t	heads;
	u_int8_t	density;
	u_int8_t	idlen;
	u_int8_t	log2bpmp;
	u_int8_t	skew;
	u_int8_t	bootoption;
	u_int8_t	lowsector;
	u_int8_t	nzones;
	u_int16_t	zone_spare;
	u_int32_t	root;
	u_int32_t	disc_size;
	u_int16_t	disc_id;
	char		disc_name[10];
	u_int32_t	disc_type;
	u_int32_t	disc_size_hi;
	u_int8_t	share_size;
	u_int8_t	big_flag;
	u_int8_t	reserved[22];
};

#endif

/* RISC OS Error numbers */

#define error_IS_ADIR			0xa8
#define error_TOO_MANY_OPEN_FILES	0xc0
#define error_FILE_NOT_FOUND		0xd6

#ifndef __ASSEMBLER__
extern int riscos_errno(os_error *);
#endif
