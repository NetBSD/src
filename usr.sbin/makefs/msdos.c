/*	$NetBSD: msdos.c,v 1.3 2013/01/23 21:42:22 christos Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of The NetBSD Foundation nor the names of its
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
#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#if defined(__RCSID) && !defined(__lint)
__RCSID("$NetBSD: msdos.c,v 1.3 2013/01/23 21:42:22 christos Exp $");
#endif	/* !__lint */

#include <sys/param.h>

#if !HAVE_NBTOOL_CONFIG_H
#include <sys/mount.h>
#endif

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "makefs.h"
#include "mkfs_msdos.h"

void
msdos_prep_opts(fsinfo_t *fsopts)
{
	struct msdos_options *msdos_opts;

	if ((msdos_opts = calloc(1, sizeof(*msdos_opts))) == NULL)
		err(1, "Allocating memory for msdos_options");

	fsopts->fs_specific = msdos_opts;
}

void
msdos_cleanup_opts(fsinfo_t *fsopts)
{
	if (fsopts->fs_specific)
		free(fsopts->fs_specific);
}

int
msdos_parse_opts(const char *option, fsinfo_t *fsopts)
{
	struct msdos_options *msdos_opt = fsopts->fs_specific;

	option_t msdos_options[] = {
#define AOPT(_opt, _type, _name, _min, _desc) { 			\
	.letter = _opt,							\
	.name = # _name,						\
	.type = _min == -1 ? OPT_STRPTR :				\
	    (_min == -2 ? OPT_BOOL :					\
	    (sizeof(_type) == 1 ? OPT_INT8 :				\
	    (sizeof(_type) == 2 ? OPT_INT16 :				\
	    (sizeof(_type) == 4 ? OPT_INT32 : OPT_INT64)))),		\
	.value = &msdos_opt->_name,					\
	.minimum = _min,						\
	.maximum = sizeof(_type) == 1 ? 0xff :				\
	    (sizeof(_type) == 2 ? 0xffff :				\
	    (sizeof(_type) == 4 ? 0xffffffff : 0xffffffffffffffffLL)),	\
	.desc = _desc,						\
},
ALLOPTS
#undef AOPT	
		{ .name = NULL }
	};
	int i, rv;

	assert(option != NULL);
	assert(fsopts != NULL);
	assert(msdos_opt != NULL);

	if (debug & DEBUG_FS_PARSE_OPTS)
		printf("msdos_parse_opts: got `%s'\n", option);

	rv = set_option(msdos_options, option);
	if (rv == 0)
		return rv;

	for (i = 0; msdos_options[i].name != NULL && (1 << i) != rv; i++)
		break;
	if (msdos_options[i].name == NULL)
		abort();

	if (strcmp(msdos_options[i].name, "volume_id") == 0)
		msdos_opt->volume_id_set = 1;
	else if (strcmp(msdos_options[i].name, "media_descriptor") == 0)
		msdos_opt->media_descriptor_set = 1;
	else if (strcmp(msdos_options[i].name, "hidden_sectors") == 0)
		msdos_opt->hidden_sectors_set = 1;
	return rv;
}


void
msdos_makefs(const char *image, const char *dir, fsnode *root, fsinfo_t *fsopts)
{
#ifdef notyet
	struct fs	*superblock;
	struct timeval	start;

	assert(image != NULL);
	assert(dir != NULL);
	assert(root != NULL);
	assert(fsopts != NULL);

	if (debug & DEBUG_FS_MAKEFS)
		printf("msdos_makefs: image %s directory %s root %p\n",
		    image, dir, root);

		/* validate tree and options */
	TIMER_START(start);
	msdos_validate(dir, root, fsopts);
	TIMER_RESULTS(start, "msdos_validate");

	printf("Calculated size of `%s': %lld bytes, %lld inodes\n",
	    image, (long long)fsopts->size, (long long)fsopts->inodes);

		/* create image */
	TIMER_START(start);
	if (msdos_create_image(image, fsopts) == -1)
		errx(1, "Image file `%s' not created.", image);
	TIMER_RESULTS(start, "msdos_create_image");

	fsopts->curinode = ROOTINO;

	if (debug & DEBUG_FS_MAKEFS)
		putchar('\n');

		/* populate image */
	printf("Populating `%s'\n", image);
	TIMER_START(start);
	if (! msdos_populate_dir(dir, root, fsopts))
		errx(1, "Image file `%s' not populated.", image);
	TIMER_RESULTS(start, "msdos_populate_dir");

		/* ensure no outstanding buffers remain */
	if (debug & DEBUG_FS_MAKEFS)
		bcleanup();

	printf("Image `%s' complete\n", image);
#endif
}
