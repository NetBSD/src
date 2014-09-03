#ifndef OR1K_ASM_H
#define OR1K_ASM_H

#define OR1K_INST(...) __VA_ARGS__

#if defined(__OR1K_NODELAY__)
#define OR1K_DELAYED(a, b) a; b
#define OR1K_DELAYED_NOP(a) a
.nodelay
#elif defined(__OR1K_DELAY__)
#define OR1K_DELAYED(a, b) b; a
#define OR1K_DELAYED_NOP(a) a; l.nop
#elif defined(__OR1K_DELAY_COMPAT__)
#define OR1K_DELAYED(a, b) a; b; l.nop
#define OR1K_DELAYED_NOP(a) a; l.nop
#else
#error One of __OR1K_NODELAY__, __OR1K_DELAY__, or __OR1K_DELAY_COMPAT__ must be defined
#endif

#endif
