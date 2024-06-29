/* $NetBSD: exfatfs_trie_basic.h,v 1.1.2.1 2024/06/29 19:43:26 perseant Exp $ */
#ifndef EXFATFS_TRIE_H_
#define EXFATFS_TRIE_H_

#define BN_CHILDREN_SHIFT 8
#define BN_CHILDREN_COUNT (1 << BN_CHILDREN_SHIFT)
#define BOTTOM_LEVEL      2 /* level at which 1 block * 8 is full count */
#define BN_BLOCK_SIZE     (BN_CHILDREN_SHIFT << BOTTOM_LEVEL)
#define INVALID           (~(uint32_t)0)

struct xf_bitmap_node {
	struct xf_bitmap_node *children[BN_CHILDREN_COUNT];
	uint32_t count;
};

int exfatfs_bitmap_init(struct exfatfs *fs, int);
void exfatfs_bitmap_destroy(struct exfatfs *fs);

int exfatfs_bitmap_alloc(struct exfatfs *fs, uint32_t start, uint32_t *cp);
int exfatfs_bitmap_dealloc(struct exfatfs *fs, uint32_t cno);

#endif /* EXFATFS_TRIE_H_ */
