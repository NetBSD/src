/*	$NetBSD: nvmeio.h,v 1.1.18.2 2017/12/03 11:37:03 jdolecek Exp $	*/

/*-
 * Copyright (C) 2012-2013 Intel Corporation
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
 * $FreeBSD: head/sys/dev/nvme/nvme.h 296617 2016-03-10 17:13:10Z mav $
 */

#ifndef __NVMEIO_H__
#define __NVMEIO_H__

#include <sys/ioccom.h>
#include <dev/ic/nvmereg.h>

#define	NVME_PASSTHROUGH_CMD		_IOWR('n', 0, struct nvme_pt_command)

#define	nvme_completion_is_error(cpl)					\
	((NVME_CQE_SC((cpl)->flags) != NVME_CQE_SC_SUCCESS)   		\
	    || (NVME_CQE_SCT((cpl)->flags) != NVME_CQE_SCT_GENERIC))

struct nvme_pt_command {

	/*
	 * cmd is used to specify a passthrough command to a controller or
	 *  namespace.
	 *
	 * The following fields from cmd may be specified by the caller:
	 *	* opcode
	 *	* nsid (namespace id) - for admin commands only
	 *	* cdw10-cdw15
	 *
	 * Remaining fields must be set to 0 by the caller.
	 */
	struct nvme_sqe		cmd;

	/*
	 * cpl returns completion status for the passthrough command
	 *  specified by cmd.
	 *
	 * The following fields will be filled out by the driver, for
	 *  consumption by the caller:
	 *	* cdw0
	 *	* flags (except for phase)
	 *
	 * Remaining fields will be set to 0 by the driver.
	 */
	struct nvme_cqe		cpl;

	/* buf is the data buffer associated with this passthrough command. */
	void			*buf;

	/*
	 * len is the length of the data buffer associated with this
	 *  passthrough command.
	 */
	uint32_t		len;

	/*
	 * is_read = 1 if the passthrough command will read data into the
	 *  supplied buffer from the controller.
	 *
	 * is_read = 0 if the passthrough command will write data from the
	 *  supplied buffer to the controller.
	 */
	uint32_t		is_read;

	/*
	 * timeout (unit: ms)
	 *
	 * 0: use default timeout value
	 */
	uint32_t		timeout;
};

#endif /* __NVMEIO_H__ */
