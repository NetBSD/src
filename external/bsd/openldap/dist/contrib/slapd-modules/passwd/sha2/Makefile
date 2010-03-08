# OpenLDAP: pkg/ldap/contrib/slapd-modules/passwd/sha2/Makefile,v 1.1.2.1 2009/01/26 21:07:06 quanah Exp

OPENLDAP := ../../../..

#CCFLAGS = -Wall -g -DSLAPD_SHA2_DEBUG
CCFLAGS = -Wall -g

slapd-sha2.so: slapd-sha2.o sha2.o
	$(CC) -I$(OPENLDAP)/include -shared -Wall -g $^ -o $@

%.o: %.c
	$(CC) -I$(OPENLDAP)/include $(CCFLAGS) -c $<

clean:
	@rm -f slapd-sha2.so *.o
