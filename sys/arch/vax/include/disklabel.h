/*	$NetBSD: disklabel.h,v 1.5.8.2 2013/04/20 09:58:22 bouyer Exp $	*/

/*
 * Copyright (c) 1994 Christopher G. Demetriou
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
 *      This product includes software developed by Christopher G. Demetriou.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#ifndef _MACHINE_DISKLABEL_H_
#define _MACHINE_DISKLABEL_H_

#define LABELUSESMBR		0	/* no MBR partitionning */
#define	LABELSECTOR		0	/* sector containing label */
#define	LABELOFFSET		64	/* offset of label in sector */
#define	MAXPARTITIONS		12	/* number of partitions */
#define	OLDMAXPARTITIONS 	8	/* number of partitions before nb-6 */
#define	RAW_PART		2	/* raw partition: xx?c */
/*
 * In NetBSD 6 we eroneously used a too large MAXPARTITIONS value (disklabel
 * overlapped with important parts of the bootblocks and made some machines
 * unbootable).
 */
#define	__TMPBIGMAXPARTITIONS	16	/* compatibility with 6.0 installs */

/*
 * We use the highest bit of the minor number for the partition number.
 * This maintains backward compatibility with device nodes created before
 * MAXPARTITIONS was increased.
 * Temporarily MAXPARTITIONS was 16, so we use that to keep compatibility
 * with existing installations.
 */
#define __VAX_MAXDISKS	((1 << 20) / __TMPBIGMAXPARTITIONS)
#define DISKUNIT(dev)	((minor(dev) / OLDMAXPARTITIONS) % __VAX_MAXDISKS)
#define DISKPART(dev)	((minor(dev) % OLDMAXPARTITIONS) + \
    ((minor(dev) / (__VAX_MAXDISKS * OLDMAXPARTITIONS)) * OLDMAXPARTITIONS))
#define	DISKMINOR(unit, part) \
    (((unit) * OLDMAXPARTITIONS) + ((part) % OLDMAXPARTITIONS) + \
     ((part) / OLDMAXPARTITIONS) * (__VAX_MAXDISKS * OLDMAXPARTITIONS))

/* Just a dummy */
#ifndef _LOCORE
struct cpu_disklabel {
	int	cd_dummy;			/* must have one element. */
};
#endif
#endif /* _MACHINE_DISKLABEL_H_ */
