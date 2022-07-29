/*	$NetBSD: printk.h,v 1.13 2022/07/29 23:50:44 riastradh Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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

#ifndef _LINUX_PRINTK_H_
#define _LINUX_PRINTK_H_

#include <sys/param.h>
#include <sys/systm.h>
#include <linux/export.h>

#define	printk		printf
#define	vprintk		vprintf
#define	printk_once	printf
#define	pr_err		printf	/* XXX */
#define	pr_cont		printf	/* XXX */
#define	pr_info		printf	/* XXX */
#define	pr_info_once	printf	/* XXX */
#define	pr_info_ratelimited	printf	/* XXX */
#define	pr_warn		printf	/* XXX */
#define	pr_warn_once	printf	/* XXX */
#define	pr_notice	printf	/* XXX */
#define	pr_debug	printf	/* XXX */
#define	KERN_EMERG	"emerg: "
#define	KERN_ALERT	"alert: "
#define	KERN_CRIT	"crit: "
#define	KERN_ERR	"error: "
#define	KERN_WARNING	"warning: "
#define	KERN_NOTICE	"notice: "
#define	KERN_INFO	""
#define	KERN_DEBUG	"debug: "
#define	KERN_CONT	""

#define	printk_ratelimit()	0 /* XXX */

struct va_format {
	const char	*fmt;
	va_list		*va;
};

#define	DUMP_PREFIX_NONE	0
#define	DUMP_PREFIX_OFFSET	1
#define	DUMP_PREFIX_ADDRESS	2

static inline size_t
hex_dump_to_buffer(const void *buf, size_t buf_size, int bytes_per_line,
    int bytes_per_group, char *output, size_t output_size, bool ascii __unused)
{
	const uint8_t *bytes = buf;
	int i = 0, t = 0, n;

	KASSERT(output_size >= 1);
	KASSERT((bytes_per_line == 16) || (bytes_per_line == 32));
	KASSERT(powerof2(bytes_per_group));
	KASSERT(0 < bytes_per_group);
	KASSERT(bytes_per_group <= 8);

	output[output_size - 1] = '\0';
	while (i < buf_size) {
		n = snprintf(output, output_size, "%02x", bytes[i++]);
		t += n;
		if (n >= output_size)
			break;
		output += n; output_size -= n;
		if ((i == buf_size) || (0 == (i % bytes_per_line)))
			n = snprintf(output, output_size, "\n");
		else if ((0 < i) && (0 == (i % bytes_per_group)))
			n = snprintf(output, output_size, " ");
		else
			n = 0;
		t += n;
		if (n >= output_size)
			break;
		output += n; output_size -= n;
	}

	return t;
}

static inline void
print_hex_dump(const char *level, const char *prefix, int prefix_type,
    int bytes_per_line, int bytes_per_group, const void *buf, size_t buf_size,
    bool ascii)
{
	const uint8_t *bytes = buf;
	/* Two digits and one space/newline per byte, plus a null.  */
	char line[32*3 + 1];

	KASSERT((bytes_per_line == 16) || (bytes_per_line == 32));
	KASSERT(powerof2(bytes_per_group));
	KASSERT(0 < bytes_per_group);
	KASSERT(bytes_per_group <= 8);

	while (0 < buf_size) {
		const size_t n = MIN(buf_size, 32);

		switch (prefix_type) {
		case DUMP_PREFIX_OFFSET:
			printf("%08zu: ", (bytes - (const uint8_t *)buf));
			break;

		case DUMP_PREFIX_ADDRESS:
			printf("%p: ", bytes);
			break;

		case DUMP_PREFIX_NONE:
		default:
			break;
		}

		hex_dump_to_buffer(bytes, n, bytes_per_line, bytes_per_group,
		    line, sizeof(line), ascii);
		KASSERT(0 < strnlen(line, sizeof(line)));
		KASSERT(line[strnlen(line, sizeof(line)) - 1] == '\n');
		printf("%s", line);

		bytes += n;
		buf_size -= n;
	}
}

#endif  /* _LINUX_PRINTK_H_ */
