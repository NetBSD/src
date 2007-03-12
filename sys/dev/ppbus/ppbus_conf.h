/* $NetBSD: ppbus_conf.h,v 1.6.26.1 2007/03/12 05:56:47 rmind Exp $ */

/*-
 * Copyright (c) 1997, 1998, 1999 Nicolas Souchu
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/dev/ppbus/ppbconf.h,v 1.17.2.1 2000/05/24 00:20:57 n_hibma Exp $
 *
 */
#ifndef __PPBUS_CONF_H
#define __PPBUS_CONF_H

#include <sys/device.h>
#include <sys/lock.h>
#include <sys/queue.h>

#include <machine/bus.h>

#include <dev/ppbus/ppbus_msq.h>
#include <dev/ppbus/ppbus_device.h>


/* Function pointer types used for interface */
typedef u_char (*PARPORT_IO_T)(struct device *, int, u_char *, int, u_char);
typedef int (*PARPORT_EXEC_MICROSEQ_T)(struct device *,
	struct ppbus_microseq **);
typedef void (*PARPORT_RESET_EPP_TIMEOUT_T)(struct device *);
typedef int (*PARPORT_SETMODE_T)(struct device *, int);
typedef int (*PARPORT_GETMODE_T)(struct device *);
typedef void (*PARPORT_ECP_SYNC_T)(struct device *);
typedef int (*PARPORT_READ_T)(struct device *, char *, int, int, size_t *);
typedef int (*PARPORT_WRITE_T)(struct device *, char *, int, int, size_t *);
typedef int (*PARPORT_READ_IVAR_T)(struct device *, int, unsigned int *);
typedef int (*PARPORT_WRITE_IVAR_T)(struct device *, int, unsigned int *);
typedef int (*PARPORT_DMA_MALLOC_T)(struct device *, void **, bus_addr_t *,
	bus_size_t);
typedef void (*PARPORT_DMA_FREE_T)(struct device *, void **, bus_addr_t *,
	bus_size_t);
typedef int (*PARPORT_ADD_HANDLER_T)(struct device *, void (*)(void *),
	void *);
typedef int (*PARPORT_REMOVE_HANDLER_T)(struct device *, void (*)(void *));

/* Adapter structure that each parport device needs to implement ppbus */
struct parport_adapter {
	u_int16_t capabilities;

	/* Functions which make up interface */
	PARPORT_IO_T parport_io;
	PARPORT_EXEC_MICROSEQ_T parport_exec_microseq;
	PARPORT_RESET_EPP_TIMEOUT_T parport_reset_epp_timeout;
	PARPORT_SETMODE_T parport_setmode;
	PARPORT_GETMODE_T parport_getmode;
	PARPORT_ECP_SYNC_T parport_ecp_sync;
	PARPORT_READ_T parport_read;
	PARPORT_WRITE_T parport_write;
	PARPORT_READ_IVAR_T parport_read_ivar;
	PARPORT_WRITE_IVAR_T parport_write_ivar;
	PARPORT_DMA_MALLOC_T parport_dma_malloc;
	PARPORT_DMA_FREE_T parport_dma_free;
	PARPORT_ADD_HANDLER_T parport_add_handler;
	PARPORT_REMOVE_HANDLER_T parport_remove_handler;
};

/* Parallel Port Bus configuration structure. */
struct ppbus_softc {
	struct device sc_dev;

	/* Lock for critical section when requesting/releasing the bus */
	struct lock sc_lock;

#define PPBUS_OK 1
#define PPBUS_NOK 0
	u_int8_t sc_dev_ok;

	/* ppbus capabilities (see ppbus_var.h) */
	u_int16_t sc_capabilities;

/* PnP device type defined in ppbus_var.h */
	int sc_class_id;		/* not a PnP device if class_id < 0 */

	/* Defined in pbus_1284.h: error and host side state. */
	u_int32_t sc_1284_state;	/* current IEEE1284 state */
	u_int32_t sc_1284_error;	/* last IEEE1284 error */

	/* Use IEEE 1284 negotiations in mode changes and direction changes */
	u_int32_t sc_use_ieee;

/* PPBUS mode masks defined in ppbus_var.h. */
	u_int32_t sc_mode;		/* IEEE 1284-1994 mode */

	/* ppbus_device which owns the bus */
	struct device * ppbus_owner;

	/* Head of list of child devices */
	SLIST_HEAD(childlist, ppbus_device_softc) sc_childlist_head;

	/* Functions which make up interface */
	PARPORT_IO_T ppbus_io;
	PARPORT_EXEC_MICROSEQ_T ppbus_exec_microseq;
	PARPORT_RESET_EPP_TIMEOUT_T ppbus_reset_epp_timeout;
	PARPORT_SETMODE_T ppbus_setmode;
	PARPORT_GETMODE_T ppbus_getmode;
	PARPORT_ECP_SYNC_T ppbus_ecp_sync;
	PARPORT_READ_T ppbus_read;
	PARPORT_WRITE_T ppbus_write;
        PARPORT_READ_IVAR_T ppbus_read_ivar;
        PARPORT_WRITE_IVAR_T ppbus_write_ivar;
	PARPORT_DMA_MALLOC_T ppbus_dma_malloc;
	PARPORT_DMA_FREE_T ppbus_dma_free;
	PARPORT_ADD_HANDLER_T ppbus_add_handler;
	PARPORT_REMOVE_HANDLER_T ppbus_remove_handler;
};

#endif /* __PPBUS_CONF_H */
