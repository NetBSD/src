#if 0
wrong
#endif

#if 1
right
#endif

#if -1
right
#endif

#if 0 + 0
wrong
#endif

#if 1 + 1
right
#endif

#if 1 - 1
wrong
#endif

#if -1 + 1
wrong
#endif

#if 3 - 2 - 1
wrong
#endif

#if 3 * 2 - 6
wrong
#endif

#if 6 - 2 * 3
wrong
#endif

#if 3 - 3 && 1
wrong
#endif

#if 3 - 3 || 0
wrong
#endif

#if 1 && 0
wrong
#endif

#if 0 && 1
wrong
#endif

#if 1 || 0
right
#endif

#if 0 || 1
right
#endif

#if (0 || 1) && (0 || 0)
wrong
#endif
