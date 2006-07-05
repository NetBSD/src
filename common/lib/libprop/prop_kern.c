/*	$NetBSD: prop_kern.c,v 1.1 2006/07/05 21:46:10 thorpej Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#if defined(__NetBSD__)

#include <sys/types.h>
#include <sys/ioctl.h>

#include <prop/proplib.h>

#if !defined(_KERNEL) && !defined(_STANDALONE)
#include <errno.h>
#include <string.h>
#include <stdlib.h>

/*
 * prop_dictionary_send_ioctl --
 *	Send a dictionary to the kernel using the specified ioctl.
 */
int
prop_dictionary_send_ioctl(prop_dictionary_t dict, int fd, unsigned long cmd)
{
	struct plistref pref;
	char *buf;
	int error;

	buf = prop_dictionary_externalize(dict);
	if (buf == NULL) {
		/* Assume we ran out of memory. */
		return (ENOMEM);
	}
	pref.pref_plist = buf;
	pref.pref_len = strlen(buf) + 1;

	if (ioctl(fd, cmd, &pref) == -1)
		error = errno;
	else
		error = 0;
	
	free(buf);

	return (error);
}

/*
 * prop_dictionary_recv_ioctl --
 *	Receive a dictionary from the kernel using the specified ioctl.
 */
int
prop_dictionary_recv_ioctl(int fd, unsigned long cmd, prop_dictionary_t *dictp)
{
	struct plistref pref;
	prop_dictionary_t dict;
	char *buf;
	size_t len;
	int error;

	pref.pref_len = 0;

	for (;;) {
		len = pref.pref_len;
		if (len != 0) {
			buf = malloc(len);
			if (buf == NULL)
				return (ENOMEM);
		} else
			buf = NULL;
		pref.pref_plist = buf;
		if (ioctl(fd, cmd, &pref) == -1) {
			error = errno;
			free(buf);
			return (error);
		}
		
		/*
		 * Kernel only copies out the plist if the buffer
		 * we provided is big enough to hold the entire
		 * thing.  Kernel provides us the needed size in
		 * the argument structure.
		 */
		if (len >= pref.pref_len)
			break;

		free(buf);
	}
	
	dict = prop_dictionary_internalize(buf);
	free(buf);
	if (dict == NULL)
		return (EIO);

	*dictp = dict;
	return (0);
}
#endif /* !_KERNEL && !_STANDALONE */

#if defined(_KERNEL)
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/systm.h>

/*
 * prop_dictionary_copyin_ioctl --
 *	Copy in a dictionary sent with an ioctl.
 */
int
prop_dictionary_copyin_ioctl(const struct plistref *pref, int ioctlflags,
			     prop_dictionary_t *dictp)
{
	prop_dictionary_t dict;
	char *buf;
	int error;

	/*
	 * Allocate an extra byte so we can guarantee NUL-termination.
	 * XXX Some sanity check on the size?
	 */
	buf = malloc(pref->pref_len + 1, M_TEMP, M_WAITOK);
	error = ioctl_copyin(ioctlflags, pref->pref_plist, buf, pref->pref_len);
	if (error) {
		free(buf, M_TEMP);
		return (error);
	}
	buf[pref->pref_len] = '\0';

	dict = prop_dictionary_internalize(buf);
	free(buf, M_TEMP);
	if (dict == NULL)
		return (EIO);
	
	*dictp = dict;
	return (0);
}

/*
 * prop_dictionary_copyout_ioctl --
 *	Copy out a dictionary being received with an ioctl.
 */
int
prop_dictionary_copyout_ioctl(struct plistref *pref, prop_dictionary_t dict,
			      int ioctlflags)
{
	char *buf;
	size_t len;
	int error = 0;

	buf = prop_dictionary_externalize(dict);
	if (buf == NULL)
		return (ENOMEM);
	
	len = strlen(buf) + 1;
	if (len <= pref->pref_len)
		error = ioctl_copyout(ioctlflags, buf,
				      __UNCONST(pref->pref_plist), len);
	if (error == 0)
		pref->pref_len = len;
	
	free(buf, M_TEMP);

	return (error);
}
#endif /* _KERNEL */

#endif /* __NetBSD__ */
