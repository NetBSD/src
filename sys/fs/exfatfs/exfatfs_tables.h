/* $NetBSD: exfatfs_tables.h,v 1.1.2.1 2024/06/29 19:43:26 perseant Exp $ */
#ifndef EXFATFS_TABLES_H_
#define EXFATFS_TABLES_H_

#include <sys/queue.h>

struct exfatfs_upcase_range_offset {
	STAILQ_ENTRY(exfatfs_upcase_range_offset) euro_list;
	uint16_t euro_begin; /* First valid character */
	uint16_t euro_end;   /* First invalid character */
	int16_t  euro_ucoff; /* Offset of uppercase version of characters */
};

int exfatfs_check_filename_ucs2(uint16_t *, int);
void exfatfs_load_uctable(struct exfatfs *, uint16_t *, int);
void exfatfs_destroy_uctable(struct exfatfs *);
void exfatfs_upcase_str(struct exfatfs *, uint16_t *, int);
int exfatfs_upcase_cmp(struct exfatfs *, uint16_t *, int, uint16_t *, int);

#endif /* EXFATFS_TABLE_H_ */
