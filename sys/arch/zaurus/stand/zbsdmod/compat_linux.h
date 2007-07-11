/*	$NetBSD: compat_linux.h,v 1.1.14.1 2007/07/11 20:03:42 mjf Exp $	*/
/*	$OpenBSD: compat_linux.h,v 1.5 2006/01/15 17:58:27 deraadt Exp $	*/

/*
 * Copyright (c) 2005 Uwe Stuehler <uwe@bsdx.de>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Declare the things that we need from the Linux headers.
 */

#define	IS_ERR(ptr)	((unsigned long)(ptr) > (unsigned long)-1000L)

#define MKDEV(ma,mi)	((ma)<<8 | (mi))

#define S_IFBLK		0060000
#define S_IFCHR		0020000

struct file;
struct inode;

#define	ELFSIZE	32
#include <sys/exec_elf.h>
#include <sys/types.h>
#include <sys/errno.h>

typedef long loff_t;

struct file_operations {
	struct module *owner;
	void (*llseek) (void);
	ssize_t (*read) (struct file *, char *, size_t, loff_t *);
	ssize_t (*write) (struct file *, const char *, size_t, loff_t *);
	void (*readdir) (void);
	void (*poll) (void);
	void (*ioctl) (void);
	void (*mmap) (void);
	int (*open) (struct inode *, struct file *);
	void (*flush) (void);
	int (*release) (struct inode *, struct file *);
	void (*fsync) (void);
	void (*fasync) (void);
	void (*lock) (void);
	void (*readv) (void);
	void (*writev) (void);
	void (*sendpage) (void);
	void (*get_unmapped_area)(void);
#ifdef MAGIC_ROM_PTR
	void (*romptr) (void);
#endif /* MAGIC_ROM_PTR */
};

extern	struct file *open_exec(const char *);
extern	int kernel_read(struct file *, unsigned long, char *, unsigned long);
extern	int memcmp(const void *, const void *, size_t);
extern	int register_chrdev(unsigned int, const char *, struct file_operations *);
extern	int unregister_chrdev(unsigned int, const char *);
extern	void printk(const char *, ...);
extern	void *memcpy(void *, const void *, size_t);

/* Linux LKM support */
static const char __module_kernel_version[] __attribute__((section(".modinfo"))) =
"kernel_version=" UTS_RELEASE;
static const char __module_using_checksums[] __attribute__((section(".modinfo"))) =
"using_checksums=1";

/* procfs support */
struct proc_dir_entry {
        unsigned short low_ino;
        unsigned short namelen;
        const char *name;
        unsigned short mode;
        unsigned short nlink;
        unsigned short uid;
        unsigned short gid;
        unsigned long size;
        void *proc_iops; /* inode operations */
        struct file_operations * proc_fops;
        void *get_info;
        struct module *owner;
        struct proc_dir_entry *next, *parent, *subdir;
        void *data;
        void *read_proc;
        void *write_proc;
        volatile int count;
        int deleted;
        unsigned short rdev;
};
extern	struct proc_dir_entry proc_root;
extern	struct proc_dir_entry *proc_mknod(const char*, unsigned short,
    struct proc_dir_entry*, unsigned short);
extern	void remove_proc_entry(const char *, struct proc_dir_entry *);
