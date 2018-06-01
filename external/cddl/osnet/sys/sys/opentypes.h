#ifndef _OPENSOLARIS_SYS_OPENTYPES_H_
#define _OPENSOLARIS_SYS_OPENTYPES_H_

#define	MAXNAMELEN	256
#define	FMNAMESZ	8

#if defined(__APPLE__) || defined(HAVE_NBTOOL_CONFIG_H)
#ifndef __defined_ll_t
#define __defined_ll_t
typedef long long longlong_t;
typedef unsigned long long u_longlong_t;
#endif
typedef unsigned long vsize_t;
#endif

typedef unsigned int	size32_t;
typedef unsigned int	caddr32_t;

typedef	struct timespec	timestruc_t;
#ifndef __defined_ts_t
#define __defined_ts_t
typedef	struct timespec	timespec_t;
#endif
typedef unsigned int	uint_t;
typedef unsigned char	uchar_t;
typedef unsigned short	ushort_t;
typedef unsigned long	ulong_t;
typedef off_t		off64_t;
typedef id_t		taskid_t;
typedef id_t		projid_t;
typedef id_t		poolid_t;
typedef id_t		zoneid_t;
typedef id_t		ctid_t;

#define	B_FALSE	0
#define	B_TRUE	1
typedef int		boolean_t;

#ifndef __defined_hr_t
#define __defined_hr_t
typedef longlong_t      hrtime_t;
#endif
typedef int32_t		t_scalar_t;
typedef uint32_t	t_uscalar_t;
#if defined(_KERNEL) || defined(_KERNTYPES)
typedef vsize_t		pgcnt_t;
#endif
typedef u_longlong_t	len_t;
typedef int		major_t;
typedef int		minor_t;
typedef int		o_uid_t;
typedef int		o_gid_t;
typedef struct kauth_cred cred_t;
typedef uintptr_t	pc_t;
typedef struct vm_page	page_t;
typedef	ushort_t	o_mode_t;	/* old file attribute type */
typedef	u_longlong_t	diskaddr_t;
typedef void		*zone_t;
typedef struct vfsops	vfsops_t;

#ifdef _KERNEL

typedef	short		index_t;
typedef	off_t		offset_t;
typedef	int64_t		rlim64_t;
typedef __caddr_t	caddr_t;	/* core address */

#else

typedef	longlong_t	offset_t;
typedef	u_longlong_t	u_offset_t;
typedef	uint64_t	upad64_t;
#ifndef __defined_ts_t
#define __defined_ts_t
typedef	struct timespec	timespec_t;
#endif
typedef	int32_t		daddr32_t;
typedef	int32_t		time32_t;

#endif	/* !_KERNEL */

#define	MAXOFFSET_T 	0x7fffffffffffffffLL
#define	seg_rw		uio_rw
#define	S_READ		UIO_READ
#define	S_WRITE		UIO_WRITE
struct aio_req;
typedef void		*dev_info_t;

#endif /* _OPENSOLARIS_SYS_OPENTYPES_H_ */
