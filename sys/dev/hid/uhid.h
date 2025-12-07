/* $NetBSD: uhid.h,v 1.1 2025/12/07 10:05:10 jmcneill Exp $ */

/*-
 * Copyright (c) 2025 Jared McNeill <jmcneill@invisible.ca>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _DEV_HID_UHID_H_
#define _DEV_HID_UHID_H_

#include <sys/device.h>

#include <dev/hid/hidev.h>

struct uhid_softc {
        device_t sc_dev;
	hidev_tag_t sc_hidev;
        uint8_t sc_report_id;
	int (*sc_ioctl)(struct uhid_softc *, u_long, void *, int, struct lwp *);

        kmutex_t sc_lock;
        kcondvar_t sc_cv;

        int sc_isize;
        int sc_osize;
        int sc_fsize;

        u_char *sc_obuf;

        struct clist sc_q;      /* protected by sc_lock */
        struct selinfo sc_rsel;
        proc_t *sc_async;       /* process that wants SIGIO */
        void *sc_sih;
        volatile uint32_t sc_state;     /* driver state */
#define UHID_IMMED      0x02    /* return read data immediately */

        int sc_raw;
        enum {
                UHID_CLOSED,
                UHID_OPENING,
                UHID_OPEN,
        } sc_open;
        bool sc_closing;
};

void	uhid_attach_common(struct uhid_softc *);
int	uhid_detach_common(struct uhid_softc *);

#endif /* _DEV_HID_UHID_H_ */
