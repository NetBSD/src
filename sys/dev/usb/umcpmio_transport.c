/*	$NetBSD: umcpmio_transport.c,v 1.1 2025/11/29 18:39:14 brad Exp $	*/

/*
 * Copyright (c) 2024, 2025 Brad Spencer <brad@anduin.eldar.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: umcpmio_transport.c,v 1.1 2025/11/29 18:39:14 brad Exp $");

#ifdef _KERNEL_OPT
#include "opt_usb.h"
#endif

#include <sys/param.h>
#include <sys/types.h>

#include <dev/hid/hid.h>

#include <dev/usb/uhidev.h>
#include <dev/usb/usb.h>
#include <dev/usb/usbdevs.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbhid.h>

#include <dev/usb/umcpmio.h>
#include <dev/usb/umcpmio_hid_reports.h>
#include <dev/usb/umcpmio_transport.h>
#include <dev/usb/umcpmio_io.h>
#include <dev/usb/umcpmio_subr.h>

#define UMCPMIO_DEBUG 1
#ifdef UMCPMIO_DEBUG
#define DPRINTF(x)	do { if (umcpmiodebug) printf x; } while (0)
#define DPRINTFN(n, x)	do { if (umcpmiodebug > (n)) printf x; } while (0)
extern int umcpmiodebug;
#else
#define DPRINTF(x)	__nothing
#define DPRINTFN(n, x)	__nothing
#endif


/* Stuff to communicate with the MCP2210 and MCP2221 / MCP2221A */


/*
 * Communication with the HID function requires sending a HID report
 * request and then waiting for a response.
 *
 * The panic that occurs when trying to use the interrupt... i.e.
 * attaching though this driver seems to be related to the fact that
 * a spin lock is held and the USB stack wants to wait.
 *
 * The USB stack *IS* going to have to wait for the response from
 * the device, somehow...
 *
 * It didn't seem possible to defer the uhidev_write to a thread.
 * Attempts to yield() while spinning hard also did not work and
 * not yield()ing didn't allow anything else to run.
 *
 */

/*
 * This is the panic you will get:
 *
 * panic: kernel diagnostic assertion "ci->ci_mtx_count == -1" failed: file "../../../../kern/kern_synch.c", line 762 mi_switch: cpu0: ci_mtx_count (-2) != -1 (block with spin-mutex held)
 */

static void
umcpmio_uhidev_intr(void *cookie, void *ibuf, u_int len)
{
	struct umcpmio_softc *sc = cookie;

	if (sc->sc_dying)
		return;

	DPRINTFN(30, ("umcpmio_uhidev_intr: len=%d\n", len));

	mutex_enter(&sc->sc_res_mutex);
	switch (len) {
	case MCP2221_RES_BUFFER_SIZE:
		if (sc->sc_res_buffer != NULL) {
			memcpy(sc->sc_res_buffer, ibuf,
			    MCP2221_RES_BUFFER_SIZE);
			sc->sc_res_ready = true;
			cv_signal(&sc->sc_res_cv);
		} else {
			int d = umcpmiodebug;
			device_printf(sc->sc_dev,
			    "umcpmio_uhidev_intr: NULL sc_res_buffer:"
			    " len=%d\n",
			    len);
			umcpmiodebug = 20;
			umcpmio_dump_buffer(true, (uint8_t *) ibuf, len,
			    "umcpmio_uhidev_intr: ibuf");
			umcpmiodebug = d;
		}

		break;
	default:
		device_printf(sc->sc_dev,
		    "umcpmio_uhidev_intr: Unknown interrupt length: %d",
		    len);
		break;
	}
	mutex_exit(&sc->sc_res_mutex);
}

/* Send a HID report.  This needs to be called with the action mutex held */

int
umcpmio_send_report(struct umcpmio_softc *sc, uint8_t *sendbuf,
    size_t sendlen, uint8_t *resbuf, int timeout)
{
	int err = 0;
	int err_count = 0;

	if (sc->sc_dying)
		return EIO;

	KASSERT(mutex_owned(&sc->sc_action_mutex));

	/* If this happens, it may be because the device was pulled from the
	 * USB slot, or sometimes when SIGINTR occurs.  Just consider this a
	 * EIO. */
	if (sc->sc_res_buffer != NULL) {
		sc->sc_res_buffer = NULL;
		sc->sc_res_ready = false;
		err = EIO;
		goto out;
	}
	sc->sc_res_buffer = resbuf;
	sc->sc_res_ready = false;

	err = uhidev_write(sc->sc_hdev, sendbuf, sendlen);

	if (err) {
		DPRINTF(("umcpmio_send_report: uhidev_write errored with:"
		    " err=%d\n", err));
		goto out;
	}
	DPRINTFN(30, ("umcpmio_send_report: about to wait on cv.  err=%d\n",
	    err));

	mutex_enter(&sc->sc_res_mutex);
	while (!sc->sc_res_ready) {
		DPRINTFN(20, ("umcpmio_send_report: LOOP for response."
		    "  sc_res_ready=%d, err_count=%d, timeout=%d\n",
		    sc->sc_res_ready, err_count, mstohz(timeout)));

		err = cv_timedwait_sig(&sc->sc_res_cv, &sc->sc_res_mutex,
		    mstohz(timeout));

		/* We are only going to allow this to loop on an error, any
		 * error at all, so many times. */
		if (err) {
			DPRINTF(("umcpmio_send_report:"
			    " cv_timedwait_sig reported an error:"
			    " err=%d, sc->sc_res_ready=%d\n",
			    err, sc->sc_res_ready));
			err_count++;
		}

		/* The CV was interrupted, but the buffer is ready so, clear
		 * the error and break out. */
		if ((err == ERESTART) && (sc->sc_res_ready)) {
			DPRINTF(("umcpmio_send_report:"
			    " ERESTART and buffer is ready\n"));
			err = 0;
			break;
		}

		/* Too many times though the loop, just break out.  Turn a
		 * ERESTART (interruption) into a I/O error at this point. */
		if (err_count > sc->sc_response_errcnt) {
			DPRINTF(("umcpmio_send_report: err_count exceeded:"
			    " err=%d\n", err));
			if (err == ERESTART)
				err = EIO;
			break;
		}

		/* This is a normal timeout, without interruption, try again */
		if (err == EWOULDBLOCK) {
			DPRINTF(("umcpmio_send_report: EWOULDBLOCK:"
			    " err_count=%d\n", err_count));
			continue;
		}

		/* The CV was interrupted and the buffer wasn't filled in, so
		 * try again */
		if ((err == ERESTART) && (!sc->sc_res_ready)) {
			DPRINTF(("umcpmio_send_report:"
			    " ERESTART and buffer is NOT ready."
			    "  err_count=%d\n", err_count));
			continue;
		}
	}

	sc->sc_res_buffer = NULL;
	sc->sc_res_ready = false;
	mutex_exit(&sc->sc_res_mutex);

	/* Turn most errors into an I/O error */
	if (err &&
	    err != ERESTART)
		err = EIO;

out:
	return err;
}

int
umcpmio_hid_open(struct umcpmio_softc *sc)
{
	int err;

	err = uhidev_open(sc->sc_hdev, &umcpmio_uhidev_intr, sc);
	if (err) {
		aprint_error_dev(sc->sc_dev, "umcpmio_hid_open: "
		    " uhidev_open: err=%d\n", err);
	}

	/* It is not clear that this should be needed, but it was noted that
	 * the MCP2221 / MCP2221A would sometimes not be ready if this delay
	 * was not present.  In fact, the attempts to set stuff a little
	 * later would sometimes fail.
	 */

	if (!err)
		delay(1000);

	return err;
}
