#include "/usr/include.orig/signal.h"
#include <stdio.h>
#include <ctype.h>

unsigned fp_res[3], em_res[3];	/* place for the result */
unsigned curproc;		/* not used */
int lineno;

union {
	unsigned w;
	float	f;
} uf;

union {
	unsigned w[2];
	double	d;
} ud;

struct optab {
	char	*name;		/* MIPS opcode name */
	int	nargs;		/* number of integer arguments needed */
	int	nresult;	/* number of integer words for the result */
} optab[] = {
	"add.s", 2, 1,
	"add.d", 4, 2,
	"sub.s", 2, 1,
	"sub.d", 4, 2,
	"mul.s", 2, 1,
	"mul.d", 4, 2,
	"div.s", 2, 1,
	"div.d", 4, 2,
	"abs.s", 1, 1,
	"abs.d", 2, 2,
	"mov.s", 1, 1,
	"mov.d", 2, 2,
	"neg.s", 1, 1,
	"neg.d", 2, 2,
	"cvt.s.d", 2, 1,
	"cvt.s.w", 1, 1,
	"cvt.d.s", 1, 2,
	"cvt.d.w", 1, 2,
	"cvt.w.s", 1, 1,
	"cvt.w.d", 2, 1,
	"c.f.s", 2, 0,
	"c.un.s", 2, 0,
	"c.eq.s", 2, 0,
	"c.ueq.s", 2, 0,
	"c.olt.s", 2, 0,
	"c.ult.s", 2, 0,
	"c.ole.s", 2, 0,
	"c.ule.s", 2, 0,
	"c.sf.s", 2, 0,
	"c.ngle.s", 2, 0,
	"c.seq.s", 2, 0,
	"c.ngl.s", 2, 0,
	"c.lt.s", 2, 0,
	"c.nge.s", 2, 0,
	"c.le.s", 2, 0,
	"c.ngt.s", 2, 0,
	"c.f.d", 4, 0,
	"c.un.d", 4, 0,
	"c.eq.d", 4, 0,
	"c.ueq.d", 4, 0,
	"c.olt.d", 4, 0,
	"c.ult.d", 4, 0,
	"c.ole.d", 4, 0,
	"c.ule.d", 4, 0,
	"c.sf.d", 4, 0,
	"c.ngle.d", 4, 0,
	"c.seq.d", 4, 0,
	"c.ngl.d", 4, 0,
	"c.lt.d", 4, 0,
	"c.nge.d", 4, 0,
	"c.le.d", 4, 0,
	"c.ngt.d", 4, 0,
};

/*
 * Read test vectors from file and test the fp emulation by
 * comparing it with the hardware.
 */
main(argc, argv)
	int argc;
	char **argv;
{
	register char *cp;
	register int c;
	register struct optab *op;
	char buf[10];
	unsigned arg[4];

	signal(SIGFPE, SIG_IGN);
	for (lineno = 1; ; lineno++) {
		/* read opcode */
		for (cp = buf; ; ) {
			c = getchar();
			if (c == EOF)
				return (0);
			if (isspace(c))
				break;
			*cp++ = c;
		}
		*cp = '\0';
		if (buf[0] == '\0') {
			if (c == '\n')
				continue;
			goto skip;
		}
		for (op = optab; op->name; op++) {
			if (strcmp(op->name, buf) == 0)
				goto fnd;
		}
		fprintf(stderr, "line %d: unknown operation '%s'\n",
			lineno, buf);
		goto skip;
	fnd:
		switch (op->nargs) {
		case 1:
			if (scanf("%x", &arg[0]) != 1) {
				fprintf(stderr, "line %d: expected 1 arg\n",
					lineno);
				goto skip;
			}
			arg[1] = arg[2] = arg[3] = 0;
			break;
		case 2:
			if (scanf("%x %x", &arg[0], &arg[2]) != 2) {
				fprintf(stderr, "line %d: expected 2 args\n",
					lineno);
				goto skip;
			}
			if (op->nresult == 1) {
				arg[1] = arg[0];
				arg[0] = arg[2];
				arg[2] = arg[3] = 0;
			} else if (op->nresult == 2) {
				arg[1] = arg[2];
				arg[2] = arg[3] = 0;
			} else {
				arg[1] = arg[3] = 0;
			}
			break;
		case 4:
			if (scanf("%x %x %x %x", &arg[1], &arg[0],
			    &arg[3], &arg[2]) != 4) {
				fprintf(stderr, "line %d: expected 4 args\n",
					lineno);
				goto skip;
			}
			break;
		}

		c = op - optab;
		dofp(c, arg[0], arg[1], arg[2], arg[3]);
		emfp(c, arg[0], arg[1], arg[2], arg[3]);

		switch (op->nresult) {
		case 0:
			if (fp_res[2] != em_res[2]) {
				printf("line %d: (%d) %s %x,%x %x,%x\n",
					lineno, c, buf,
					arg[0], arg[1], arg[2], arg[3]);
				printf("\tcsr %x != %x\n",
					fp_res[2], em_res[2]);
			}
			break;
		case 1:
			if (fp_res[0] != em_res[0] ||
			    fp_res[2] != em_res[2]) {
				printf("line %d: (%d) %s %x,%x %x,%x\n",
					lineno, c, buf,
					arg[0], arg[1], arg[2], arg[3]);
				printf("\t%x != %x (csr %x %x)\n",
					fp_res[0], em_res[0],
					fp_res[2], em_res[2]);
			}
			break;
		case 2:
			if (fp_res[0] != em_res[0] ||
			    fp_res[1] != em_res[1] ||
			    fp_res[2] != em_res[2]) {
				printf("line %d: (%d) %s %x,%x %x,%x\n",
					lineno, c, buf,
					arg[0], arg[1], arg[2], arg[3]);
				printf("\t%x,%x != %x,%x (csr %x %x)\n",
					fp_res[0], fp_res[1],
					em_res[0], em_res[1],
					fp_res[2], em_res[2]);
			}
			break;
		}
	skip:
		while ((c = getchar()) != EOF && c != '\n')
			;
	}
}

trapsignal(p, sig, code)
	int p, sig, code;
{
	printf("line %d: signal(%d, %x)\n", lineno, sig, code);
}
