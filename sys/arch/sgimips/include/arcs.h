/*	$NetBSD: arcs.h,v 1.1.4.2 2000/06/22 17:03:11 minoura Exp $	*/

/*
 * Copyright (c) 2000 Soren S. Jorvang
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.netbsd.org/ for
 *          information about NetBSD.
 * 4. The name of the author may not be used to endorse or promote products
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

/*
 * ARC (not quite ARCS) documentation is available at
 * http://www.microsoft.com/hwdev/download/respec/riscspec.zip .
 */

#define ARCS_STDIN		0
#define ARCS_STDOUT		1

/* SGI ARCS firmware error codes */

#define ARCS_ESUCCESS		0
#define ARCS_E2BIG		1
#define ARCS_EACCES		2
#define ARCS_EAGAIN		3
#define ARCS_EBADF		4
#define ARCS_EBUSY		5
#define ARCS_EFAULT		6
#define ARCS_EINVAL		7
#define ARCS_EIO		8
#define ARCS_EISDIR		9
#define ARCS_EMFILE		10
#define ARCS_EMLINK		11
#define ARCS_ENAMETOOLONG	12
#define ARCS_ENODEV		13
#define ARCS_ENOENT		14
#define ARCS_ENOEXEC		15
#define ARCS_ENOMEM		16
#define ARCS_ENOSPC		17
#define ARCS_ENOTDIR		18
#define ARCS_ENOTTY		19
#define ARCS_ENXIO		20
#define ARCS_EROFS		21
#define ARCS_EADDRNOTAVAIL	31
#define ARCS_ETIMEDOUT		32
#define ARCS_ECONNABORTED	33
#define ARCS_ENOCONNECT		34

struct arcs_sysid {
	char Vendor[8];
	char Serial[8];
};

#define ARCS_MEM_EXCEP	0		/* Exception Block */
#define ARCS_MEM_SPB	1		/* System Parameter Block */
#define ARCS_MEM_CONT	2		/* Contiguous Free Memory */
#define ARCS_MEM_FREE	3		/* Free Memory */
#define ARCS_MEM_BAD	4		/* Bad Memory */
#define ARCS_MEM_PROG	5		/* Loaded Program */
#define ARCS_MEM_TEMP	6		/* Firmware Temporary */
#define ARCS_MEM_PERM	7		/* Firmware Permanent */

struct arcs_mem {
	int32_t		Type;
	u_int32_t	BasePage;
	u_int32_t	PageCount;
};

struct arcs_component {
	u_int32_t	Class;
	u_int32_t	Type;
	u_int32_t	Flag;
	u_int16_t	Version;
	u_int16_t	Revision;
	u_int32_t	Key;
	u_int32_t	AffinityMask;
	u_int32_t	ConfigurationDataSize;
	u_int32_t	IdentifierLength;
	char		*Identifier;
};

#define ARCS_PAGESIZE	4096

/*
 * Adding real types as we go along..
 */
struct arcs_fv {
	int32_t		(*Load)(char *, u_int32_t, u_int32_t *, u_int32_t *);
	int32_t		(*Invoke)(u_int32_t,u_int32_t,int32_t,char **,char **);
	int32_t		(*Execute)(char *file, int32_t, char **, char **);
	void		(*Halt)(void);
	void		(*PowerDown)(void);
	void		(*Restart)(void);
	void		(*Reboot)(void);
	void		(*EnterInteractiveMode)(void);
	int32_t		_reserved1;
	void		*(*GetPeer)(void *);
	struct arcs_component *(*GetChild)(void *);
	void		*(*GetParent)(void *);
	int32_t		(*GetConfigurationData)(void *, void *);
	void		*(*AddChild)(void *, void *, void *);
	int32_t		(*DeleteComponent)(void *);
	void		*(*GetComponent)(char *);
	long		(*SaveConfiguration)(void);
	struct arcs_sysid *(*GetSystemId)(void);
	struct arcs_mem	*(*GetMemoryDescriptor)(struct arcs_mem *);
	int32_t		_reserved2;
	void		*(*GetTime)(void);
	u_int32_t	(*GetRelativeTime)(void);
	int32_t		(*GetDirectoryEntry)(void);
	int32_t		(*Open)(void);
	int32_t		(*Close)(u_int32_t);
	int32_t		(*Read)(u_int32_t, void *, u_int32_t, u_int32_t *);
	int32_t		(*GetReadStatus)(unsigned long fd);
	int32_t		(*Write)(u_int32_t, void *, u_int32_t, u_int32_t *);
	int32_t		(*Seek)(u_int32_t, int64_t *, int32_t);
	int32_t		(*Mount)(char *file, int);
	char		*(*GetEnvironmentVariable)(char *);
	int32_t		(*SetEnvironmentVariable)(char *, char *);
	int32_t		(*GetFileInformation)(unsigned long fd, void *);
	int32_t		(*SetFileInformation)(u_int32_t, u_int32_t, u_int32_t);
	void		(*FlushAllCaches)(void);
};      

struct arcs_spb {
#define ARCS_MAGIC	0x53435241		/* 'S' 'C' 'R' 'A' */
	u_int32_t	SPBSignature;
	u_int32_t	SPBLength;
	u_int16_t	Version;
	u_int16_t	Revision;
	int32_t		*RestartBlock;
	int32_t		*DebugBlock;
	int32_t		*GEVector;
	int32_t		*UTLBMissVector;
	u_int32_t	FirmwareVectorLength;
	struct arcs_fv	*FirmwareVector;
	u_int32_t	PrivateVectorLength;
/*XXX*/	struct arcs_fv	*PrivateVector;
	u_int32_t	AdapterCount;
	struct {
		u_int32_t	AdapterType;
		u_int32_t	AdapterVectorLength;
		u_int32_t	*AdapterVector;
	} Adapter[1];
};

extern const struct arcs_fv *ARCS;
