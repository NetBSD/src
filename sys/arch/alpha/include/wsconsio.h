/* $NetBSD: wsconsio.h,v 1.1.2.3 1997/06/01 04:12:43 cgd Exp $ */

/*
 * Copyright (c) 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#ifndef __ALPHA_INCLUDE_WSCONSIO_H_
#define	__ALPHA_INCLUDE_WSCONSIO_H_

/*
 * WSCONS (wsdisplay, wskbd, wsmouse) exported interfaces.
 *
 * Ioctls are all in group 'W'.  Ioctl number space is partitioned like:
 *	0-31	keyboard ioctls (WSKBDIO)
 *	32-63	mouse ioctls (WSMOUSEIO)
 *	64-95	display ioctls (WSDISPLAYIO)
 *	96-255	reserved for future use
 */

#include <sys/types.h>
#include <sys/ioccom.h>


/*
 * Common event structure (used by keyboard and mouse)
 */
struct wscons_event {
	u_int		type;
	int		value;
	struct timespec	time;
};

/* Event type definitions.  Comment for each is information in value. */
#define	WSCONS_EVENT_KEY_UP		1	/* key code */
#define	WSCONS_EVENT_KEY_DOWN		2	/* key code */
#define	WSCONS_EVENT_KEY_OTHER		3	/* key code */
#define	WSCONS_EVENT_MOUSE_UP		4	/* button # (leftmost = 0) */
#define	WSCONS_EVENT_MOUSE_DOWN		5	/* button # (leftmost = 0)  */
#define	WSCONS_EVENT_MOUSE_DELTA_X	6	/* X delta amount */
#define	WSCONS_EVENT_MOUSE_DELTA_Y	7	/* Y delta amount */
#define	WSCONS_EVENT_MOUSE_ABSOLUTE_X	8	/* X location */
#define	WSCONS_EVENT_MOUSE_ABSOLUTE_Y	9	/* Y location */


/*
 * Keyboard ioctls (0 - 31)
 */

/* Get keyboard type. */
#define	WSKBDIO_GTYPE		_IOR('W', 0, u_int)
#define		WSKBD_TYPE_LK201	1	/* lk-201 */
#define		WSKBD_TYPE_LK401	2	/* lk-401 */
#define		WSKBD_TYPE_PC		3	/* PC-ish */

/* Manipulate the keyboard bell. */
struct wskbd_bell_data {
	u_int	which;				/* values to get/set */
	u_int	pitch;				/* pitch, in Hz */
	u_int	period;				/* period, in milliseconds */
	u_int	volume;				/* percentage of max volume */
};
#define		WSKBD_BELL_DOPITCH	0x1		/* get/set pitch */
#define		WSKBD_BELL_DOPERIOD	0x2		/* get/set period */
#define		WSKBD_BELL_DOVOLUME	0x4		/* get/set volume */
#define		WSKBD_BELL_DOALL	0x7		/* all of the above */

#define	WSKBDIO_BELL		_IO('W', 1)
#define	WSKBDIO_COMPLEXBELL	_IOW('W', 2, struct wskbd_bell_data)
#define	WSKBDIO_SETBELL		_IOW('W', 3, struct wskbd_bell_data)
#define	WSKBDIO_GETBELL		_IOR('W', 4, struct wskbd_bell_data)
#define	WSKBDIO_SETDEFAULTBELL	_IOW('W', 5, struct wskbd_bell_data)
#define	WSKBDIO_GETDEFAULTBELL	_IOR('W', 6, struct wskbd_bell_data)

/* Manipulate the emulation key repeat settings. */
struct wskbd_keyrepeat_data {
	u_int	which;				/* values to get/set */
	u_int	del1;				/* delay before first, ms */
	u_int	delN;				/* delay before rest, ms */
};
#define		WSKBD_KEYREPEAT_DODEL1	0x1		/* get/set del1 */
#define		WSKBD_KEYREPEAT_DODELN	0x2		/* get/set delN */
#define		WSKBD_KEYREPEAT_DOALL	0x3		/* all of the above */

#define	WSKBDIO_SETKEYREPEAT	_IOW('W', 7, struct wskbd_keyrepeat_data)
#define	WSKBDIO_GETKEYREPEAT	_IOR('W', 8, struct wskbd_keyrepeat_data)
#define	WSKBDIO_SETDEFAULTKEYREPEAT \
	    _IOW('W', 9, struct wskbd_keyrepeat_data)
#define	WSKBDIO_GETDEFAULTKEYREPEAT \
	    _IOR('W', 10, struct wskbd_keyrepeat_data)


/*
 * Mouse ioctls (32 - 63)
 */

/* Get keyboard type */
#define	WSMOUSEIO_GTYPE		_IOR('W', 32, u_int)
#define		WSMOUSE_TYPE_VSXXX	1	/* DEC TC(?) serial */
#define		WSMOUSE_TYPE_PS2	2	/* PS/2-compatible */

/*
 * Display ioctls (64 - 95)
 */

/* Get display type */
#define	WSDISPLAYIO_GTYPE	_IOR('W', 64, u_int)
#define		WSDISPLAY_TYPE_PM_MONO	1	/* ??? */
#define		WSDISPLAY_TYPE_PM_COLOR	2	/* ??? */
#define		WSDISPLAY_TYPE_CFB	3	/* DEC TC CFB */
#define		WSDISPLAY_TYPE_XCFB	4	/* ??? */
#define		WSDISPLAY_TYPE_MFB	5	/* DEC TC MFB */
#define		WSDISPLAY_TYPE_SFB	6	/* DEC TC SFB */
#define		WSDISPLAY_TYPE_ISAVGA	7	/* (generic) ISA VGA */
#define		WSDISPLAY_TYPE_PCIVGA	8	/* (generic) PCI VGA */
#define		WSDISPLAY_TYPE_TGA	9	/* DEC PCI TGA */
#define		WSDISPLAY_TYPE_SFBP	10	/* DEC TC SFB+ */
#define		WSDISPLAY_TYPE_PCIMISC	11	/* (generic) PCI misc. disp. */

/* Basic display information.  Not applicable to all display types. */
struct wsdisplay_fbinfo {
	u_int	height;				/* height in pixels */
	u_int	width;				/* width in pixels */
	u_int	depth;				/* bits per pixel */
	u_int	cmsize;				/* color map size (entries) */
};
#define	WSDISPLAYIO_GINFO	_IOR('W', 65, struct wsdisplay_fbinfo)

/* Colormap operations.  Not applicable to all display types. */
struct wsdisplay_cmap {
	u_int	index;				/* first element (0 origin) */
	u_int	count;				/* number of elements */
	u_char	*red;				/* red color map elements */
	u_char	*green;				/* green color map elements */
	u_char	*blue;				/* blue color map elements */
};      
#define WSDISPLAYIO_GETCMAP	_IOW('W', 66, struct wsdisplay_cmap)
#define WSDISPLAYIO_PUTCMAP	_IOW('W', 67, struct wsdisplay_cmap)

/* Video control.  Not applicable to all display types. */
#define	WSDISPLAYIO_GVIDEO	_IOR('W', 68, u_int)
#define	WSDISPLAYIO_SVIDEO	_IOW('W', 69, u_int)
#define		WSDISPLAYIO_VIDEO_OFF	0	/* video off */
#define		WSDISPLAYIO_VIDEO_ON	1	/* video on */

/* Cursor control.  Not applicable to all display types. */
struct wsdisplay_curpos {			/* cursor "position" */
	u_int x, y;
};

struct wsdisplay_cursor {
	u_int	which;				/* values to get/set */
	u_int	enable;				/* enable/disable */
	struct wsdisplay_curpos pos;		/* position */
	struct wsdisplay_curpos hot;		/* hot spot */
	struct wsdisplay_cmap cmap;		/* color map info */
	struct wsdisplay_curpos size;		/* bit map size */
	u_char *image;				/* image data */
	u_char *mask;				/* mask data */
};
#define		WSDISPLAY_CURSOR_DOCUR		0x01	/* get/set enable */
#define		WSDISPLAY_CURSOR_DOPOS		0x02	/* get/set pos */
#define		WSDISPLAY_CURSOR_DOHOT		0x04	/* get/set hot spot */
#define		WSDISPLAY_CURSOR_DOCMAP		0x08	/* get/set cmap */
#define		WSDISPLAY_CURSOR_DOSHAPE	0x10	/* get/set img/mask */
#define		WSDISPLAY_CURSOR_DOALL		0x1f	/* all of the above */

/* Cursor control: get and set position */
#define	WSDISPLAYIO_GCURPOS	_IOR('W', 70, struct wsdisplay_curpos)
#define	WSDISPLAYIO_SCURPOS	_IOW('W', 71, struct wsdisplay_curpos)

/* Cursor control: get maximum size */
#define	WSDISPLAYIO_GCURMAX	_IOR('W', 72, struct wsdisplay_curpos)

/* Cursor control: get/set cursor attributes/shape */
#define	WSDISPLAYIO_GCURSOR	_IOWR('W', 73, struct wsdisplay_cursor)
#define	WSDISPLAYIO_SCURSOR	_IOW('W', 74, struct wsdisplay_cursor)

/* Display mode: Emulation (text) vs. Mapped (graphics) mode */
#define	WSDISPLAYIO_GMODE	_IOR('W', 75, u_int)
#define	WSDISPLAYIO_SMODE	_IOW('W', 76, u_int)
#define		WSDISPLAYIO_MODE_EMUL	0	/* emulation (text) mode */
#define		WSDISPLAYIO_MODE_MAPPED	1	/* mapped (graphics) mode */

/* XXX NOT YET DEFINED */
/* Mapping information retrieval. */

#endif /* __ALPHA_INCLUDE_WSCONSIO_H_ */
