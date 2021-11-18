# History

This is NetBSD indent.  It originally came from the University of Illinois via
some distribution tape for PDP-11 Unix.  It has subsequently been hacked upon 
by James Gosling @ CMU.  At some point in the 1970s or even 1980s, it was 
thought to be "the nicest C pretty printer around".  Around 1985, further
additions to provide "Kernel Normal Form" were contributed by the folks at Sun 
Microsystems.

Between 2000 and 2019, FreeBSD maintained the code, adding several features.
NetBSD imported these changes on 2019-04.04.

In 2021, indent was updated to handle C99 comments, cleaning up the code.  It
got a proper test suite, uncovering many inconsistencies and bugs that either
had been there forever or had been introduced by importing the FreeBSD version
in 2019.

# References

* https://github.com/freebsd/freebsd-src/tree/main/usr.bin/indent
* https://github.com/pstef/freebsd_indent
