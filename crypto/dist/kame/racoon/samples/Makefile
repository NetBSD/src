HOSTNAME!=	/bin/hostname

.if ${HOSTNAME} == "lychee.itojun.org"
SRC=	206.175.160.20
SRC6=	3ffe:501:410:ffff:200:86ff:fe05:80fa
.elif ${HOSTNAME} == "cardamom.itojun.org"
SRC=	206.175.160.21
SRC6=	3ffe:501:410:ffff:210:4bff:fea2:8baa
.endif

TARGETS=	cardamom.conf lychee.conf cardamom6.conf lychee6.conf ssh.conf

all: $(TARGETS)

lychee.conf: sandiego.pl
	perl sandiego.pl $(SRC) 206.175.160.20 > lychee.conf
cardamom.conf: sandiego.pl
	perl sandiego.pl $(SRC) 206.175.160.21 > cardamom.conf
lychee6.conf: sandiego.pl
	perl sandiego.pl $(SRC6) 3ffe:501:410:ffff:200:86ff:fe05:80fa > lychee6.conf
cardamom6.conf: sandiego.pl
	perl sandiego.pl $(SRC6) 3ffe:501:410:ffff:210:4bff:fea2:8baa > cardamom6.conf
ssh.conf: sandiego.pl
	perl sandiego.pl $(SRC) 194.100.55.1 > ssh.conf

# itojun
ms.conf: sandiego.pl
	perl sandiego.pl $(SRC) 206.175.161.182 > ms.conf

clean:
	-rm -f $(TARGETS)
