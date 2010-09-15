/*  $NetBSD: debug.c,v 1.2 2010/09/15 01:51:43 manu Exp $ */

/*-
 *  Copyright (c) 2010 Emmanuel Dreyfus. All rights reserved.
 * 
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 
 *  THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 *  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 *  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */ 

#include <puffs.h>
#include <sys/types.h>

#include "perfuse_if.h"
#include "fuse.h"

struct perfuse_opcode {
	int  opcode;
	const char *opname;
};

const struct perfuse_opcode perfuse_opcode[] = {
	{ FUSE_LOOKUP, "LOOKUP" },
	{ FUSE_FORGET, "FORGET" },
	{ FUSE_GETATTR, "GETATTR" },
	{ FUSE_SETATTR, "SETATTR" },
	{ FUSE_READLINK, "READLINK" },
	{ FUSE_SYMLINK, "SYMLINK" },
	{ FUSE_MKNOD, "MKNOD" },
	{ FUSE_MKDIR, "MKDIR" },
	{ FUSE_UNLINK, "UNLINK" },
	{ FUSE_RMDIR, "RMDIR" },
	{ FUSE_RENAME, "RENAME" },
	{ FUSE_LINK, "LINK" },
	{ FUSE_OPEN, "OPEN" },
	{ FUSE_READ, "READ" },
	{ FUSE_WRITE, "WRITE" },
	{ FUSE_STATFS, "STATFS" },
	{ FUSE_RELEASE, "RELEASE" },
	{ FUSE_FSYNC, "FSYNC" },
	{ FUSE_SETXATTR, "SETXATTR" },
	{ FUSE_GETXATTR, "GETXATTR" },
	{ FUSE_LISTXATTR, "LISTXATTR" },
	{ FUSE_REMOVEXATTR, "REMOVEXATTR" },
	{ FUSE_FLUSH, "FLUSH" },
	{ FUSE_INIT, "INIT" },
	{ FUSE_OPENDIR, "OPENDIR" },
	{ FUSE_READDIR, "READDIR" },
	{ FUSE_RELEASEDIR, "RELEASEDIR" },
	{ FUSE_FSYNCDIR, "FSYNCDIR" },
	{ FUSE_GETLK, "GETLK" },
	{ FUSE_SETLK, "SETLK" },
	{ FUSE_SETLKW, "SETLKW" },
	{ FUSE_ACCESS, "ACCESS" },
	{ FUSE_CREATE, "CREATE" },
	{ FUSE_INTERRUPT, "INTERRUPT" },
	{ FUSE_BMAP, "BMAP" },
	{ FUSE_DESTROY, "DESTROY" },
	{ FUSE_IOCTL, "IOCTL" },
	{ FUSE_POLL, "POLL" },
	{ FUSE_CUSE_INIT, "CUSE_INIT" },
	{ 0, "UNKNOWN" },
};

const char *perfuse_qtypestr[] = { "READDIR", "READ", "WRITE", "AFTERWRITE" };

const char *
perfuse_opname(opcode)
	int opcode;
{
	const struct perfuse_opcode *po;

	for (po = perfuse_opcode; po->opcode; po++) {
		if (po->opcode == opcode)
			return po->opname;
	}

	return po->opname; /* "UNKNOWN" */
}
