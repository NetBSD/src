/*	$NetBSD: pbsdboot.h,v 1.10.78.1 2007/03/12 05:48:14 rmind Exp $	*/

/*-
 * Copyright (c) 1999 Shin Takemura.
 * All rights reserved.
 *
 * This software is part of the PocketBSD.
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
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
#define STANDALONE_WINDOWS_SIDE
#include <stand.h>
#include <machine/bootinfo.h>
#include <machine/platid.h>

extern TCHAR szAppName[ ];
#define whoami szAppName
#define PREFNAME TEXT("pbsdboot.ini")
#define LOGNAME TEXT("pbsdboot.log")
#define PATHBUFLEN 200


/*
 *  For some reason, we can't include Windows' header files.
 *  So we must declare here.
 */
double wcstod(const wchar_t *, wchar_t **);
long wcstol(const wchar_t *, wchar_t **, int);
unsigned long wcstoul(const wchar_t *, wchar_t **, int);
BOOL VirtualCopy(LPVOID, LPVOID, DWORD, DWORD);

/*
 *  structure declarations
 */
struct map_s {
	void *entry;
	void *base;
	int pagesize;
	int leafsize;
	int nleaves;
	void *arg0;
	void *arg1;
	void *arg2;
	void *arg3;
	void **leaf[32];
};

struct preference_s {
	int setting_idx;
	int fb_type;
	int fb_width, fb_height, fb_linebytes;
	int boot_time;
	long fb_addr;
	unsigned long platid_cpu, platid_machine;
	TCHAR setting_name[PATHBUFLEN];
	TCHAR kernel_name[PATHBUFLEN];
	TCHAR options[PATHBUFLEN];
	BOOL check_last_chance;
	BOOL load_debug_info;
	BOOL serial_port;
	BOOL reverse_video;
	BOOL autoboot;	
};

struct path_s {
	TCHAR* name;
	LANGID	langid;
	unsigned long flags;
#define PATH_SAVE	1
};

/*
 * Machine dependent information
 */
struct system_info {
	unsigned int si_dramstart;
	unsigned int si_drammaxsize;
	DWORD si_pagesize;
	unsigned char *si_asmcode;
	int si_asmcodelen;
	int (*si_boot) __P((void *));
	int si_intrvec;
};
extern struct system_info system_info;

extern struct preference_s pref;
extern TCHAR* where_pref_load_from;

/*
 *  main.c
 */
BOOL CheckCancel(int progress);
extern HWND hDlgMain;

/*
 *  layout.c
 */
int CreateMainWindow(HINSTANCE hInstance, HWND hWnd, LPCTSTR name, int cmdbar_height);

/*
 *  vmem.c
 */
int vmem_exec(void *entry, int argc, char *argv[], struct bootinfo *bi);
void *vmem_get(void *phys_addr, int *length);
int vmem_init(void *start, void *end);
void vmem_dump_map(void);
void *vtophysaddr(void *page);
void vmem_free(void);
void *vmem_alloc(void);

/*
 *  elf.c
 */
int getinfo(int fd, void **start, void **end);
int loadfile(int fd, void **entry);

/*
 *  mips.c
 */
int mips_boot(void *map);

/*
 *  pbsdboot.c
 */
int pbsdboot(TCHAR*, int argc, char *argv[], struct bootinfo *bi);

/*
 *  print.c
 */
int debug_printf(LPWSTR lpszFmt, ...);
int msg_printf(UINT type, LPWSTR caption, LPWSTR lpszFmt, ...);
int stat_printf(LPWSTR lpszFmt, ...);
int set_debug_log(TCHAR* path);
void close_debug_log(void);


#define	MSG_ERROR	(MB_OK | MB_ICONERROR)
#define MSG_INFO	(MB_OK | MB_ICONINFORMATION)

/*
 *  disptest.c
 */
void hardware_test(void);


/*
 *  preference.c
 */
void pref_init(struct preference_s* pref);
void pref_dump(struct preference_s* pref);
int pref_read(TCHAR* filename, struct preference_s* pref);
int pref_load(struct path_s load_path[], int pathlen);
int pref_save(struct path_s load_path[], int pathlen);
int pref_write(TCHAR* filename, struct preference_s* buf);


/*
 *  systeminfo.c
 */
int set_system_info(platid_t* platid);


/*
 *  palette.c
 */
enum palette_status { PAL_ERROR, PAL_NOERROR, PAL_SUCCEEDED };
extern enum palette_status palette_succeeded;
void palette_init(HWND hWnd);
void palette_set(HWND hWnd);
void palette_check(HWND hWnd);

/*
 *  vr41xx.c
 */
void vr41xx_init(SYSTEM_INFO *info);


/*
 *  tx39xx.c
 */
void tx39xx_init(SYSTEM_INFO *info);
