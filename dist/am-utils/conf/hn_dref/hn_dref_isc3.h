/* $srcdir/conf/hn_dref/hn_dref_isc3.h */
#define NFS_HN_DREF(dst, src) { \
		strncpy((dst), (src), MAXHOSTNAMELEN); \
		(dst)[MAXHOSTNAMELEN] = '\0'; \
	}
