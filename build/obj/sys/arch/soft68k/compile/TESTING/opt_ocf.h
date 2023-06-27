/* option `CRYPTO_TIMING' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_CRYPTO_TIMING
 .global _KERNEL_OPT_CRYPTO_TIMING
 .equiv _KERNEL_OPT_CRYPTO_TIMING,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_CRYPTO_TIMING\n .global _KERNEL_OPT_CRYPTO_TIMING\n .equiv _KERNEL_OPT_CRYPTO_TIMING,0x6e074def\n .endif");
#endif
/* option `CRYPTO_DEBUG' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_CRYPTO_DEBUG
 .global _KERNEL_OPT_CRYPTO_DEBUG
 .equiv _KERNEL_OPT_CRYPTO_DEBUG,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_CRYPTO_DEBUG\n .global _KERNEL_OPT_CRYPTO_DEBUG\n .equiv _KERNEL_OPT_CRYPTO_DEBUG,0x6e074def\n .endif");
#endif
/* option `CRYPTO_RET_KQ_MAXLEN' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_CRYPTO_RET_KQ_MAXLEN
 .global _KERNEL_OPT_CRYPTO_RET_KQ_MAXLEN
 .equiv _KERNEL_OPT_CRYPTO_RET_KQ_MAXLEN,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_CRYPTO_RET_KQ_MAXLEN\n .global _KERNEL_OPT_CRYPTO_RET_KQ_MAXLEN\n .equiv _KERNEL_OPT_CRYPTO_RET_KQ_MAXLEN,0x6e074def\n .endif");
#endif
/* option `CRYPTO_RET_Q_MAXLEN' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_CRYPTO_RET_Q_MAXLEN
 .global _KERNEL_OPT_CRYPTO_RET_Q_MAXLEN
 .equiv _KERNEL_OPT_CRYPTO_RET_Q_MAXLEN,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_CRYPTO_RET_Q_MAXLEN\n .global _KERNEL_OPT_CRYPTO_RET_Q_MAXLEN\n .equiv _KERNEL_OPT_CRYPTO_RET_Q_MAXLEN,0x6e074def\n .endif");
#endif
