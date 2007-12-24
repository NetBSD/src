/*
 *	dos.h
 *	Human68k DOS call interface
 *
 *	written by Yasha (ITOH Yasufumi)
 *	(based on PD libc 1.1.32 by PROJECT C Library)
 *	public domain
 *
 *	$NetBSD: dos.h,v 1.5 2007/12/24 15:46:46 perry Exp $
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

/* ff00 ; noret */	__dead void DOS_EXIT __P((void));
/* ff01 */		int DOS_GETCHAR __P((void));
/* ff02 w */		void DOS_PUTCHAR __P((int));
/* ff03 */		int DOS_COMINP __P((void));
/* ff04 w */		void DOS_COMOUT __P((int));
/* ff05 w */		void DOS_PRNOUT __P((int));
/* ff06 w */		int DOS_INPOUT __P((int));
/* ff07 */		int DOS_INKEY __P((void));
/* ff08 */		int DOS_GETC __P((void));
/* ff09 l */		void DOS_PRINT __P((const char *));
/* ff0a l */		int DOS_GETS __P((struct dos_inpptr *));
/* ff0b */		int DOS_KEYSNS __P((void));
/* ff0c 1.w */		int DOS_KFLUSHGP __P((void));
/* ff0c 6.w w */	int DOS_KFLUSHIO __P((int));
/* ff0c 7.w */		int DOS_KFLUSHIN __P((void));
/* ff0c 8.w */		int DOS_KFLUSHGC __P((void));
/* ff0c 10.w l */	int DOS_KFLUSHGS __P((struct dos_inpptr *));
/* ff0d */		void DOS_FFLUSH __P((void));
/* ff0e w */		int DOS_CHGDRV __P((int));
/* ff0f drvctrl */	int DOS_DRVCTRL __P((int, int));
/* ff10 */		int DOS_CONSNS __P((void));
/* ff11 */		int DOS_PRNSNS __P((void));
/* ff12 */		int DOS_CINSNS __P((void));
/* ff13 */		int DOS_COUTSNS __P((void));
/* ff17 l l ; e */	int DOS_FATCHK __P((const char *, unsigned short *));
/* ff17 l lb31 w ; e */	int DOS_FATCHK2 __P((const char *, unsigned short *, int));
/* ff18 0.w */		int DOS_HENDSPMO __P((void));
/* ff18 1.w w l */	int DOS_HENDSPMP __P((int, const char *));
/* ff18 2.w w l */	int DOS_HENDSPMR __P((int, const char *));
/* ff18 3.w */		void DOS_HENDSPMC __P((void));
/* ff18 4.w */		int DOS_HENDSPIO __P((void));
/* ff18 5.w w l */	int DOS_HENDSPIP __P((int, const char *));
/* ff18 6.w w l */	int DOS_HENDSPIR __P((int, const char *));
/* ff18 7.w w */	void DOS_HENDSPIC __P((int));
/* ff18 8.w */		int DOS_HENDSPSO __P((void));
/* ff18 9.w w l */	int DOS_HENDSPSP __P((int, const char *));
/* ff18 10.w w l */	int DOS_HENDSPSR __P((int, const char *));
/* ff18 11.w */		void DOS_HENDSPSC __P((void));
/* ff19 */		int DOS_CURDRV __P((void));
/* ff1a l */		int DOS_GETSS __P((struct dos_inpptr *));
/* ff1b w */		int DOS_FGETC __P((int));
/* ff1c l w */		int DOS_FGETS __P((struct dos_inpptr *, int));
/* ff1d w w */		void DOS_FPUTC __P((int, int));
/* ff1e l w */		void DOS_FPUTS __P((const char *, int));
/* ff1f */		void DOS_ALLCLOSE __P((void));
/* ff20 super ; super e */	int DOS_SUPER __P((int));
/* ff21 w l ; e */	void DOS_FNCKEYGT __P((int, char *));
/* ff21 wb8 l ; e */	void DOS_FNCKEYST __P((int, const char *));
/* ff23 0.w w */	int DOS_C_PUTC __P((int));
/* ff23 1.w l */	int DOS_C_PRINT __P((const char *));
/* ff23 2.w w */	int DOS_C_COLOR __P((int));
/* ff23 3.w w w */	int DOS_C_LOCATE __P((int, int));
/* ff23 4.w */		int DOS_C_DOWN_S __P((void));
/* ff23 5.w */		int DOS_C_UP_S __P((void));
/* ff23 6.w w */	int DOS_C_UP __P((int));
/* ff23 7.w w */	int DOS_C_DOWN __P((int));
/* ff23 8.w w */	int DOS_C_RIGHT __P((int));
/* ff23 9.w w */	int DOS_C_LEFT __P((int));
/* ff23 10.w 0.w */	int DOS_C_CLS_ED __P((void));
/* ff23 10.w 1.w */	int DOS_C_CLS_ST __P((void));
/* ff23 10.w 2.w */	int DOS_C_CLS_AL __P((void));
/* ff23 11.w 0.w */	int DOS_C_ERA_ED __P((void));
/* ff23 11.w 1.w */	int DOS_C_ERA_ST __P((void));
/* ff23 11.w 2.w */	int DOS_C_ERA_AL __P((void));
/* ff23 12.w w */	int DOS_C_INS __P((int));
/* ff23 13.w w */	int DOS_C_DEL __P((int));
/* ff23 14.w w */	int DOS_C_FNKMOD __P((int));
/* ff23 15.w w w */	int DOS_C_WINDOW __P((int, int));
/* ff23 16.w w */	int DOS_C_WIDTH __P((int));
/* ff23 17.w */		int DOS_C_CURON __P((void));
/* ff23 18.w */		int DOS_C_CUROFF __P((void));
/* ff24 0.w */		int DOS_K_KEYINP __P((void));
/* ff24 1.w */		int DOS_K_KEYSNS __P((void));
/* ff24 2.w */		int DOS_K_SFTSNS __P((void));
/* ff24 3.w w */	int DOS_K_KEYBIT __P((int));
/* ff24 4.w w */	void DOS_K_INSMOD __P((int));
/* ff25 w l */		void *DOS_INTVCS __P((int, void *));
/* ff26 l */		void DOS_PSPSET __P((struct dos_psp *));
/* ff27 */		int DOS_GETTIM2 __P((void));
/* ff28 l ; e */	int DOS_SETTIM2 __P((int));
/* ff29 l l ; e */	int DOS_NAMESTS __P((const char *, struct dos_namestbuf *));
/* ff2a */		int DOS_GETDATE __P((void));
/* ff2b w ; e */	int DOS_SETDATE __P((int));
/* ff2c */		int DOS_GETTIME __P((void));
/* ff2d w ; e */	int DOS_SETTIME __P((int));
/* ff2e w */		void DOS_VERIFY __P((int));
/* ff2f w w ; e */	int DOS_DUP0 __P((int, int));
/* ff30 */		int __pure DOS_VERNUM __P((void));
/* ff31 l w ; noret */	__dead void DOS_KEEPPR __P((int, int));
/* ff32 w l ; e */	int DOS_GETDPB __P((int, struct dos_dpbptr *));
/* ff33 w */		int DOS_BREAKCK __P((int));
/* ff34 w w ; e */	void DOS_DRVXCHG __P((int, int));
/* ff35 w */		void *DOS_INTVCG __P((int));
/* ff36 w l ; estrct */	int DOS_DSKFRE __P((int, struct dos_freeinf *));
/* ff37 l l ; e */	int DOS_NAMECK __P((const char *, struct dos_nameckbuf *));
/* ff39 l ; e */	int DOS_MKDIR __P((const char *));
/* ff3a l ; e */	int DOS_RMDIR __P((const char *));
/* ff3b l ; e */	int DOS_CHDIR __P((const char *));
/* ff3c l w ; e */	int DOS_CREATE __P((const char *, dos_mode_t));
/* ff3d l w ; e */	int DOS_OPEN __P((const char *, int));
/* ff3e w ; e */	int DOS_CLOSE __P((int));
/* ff3f w l l ; e */	int DOS_READ __P((int, char *, int));
/* ff40 w l l ; e */	int DOS_WRITE __P((int, const char *, int));
/* ff41 l ; e */	int DOS_DELETE __P((const char *));
/* ff42 w l w ; e */	long DOS_SEEK __P((int, int, int));
/* ff43 l w ; e */	dos_mode_t DOS_CHMOD __P((const char *, dos_mode_t));
/* ff44 0.w w ; e */	int DOS_IOCTRLGT __P((int));
/* ff44 1.w w w ; e */	int DOS_IOCTRLST __P((int, int));
/* ff44 2.w w l l ; e */	int DOS_IOCTRLRH __P((int, char *, int));
/* ff44 3.w w l l ; e */	int DOS_IOCTRLWH __P((int, const char *, int));
/* ff44 4.w w l l ; e */	int DOS_IOCTRLRD __P((int, char *, int));
/* ff44 5.w w l l ; e */	int DOS_IOCTRLWD __P((int, const char *, int));
/* ff44 6.w w ; e */	int DOS_IOCTRLIS __P((int));
/* ff44 7.w w ; e */	int DOS_IOCTRLOS __P((int));
/* ff44 9.w w ; e */	int DOS_IOCTRLDVGT __P((int));
/* ff44 10.w w ; e */	int DOS_IOCTRLFDGT __P((int));
/* ff44 11.w w w ; e */	int DOS_IOCTRLRTSET __P((int, int));
/* ff44 12.w w w l ; e */	int DOS_IOCTRLDVCTL __P((int, int, char *));
/* ff44 13.w w w l ; e */	int DOS_IOCTRLFDCTL __P((int, int, char *));
/* ff45 w ; e */	int DOS_DUP __P((int));
/* ff46 w w ; e */	int DOS_DUP2 __P((int, int));
/* ff47 w l ; e */	int DOS_CURDIR __P((int, char *));
/* ff48 l ; ealloc */	void *DOS_MALLOC __P((int));
/* ff49 l ; e */	int DOS_MFREE __P((void *));
/* ff4a l l ; ealloc */	int DOS_SETBLOCK __P((void *, int));
/* ff4b 0.w l l l ; sv e */	int DOS_LOADEXEC __P((const char *, const struct dos_comline *, const char *));
/* ff4b 1.w l l l ; sv e */	int DOS_LOAD __P((const char *, const struct dos_comline *, const char *));
/* ff4b 2.w l l l ; e */	int DOS_PATHCHK __P((const char *, const struct dos_comline *, const char *));
/* ff4b 3.w l l l ; e */	int DOS_LOADONLY __P((const char *, const void *, const void *));
/* ff4b 4.w l ; sv e */		int DOS_EXECONLY __P((void *));
/* ff4b 5.w l l 0.l ; sv e */	int DOS_BINDNO __P((const char *, const char *));
		/*^ 0.l is required?? */
/* ff4b w l l l ; sv e */	int DOS_EXEC2 __P((int, const char *, const char *, const char *));
/* ff4c w ; noret */	__dead void DOS_EXIT2 __P((int));
/* ff4d */		int DOS_WAIT __P((void));
/* ff4e l l w ; e */	int DOS_FILES __P((struct dos_filbuf *, const char *, int));
/* ff4e lb31 l w ; e */	int DOS_EXFILES __P((struct dos_exfilbuf *, const char *, int));
/* ff4f l ; e */	int DOS_NFILES __P((struct dos_filbuf *));
/* ff4f lb31 ; e */	int DOS_EXNFILES __P((struct dos_exfilbuf *));
/* ff80 l */		struct dos_psp *DOS_SETPDB __P((struct dos_psp *));
/* ff81 */		struct dos_psp *DOS_GETPDB __P((void));
/* ff82 l l l ; e */	int DOS_SETENV __P((const char *, const char *, const char *));
/* ff83 l l l ; e */	int DOS_GETENV __P((const char *, const char *, char *));
/* ff84 */		int DOS_VERIFYG __P((void));
/* ff85 0.w l ; e */		int DOS_COMMON_CK __P((const char *));
/* ff85 1.w l l l l ; e */	int DOS_COMMON_RD __P((const char *, int, char *, int));
/* ff85 2.w l l l l ; e */	int DOS_COMMON_WT __P((const char *, int, const char *, int));
/* ff85 3.w l l l l ; e */	int DOS_COMMON_LK __P((const char *, int, int, int));
/* ff85 4.w l l l l ; e */	int DOS_COMMON_FRE __P((const char *, int, int, int));
/* ff85 5.w l ; e */		int DOS_COMMON_DEL __P((const char *));
/* ff86 l l ; e */	int DOS_MOVE __P((const char *, const char *));
			int DOS_RENAME __P((const char *, const char *));
/* ff87 w l ; estrct */	int DOS_FILEDATE __P((int, int));
/* ff88 w l ; ealloc */	void *DOS_MALLOC2 __P((int, int));
/* ff88 wb15 l l ; ealloc */	void *DOS_MALLOC0 __P((int, int, int));
/* ff8a l w ; e */	int DOS_MAKETMP __P((const char *, int));
/* ff8b l w ; e */	int DOS_NEWFILE __P((const char *, dos_mode_t));
/* ff8c 0.w w l l ; e */	int DOS_LOCK __P((int, int, int));
/* ff8c 1.w w l l ; e */	int DOS_UNLOCK __P((int, int, int));
/* ff8f 0.w l l ; e */	int DOS_GETASSIGN __P((const char *, char *));
/* ff8f 1.w l l w ; e */	int DOS_MAKEASSIGN __P((const char *, const char *, int));
/* ff8f 4.w l ; e */	int DOS_RASSIGN __P((const char *));
/* ffaa w */		int DOS_FFLUSH_SET __P((int));
/* ffab w l ; e */	void *DOS_OS_PATCH __P((unsigned int, void *));
/* ffac w ; e */	union dos_fcb *DOS_GET_FCB_ADR __P((unsigned int));
/* ffad w l ; ealloc */	void *DOS_S_MALLOC __P((int, int));
/* ffad wb15 l l ; ealloc */	void *DOS_S_MALLOC0 __P((int, int));
/* ffae l ; e */	int DOS_S_MFREE __P((void *));
/* ffaf w l l l ; ep */	int DOS_S_PROCESS __P((int, int, int, int));
/* fff0 ; alias DOS_EXITVC noret */	__dead void DOS_RETSHELL __P((void));
			__dead void DOS_EXITVC __P((void));
/* fff1 ; noret */	__dead void DOS_CTLABORT __P((void));
/* fff2 ; noret */	__dead void DOS_ERRABORT __P((void));
/* fff3 l w w w */	void DOS_DISKRED __P((void *, int, int, int));
/* fff3 lb31 w l l */	void DOS_DISKRED2 __P((void *, int, int, int));
/* fff4 l w w w */	void DOS_DISKWRT __P((const void *, int, int, int));
/* fff4 lb31 w l l */	void DOS_DISKWRT2 __P((const void *, int, int, int));
/* fff5 */		struct dos_indos *DOS_INDOSFLG __P((void));
/* fff6 l ; super_jsr sv */	void DOS_SUPER_JSR __P((void (*) __P((void)), struct dos_dregs *, struct dos_dregs *));
/* fff7 l l w ; alias DOS_BUS_ERR e */	int DOS_MEMCPY __P((void *, void *, int));
			int DOS_BUS_ERR __P((void *, void *, int));
/* fff8 l w l l w l l l ; e */	int DOS_OPEN_PR __P((const char *, int, int, int, int, int, struct dos_prcctrl *, long));
/* fff9 ; e */		int DOS_KILL_PR __P((void));
/* fffa w l ; e */	int DOS_GET_PR __P((int, struct dos_prcptr *));
/* fffb w ; ep */	int DOS_SUSPEND_PR __P((int));
/* fffc l */		int DOS_SLEEP_PR __P((long));
/* fffd w w w l l ; ep */	int DOS_SEND_PR __P((int, int, int, char *, long));
/* fffe */		long DOS_TIME_PR __P((void));
/* ffff */		void DOS_CHANGE_PR __P((void));

#endif /* __X68K_DOS_H__ */
