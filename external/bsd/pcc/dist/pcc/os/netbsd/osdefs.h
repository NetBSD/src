/*	Id: osdefs.h	
/*	$NetBSD: osdefs.h,v 1.1.1.1 2016/02/09 20:29:20 plunky Exp $

/*
 * CPPADD	a comma separated list of strings, to be added to
 *		the default definitions for the OS
 */
#define CPPADD		"-D__NetBSD__"

#if defined(mach_powerpc)
#define STARTLABEL "_start"
#endif

#if defined(mach_vax)
#define	PCC_EARLY_SETUP { kflag = 1; }
#endif

#if defined(mach_i386)
#define WCHAR_TYPE	INT
#define WINT_TYPE	INT
#define SIZE_TYPE	INT
#define PTRDIFF_TYPE	INT
#else
#define WCHAR_TYPE	INT
#define WINT_TYPE	INT
#define SIZE_TYPE	ULONG
#define PTRDIFF_TYPE	LONG
#endif
