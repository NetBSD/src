/*	$NetBSD: apei_reg.c,v 1.1 2024/03/20 17:11:44 riastradh Exp $	*/

/*-
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
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

/*
 * APEI register access for ERST/EINJ action instructions
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: apei_reg.c,v 1.1 2024/03/20 17:11:44 riastradh Exp $");

#include <sys/types.h>

#include <dev/acpi/acpivar.h>
#include <dev/acpi/apei_reg.h>

/*
 * apei_read_register(Register, Mask, &X)
 *
 *	Read from Register, shifted out of position and then masked
 *	with Mask, and store the result in X.
 *
 *	https://uefi.org/specs/ACPI/6.5/18_Platform_Error_Interfaces.html#read-register
 *
 *	(I'm guessing this applies to both ERST and EINJ, even though
 *	that section is under the ERST part.)
 */
ACPI_STATUS
apei_read_register(ACPI_GENERIC_ADDRESS *Register, uint64_t Mask, uint64_t *p)
{
	const uint8_t BitOffset = Register->BitOffset;
	uint64_t X;
	ACPI_STATUS rv;

	rv = AcpiRead(&X, Register);
	if (ACPI_FAILURE(rv)) {
		*p = 0;		/* XXX */
		return rv;
	}
	X >>= BitOffset;
	X &= Mask;

	*p = X;
	return AE_OK;
}

/*
 * apei_write_register(Register, Mask, preserve_register, X)
 *
 *	Write X, masked with Mask and shifted into position, to
 *	Register, preserving other bits if preserve_register is true.
 *
 *	https://uefi.org/specs/ACPI/6.5/18_Platform_Error_Interfaces.html#write-register
 *
 *	Note: The Preserve Register semantics is based on the clearer
 *	indentation at
 *	https://uefi.org/sites/default/files/resources/ACPI_5_1release.pdf#page=714
 *	which has been lost in more recent versions of the spec.
 */
ACPI_STATUS
apei_write_register(ACPI_GENERIC_ADDRESS *Register, uint64_t Mask,
    bool preserve_register, uint64_t X)
{
	const uint8_t BitOffset = Register->BitOffset;
	ACPI_STATUS rv;

	X &= Mask;
	X <<= BitOffset;
	if (preserve_register) {
		uint64_t Y;

		rv = AcpiRead(&Y, Register);
		if (ACPI_FAILURE(rv))
			return rv;
		Y &= ~(Mask << BitOffset);
		X |= Y;
	}
	return AcpiWrite(X, Register);
}
