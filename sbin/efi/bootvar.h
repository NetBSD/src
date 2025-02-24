/* $NetBSD: bootvar.h,v 1.1 2025/02/24 13:47:55 christos Exp $ */

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _BOOTVAR_H_
#define _BOOTVAR_H_

#ifndef lint
__RCSID("$NetBSD: bootvar.h,v 1.1 2025/02/24 13:47:55 christos Exp $");
#endif /* not lint */

/*
 * Structure of Boot####, Device####, SysPrep#### variables.
 */
typedef struct boot_var {
	uint32_t Attributes;
#define LOAD_OPTION_ACTIVE		__BIT(0)
#define IS_ACTIVE(attr)	((attr) & LOAD_OPTION_ACTIVE)
#define LOAD_OPTION_FORCE_RECONNECT	__BIT(1)
#define LOAD_OPTION_HIDDEN		__BIT(3)
#define LOAD_OPTION_CATEGORY		__BITS(12,8)
#define LOAD_OPTION_CATEGORY_BOOT	0
#define LOAD_OPTION_CATEGORY_APP	__BIT(8)
#define LOAD_OPTION_BITS \
	"\177\020"		\
	"b\x00""Active\0"	\
	"b\x01""Reconnect\0"	\
	"b\x03""Hidden\0"	\
	"b\x08""CatApp\0"

	uint16_t FilePathListLength;
	uint16_t Description[];	/* XXX: gcc warns about alignment when packed */
//	devpath_t FilePathList[];
//	uint8_t OptionalData[];
} __packed boot_var_t;

int find_new_bootvar(efi_var_t **, size_t, const char *);
void *make_bootvar_data(const char *, uint, uint32_t, const char *,
    const char *, const char *, size_t *);

#endif /* _BOOTVAR_H_ */
