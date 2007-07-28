/*	$NetBSD: wsdisplay_vcons.c,v 1.12 2007/07/28 20:28:57 mjf Exp $ */

/*-
 * Copyright (c) 2005, 2006 Michael Lorenz
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wsdisplay_vcons.c,v 1.12 2007/07/28 20:28:57 mjf Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/mman.h>
#include <sys/tty.h>
#include <sys/conf.h>
#include <sys/proc.h>
#include <sys/kthread.h>
#include <sys/tprintf.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wsfont/wsfont.h>
#include <dev/rasops/rasops.h>

#include <dev/wscons/wsdisplay_vconsvar.h>

static void vcons_dummy_init_screen(void *, struct vcons_screen *, int, 
	    long *);

static int  vcons_ioctl(void *, void *, u_long, void *, int, struct lwp *);
static int  vcons_alloc_screen(void *, const struct wsscreen_descr *, void **, 
	    int *, int *, long *);
static void vcons_free_screen(void *, void *);
static int  vcons_show_screen(void *, void *, int, void (*)(void *, int, int),
	    void *);

static void vcons_do_switch(struct vcons_data *);

/* methods that work only on text buffers */
static void vcons_copycols_buffer(void *, int, int, int, int);
static void vcons_erasecols_buffer(void *, int, int, int, long);
static void vcons_copyrows_buffer(void *, int, int, int);
static void vcons_eraserows_buffer(void *, int, int, long);
static void vcons_putchar_buffer(void *, int, int, u_int, long);

/*
 * actual wrapper methods which call both the _buffer ones above and the
 * driver supplied ones to do the drawing
 */
static void vcons_copycols(void *, int, int, int, int);
static void vcons_erasecols(void *, int, int, int, long);
static void vcons_copyrows(void *, int, int, int);
static void vcons_eraserows(void *, int, int, long);
static void vcons_putchar(void *, int, int, u_int, long);
static void vcons_cursor(void *, int, int, int);

/* support for readin/writing text buffers. For wsmoused */
static int  vcons_putwschar(struct vcons_screen *, struct wsdisplay_char *);
static int  vcons_getwschar(struct vcons_screen *, struct wsdisplay_char *);

static void vcons_lock(struct vcons_screen *);
static void vcons_unlock(struct vcons_screen *);


int
vcons_init(struct vcons_data *vd, void *cookie, struct wsscreen_descr *def,
    struct wsdisplay_accessops *ao)
{

	/* zero out everything so we can rely on untouched fields being 0 */
	memset(vd, 0, sizeof(struct vcons_data));
	
	vd->cookie = cookie;

	vd->init_screen = vcons_dummy_init_screen;
	vd->show_screen_cb = NULL;

	/* keep a copy of the accessops that we replace below with our
	 * own wrappers */
	vd->ioctl = ao->ioctl;

	/* configure the accessops */
	ao->ioctl = vcons_ioctl;
	ao->alloc_screen = vcons_alloc_screen;
	ao->free_screen = vcons_free_screen;
	ao->show_screen = vcons_show_screen;

	LIST_INIT(&vd->screens);
	vd->active = NULL;
	vd->wanted = NULL;
	vd->currenttype = def;
	callout_init(&vd->switch_callout, 0);

	/*
	 * a lock to serialize access to the framebuffer.
	 * when switching screens we need to make sure there's no rasops
	 * operation in progress
	 */
#ifdef DIAGNOSTIC
	vd->switch_poll_count = 0;
#endif
	return 0;
}

static void
vcons_lock(struct vcons_screen *scr)
{
#ifdef VCONS_PARANOIA
	int s;

	s = splhigh();
#endif
	SCREEN_BUSY(scr);
#ifdef VCONS_PARANOIA
	splx(s);
#endif
}

static void
vcons_unlock(struct vcons_screen *scr)
{
#ifdef VCONS_PARANOIA
	int s;

	s = splhigh();
#endif
	SCREEN_IDLE(scr);
#ifdef VCONS_PARANOIA
	splx(s);
#endif
}

static void
vcons_dummy_init_screen(void *cookie,
    struct vcons_screen *scr, int exists,
    long *defattr)
{

	/*
	 * default init_screen() method.
	 * Needs to be overwritten so we bitch and whine in case anyone ends
	 * up in here.
	 */
	printf("vcons_init_screen: dummy function called. Your driver is "
	       "supposed to supply a replacement for proper operation\n");
}

int
vcons_init_screen(struct vcons_data *vd, struct vcons_screen *scr,
    int existing, long *defattr)
{
	struct rasops_info *ri = &scr->scr_ri;
	int cnt, i;

	scr->scr_cookie = vd->cookie;
	scr->scr_vd = scr->scr_origvd = vd;
	SCREEN_IDLE(scr);
	
	/*
	 * call the driver-supplied init_screen function which is expected
	 * to set up rasops_info, override cursor() and probably others
	 */
	vd->init_screen(vd->cookie, scr, existing, defattr);

	/* 
	 * save the non virtual console aware rasops and replace them with 
	 * our wrappers
	 */
	vd->eraserows = ri->ri_ops.eraserows;
	vd->copyrows  = ri->ri_ops.copyrows;
	vd->erasecols = ri->ri_ops.erasecols;
	vd->copycols  = ri->ri_ops.copycols;
	vd->putchar   = ri->ri_ops.putchar;
	vd->cursor    = ri->ri_ops.cursor;

	ri->ri_ops.eraserows = vcons_eraserows;	
	ri->ri_ops.copyrows  = vcons_copyrows;	
	ri->ri_ops.erasecols = vcons_erasecols;	
	ri->ri_ops.copycols  = vcons_copycols;	
	ri->ri_ops.putchar   = vcons_putchar;
	ri->ri_ops.cursor    = vcons_cursor;
	ri->ri_hw = scr;

	/* 
	 * we allocate both chars and attributes in one chunk, attributes first 
	 * because they have the (potentially) bigger alignment 
	 */
	cnt = ri->ri_rows * ri->ri_cols;
	scr->scr_attrs = (long *)malloc(cnt * (sizeof(long) + 
	    sizeof(uint16_t)), M_DEVBUF, M_WAITOK);
	if (scr->scr_attrs == NULL)
		return ENOMEM;

	scr->scr_chars = (uint16_t *)&scr->scr_attrs[cnt];

	ri->ri_ops.allocattr(ri, WS_DEFAULT_FG, WS_DEFAULT_BG, 0, defattr);
	scr->scr_defattr = *defattr;

	/* 
	 * fill the attribute buffer with *defattr, chars with 0x20 
	 * since we don't know if the driver tries to mimic firmware output or
	 * reset everything we do nothing to VRAM here, any driver that feels
	 * the need to clear screen or something will have to do it on its own
	 * Additional screens will start out in the background anyway so
	 * cleaning or not only really affects the initial console screen
	 */
	for (i = 0; i < cnt; i++) {
		scr->scr_attrs[i] = *defattr;
		scr->scr_chars[i] = 0x20;
	}

	if(vd->active == NULL) {
		vd->active = scr;
		SCREEN_VISIBLE(scr);
	}
	
	if (existing) {
		SCREEN_VISIBLE(scr);
		vd->active = scr;
	} else {
		SCREEN_INVISIBLE(scr);
	}
	
	LIST_INSERT_HEAD(&vd->screens, scr, next);
	return 0;
}

static void
vcons_do_switch(struct vcons_data *vd)
{
	struct vcons_screen *scr, *oldscr;

	scr = vd->wanted;
	if (!scr) {
		printf("vcons_switch_screen: disappeared\n");
		vd->switch_cb(vd->switch_cb_arg, EIO, 0);
		return;
	}
	oldscr = vd->active; /* can be NULL! */

	/* 
	 * if there's an old, visible screen we mark it invisible and wait
	 * until it's not busy so we can safely switch 
	 */
	if (oldscr != NULL) {
		SCREEN_INVISIBLE(oldscr);
		if (SCREEN_IS_BUSY(oldscr)) {
			callout_reset(&vd->switch_callout, 1,
			    (void(*)(void *))vcons_do_switch, vd);
#ifdef DIAGNOSTIC
			/* bitch if we wait too long */
			vd->switch_poll_count++;
			if (vd->switch_poll_count > 100) {
				panic("vcons: screen still busy");
			}
#endif
			return;
		}
		/* invisible screen -> no visible cursor image */
		oldscr->scr_ri.ri_flg &= ~RI_CURSOR;
#ifdef DIAGNOSTIC
		vd->switch_poll_count = 0;
#endif
	}

	if (scr == oldscr)
		return;

#ifdef DIAGNOSTIC
	if (SCREEN_IS_VISIBLE(scr))
		panic("vcons_switch_screen: already active");
#endif

#ifdef notyet
	if (vd->currenttype != type) {
		vcons_set_screentype(vd, type);
		vd->currenttype = type;
	}
#endif

	SCREEN_VISIBLE(scr);
	vd->active = scr;
	vd->wanted = NULL;

	if (vd->show_screen_cb != NULL)
		vd->show_screen_cb(scr);

	if ((scr->scr_flags & VCONS_NO_REDRAW) == 0)
		vcons_redraw_screen(scr);

	if (vd->switch_cb)
		vd->switch_cb(vd->switch_cb_arg, 0, 0);
}

void
vcons_redraw_screen(struct vcons_screen *scr)
{
	uint16_t *charptr = scr->scr_chars;
	long *attrptr = scr->scr_attrs;
	struct rasops_info *ri = &scr->scr_ri;
	int i, j, offset;

	vcons_lock(scr);
	if (SCREEN_IS_VISIBLE(scr) && SCREEN_CAN_DRAW(scr)) {

		/*
		 * only clear the screen when RI_FULLCLEAR is set since we're
		 * going to overwrite every single character cell anyway
		 */
		if (ri->ri_flg & RI_FULLCLEAR) {
			scr->scr_vd->eraserows(ri, 0, ri->ri_rows,
			    scr->scr_defattr);
		}

		/* redraw the screen */
		offset = 0;
		for (i = 0; i < ri->ri_rows; i++) {
			for (j = 0; j < ri->ri_cols; j++) {
				/*
				 * no need to use the wrapper function - we 
				 * don't change any characters or attributes
				 * and we already made sure the screen we're
				 * working on is visible
				 */
				scr->scr_vd->putchar(ri, i, j, 
				    charptr[offset], attrptr[offset]);
				offset++;
			}
		}
		ri->ri_flg &= ~RI_CURSOR;
		scr->scr_vd->cursor(ri, 1, ri->ri_crow, ri->ri_ccol);
	}
	vcons_unlock(scr);
}

static int
vcons_ioctl(void *v, void *vs, u_long cmd, void *data, int flag,
	struct lwp *l)
{
	struct vcons_data *vd = v;
	int error;

	switch (cmd) {
	case WSDISPLAYIO_GETWSCHAR:
		error = vcons_getwschar((struct vcons_screen *)vs,
			(struct wsdisplay_char *)data);
		break;

	case WSDISPLAYIO_PUTWSCHAR:
		error = vcons_putwschar((struct vcons_screen *)vs,
			(struct wsdisplay_char *)data);
		break;

	default:
		if (vd->ioctl != NULL)
			error = (*vd->ioctl)(v, vs, cmd, data, flag, l);
		else
			error = EPASSTHROUGH;
	}

	return error;
}

static int
vcons_alloc_screen(void *v, const struct wsscreen_descr *type, void **cookiep,
    int *curxp, int *curyp, long *defattrp)
{
	struct vcons_data *vd = v;
	struct vcons_screen *scr;
	int ret;

	scr = malloc(sizeof(struct vcons_screen), M_DEVBUF, M_WAITOK | M_ZERO);
	if (scr == NULL)
		return ENOMEM;

	scr->scr_flags = 0;		
	scr->scr_status = 0;
	scr->scr_busy = 0;
	scr->scr_type = type;

	ret = vcons_init_screen(vd, scr, 0, defattrp);
	if (ret != 0) {
		free(scr, M_DEVBUF);
		return ret;
	}

	if (vd->active == NULL) {
		SCREEN_VISIBLE(scr);
		vd->active = scr;
		vd->currenttype = type;
	}

	*cookiep = scr;
	*curxp = scr->scr_ri.ri_ccol;
	*curyp = scr->scr_ri.ri_crow;
	return 0;
}

static void
vcons_free_screen(void *v, void *cookie)
{
	struct vcons_data *vd = v;
	struct vcons_screen *scr = cookie;

	vcons_lock(scr);
	/* there should be no rasops activity here */

	LIST_REMOVE(scr, next);

	if ((scr->scr_flags & VCONS_SCREEN_IS_STATIC) == 0) {
		free(scr->scr_attrs, M_DEVBUF);
		free(scr, M_DEVBUF);
	} else {
		/*
		 * maybe we should just restore the old rasops_info methods
		 * and free the character/attribute buffer here?
		 */
#ifdef VCONS_DEBUG
		panic("vcons_free_screen: console");
#else
		printf("vcons_free_screen: console\n");
#endif
	}

	if (vd->active == scr)
		vd->active = NULL;
}

static int
vcons_show_screen(void *v, void *cookie, int waitok,
    void (*cb)(void *, int, int), void *cb_arg)
{
	struct vcons_data *vd = v;
	struct vcons_screen *scr;

	scr = cookie;
	if (scr == vd->active)
		return 0;

	vd->wanted = scr;
	vd->switch_cb = cb;
	vd->switch_cb_arg = cb_arg;
	if (cb) {
		callout_reset(&vd->switch_callout, 0,
		    (void(*)(void *))vcons_do_switch, vd);
		return EAGAIN;
	}

	vcons_do_switch(vd);
	return 0;
}

/* wrappers for rasops_info methods */

static void
vcons_copycols_buffer(void *cookie, int row, int srccol, int dstcol, int ncols)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	int from = srccol + row * ri->ri_cols;
	int to = dstcol + row * ri->ri_cols;

	memmove(&scr->scr_attrs[to], &scr->scr_attrs[from],
	    ncols * sizeof(long));
	memmove(&scr->scr_chars[to], &scr->scr_chars[from],
	    ncols * sizeof(uint16_t));
}

static void
vcons_copycols(void *cookie, int row, int srccol, int dstcol, int ncols)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;

	vcons_copycols_buffer(cookie, row, srccol, dstcol, ncols);

	vcons_lock(scr);
	if (SCREEN_IS_VISIBLE(scr) && SCREEN_CAN_DRAW(scr)) {
		scr->scr_vd->copycols(cookie, row, srccol, dstcol, ncols);
	}
	vcons_unlock(scr);
}

static void
vcons_erasecols_buffer(void *cookie, int row, int startcol, int ncols, long fillattr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	int start = startcol + row * ri->ri_cols;
	int end = start + ncols, i;

	for (i = start; i < end; i++) {
		scr->scr_attrs[i] = fillattr;
		scr->scr_chars[i] = 0x20;
	}
}

static void
vcons_erasecols(void *cookie, int row, int startcol, int ncols, long fillattr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;

	vcons_erasecols_buffer(cookie, row, startcol, ncols, fillattr);

	vcons_lock(scr);
	if (SCREEN_IS_VISIBLE(scr) && SCREEN_CAN_DRAW(scr)) {
		scr->scr_vd->erasecols(cookie, row, startcol, ncols, 
		    fillattr);
	}
	vcons_unlock(scr);
}

static void
vcons_copyrows_buffer(void *cookie, int srcrow, int dstrow, int nrows)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	int from, to, len;

	from = ri->ri_cols * srcrow;
	to = ri->ri_cols * dstrow;
	len = ri->ri_cols * nrows;

	memmove(&scr->scr_attrs[to], &scr->scr_attrs[from],
	    len * sizeof(long));
	memmove(&scr->scr_chars[to], &scr->scr_chars[from],
	    len * sizeof(uint16_t));
}

static void
vcons_copyrows(void *cookie, int srcrow, int dstrow, int nrows)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;

	vcons_copyrows_buffer(cookie, srcrow, dstrow, nrows);

	vcons_lock(scr);
	if (SCREEN_IS_VISIBLE(scr) && SCREEN_CAN_DRAW(scr)) {
		scr->scr_vd->copyrows(cookie, srcrow, dstrow, nrows);
	}
	vcons_unlock(scr);
}

static void
vcons_eraserows_buffer(void *cookie, int row, int nrows, long fillattr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	int start, end, i;

	start = ri->ri_cols * row;
	end = ri->ri_cols * (row + nrows);

	for (i = start; i < end; i++) {
		scr->scr_attrs[i] = fillattr;
		scr->scr_chars[i] = 0x20;
	}
}

static void
vcons_eraserows(void *cookie, int row, int nrows, long fillattr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;

	vcons_eraserows_buffer(cookie, row, nrows, fillattr);

	vcons_lock(scr);
	if (SCREEN_IS_VISIBLE(scr) && SCREEN_CAN_DRAW(scr)) {
		scr->scr_vd->eraserows(cookie, row, nrows, fillattr);
	}
	vcons_unlock(scr);
}

static void
vcons_putchar_buffer(void *cookie, int row, int col, u_int c, long attr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	int pos;
	
	if ((row >= 0) && (row < ri->ri_rows) && (col >= 0) && 
	     (col < ri->ri_cols)) {
		pos = col + row * ri->ri_cols;
		scr->scr_attrs[pos] = attr;
		scr->scr_chars[pos] = c;
	}
}

static void
vcons_putchar(void *cookie, int row, int col, u_int c, long attr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	
	vcons_putchar_buffer(cookie, row, col, c, attr);
	
	vcons_lock(scr);
	if (SCREEN_IS_VISIBLE(scr) && SCREEN_CAN_DRAW(scr)) {
		scr->scr_vd->putchar(cookie, row, col, c, attr);
	}
	vcons_unlock(scr);
}

static void
vcons_cursor(void *cookie, int on, int row, int col)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;

	vcons_lock(scr);
	if (SCREEN_IS_VISIBLE(scr) && SCREEN_CAN_DRAW(scr)) {
		scr->scr_vd->cursor(cookie, on, row, col);
	} else {
		scr->scr_ri.ri_crow = row;
		scr->scr_ri.ri_ccol = col;
	}
	vcons_unlock(scr);
}

/* methods to read/write characters via ioctl() */

static int
vcons_putwschar(struct vcons_screen *scr, struct wsdisplay_char *wsc)
{
	long attr;
	struct rasops_info *ri;

	KASSERT(scr != NULL && wsc != NULL);

	if (__predict_false((unsigned int)wsc->col > ri->ri_cols ||
	    (unsigned int)wsc->row > ri->ri_rows))
			return (EINVAL);

	ri = &scr->scr_ri;
	
	ri->ri_ops.allocattr(ri, wsc->foreground, wsc->background,
	    wsc->flags, &attr);
	vcons_putchar(ri, wsc->row, wsc->col, wsc->letter, attr);
	return 0;
}

static int
vcons_getwschar(struct vcons_screen *scr, struct wsdisplay_char *wsc)
{
	int offset;
	long attr;
	struct rasops_info *ri;

	KASSERT(scr != NULL && wsc != NULL);

	ri = &scr->scr_ri;
	offset = ri->ri_cols * wsc->row + wsc->col;
	wsc->letter = scr->scr_chars[offset];
	attr = scr->scr_attrs[offset];

	/* 
	 * this is ugly. We need to break up an attribute into colours and
	 * flags but there's no rasops method to do that so we must rely on
	 * the 'canonical' encoding.
	 */
	wsc->foreground = (attr & 0xff000000) >> 24;
	wsc->background = (attr & 0x00ff0000) >> 16;
	wsc->flags      = (attr & 0x0000ff00) >> 8;
	return 0;
}
