/* $NetBSD: exfatfs_cksum.h,v 1.1.2.1 2024/06/29 19:43:26 perseant Exp $ */
#ifndef EXFATFS_CKSUM_H_
#define EXFATFS_CKSUM_H_

#include <sys/types.h>
#include <sys/endian.h>

#include <fs/exfatfs/exfatfs.h>
#include <fs/exfatfs/exfatfs_dirent.h>

uint32_t exfatfs_cksum32(uint32_t seed, uint8_t *data, uint64_t datalen,
			uint8_t *ignore, uint8_t ignorelen);
uint16_t exfatfs_namehash(struct exfatfs *fs,
			   uint16_t seed, uint16_t *name, int namelen);
uint16_t exfatfs_cksum16(uint16_t seed, uint8_t *data, uint64_t datalen,
			uint8_t *ignore, uint8_t ignorelen);
void htole_bootblock(struct exfatfs *out, struct exfatfs *in);
void letoh_bootblock(struct exfatfs *out, struct exfatfs *in);
void htole_dfe(struct exfatfs_dfe *out,
		       struct exfatfs_dfe *in);
void letoh_dfe(struct exfatfs_dfe *out,
		       struct exfatfs_dfe *in);

#endif /* EXFATFS_CKSUM_H_ */
