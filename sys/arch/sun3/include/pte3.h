
#ifndef _MACHINE_PTE_H
#define _MACHINE_PTE_H

#define NCONTEXT 8
#define NBSG 131072
#define SEGINV 255
#define NPAGSEG 16
#define NSEGMAP 2048

#define PG_VALID   0x80000000
#define PG_WRITE   0x40000000
#define PG_SYSTEM  0x20000000
#define PG_NC      0x10000000
#define PG_TYPE    0x0C000000
#define PG_ACCESS  0x02000000
#define PG_MOD     0x01000000

#define PG_SPECIAL (PG_VALID|PG_WRITE|PG_SYSTEM|PG_NC|PG_ACCESS|PG_MOD)
#define PG_PERM    (PG_VALID|PG_WRITE|PG_SYSTEM|PG_NC)
#define PG_FRAME   0x0007FFFF

#define PG_MOD_SHIFT 24
#define PG_PERM_SHIFT 28

#define PG_MMEM      0
#define PG_OBIO      1
#define PG_VME16D    2
#define PG_VME32D    3
#define PG_TYPE_SHIFT 26

#define PG_INVAL   0x0

#define MAKE_PGTYPE(x) ((x) << PG_TYPE_SHIFT)
#define PG_PGNUM(pte) (pte & PG_FRAME)
#define PG_PA(pte) ((pte & PG_FRAME) <<PGSHIFT)

#define VA_PTE_NUM_SHIFT  13
#define VA_PTE_NUM_MASK (0xF << VA_PTE_NUM_SHIFT)
#define VA_PTE_NUM(va) ((va & VA_PTE_NUM_MASK) >> VA_PTE_NUM_SHIFT)


#endif /* !_MACHINE_PTE_H*/
