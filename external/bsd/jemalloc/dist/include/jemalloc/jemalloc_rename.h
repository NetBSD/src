/*
 * Name mangling for public symbols is controlled by --with-mangling and
 * --with-jemalloc-prefix.  With default settings the je_ prefix is stripped by
 * these macro definitions.
 */
#ifndef JEMALLOC_NO_RENAME
#  define je_aligned_alloc aligned_alloc
#  define je_calloc calloc
#  define je_dallocx dallocx
#  define je_free free
#  define je_free_sized free_sized
#  define je_free_aligned_sized free_aligned_sized
#  define je_mallctl mallctl
#  define je_mallctlbymib mallctlbymib
#  define je_mallctlnametomib mallctlnametomib
#  define je_malloc malloc
#  define je_malloc_conf malloc_conf
#  define je_malloc_conf_2_conf_harder malloc_conf_2_conf_harder
#  define je_malloc_message malloc_message
#  define je_malloc_stats_print malloc_stats_print
#  define je_malloc_usable_size malloc_usable_size
#  define je_mallocx mallocx
#  define je_smallocx_81034ce1f1373e37dc865038e1bc8eeecf559ce8 smallocx_81034ce1f1373e37dc865038e1bc8eeecf559ce8
#  define je_nallocx nallocx
#  define je_posix_memalign posix_memalign
#  define je_rallocx rallocx
#  define je_realloc realloc
#  define je_sallocx sallocx
#  define je_sdallocx sdallocx
#  define je_xallocx xallocx
#  define je_memalign memalign
#  define je_valloc valloc
#  define je_pvalloc pvalloc
#endif
