/* Hack to extend HOST_WIDE_INT on 64-bit target cross compilers. */

#ifdef __GNUC__
#define HOST_WIDE_INT long long
#define HOST_BITS_PER_WIDE_INT 64
#endif
