/*
 * X680x0 ROM IOCS access definitions.
 *  based on Project C Library X68000 Programing Interface Definition
 *           /usr/include/sys/scsi.h
 *  $Id: scsi.h,v 1.2 2005/12/24 20:07:41 perry Exp $
 */
/*
 * PROJECT C Library, X68000 PROGRAMMING INTERFACE DEFINITION
 * --------------------------------------------------------------------
 * This file is written by the Project C Library Group,  and completely
 * in public domain. You can freely use, copy, modify, and redistribute
 * the whole contents, without this notice.
 * --------------------------------------------------------------------
 * from RCS Id: scsi.h,v 1.3 1994/07/31 17:21:50 mura Exp 
 */

#ifndef __INLINE_SCSI_H__
#define __INLINE_SCSI_H__

struct _readcap {
    unsigned long block;
    unsigned long size;
};

struct _inquiry {
    unsigned char unit;
    unsigned char info;
    unsigned char ver;
    unsigned char reserve;
    unsigned char size;
    unsigned char buff[0];
};

static inline int _scsi_inquiry (int n, int id, struct _inquiry *buf)
{
    register int reg_d0 __asm ("%d0");

    __asm volatile ("moveml %%d3-%%d4,%%sp@-\n\t"
		      "movel %2,%%d3\n\t"
		      "movel %3,%%d4\n\t"
		      "movel %4,%%a1\n\t"
		      "movel #0x20,%%d1\n\t"
		      "movel #0xf5,%%d0\n\t"
		      "trap #15\n\t"
		      "moveml %%sp@+,%%d3-%%d4"
			: "=d" (reg_d0), "=m" (*buf)
			: "ri" (n), "ri" (id), "g" ((int) buf)
			: "%d1", "%d2", "%a1");

    return reg_d0;
}
static inline int _scsi_modesense (int page, int n, int id, void *buf)
{
    register int reg_d0 __asm ("%d0");

    __asm volatile ("moveml %%d3-%%d4,%%sp@-\n\t"
		      "movel %2,%%d2\n\t"
		      "movel %3,%%d3\n\t"
		      "movel %4,%%d4\n\t"
		      "movel %5,%%a1\n\t"
		      "movel #0x29,%%d1\n\t"
		      "movel #0xf5,%%d0\n\t"
		      "trap #15\n\t"
		      "moveml %%sp@+,%%d3-%%d4"
			: "=d" (reg_d0), "=m" (*(char*) buf)
			: "ri" (page), "ri" (n), "ri" (id), "g" ((int) buf)
			: "%d1", "%d2", "%a1");

    return reg_d0;
}

static inline int _scsi_read (int pos, int blk, int id, int size, void *buf)
{
    register int reg_d0 __asm ("%d0");

    __asm volatile ("moveml %%d3-%%d5,%%sp@-\n\t"
		      "movel %2,%%d2\n\t"
		      "movel %3,%%d3\n\t"
		      "movel %4,%%d4\n\t"
		      "movel %5,%%d5\n\t"
		      "movel %6,%%a1\n\t"
		      "movel #0x21,%%d1\n\t"
		      "movel #0xf5,%%d0\n\t"
		      "trap #15\n\t"
		      "moveml %%sp@+,%%d3-%%d5"
			: "=d" (reg_d0), "=m" (*(char*) buf)
			: "ri" (pos), "ri" (blk), "ri" (id), "ri" (size), "g" ((int) buf)
			: "%d1", "%d2", "%a1");

    return reg_d0;
}

static inline int _scsi_readcap (int id, struct _readcap *buf)
{
    register int reg_d0 __asm ("%d0");

    __asm volatile ("moveml %%d4,%%sp@-\n\t"
		      "movel %2,%%d4\n\t"
		      "movel %3,%%a1\n\t"
		      "movel #0x25,%%d1\n\t"
		      "movel #0xf5,%%d0\n\t"
		      "trap #15\n\t"
		      "moveml %%sp@+,%%d4"
			: "=d" (reg_d0), "=m" (*buf)
			: "ri" (id), "g" ((int) buf)
			: "%d1", "%a1");

    return reg_d0;
}

static inline int _scsi_seek (int pos, int id)
{
    register int reg_d0 __asm ("%d0");

    __asm volatile ("moveml %%d4,%%sp@-\n\t"
		      "movel %1,%%d2\n\t"
		      "movel %2,%%d4\n\t"
		      "movel #0x2d,%%d1\n\t"
		      "movel #0xf5,%%d0\n\t"
		      "trap #15\n\t"
		      "moveml %%sp@+,%%d4"
			: "=d" (reg_d0)
			: "ri" (pos), "ri" (id)
			: "%d1", "%d2");

    return reg_d0;
}

static inline int _scsi_testunit (int id)
{
    register int reg_d0 __asm ("%d0");

    __asm volatile ("moveml %%d4,%%sp@-\n\t"
		      "movel %1,%%d4\n\t"
		      "movel #0x24,%%d1\n\t"
		      "movel #0xf5,%%d0\n\t"
		      "trap #15\n\t"
		      "moveml %%sp@+,%%d4"
			: "=d" (reg_d0)
			: "ri" (id)
			: "%d1");

    return reg_d0;
}

static inline int _scsi_write (int pos, int blk, int id, int size, void *buf)
{
    register int reg_d0 __asm ("%d0");

    __asm volatile ("moveml %%d3-%%d5,%%sp@-\n\t"
		      "movel %1,%%d2\n\t"
		      "movel %2,%%d3\n\t"
		      "movel %3,%%d4\n\t"
		      "movel %4,%%d5\n\t"
		      "movel %5,%%a1\n\t"
		      "movel #0x22,%%d1\n\t"
		      "movel #0xf5,%%d0\n\t"
		      "trap #15\n\t"
		      "moveml %%sp@+,%%d3-%%d5"
			: "=d" (reg_d0)
			: "ri" (pos), "ri" (blk), "ri" (id), "ri" (size), "g" ((int) buf)
			: "%d1", "%d2", "%a1");

    return reg_d0;
}

static inline void _scsi_reset (void)
{
    __asm volatile ("movel #0,%%d1\n\t"
		      "movel #0xf5,%%d0\n\t"
		      "trap #15\n\t"
			:
			:
			: "%d0", "%d1");
}

#endif
