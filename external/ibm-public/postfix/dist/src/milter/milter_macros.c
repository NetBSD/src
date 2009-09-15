/*	$NetBSD: milter_macros.c,v 1.1.1.1.2.2 2009/09/15 06:03:20 snj Exp $	*/

/*++
/* NAME
/*	milter_macros
/* SUMMARY
/*	manipulate MILTER_MACROS structures
/* SYNOPSIS
/*	#include <milter.h>
/*
/*	MILTER_MACROS *milter_macros_create(conn_macros, helo_macros,
/*					mail_macros, rcpt_macros,
/*					data_macros, eoh_macros,
/*					eod_macros, unk_macros)
/*	const char *conn_macros;
/*	const char *helo_macros;
/*	const char *mail_macros;
/*	const char *rcpt_macrps;
/*	const char *data_macros;
/*	const char *eoh_macros;
/*	const char *eod_macros;
/*	const char *unk_macros;
/*
/*	MILTER_MACROS *milter_macros_alloc(init_mode)
/*	int	init_mode;
/*
/*	void	milter_macros_free(mp)
/*	MILTER_MACROS *mp;
/*
/*	int     milter_macros_print(print_fn, stream, flags, ptr)
/*	ATTR_PRINT_MASTER_FN print_fn;
/*	VSTREAM *stream;
/*	int	flags;
/*	void	*ptr;
/*
/*	int	milter_macros_scan(scan_fn, fp, flags, ptr)
/*	ATTR_SCAN_MASTER_FN scan_fn;
/*	VSTREAM	*fp;
/*	int	flags;
/*	void	*ptr;
/* DESCRIPTION
/*	Sendmail mail filter (Milter) applications receive sets of
/*	macro name=value pairs with each SMTP or content event.
/*	In Postfix, these macro names are stored in MILTER_MACROS
/*	structures, as one list for each event type. By default,
/*	the same structure is shared by all Milter applications;
/*	it is initialized with information from main.cf. With
/*	Sendmail 8.14 a Milter can override one or more lists of
/*	macro names. Postfix implements this by giving the Milter
/*	its own MILTER_MACROS structure and by storing the per-Milter
/*	information there.
/*
/*	This module maintains per-event macro name lists as
/*	mystrdup()'ed values. The user is explicitly allowed to
/*	update these values directly, as long as the result is
/*	compatible with mystrdup().
/*
/*	milter_macros_create() creates a MILTER_MACROS structure
/*	and initializes it with copies of its string arguments.
/*	Null pointers are not valid as input.
/*
/*	milter_macros_alloc() creates am empty MILTER_MACROS structure
/*	that is initialized according to its init_mode argument.
/* .IP MILTER_MACROS_ALLOC_ZERO
/*	Initialize all structure members as null pointers. This
/*	mode must be used with milter_macros_scan(), because that
/*	function blindly overwrites all structure members.  No other
/*	function except milter_macros_free() allows structure members
/*	with null pointer values.
/* .IP MILTER_MACROS_ALLOC_EMPTY
/*	Initialize all structure members with mystrdup(""). This
/*	is not as expensive as it appears to be.
/* .PP
/*	milter_macros_free() destroys a MILTER_MACROS structure and
/*	frees any strings referenced by it.
/*
/*	milter_macros_print() writes the contents of a MILTER_MACROS
/*	structure to the named stream using the specified attribute
/*	print routine.  milter_macros_print() is meant to be passed
/*	as a call-back to attr_print*(), thusly:
/*
/*	ATTR_TYPE_FUNC, milter_macros_print, (void *) macros,
/*
/*	milter_macros_scan() reads a MILTER_MACROS structure from
/*	the named stream using the specified attribute scan routine.
/*	No attempt is made to free the memory of existing structure
/*	members.  milter_macros_scan() is meant to be passed as a
/*	call-back to attr_scan*(), thusly:
/*
/*	ATTR_TYPE_FUNC, milter_macros_scan, (void *) macros,
/* DIAGNOSTICS
/*	Fatal: out of memory.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this
/*	software.
/* AUTHOR(S)
/*	Wietse Venema IBM T.J. Watson Research P.O. Box 704 Yorktown
/*	Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>

/* Utility library. */

#include <msg.h>
#include <attr.h>
#include <mymalloc.h>
#include <vstring.h>

/* Global library. */

#include <mail_proto.h>
#include <milter.h>

 /*
  * Ad-hoc protocol to send/receive milter macro name lists.
  */
#define MAIL_ATTR_MILT_MAC_CONN	"conn_macros"
#define MAIL_ATTR_MILT_MAC_HELO	"helo_macros"
#define MAIL_ATTR_MILT_MAC_MAIL	"mail_macros"
#define MAIL_ATTR_MILT_MAC_RCPT	"rcpt_macros"
#define MAIL_ATTR_MILT_MAC_DATA	"data_macros"
#define MAIL_ATTR_MILT_MAC_EOH	"eoh_macros"
#define MAIL_ATTR_MILT_MAC_EOD	"eod_macros"
#define MAIL_ATTR_MILT_MAC_UNK	"unk_macros"

/* milter_macros_print - write macros structure to stream */

int     milter_macros_print(ATTR_PRINT_MASTER_FN print_fn, VSTREAM *fp,
			            int flags, void *ptr)
{
    MILTER_MACROS *mp = (MILTER_MACROS *) ptr;
    int     ret;

    /*
     * The attribute order does not matter, except that it must be the same
     * as in the milter_macros_scan() function.
     */
    ret = print_fn(fp, flags | ATTR_FLAG_MORE,
		   ATTR_TYPE_STR, MAIL_ATTR_MILT_MAC_CONN, mp->conn_macros,
		   ATTR_TYPE_STR, MAIL_ATTR_MILT_MAC_HELO, mp->helo_macros,
		   ATTR_TYPE_STR, MAIL_ATTR_MILT_MAC_MAIL, mp->mail_macros,
		   ATTR_TYPE_STR, MAIL_ATTR_MILT_MAC_RCPT, mp->rcpt_macros,
		   ATTR_TYPE_STR, MAIL_ATTR_MILT_MAC_DATA, mp->data_macros,
		   ATTR_TYPE_STR, MAIL_ATTR_MILT_MAC_EOH, mp->eoh_macros,
		   ATTR_TYPE_STR, MAIL_ATTR_MILT_MAC_EOD, mp->eod_macros,
		   ATTR_TYPE_STR, MAIL_ATTR_MILT_MAC_UNK, mp->unk_macros,
		   ATTR_TYPE_END);
    return (ret);
}

/* milter_macros_scan - receive macros structure from stream */

int     milter_macros_scan(ATTR_SCAN_MASTER_FN scan_fn, VSTREAM *fp,
			           int flags, void *ptr)
{
    MILTER_MACROS *mp = (MILTER_MACROS *) ptr;
    int     ret;

    /*
     * We could simplify this by moving memory allocation into attr_scan*().
     */
    VSTRING *conn_macros = vstring_alloc(10);
    VSTRING *helo_macros = vstring_alloc(10);
    VSTRING *mail_macros = vstring_alloc(10);
    VSTRING *rcpt_macros = vstring_alloc(10);
    VSTRING *data_macros = vstring_alloc(10);
    VSTRING *eoh_macros = vstring_alloc(10);
    VSTRING *eod_macros = vstring_alloc(10);
    VSTRING *unk_macros = vstring_alloc(10);

    /*
     * The attribute order does not matter, except that it must be the same
     * as in the milter_macros_print() function.
     */
    ret = scan_fn(fp, flags | ATTR_FLAG_MORE,
		  ATTR_TYPE_STR, MAIL_ATTR_MILT_MAC_CONN, conn_macros,
		  ATTR_TYPE_STR, MAIL_ATTR_MILT_MAC_HELO, helo_macros,
		  ATTR_TYPE_STR, MAIL_ATTR_MILT_MAC_MAIL, mail_macros,
		  ATTR_TYPE_STR, MAIL_ATTR_MILT_MAC_RCPT, rcpt_macros,
		  ATTR_TYPE_STR, MAIL_ATTR_MILT_MAC_DATA, data_macros,
		  ATTR_TYPE_STR, MAIL_ATTR_MILT_MAC_EOH, eoh_macros,
		  ATTR_TYPE_STR, MAIL_ATTR_MILT_MAC_EOD, eod_macros,
		  ATTR_TYPE_STR, MAIL_ATTR_MILT_MAC_UNK, unk_macros,
		  ATTR_TYPE_END);

    /*
     * Don't optimize for error.
     */
    mp->conn_macros = vstring_export(conn_macros);
    mp->helo_macros = vstring_export(helo_macros);
    mp->mail_macros = vstring_export(mail_macros);
    mp->rcpt_macros = vstring_export(rcpt_macros);
    mp->data_macros = vstring_export(data_macros);
    mp->eoh_macros = vstring_export(eoh_macros);
    mp->eod_macros = vstring_export(eod_macros);
    mp->unk_macros = vstring_export(unk_macros);

    return (ret == 8 ? 1 : -1);
}

/* milter_macros_create - create and initialize macros structure */

MILTER_MACROS *milter_macros_create(const char *conn_macros,
				            const char *helo_macros,
				            const char *mail_macros,
				            const char *rcpt_macros,
				            const char *data_macros,
				            const char *eoh_macros,
				            const char *eod_macros,
				            const char *unk_macros)
{
    MILTER_MACROS *mp;

    mp = (MILTER_MACROS *) mymalloc(sizeof(*mp));
    mp->conn_macros = mystrdup(conn_macros);
    mp->helo_macros = mystrdup(helo_macros);
    mp->mail_macros = mystrdup(mail_macros);
    mp->rcpt_macros = mystrdup(rcpt_macros);
    mp->data_macros = mystrdup(data_macros);
    mp->eoh_macros = mystrdup(eoh_macros);
    mp->eod_macros = mystrdup(eod_macros);
    mp->unk_macros = mystrdup(unk_macros);

    return (mp);
}

/* milter_macros_alloc - allocate macros structure with simple initialization */

MILTER_MACROS *milter_macros_alloc(int mode)
{
    MILTER_MACROS *mp;

    /*
     * This macro was originally in milter.h, but no-one else needed it.
     */
#define milter_macros_init(mp, expr) do { \
	MILTER_MACROS *__mp = (mp); \
	char *__expr = (expr); \
	__mp->conn_macros = __expr; \
	__mp->helo_macros = __expr; \
	__mp->mail_macros = __expr; \
	__mp->rcpt_macros = __expr; \
	__mp->data_macros = __expr; \
	__mp->eoh_macros = __expr; \
	__mp->eod_macros = __expr; \
	__mp->unk_macros = __expr; \
    } while (0)

    mp = (MILTER_MACROS *) mymalloc(sizeof(*mp));
    switch (mode) {
    case MILTER_MACROS_ALLOC_ZERO:
	milter_macros_init(mp, 0);
	break;
    case MILTER_MACROS_ALLOC_EMPTY:
	milter_macros_init(mp, mystrdup(""));
	break;
    default:
	msg_panic("milter_macros_alloc: unknown mode %d", mode);
    }
    return (mp);
}

/* milter_macros_free - destroy memory for MILTER_MACROS structure */

void    milter_macros_free(MILTER_MACROS *mp)
{

    /*
     * This macro was originally in milter.h, but no-one else needed it.
     */
#define milter_macros_wipe(mp) do { \
	MILTER_MACROS *__mp = mp; \
	if (__mp->conn_macros) \
	    myfree(__mp->conn_macros); \
	if (__mp->helo_macros) \
	    myfree(__mp->helo_macros); \
	if (__mp->mail_macros) \
	    myfree(__mp->mail_macros); \
	if (__mp->rcpt_macros) \
	    myfree(__mp->rcpt_macros); \
	if (__mp->data_macros) \
	    myfree(__mp->data_macros); \
	if (__mp->eoh_macros) \
	    myfree(__mp->eoh_macros); \
	if (__mp->eod_macros) \
	    myfree(__mp->eod_macros); \
	if (__mp->unk_macros) \
	    myfree(__mp->unk_macros); \
    } while (0)

    milter_macros_wipe(mp);
    myfree((char *) mp);
}
