/*	$NetBSD: tpctl.h,v 1.5.6.1 2009/05/13 19:20:42 jym Exp $	*/

/*-
 * Copyright (c) 2002, 2003 TAKEMRUA Shin
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
 * 3. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __TPCTL_H__
#define __TPCTL_H__

#include <sys/queue.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplay_usl_io.h>
#include <dev/hpc/hpcfbio.h>

#define MAXDATALEN		128
#define TPCTL_DB_FILENAME	"/etc/tpctl.dat"
#define TPCTL_TMP_FILENAME	"tpctl.tmp"
#define TPCTL_TP_DEVICE		"/dev/wsmux0"
#define TPCTL_FB_DEVICE		"/dev/ttyE0"


enum tpctl_data_type {
	TPCTL_CALIBCOORDS,
	TPCTL_COMMENT,
};

enum tpctl_data_ERROR {
	ERR_NONE,
	ERR_NOFILE,
	ERR_IO,
	ERR_SYNTAX,
	ERR_DUPNAME,
};

struct tpctl_data_elem {
	enum tpctl_data_type type;
	TAILQ_ENTRY(tpctl_data_elem) link;
	char *name;
	struct wsmouse_calibcoords calibcoords;
};

struct tpctl_data {
	int lineno;
	TAILQ_HEAD(,tpctl_data_elem) list;
};

struct tp {
	int fd;
	char id[WSMOUSE_ID_MAXLEN];
};

typedef u_int32_t fb_pixel_t;
struct fb {
	int fd;
	int dispmode;
	struct hpcfb_fbconf conf;
	unsigned char *baseaddr;
	fb_pixel_t *linecache, *workbuf;
	fb_pixel_t white, black;
	int linecache_y;
};

int init_data(struct tpctl_data *);
int read_data(const char *, struct tpctl_data *);
int write_data(const char *, struct tpctl_data *);
void write_coords(FILE *, char *, struct wsmouse_calibcoords *);
void free_data(struct tpctl_data *);
int replace_data(struct tpctl_data *, char *, struct wsmouse_calibcoords *);
struct wsmouse_calibcoords *search_data(struct tpctl_data *, char *);

int tp_init(struct tp *, int);
int tp_setrawmode(struct tp *);
int tp_setcalibcoords(struct tp *, struct wsmouse_calibcoords *);
int tp_flush(struct tp *);
int tp_get(struct tp *, int *, int *, int (*)(void *), void *);
int tp_waitup(struct tp *, int, int (*)(void *), void *);

int fb_dispmode(struct fb *, int);
int fb_init(struct fb *, int);
void fb_getline(struct fb *, int);
void fb_putline(struct fb *, int);
void fb_fetchline(struct fb *, int);
void fb_flush(struct fb *);
void fb_drawline(struct fb *, int, int, int, int, fb_pixel_t);
void fb_drawpixel(struct fb *, int, int, fb_pixel_t);

#endif /* __TPCTL_TP_H__ */
