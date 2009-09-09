/* $NetBSD: ultrix_flock.h,v 1.1.162.1 2009/09/09 21:15:58 matt Exp $ */

struct ultrix_flock {
	int16_t l_type;
#define ULTRIX_F_RDLCK 1
#define ULTRIX_F_WRLCK 2
#define ULTRIX_F_UNLCK 3
	int16_t l_whence;
	int32_t l_start;
	int32_t l_len;
	int32_t l_pid;
};
