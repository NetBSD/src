#ifndef _VSTRING_H_INCLUDED_
#define _VSTRING_H_INCLUDED_

/*++
/* NAME
/*	vstring 3h
/* SUMMARY
/*	arbitrary-length string manager
/* SYNOPSIS
/*	#include "vstring.h"
/* DESCRIPTION
/* .nf

 /*
  * System library.
  */
#include <stdarg.h>

 /*
  * Utility library.
  */
#include <vbuf.h>

 /*
  * We can't allow bare VBUFs in the interface, because VSTRINGs have a
  * specific initialization and destruction sequence.
  */
typedef struct VSTRING {
    VBUF    vbuf;
    int     maxlen;
} VSTRING;

extern VSTRING *vstring_alloc(int);
extern void vstring_ctl(VSTRING *,...);
extern VSTRING *vstring_truncate(VSTRING *, int);
extern VSTRING *vstring_free(VSTRING *);
extern VSTRING *vstring_strcpy(VSTRING *, const char *);
extern VSTRING *vstring_strncpy(VSTRING *, const char *, int);
extern VSTRING *vstring_strcat(VSTRING *, const char *);
extern VSTRING *vstring_strncat(VSTRING *, const char *, int);
extern VSTRING *vstring_memcpy(VSTRING *, const char *, int);
extern VSTRING *vstring_memcat(VSTRING *, const char *, int);
extern VSTRING *PRINTFLIKE(2, 3) vstring_sprintf(VSTRING *, const char *,...);
extern VSTRING *PRINTFLIKE(2, 3) vstring_sprintf_append(VSTRING *, const char *,...);
extern char *vstring_export(VSTRING *);
extern VSTRING *vstring_import(char *);

#define VSTRING_CTL_MAXLEN	1
#define VSTRING_CTL_END		0

 /*
  * Macros. Unsafe macros have UPPERCASE names.
  */
#define VSTRING_SPACE(vp, len)	((vp)->vbuf.space(&(vp)->vbuf, len))
#define vstring_str(vp)		((char *) (vp)->vbuf.data)
#define VSTRING_LEN(vp)		((vp)->vbuf.ptr - (vp)->vbuf.data)
#define vstring_end(vp)		((char *) (vp)->vbuf.ptr)
#define VSTRING_TERMINATE(vp)	{ if ((vp)->vbuf.cnt <= 0) \
					VSTRING_SPACE((vp),1); \
				  *(vp)->vbuf.ptr = 0; }
#define VSTRING_RESET(vp)	{ (vp)->vbuf.ptr = (vp)->vbuf.data; \
				  (vp)->vbuf.cnt = (vp)->vbuf.len; }
#define	VSTRING_ADDCH(vp, ch)	VBUF_PUT(&(vp)->vbuf, ch)
#define VSTRING_SKIP(vp)	{ while ((vp)->vbuf.cnt > 0 && *(vp)->vbuf.ptr) \
				  (vp)->vbuf.ptr++, (vp)->vbuf.cnt--; }
#define vstring_avail(vp)	((vp)->vbuf.cnt)

 /*
  * The following macro is not part of the public interface, because it can
  * really screw up a buffer by positioning past allocated memory.
  */
#define VSTRING_AT_OFFSET(vp, offset) { \
	(vp)->vbuf.ptr = (vp)->vbuf.data + (offset); \
	(vp)->vbuf.cnt = (vp)->vbuf.len - (offset); \
    }

extern VSTRING *vstring_vsprintf(VSTRING *, const char *, va_list);
extern VSTRING *vstring_vsprintf_append(VSTRING *, const char *, va_list);

/* BUGS
/*	Auto-resizing may change the address of the string data in
/*	a vstring structure. Beware of dangling pointers.
/* HISTORY
/* .ad
/* .fi
/*	A vstring module appears in the UNPROTO software by Wietse Venema.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

#endif
