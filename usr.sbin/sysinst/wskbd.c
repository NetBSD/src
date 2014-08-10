/*	$NetBSD: wskbd.c,v 1.1.2.2 2014/08/10 07:00:24 tls Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by David Laight.
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

#include <sys/cdefs.h>
__RCSID("$NetBSD: wskbd.c,v 1.1.2.2 2014/08/10 07:00:24 tls Exp $");

#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsksymdef.h>

#include "defs.h"
#include "menu_defs.h"
#include "msg_defs.h"
void save_kb_encoding(void);

#define nelem(x) (sizeof (x) / sizeof *(x))

/* wscons setup for sysinst */

static const char *kbd_name = 0;
static int kb_default = 0;

struct kb_types {
	kbd_t		kb_encoding;
	const char	*kb_enc_txt;
	const char	*kb_name;
};

/* Types and names of keyboards, maybe the names should be translated... */
static const struct kb_types kb_types[] = {
#define KB_sysinst(tag, tagf, value, cc, ccf, country) \
	{tag | tagf, cc ccf, country},
KB_ENC_FUN(KB_sysinst)
{KB_US | KB_COLEMAK, "us" ".colemak", "US-Colemak"},
{KB_US | KB_DVORAK, "us" ".dvorak", "US-Dvorak"}
};

static int
set_kb_encoding(menudesc *m, void *arg)
{
	int fd = *(int *)arg;
	const struct kb_types *kbt = kb_types + m->cursel;

	if (kbt->kb_encoding != KB_USER) {
		ioctl(fd, WSKBDIO_SETENCODING, &kbt->kb_encoding);
		kbd_name = kbt->kb_enc_txt;
	}
	return 1;
}

static void
set_kb_default(menudesc *m, void *arg)
{
	m->cursel = kb_default;
}

void
get_kb_encoding(void)
{
	int fd;
	unsigned int i;
	int kb_menu;
	kbd_t kbdencoding;
	menu_ent opt[nelem(kb_types)];
	const char *dflt = msg_string(MSG_kb_default);

	fd = open("/dev/wskbd0", O_WRONLY);
	if (fd < 0)
		return;
	if (ioctl(fd, WSKBDIO_GETENCODING, &kbdencoding) >=  0) {
		memset(&opt, 0, sizeof opt);
		for (i = 0; i < nelem(opt); i++) {
			if (kb_types[i].kb_encoding == KB_USER)
				opt[0].opt_name = MSG_unchanged;
			else {
				opt[i].opt_name = kb_types[i].kb_name;
				if (strcmp(kb_types[i].kb_name, dflt) == 0)
					kb_default = i;
			}
			opt[i].opt_menu = OPT_NOMENU;
			opt[i].opt_action = set_kb_encoding;
		}
		kb_menu = new_menu(MSG_Keyboard_type, opt, nelem(opt),
			-1, -8, 0, 0,
			MC_SCROLL | MC_NOEXITOPT,
			set_kb_default, NULL, NULL, NULL, NULL);
		if (kb_menu != -1) {
			msg_display(MSG_hello);
			process_menu(kb_menu, &fd);
			free_menu(kb_menu);
		}
	}
	close(fd);
}

void
save_kb_encoding(void)
{
	const char *tp = target_prefix();

	if (kbd_name == NULL)
		return;
	/*
	 * Put the keyboard encoding into the wscons.conf file. Either:
	 * 1) replace an exiting line
	 * 2) replace a commented out line
	 * or
	 * 3) add a line to the end of the file
	 */
	run_program(0, "sed -an -e 'H;$!d;g'"
	    " -e 's/\\nencoding [a-zA-Z0-9.]*\\n/\\\nencoding %s\\\n/; t done'"
	    " -e 's/\\n#encoding [a-zA-Z0-9.]*\\n/\\\nencoding %s\\\n/; t done'"
	    " -e 's/$/\\\nencoding %s/'"
	    " -e ':done'"
	    " -e 'w %s/etc/wscons.conf' %s/etc/wscons.conf",
	    kbd_name, kbd_name, kbd_name, tp, tp);
}
