/*	$NetBSD: malloc.c,v 1.1.1.1.4.2 2013/06/23 06:26:34 tls Exp $	*/

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
