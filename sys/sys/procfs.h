/*
 *	%Id% (Erasmus)
 */

#ifndef _PROCFS_
#define _PROCFS_

struct procmap {	/* Simplified VM maps */
	vm_offset_t	vaddr;
	vm_size_t	size;
	vm_offset_t	offset;
	vm_prot_t	prot;
};

struct vmfd {		/* Mapped file descriptor */
	vm_offset_t	vaddr;	/* IN */
	int		fd;	/* OUT */
};

typedef	unsigned long	fltset_t;

#define PIOCGPINFO	_IOR('P', 0, struct kinfo_proc)
#define PIOCGSIGSET	_IOR('P', 1, sigset_t)
#define PIOCSSIGSET	_IOW('P', 2, sigset_t)
#define PIOCGFLTSET	_IOR('P', 3, fltset_t)
#define PIOCSFLTSET	_IOW('P', 4, fltset_t)
#define PIOCGNMAP	_IOR('P', 5, int)
#define PIOCGMAP	_IO ('P', 6)
#define PIOCGMAPFD	_IOWR('P', 7, struct vmfd)

#endif /* _PROCFS_ */
