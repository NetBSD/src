/* $NetBSD: aucomvar.h,v 1.2 2004/05/01 19:04:00 thorpej Exp $ */

/* copyright */

/* Renamed externally visible functions */
#define	com_attach_subr	aucom_attach_subr
#define	comintr		aucomintr
#define	comcnattach	aucomcnattach
#define	com_is_console	aucom_is_console
#define	com_kgdb_attach	aucom_kgdb_attach

#define	comprobe1	xxx_aucomprobe1		/* unused */
#define	com_detach	xxx_aucom_detach	/* unused */
#define	com_activate	xxx_aucom_activate	/* unused */

/* Renamed externally visible variables */
#define	comcons		aucomcons
#define	com_cd		aucom_cd
#define	com_rbuf_size	aucom_rbuf_size
#define	com_rbuf_hiwat	aucom_rbuf_hiwat
#define	com_rbuf_lowat	aucom_rbuf_lowat
#define	com_debug	aucom_debug

#include <dev/ic/comvar.h>

#undef COM_MPLOCK	/* just in case... */
#undef COM_DEBUG
#undef COM_HAYESP
#undef COM_16650
