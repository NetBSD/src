/*	$NetBSD: fdt_platform.h,v 1.2 2026/08/02 16:41:59 thorpej Exp $	*/

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

#ifndef _M68K_FDT_PLATFORM_H_
#define	_M68K_FDT_PLATFORM_H_

struct fdt_platform {
	/*
	 * paddr_t fp_bootstrap(paddr_t nextpa, vaddr_t reloff)
	 *
	 *	Very early bootstrap routine, mainly for the purpose of
	 *	providing data necessary to configure the MMU.
	 *
	 *	Arguments:
	 *	nextpa		The first physical address beyond the
	 *			static kernel image and associated data.
	 *
	 *	reloff		Relocation offset necessary to access
	 *			global variables.
	 *
	 *	Returns:	Adjusted value of nextpa argument if
	 *			fp_bootstrap() allocated any additional
	 *			memory.
	 */
	paddr_t			(*fp_bootstrap)(paddr_t, vaddr_t);

	/*
	 * paddr_t fp_machine_init(paddr_t nextpa)
	 *
	 *	Early machine init routine.  Makes any additional
	 *	machine-specific initializations or allocations
	 *	before main() is called.
	 *
	 *	Arguments:
	 *	nextpa		The first physical address beyond the
	 *			static kernel image and associated data.
	 *
	 *	Returns:	Adjusted value of nextpa argument if
	 *			fp_machine_init() allocated any additional
	 *			memory.
	 */
	paddr_t			(*fp_machine_init)(paddr_t);

	/*
	 * u_int fp_uart_freq(void)
	 *
	 *	Returns the clock frequency of the console UART's
	 *	baud rate generator if this information cannot be
	 *	obtained from the device tree.
	 */
	u_int			(*fp_uart_freq)(void);

	/*
	 * void fp_device_register(device_t dev, void *aux)
	 *
	 *	Platform-specific hook for device_register(),
	 *	which is called as device driver instances are
	 *	attached during autoconfiguration.  Called just
	 *	prior to the driver's "attach" routine.  This can
	 *	be used to determine the boot device or set
	 *	additional device properties for the driver to
	 *	consume.
	 *
	 *	Arguments:
	 *	dev		The device_t associated with the driver
	 *			instance being attached.
	 *
	 *	aux		The "attach_args" specific to that driver
	 *			instance.
	 */
	void			(*fp_device_register)(device_t, void *);

	/*
	 * fp_powerdown(void)
	 *
	 *	Platform-specific power-down routine.
	 */
	void			(*fp_powerdown)(void);

	/*
	 * fp_halt(void)
	 *
	 *	Platform-specific halt routine.
	 */
	void			(*fp_halt)(void);

	/*
	 * fp_reboot(int howto, char *bootstr)
	 *
	 *	Platform-specific reboot routine.
	 *
	 *	Arguments:
	 *	howto		RB_* flags that indicate what sort of
	 *			reboot is to take place.
	 *
	 *	bootstr		A string with boot arguments for the
	 *			subsequent reboot.  Simply ignore them
	 *			if your platform's firmware doesn't have
	 *			this capability.
	 */
	void			(*fp_reboot)(int, char *);
};

paddr_t	fdt_bootstrap1(paddr_t, vaddr_t);

#endif /* _M68K_FDT_PLATFORM_H_ */
