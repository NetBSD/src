/*	$NetBSD: malloc.c,v 1.1.1.1.10.2 2014/05/22 15:45:07 yamt Exp $	*/

     #include <config.h>
     #undef malloc
     
     #include <sys/types.h>
     
     void *malloc ();
     
     /* Allocate an N-byte block of memory from the heap.
        If N is zero, allocate a 1-byte block.  */
     
     void *
     rpl_malloc (size_t n)
     {
       if (n == 0)
         n = 1;
       return malloc (n);
     }
