
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

#define CONTROL_ADDR_MASK 0x0FFFFFFC


#define NBSEG    0x20000
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
#define CONTEXT_MASK 0x7

#include <sys/types.h>

void control_copy_byte __P((char *, char *, int ));

unsigned char get_control_byte __P((char *));
unsigned int get_control_word __P((char *));
void set_control_byte __P((char *, unsigned char));
void set_control_word __P((char *, unsigned int));

vm_offset_t get_pte_pmeg __P((unsigned char, unsigned int));
void set_pte_pmeg __P((unsigned char, unsigned int, vm_offset_t));

int get_context __P((void));
void set_context __P((int));
     
vm_offset_t get_pte __P((vm_offset_t va));
void set_pte __P((vm_offset_t, vm_offset_t));
     
unsigned char get_segmap __P((vm_offset_t));
void set_segmap __P((vm_offset_t va, unsigned char));
