### /u/sy/beebe/xml/shbook/fmttest.awk, Sat May 31 09:13:52 2003
### Edit by Nelson H. F. Beebe <beebe@math.utah.edu>
### ====================================================================
### Test the degree of support for printf format items in awk
### implementations.
###
### Usage:
###	awk -f fmttest.awk
### [31-May-2003]
### ====================================================================

BEGIN {
    ## -----------------------------------------------------------------
    print "\n\nFormat item: c\n"

    printf("ABC with %%c : %c\n", "ABC")
    printf("123 with %%c : %c\n", 123)

    printf("ABC with %%.15c : %.15c\n", "ABC")
    printf("123 with %%.15c : %.15c\n", 123)

    printf("ABC with %%15c : %15c\n", "ABC")
    printf("123 with %%15c : %15c\n", 123)

    printf("ABC with %%-15c : %-15c\n", "ABC")
    printf("123 with %%-15c : %-15c\n", 123)

    ## -----------------------------------------------------------------
    print "\n\nFormat item: d\n"

    printf("ABC with %%d : %d\n", "ABC")
    printf("123 with %%d : %d\n", 123)

    printf("ABC with %%.15d : %.15d\n", "ABC")
    printf("123 with %%.15d : %.15d\n", 123)

    printf("ABC with %%15d : %15d\n", "ABC")
    printf("123 with %%15d : %15d\n", 123)

    printf("ABC with %%-15d : %-15d\n", "ABC")
    printf("123 with %%-15d : %-15d\n", 123)

    ## -----------------------------------------------------------------
    print "\n\nFormat item: e\n"

    printf("ABC with %%e : %e\n", "ABC")
    printf("123 with %%e : %e\n", 123)

    printf("ABC with %%.25e : %.25e\n", "ABC")
    printf("123 with %%.25e : %.25e\n", 123)

    printf("ABC with %%25e : %25e\n", "ABC")
    printf("123 with %%25e : %25e\n", 123)

    printf("ABC with %%-25e : %-25e\n", "ABC")
    printf("123 with %%-25e : %-25e\n", 123)

    ## -----------------------------------------------------------------
    print "\n\nFormat item: f\n"

    printf("ABC with %%f : %f\n", "ABC")
    printf("123 with %%f : %f\n", 123)

    printf("ABC with %%.25f : %.25f\n", "ABC")
    printf("123 with %%.25f : %.25f\n", 123)

    printf("ABC with %%25f : %25f\n", "ABC")
    printf("123 with %%25f : %25f\n", 123)

    printf("ABC with %%-25f : %-25f\n", "ABC")
    printf("123 with %%-25f : %-25f\n", 123)

    ## -----------------------------------------------------------------
    print "\n\nFormat item: g\n"

    printf("ABC with %%g : %g\n", "ABC")
    printf("123 with %%g : %g\n", 123)

    printf("ABC with %%.25g : %.25g\n", "ABC")
    printf("123 with %%.25g : %.25g\n", 123)

    printf("ABC with %%25g : %25g\n", "ABC")
    printf("123 with %%25g : %25g\n", 123)

    printf("ABC with %%-25g : %-25g\n", "ABC")
    printf("123 with %%-25g : %-25g\n", 123)

    ## -----------------------------------------------------------------
    print "\n\nFormat item: o\n"

    printf("ABC with %%o : %o\n", "ABC")
    printf("123 with %%o : %o\n", 123)

    printf("ABC with %%.15o : %.15o\n", "ABC")
    printf("123 with %%.15o : %.15o\n", 123)

    printf("ABC with %%15o : %15o\n", "ABC")
    printf("123 with %%15o : %15o\n", 123)

    printf("ABC with %%-15o : %-15o\n", "ABC")
    printf("123 with %%-15o : %-15o\n", 123)

    ## -----------------------------------------------------------------
    print "\n\nFormat item: s\n"

    printf("ABC with %%s : %s\n", "ABC")
    printf("123 with %%s : %s\n", 123)

    printf("ABC with %%.15s : %.15s\n", "ABC")
    printf("123 with %%.15s : %.15s\n", 123)

    printf("ABC with %%15s : %15s\n", "ABC")
    printf("123 with %%15s : %15s\n", 123)

    printf("ABC with %%-15s : %-15s\n", "ABC")
    printf("123 with %%-15s : %-15s\n", 123)

    ## -----------------------------------------------------------------
    print "\n\nFormat item: u\n"

    printf("ABC with %%u : %u\n", "ABC")
    printf("123 with %%u : %u\n", 123)

    printf("ABC with %%.15u : %.15u\n", "ABC")
    printf("123 with %%.15u : %.15u\n", 123)

    printf("ABC with %%15u : %15u\n", "ABC")
    printf("123 with %%15u : %15u\n", 123)

    printf("ABC with %%-15u : %-15u\n", "ABC")
    printf("123 with %%-15u : %-15u\n", 123)

    ## -----------------------------------------------------------------
    print "\n\nFormat item: x\n"

    printf("ABC with %%x : %x\n", "ABC")
    printf("123 with %%x : %x\n", 123)

    printf("ABC with %%.15x : %.15x\n", "ABC")
    printf("123 with %%.15x : %.15x\n", 123)

    printf("ABC with %%15x : %15x\n", "ABC")
    printf("123 with %%15x : %15x\n", 123)

    printf("ABC with %%-15x : %-15x\n", "ABC")
    printf("123 with %%-15x : %-15x\n", 123)

    ## -----------------------------------------------------------------
    print "\n\nFormat item: X\n"

    printf("ABC with %%X : %X\n", "ABC")
    printf("123 with %%X : %X\n", 123)

    printf("ABC with %%.15X : %.15X\n", "ABC")
    printf("123 with %%.15X : %.15X\n", 123)

    printf("ABC with %%15X : %15X\n", "ABC")
    printf("123 with %%15X : %15X\n", 123)

    printf("ABC with %%-15X : %-15X\n", "ABC")
    printf("123 with %%-15X : %-15X\n", 123)

    exit(0)
}
