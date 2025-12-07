/* $NetBSD: hidev.c,v 1.2 2025/12/07 19:59:51 jmcneill Exp $ */

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

#include <dev/hid/hidev.h>

void
hidev_get_report_desc(struct hidev_tag *t, void **desc, int *size)
{
	t->_get_report_desc(t->_cookie, desc, size);
}

int
hidev_open(struct hidev_tag *t, void (*intr)(void *, void *, u_int), void *cookie)
{
	return t->_open(t->_cookie, intr, cookie);
}

void
hidev_stop(struct hidev_tag *t)
{
	t->_stop(t->_cookie);
}

void
hidev_close(struct hidev_tag *t)
{
	t->_close(t->_cookie);
}

usbd_status
hidev_set_report(struct hidev_tag *t, int type, void *data, int len)
{
	return t->_set_report(t->_cookie, type, data, len);
}

usbd_status
hidev_get_report(struct hidev_tag *t, int type, void *data, int len)
{
	return t->_get_report(t->_cookie, type, data, len);
}

usbd_status
hidev_write(struct hidev_tag *t, void *data, int len)
{
	return t->_write(t->_cookie, data, len);
}
