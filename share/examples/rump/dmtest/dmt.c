/*	$NetBSD: dmt.c,v 1.3 2009/12/05 11:44:56 haad Exp $

/*
 * Copyright (c) 2009 Adam Hamsik.  All Rights Reserved.
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


#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/disklabel.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

#include <dev/dm/netbsd-dm.h>

int
main(int argc, char *argv[])
{
	int fd;
	int err;

	prop_dictionary_t dict_in, dict_out;
	struct plistref prefp;
	char *xml;
	
	err = rump_init();
	if (err != 0)
		printf("rump_init failed with %s\n", strerror(err));
	
	fd = rump_sys_open("/dev/mapper/control", O_RDWR, 0);
	if (fd == -1)
		printf("Cannot open control device.\n");

	dict_in  = prop_dictionary_internalize_from_file("prop2.plist");
	dict_out = prop_dictionary_create();
	
	prop_dictionary_externalize_to_pref(dict_in, &prefp);
	
	err = rump_sys_ioctl(fd, NETBSD_DM_IOCTL, &prefp);
	if ( err < 0)
		printf("ioctl failed %d\n", err);

	dict_out = prop_dictionary_internalize(prefp.pref_plist);
	
	xml = prop_dictionary_externalize(dict_out);
	printf("%s\n",xml);
	
	rump_sys_close(fd);
}
