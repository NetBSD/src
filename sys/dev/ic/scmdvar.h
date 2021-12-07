/*	$NetBSD: scmdvar.h,v 1.1 2021/12/07 17:39:54 brad Exp $	*/

/*
 * Copyright (c) 2021 Brad Spencer <brad@anduin.eldar.org>
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

#ifndef _DEV_SCMDVAR_H_
#define _DEV_SCMDVAR_H_

struct scmd_sc {
	int 		sc_scmddebug;
	device_t 	sc_dev;
	i2c_tag_t 	sc_tag;
	i2c_addr_t 	sc_addr;
	struct spi_handle *sc_sh;
	kmutex_t 	sc_mutex; /* for reading the i2c or spi bus */
	kmutex_t        sc_dying_mutex; /* for cleaning up */
	kmutex_t 	sc_condmutex; /* for waiting a long time */
	kcondvar_t	sc_condvar;
	kcondvar_t	sc_cond_dying; /* interlock when cleaning up */
	struct sysctllog *sc_scmdlog;
	uint8_t		sc_topaddr;
	bool		sc_dying;
	bool		sc_opened;
	void		(*sc_func_attach)(struct scmd_sc *);
	int		(*sc_func_acquire_bus)(struct scmd_sc *);
	void		(*sc_func_release_bus)(struct scmd_sc *);
	int		(*sc_func_read_register)(struct scmd_sc *, uint8_t, uint8_t *);
	int		(*sc_func_write_register)(struct scmd_sc *, uint8_t, uint8_t);
};

#endif
