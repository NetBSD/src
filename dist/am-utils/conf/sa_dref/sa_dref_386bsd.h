/* $srcdir/conf/sa_dref/sa_dref_386bsd.h */
#define	NFS_SA_DREF(dst, src) { \
		(dst)->addr = (struct sockaddr *) (src); \
	}
