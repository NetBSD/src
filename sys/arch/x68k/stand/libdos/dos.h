/*
 *	dos.h
 *	Human68k DOS call interface
 *
 *	written by Yasha (ITOH Yasufumi)
 *	(based on PD libc 1.1.32 by PROJECT C Library)
 *	public domain
 *
 *	$NetBSD: dos.h,v 1.5.24.1 2009/05/13 17:18:43 jym Exp $
 */
/*
 * PROJECT C Library, X68000 PROGRAMMING INTERFACE DEFINITION
 * --------------------------------------------------------------------
 * This file is written by the Project C Library Group,  and completely
 * in public domain. You can freely use, copy, modify, and redistribute
 * the whole contents, without this notice.
 * --------------------------------------------------------------------
 * Id: dos.h,v 1.6 1994/07/27 13:44:11 mura Exp
 * Id: dos_p.h,v 1.3 1993/10/06 16:46:07 mura Exp
 */

#ifndef __X68K_DOS_H__
#define __X68K_DOS_H__

#include <sys/cdefs.h>

typedef int	dos_mode_t;

struct dos_inpptr {
	unsigned char	max;
	unsigned char	length;
	char		buffer[256];
};

struct dos_nameckbuf {
	char	drive[2];
	char	path[65];
	char	name[19];
	char	ext[5];
};

union dos_fcb {
	struct {
		unsigned char	dupcnt;
		unsigned char	devattr;
		void		*deventry;
		char		nouse_1[8];
		unsigned char	openmode;
		char		nouse_2[21];
		char		name1[8];
		char		ext[3];
		char		nouse_3;
		char		name2[10];
		char		nouse_4[38];
	} __attribute__((__packed__)) chr;
	struct {
		unsigned char	dupcnt;
		unsigned	mode	: 1;
		unsigned	unknown : 2;
		unsigned	physdrv : 5;
		void		*deventry;
		unsigned int	fileptr;
		unsigned int	exclptr;
		unsigned char	openmode;
		unsigned char	entryidx;
		unsigned char	clustidx;
		char		nouse_2;
		unsigned short	acluster;
		unsigned int	asector;
		void		*iobuf;
		unsigned long	dirsec;
		unsigned int	fptrmax;
		char		name1[8];
		char		ext[3];
		unsigned char	attr;
		char		name2[10];
		unsigned short	time;
		unsigned short	date;
		unsigned short	fatno;
		unsigned long	size;
		char		nouse_4[28];
	} __attribute__((__packed__)) blk;
};

struct dos_indos {
	unsigned short	indosf;
	unsigned char	doscmd;
	unsigned char	fat_flg;
	unsigned short	retry_count;
	unsigned short	retry_time;
	unsigned short	verifyf;
	unsigned char	breakf;
	unsigned char	ctrlpf;
	unsigned char	reserved;
	unsigned char	wkcurdrv;
};

struct dos_mep {
	void	*prev_mp;
	void	*parent_mp;
	void	*block_end;
	void	*next_mp;
};

struct dos_psp {
	char		*env;
	void		*exit;
	void		*ctrlc;
	void		*errexit;
	char		*comline;
	unsigned char	handle[12];
	void		*bss;
	void		*heap;
	void		*stack;
	void		*usp;
	void		*ssp;
	unsigned short	sr;
	unsigned short	abort_sr;
	void		*abort_ssp;
	void		*trap10;
	void		*trap11;
	void		*trap12;
	void		*trap13;
	void		*trap14;
	unsigned int	osflg;
	unsigned char	reserve_1[28];
	char		exe_path[68];
	char		exe_name[24];
	char		reserve_2[36];
};

struct dos_comline {
	unsigned char	len;
	char		buffer[255];
};

struct dos_namestbuf {
	unsigned char	flg;
	unsigned char	drive;
	char		path[65];
	char		name1[8];
	char		ext[3];
	char		name2[10];
};

struct dos_freeinf {
	unsigned short	free;
	unsigned short	max;
	unsigned short	sec;
	unsigned short	byte;
};

struct dos_dpbptr {
	unsigned char	drive;
	unsigned char	unit;
	unsigned short	byte;
	unsigned char	sec;
	unsigned char	shift;
	unsigned short	fatsec;
	unsigned char	fatcount;
	unsigned char	fatlen;
	unsigned short	dircount;
	unsigned short	datasec;
	unsigned short	maxfat;
	unsigned short	dirsec;
	int		driver;
	unsigned char	ide;
	unsigned char	flg;
	struct dos_dpbptr *next;
	unsigned short	dirfat;
	char		dirbuf[64];
} __attribute__((__packed__));

struct dos_filbuf {
	unsigned char	searchatr;
	unsigned char	driveno;
	unsigned long	dirsec;
	unsigned short	dirlft;
	unsigned short	dirpos;
	char		filename[8];
	char		ext[3];
	unsigned char	atr;
	unsigned short	time;
	unsigned short	date;
	unsigned int	filelen;
	char		name[23];
} __attribute__((__packed__));

struct dos_exfilbuf {
	unsigned char	searchatr;
	unsigned char	driveno;
	unsigned long	dirsec;
	unsigned short	dirlft;
	unsigned short	dirpos;
	char		filename[8];
	char		ext[3];
	unsigned char	atr;
	unsigned short	time;
	unsigned short	date;
	unsigned int	filelen;
	char		name[23];
	char		drive[2];
	char		path[65];
	char		unused[21];
} __attribute__((__packed__));

struct dos_dregs {
	int	d0;
	int	d1;
	int	d2;
	int	d3;
	int	d4;
	int	d5;
	int	d6;
	int	d7;
	int	a0;
	int	a1;
	int	a2;
	int	a3;
	int	a4;
	int	a5;
	int	a6;
};

struct dos_prcctrl {
	long		length;
	unsigned char	*buf_ptr;
	unsigned short	command;
	unsigned short	your_id;
};

struct dos_prcptr {
	struct dos_prcptr *next_ptr;
	unsigned char	wait_flg;
	unsigned char	counter;
	unsigned char	max_counter;
	unsigned char	doscmd;
	unsigned int	psp_id;
	unsigned int	usp_reg;
	unsigned int	d_reg[8];
	unsigned int	a_reg[7];
	unsigned short	sr_reg;
	unsigned int	pc_reg;
	unsigned int	ssp_reg;
	unsigned short	indosf;
	unsigned int	indosp;
	struct dos_prcctrl *buf_ptr;
	unsigned char	name[16];
	long		wait_time;
} __attribute__((__packed__));

/*
 * arguments:
 *	l		32bit arg -> 32bit DOS arg
 *	w		32bit arg -> lower 16bit DOS arg
 *	lb31		32bit arg -> 32bit (arg | 0x80000000) DOS arg
 *	wb8		32bit arg -> 16bit (arg | 0x100) DOS arg
 *	wb15		32bit arg -> 16bit (arg | 0x8000) DOS arg
 *	(num).l		(no C arg)-> 32bit (num) DOS arg
 *	(num).w		(no C arg)-> 16bit (num) DOS arg
 *	drvctrl		special arg for DOS_DRVCTRL
 *	super		special arg for DOS_SUPER
 * opts:
 *	e		d0 < 0  is an error code
 *	estrct		d0 > 0xffffff00  (unsigned) is an error code
 *	ealloc		error handling of memory allocation
 *	ep		error handling of process operation
 *	sv		save and restore  d2-d7/a2-a6
 *	noret		the DOS call never returns
 *	alias NAME	specify another entry name
 *	super		special for DOS_SUPER
 *	super_jsr	special for DOS_SUPER_JSR
 */

/* ff00 ; noret */	__dead void DOS_EXIT (void);
/* ff01 */		int DOS_GETCHAR (void);
/* ff02 w */		void DOS_PUTCHAR (int);
/* ff03 */		int DOS_COMINP (void);
/* ff04 w */		void DOS_COMOUT (int);
/* ff05 w */		void DOS_PRNOUT (int);
/* ff06 w */		int DOS_INPOUT (int);
/* ff07 */		int DOS_INKEY (void);
/* ff08 */		int DOS_GETC (void);
/* ff09 l */		void DOS_PRINT (const char *);
/* ff0a l */		int DOS_GETS (struct dos_inpptr *);
/* ff0b */		int DOS_KEYSNS (void);
/* ff0c 1.w */		int DOS_KFLUSHGP (void);
/* ff0c 6.w w */	int DOS_KFLUSHIO (int);
/* ff0c 7.w */		int DOS_KFLUSHIN (void);
/* ff0c 8.w */		int DOS_KFLUSHGC (void);
/* ff0c 10.w l */	int DOS_KFLUSHGS (struct dos_inpptr *);
/* ff0d */		void DOS_FFLUSH (void);
/* ff0e w */		int DOS_CHGDRV (int);
/* ff0f drvctrl */	int DOS_DRVCTRL (int, int);
/* ff10 */		int DOS_CONSNS (void);
/* ff11 */		int DOS_PRNSNS (void);
/* ff12 */		int DOS_CINSNS (void);
/* ff13 */		int DOS_COUTSNS (void);
/* ff17 l l ; e */	int DOS_FATCHK (const char *, unsigned short *);
/* ff17 l lb31 w ; e */	int DOS_FATCHK2 (const char *, unsigned short *, int);
/* ff18 0.w */		int DOS_HENDSPMO (void);
/* ff18 1.w w l */	int DOS_HENDSPMP (int, const char *);
/* ff18 2.w w l */	int DOS_HENDSPMR (int, const char *);
/* ff18 3.w */		void DOS_HENDSPMC (void);
/* ff18 4.w */		int DOS_HENDSPIO (void);
/* ff18 5.w w l */	int DOS_HENDSPIP (int, const char *);
/* ff18 6.w w l */	int DOS_HENDSPIR (int, const char *);
/* ff18 7.w w */	void DOS_HENDSPIC (int);
/* ff18 8.w */		int DOS_HENDSPSO (void);
/* ff18 9.w w l */	int DOS_HENDSPSP (int, const char *);
/* ff18 10.w w l */	int DOS_HENDSPSR (int, const char *);
/* ff18 11.w */		void DOS_HENDSPSC (void);
/* ff19 */		int DOS_CURDRV (void);
/* ff1a l */		int DOS_GETSS (struct dos_inpptr *);
/* ff1b w */		int DOS_FGETC (int);
/* ff1c l w */		int DOS_FGETS (struct dos_inpptr *, int);
/* ff1d w w */		void DOS_FPUTC (int, int);
/* ff1e l w */		void DOS_FPUTS (const char *, int);
/* ff1f */		void DOS_ALLCLOSE (void);
/* ff20 super ; super e */	int DOS_SUPER (int);
/* ff21 w l ; e */	void DOS_FNCKEYGT (int, char *);
/* ff21 wb8 l ; e */	void DOS_FNCKEYST (int, const char *);
/* ff23 0.w w */	int DOS_C_PUTC (int);
/* ff23 1.w l */	int DOS_C_PRINT (const char *);
/* ff23 2.w w */	int DOS_C_COLOR (int);
/* ff23 3.w w w */	int DOS_C_LOCATE (int, int);
/* ff23 4.w */		int DOS_C_DOWN_S (void);
/* ff23 5.w */		int DOS_C_UP_S (void);
/* ff23 6.w w */	int DOS_C_UP (int);
/* ff23 7.w w */	int DOS_C_DOWN (int);
/* ff23 8.w w */	int DOS_C_RIGHT (int);
/* ff23 9.w w */	int DOS_C_LEFT (int);
/* ff23 10.w 0.w */	int DOS_C_CLS_ED (void);
/* ff23 10.w 1.w */	int DOS_C_CLS_ST (void);
/* ff23 10.w 2.w */	int DOS_C_CLS_AL (void);
/* ff23 11.w 0.w */	int DOS_C_ERA_ED (void);
/* ff23 11.w 1.w */	int DOS_C_ERA_ST (void);
/* ff23 11.w 2.w */	int DOS_C_ERA_AL (void);
/* ff23 12.w w */	int DOS_C_INS (int);
/* ff23 13.w w */	int DOS_C_DEL (int);
/* ff23 14.w w */	int DOS_C_FNKMOD (int);
/* ff23 15.w w w */	int DOS_C_WINDOW (int, int);
/* ff23 16.w w */	int DOS_C_WIDTH (int);
/* ff23 17.w */		int DOS_C_CURON (void);
/* ff23 18.w */		int DOS_C_CUROFF (void);
/* ff24 0.w */		int DOS_K_KEYINP (void);
/* ff24 1.w */		int DOS_K_KEYSNS (void);
/* ff24 2.w */		int DOS_K_SFTSNS (void);
/* ff24 3.w w */	int DOS_K_KEYBIT (int);
/* ff24 4.w w */	void DOS_K_INSMOD (int);
/* ff25 w l */		void *DOS_INTVCS (int, void *);
/* ff26 l */		void DOS_PSPSET (struct dos_psp *);
/* ff27 */		int DOS_GETTIM2 (void);
/* ff28 l ; e */	int DOS_SETTIM2 (int);
/* ff29 l l ; e */	int DOS_NAMESTS (const char *, struct dos_namestbuf *);
/* ff2a */		int DOS_GETDATE (void);
/* ff2b w ; e */	int DOS_SETDATE (int);
/* ff2c */		int DOS_GETTIME (void);
/* ff2d w ; e */	int DOS_SETTIME (int);
/* ff2e w */		void DOS_VERIFY (int);
/* ff2f w w ; e */	int DOS_DUP0 (int, int);
/* ff30 */		int __pure DOS_VERNUM (void);
/* ff31 l w ; noret */	__dead void DOS_KEEPPR (int, int);
/* ff32 w l ; e */	int DOS_GETDPB (int, struct dos_dpbptr *);
/* ff33 w */		int DOS_BREAKCK (int);
/* ff34 w w ; e */	void DOS_DRVXCHG (int, int);
/* ff35 w */		void *DOS_INTVCG (int);
/* ff36 w l ; estrct */	int DOS_DSKFRE (int, struct dos_freeinf *);
/* ff37 l l ; e */	int DOS_NAMECK (const char *, struct dos_nameckbuf *);
/* ff39 l ; e */	int DOS_MKDIR (const char *);
/* ff3a l ; e */	int DOS_RMDIR (const char *);
/* ff3b l ; e */	int DOS_CHDIR (const char *);
/* ff3c l w ; e */	int DOS_CREATE (const char *, dos_mode_t);
/* ff3d l w ; e */	int DOS_OPEN (const char *, int);
/* ff3e w ; e */	int DOS_CLOSE (int);
/* ff3f w l l ; e */	int DOS_READ (int, char *, int);
/* ff40 w l l ; e */	int DOS_WRITE (int, const char *, int);
/* ff41 l ; e */	int DOS_DELETE (const char *);
/* ff42 w l w ; e */	long DOS_SEEK (int, int, int);
/* ff43 l w ; e */	dos_mode_t DOS_CHMOD (const char *, dos_mode_t);
/* ff44 0.w w ; e */	int DOS_IOCTRLGT (int);
/* ff44 1.w w w ; e */	int DOS_IOCTRLST (int, int);
/* ff44 2.w w l l ; e */	int DOS_IOCTRLRH (int, char *, int);
/* ff44 3.w w l l ; e */	int DOS_IOCTRLWH (int, const char *, int);
/* ff44 4.w w l l ; e */	int DOS_IOCTRLRD (int, char *, int);
/* ff44 5.w w l l ; e */	int DOS_IOCTRLWD (int, const char *, int);
/* ff44 6.w w ; e */	int DOS_IOCTRLIS (int);
/* ff44 7.w w ; e */	int DOS_IOCTRLOS (int);
/* ff44 9.w w ; e */	int DOS_IOCTRLDVGT (int);
/* ff44 10.w w ; e */	int DOS_IOCTRLFDGT (int);
/* ff44 11.w w w ; e */	int DOS_IOCTRLRTSET (int, int);
/* ff44 12.w w w l ; e */	int DOS_IOCTRLDVCTL (int, int, char *);
/* ff44 13.w w w l ; e */	int DOS_IOCTRLFDCTL (int, int, char *);
/* ff45 w ; e */	int DOS_DUP (int);
/* ff46 w w ; e */	int DOS_DUP2 (int, int);
/* ff47 w l ; e */	int DOS_CURDIR (int, char *);
/* ff48 l ; ealloc */	void *DOS_MALLOC (int);
/* ff49 l ; e */	int DOS_MFREE (void *);
/* ff4a l l ; ealloc */	int DOS_SETBLOCK (void *, int);
/* ff4b 0.w l l l ; sv e */	int DOS_LOADEXEC (const char *, const struct dos_comline *, const char *);
/* ff4b 1.w l l l ; sv e */	int DOS_LOAD (const char *, const struct dos_comline *, const char *);
/* ff4b 2.w l l l ; e */	int DOS_PATHCHK (const char *, const struct dos_comline *, const char *);
/* ff4b 3.w l l l ; e */	int DOS_LOADONLY (const char *, const void *, const void *);
/* ff4b 4.w l ; sv e */		int DOS_EXECONLY (void *);
/* ff4b 5.w l l 0.l ; sv e */	int DOS_BINDNO (const char *, const char *);
		/*^ 0.l is required?? */
/* ff4b w l l l ; sv e */	int DOS_EXEC2 (int, const char *, const char *, const char *);
/* ff4c w ; noret */	__dead void DOS_EXIT2 (int);
/* ff4d */		int DOS_WAIT (void);
/* ff4e l l w ; e */	int DOS_FILES (struct dos_filbuf *, const char *, int);
/* ff4e lb31 l w ; e */	int DOS_EXFILES (struct dos_exfilbuf *, const char *, int);
/* ff4f l ; e */	int DOS_NFILES (struct dos_filbuf *);
/* ff4f lb31 ; e */	int DOS_EXNFILES (struct dos_exfilbuf *);
/* ff80 l */		struct dos_psp *DOS_SETPDB (struct dos_psp *);
/* ff81 */		struct dos_psp *DOS_GETPDB (void);
/* ff82 l l l ; e */	int DOS_SETENV (const char *, const char *, const char *);
/* ff83 l l l ; e */	int DOS_GETENV (const char *, const char *, char *);
/* ff84 */		int DOS_VERIFYG (void);
/* ff85 0.w l ; e */		int DOS_COMMON_CK (const char *);
/* ff85 1.w l l l l ; e */	int DOS_COMMON_RD (const char *, int, char *, int);
/* ff85 2.w l l l l ; e */	int DOS_COMMON_WT (const char *, int, const char *, int);
/* ff85 3.w l l l l ; e */	int DOS_COMMON_LK (const char *, int, int, int);
/* ff85 4.w l l l l ; e */	int DOS_COMMON_FRE (const char *, int, int, int);
/* ff85 5.w l ; e */		int DOS_COMMON_DEL (const char *);
/* ff86 l l ; e */	int DOS_MOVE (const char *, const char *);
			int DOS_RENAME (const char *, const char *);
/* ff87 w l ; estrct */	int DOS_FILEDATE (int, int);
/* ff88 w l ; ealloc */	void *DOS_MALLOC2 (int, int);
/* ff88 wb15 l l ; ealloc */	void *DOS_MALLOC0 (int, int, int);
/* ff8a l w ; e */	int DOS_MAKETMP (const char *, int);
/* ff8b l w ; e */	int DOS_NEWFILE (const char *, dos_mode_t);
/* ff8c 0.w w l l ; e */	int DOS_LOCK (int, int, int);
/* ff8c 1.w w l l ; e */	int DOS_UNLOCK (int, int, int);
/* ff8f 0.w l l ; e */	int DOS_GETASSIGN (const char *, char *);
/* ff8f 1.w l l w ; e */	int DOS_MAKEASSIGN (const char *, const char *, int);
/* ff8f 4.w l ; e */	int DOS_RASSIGN (const char *);
/* ffaa w */		int DOS_FFLUSH_SET (int);
/* ffab w l ; e */	void *DOS_OS_PATCH (unsigned int, void *);
/* ffac w ; e */	union dos_fcb *DOS_GET_FCB_ADR (unsigned int);
/* ffad w l ; ealloc */	void *DOS_S_MALLOC (int, int);
/* ffad wb15 l l ; ealloc */	void *DOS_S_MALLOC0 (int, int);
/* ffae l ; e */	int DOS_S_MFREE (void *);
/* ffaf w l l l ; ep */	int DOS_S_PROCESS (int, int, int, int);
/* fff0 ; alias DOS_EXITVC noret */	__dead void DOS_RETSHELL (void);
			__dead void DOS_EXITVC (void);
/* fff1 ; noret */	__dead void DOS_CTLABORT (void);
/* fff2 ; noret */	__dead void DOS_ERRABORT (void);
/* fff3 l w w w */	void DOS_DISKRED (void *, int, int, int);
/* fff3 lb31 w l l */	void DOS_DISKRED2 (void *, int, int, int);
/* fff4 l w w w */	void DOS_DISKWRT (const void *, int, int, int);
/* fff4 lb31 w l l */	void DOS_DISKWRT2 (const void *, int, int, int);
/* fff5 */		struct dos_indos *DOS_INDOSFLG (void);
/* fff6 l ; super_jsr sv */	void DOS_SUPER_JSR (void (*)(void), struct dos_dregs *, struct dos_dregs *);
/* fff7 l l w ; alias DOS_BUS_ERR e */	int DOS_MEMCPY (void *, void *, int);
			int DOS_BUS_ERR (void *, void *, int);
/* fff8 l w l l w l l l ; e */	int DOS_OPEN_PR (const char *, int, int, int, int, int, struct dos_prcctrl *, long);
/* fff9 ; e */		int DOS_KILL_PR (void);
/* fffa w l ; e */	int DOS_GET_PR (int, struct dos_prcptr *);
/* fffb w ; ep */	int DOS_SUSPEND_PR (int);
/* fffc l */		int DOS_SLEEP_PR (long);
/* fffd w w w l l ; ep */	int DOS_SEND_PR (int, int, int, char *, long);
/* fffe */		long DOS_TIME_PR (void);
/* ffff */		void DOS_CHANGE_PR (void);

#endif /* __X68K_DOS_H__ */
