/* $NetBSD: exfatfs_conv.h,v 1.1.2.1 2024/06/29 19:43:26 perseant Exp $ */
#ifndef EXFATFS_CONV_H_
#define EXFATFS_CONV_H_

struct timespec;

#include <sys/types.h>
#include <sys/time.h>
#include <sys/timespec.h>

#define DOS_EPOCH_DOS_FMT 0x214000

int exfatfs_ucs2utf8str(const uint16_t *, int, uint8_t *, int);
int exfatfs_utf8ucs2str(const uint8_t *, int, uint16_t *, int);
void exfatfs_unix2dostime(const struct timespec *tsp, int gmtoff, uint32_t *, uint8_t *);
struct timespec *exfatfs_dos2unixtime(uint32_t dt, uint8_t dh, int gmtoff, struct timespec *tsp);
#endif /* EXFATFS_CONV_H_ */
