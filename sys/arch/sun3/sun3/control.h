
/*
 * defines for sun3 control space
 *
 */

#define IDPROM_BASE 0x00000000
#define PGMAP_BASE  0x10000000
#define SEGMAP_BASE  0x20000000
#define CONTEXT_REG 0x30000000
#define SYSTEM_ENAB 0x40000000
#define UDVMA_ENAB  0x50000000
#define BUSERR_REG  0x60000000
#define DIAG_REG    0x70000000

#define SEG_SIZE    0x20000
#define NPMEG       0x100

#define VAC_CACHE_TAGS    0x80000000
#define VAC_CACHE_DATA    0x90000000
#define VAC_FLUSH_BASE    0xA0000000
#define VAC_FLUSH_CONTEXT 0x1
#define VAC_FLUSH_PAGE    0x2
#define VAC_FLUSH_SEGMENT 0x3

#define CONTEXT_0  0x0
#define CONTEXT_1  0x1
#define CONTEXT_2  0x2
#define CONTEXT_3  0x3
#define CONTEXT_4  0x4
#define CONTEXT_5  0x5
#define CONTEXT_6  0x6
#define CONTEXT_7  0x7
#define CONTEXT_NUM 0x8

