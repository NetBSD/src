/*	$NetBSD: xdrtest.c,v 1.3 2002/02/10 13:22:58 bjh21 Exp $	*/

/*-
 * Copyright (c) 2001 Ben Harris
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <err.h>
#include <stdio.h>

#include <rpc/types.h>
#include <rpc/xdr.h>

#include <testbits.h>

char xdrdata[] = {
	0x3f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* double 1.0 */
	0x00, 0x00, 0x00, 0x01, /* enum smallenum SE_ONE */
	0xff, 0xff, 0xfb, 0x2e, /* enum medenum ME_NEG */
	0x00, 0x12, 0xd6, 0x87, /* enum bigenum BE_LOTS */
};

int
main(int argc, char **argv)
{
	XDR x;
	double d;
	smallenum s;
	medenum m;
	bigenum b;
	char newdata[sizeof(xdrdata)];

	xdrmem_create(&x, xdrdata, sizeof(xdrdata), XDR_DECODE);
	if (!xdr_double(&x, &d))
		errx(1, "xdr_double DECODE failed.");
	if (d != 1.0)
		errx(1, "double 1.0 decoded as %g.", d);
	if (!xdr_smallenum(&x, &s))
		errx(1, "xdr_smallenum DECODE failed.");
	if (s != SE_ONE)
		errx(1, "SE_ONE decoded as %d.", s);
	if (!xdr_medenum(&x, &m))
		errx(1, "xdr_medenum DECODE failed.");
	if (m != ME_NEG)
		errx(1, "ME_NEG decoded as %d.", m);
	if (!xdr_bigenum(&x, &b))
		errx(1, "xdr_bigenum DECODE failed.");
	if (b != BE_LOTS)
		errx(1, "BE_LOTS decoded as %d.", b);
	xdr_destroy(&x);

	xdrmem_create(&x, newdata, sizeof(newdata), XDR_ENCODE);
	if (!xdr_double(&x, &d))
		errx(1, "xdr_double ENCODE failed.");
	if (!xdr_smallenum(&x, &s))
		errx(1, "xdr_smallenum ENCODE failed.");
	if (!xdr_medenum(&x, &m))
		errx(1, "xdr_medenum ENCODE failed.");
	if (!xdr_bigenum(&x, &b))
		errx(1, "xdr_bigenum ENCODE failed.");
	if (memcmp(newdata, xdrdata, sizeof(xdrdata)) != 0)
		errx(1, "xdr ENCODE result differs.");
	xdr_destroy(&x);

	exit(0);
}
