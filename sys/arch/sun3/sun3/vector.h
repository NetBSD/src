
#define COPY_ENTRY 0
#define NVECTORS 256

#define VEC_INTERRUPT_BASE 0x18
#define VEC_LEVEL_7_INT    0x1F

extern unsigned int vector_table[];

void set_vector_entry __P((int, void (*handler)()));
unsigned int get_vector_entry __P((int));
