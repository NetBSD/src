/*	$NetBSD: decode.h,v 1.1.26.1 2007/02/27 16:51:56 yamt Exp $	*/

/* Contributed to the NetBSD Foundation by Cherry G. Mathew <cherry@mahiti.org>
 * This file contains prototypes to decode unwind descriptors.
 */

#define MAXSTATERECS 20	/* The maximum number of descriptor records per region */

#define IS_R1(byte) (( (byte) & 0xc0) == 0)
#define IS_R2(byte) (((byte) & 0xf8) == 0x40)
#define IS_R3(byte) (((byte) & 0xfc) == 0x60)
#define IS_P1(byte) (((byte) & 0xe0) == 0x80)
#define IS_P2(byte) (((byte) & 0xf0) == 0xa0)
#define IS_P3(byte) (((byte) & 0xf8) == 0xb0)
#define IS_P4(byte) ((byte) == (char) 0xb8)
#define IS_P5(byte) ((byte) == (char) 0xb9)
#define IS_P6(byte) (((byte) & 0xe0) == 0xc0)
#define IS_P7(byte) (((byte) & 0xf0) == 0xe0)
#define IS_P8(byte) ((byte) == (char) 0xf0)
#define IS_P9(byte) ((byte) == (char) 0xf1)
#define IS_P10(byte) ((byte) ==(char) 0xff)
#define IS_B1(byte) (((byte) & 0xc0) == 0x80)
#define IS_B2(byte) (((byte) & 0xe0) == 0xc0)
#define IS_B3(byte) ((byte) == (char) 0xe0)
#define IS_B4(byte) (((byte) & 0xf7) == 0xf0)
#define IS_X1(byte) ((byte) == (char) 0xf9)
#define IS_X2(byte) ((byte) == (char) 0xfa)
#define IS_X3(byte) ((byte) == (char) 0xfb)
#define IS_X4(byte) ((byte) == (char) 0xfc)

struct unwind_desc_R1 {
	bool r;
	vsize_t rlen;
};

struct unwind_desc_R2 {
	u_int mask;
#define R2MASKRP	0x8
#define R2MASKPFS	0x4
#define R2MASKPSP	0x2

	u_int grsave;
	vsize_t rlen;
};

struct unwind_desc_R3 {
	bool r;
	vsize_t rlen;
};

struct unwind_desc_P1 {
	u_int brmask;
};

struct unwind_desc_P2 {
	u_int brmask;
	u_int gr;
};

struct unwind_desc_P3 {
	u_int r;
	u_int grbr;
};

struct unwind_desc_P4 {
	vsize_t imask;
};

struct unwind_desc_P5 {
	u_int grmask;
	u_int frmask;
};

struct unwind_desc_P6 {
	bool r;
	u_int rmask;
};

struct unwind_desc_P7 {
	u_int r;
	vsize_t t;
	vsize_t size;
};

struct unwind_desc_P8 {
	u_int r;
	vsize_t t;
};

struct unwind_desc_P9 {
	u_int grmask;
	u_int gr;
};

struct unwind_desc_P10 {
	u_int abi;
	u_int context;
};

struct unwind_desc_B1 {
	bool r;
	u_int label;
};

struct unwind_desc_B2 {
	u_int ecount;
	vsize_t t;
};

struct unwind_desc_B3 {
	vsize_t t;
	vsize_t ecount;
};

struct unwind_desc_B4 {
	bool r;
	vsize_t label;
};

struct unwind_desc_X1 {
	bool r;
	bool a;
	bool b;
	u_int reg;
	vsize_t t;
	vsize_t offset;
};

struct unwind_desc_X2 {
	bool x;
	bool a;
	bool b;
	u_int reg;
	bool y;
	u_int treg;
	vsize_t t;
};



struct unwind_desc_X3 {
	bool r;
	u_int qp;
	bool a;
	bool b;
	u_int reg;
	vsize_t t;
	vsize_t offset;
};

struct unwind_desc_X4 {
	u_int qp;
	bool x;
	bool a;
	bool b;
	u_int reg;
	bool y;
	u_int treg;
	vsize_t t;
};

union unwind_desc {
	struct unwind_desc_R1 R1;
	struct unwind_desc_R2 R2;
	struct unwind_desc_R3 R3;

	struct unwind_desc_P1 P1;
	struct unwind_desc_P2 P2;
	struct unwind_desc_P3 P3;
	struct unwind_desc_P4 P4;
	struct unwind_desc_P5 P5;
	struct unwind_desc_P6 P6;
	struct unwind_desc_P7 P7;
	struct unwind_desc_P8 P8;
	struct unwind_desc_P9 P9;
	struct unwind_desc_P10 P10;

	struct unwind_desc_B1 B1;
	struct unwind_desc_B2 B2;
	struct unwind_desc_B3 B3;
	struct unwind_desc_B4 B4;

	struct unwind_desc_X1 X1;
	struct unwind_desc_X2 X2;
	struct unwind_desc_X3 X3;
	struct unwind_desc_X4 X4;
};
		
enum record_type {
	R1, R2, R3,
	P1, P2, P3, P4, P5, P6, P7, P8, P9, P10,
	B1, B2, B3, B4,
	X1, X2, X3, X4
};
	

/* A record chain is a decoded unwind descriptor.
 * It is usefull for post processing unwind descriptors.
 */

struct recordchain {
	enum record_type type;
	union unwind_desc udesc;
};



/* Decode Function prototypes. */

char *
unwind_decode_ule128(char *buf, unsigned long *);
char *
unwind_decode_R1(char *buf, union unwind_desc *uwd);
char *
unwind_decode_R2(char *buf, union unwind_desc *uwd);
char *
unwind_decode_R3(char *buf, union unwind_desc *uwd);
char *
unwind_decode_P1(char *buf, union unwind_desc *uwd);
char *
unwind_decode_P2(char *buf, union unwind_desc *uwd);
char *
unwind_decode_P3(char *buf, union unwind_desc *uwd);
char *
unwind_decode_P4(char *buf, union unwind_desc *uwd, vsize_t len);
char *
unwind_decode_P5(char *buf, union unwind_desc *uwd);
char *
unwind_decode_P6(char *buf, union unwind_desc *uwd);
char *
unwind_decode_P7(char *buf, union unwind_desc *uwd);
char *
unwind_decode_P8(char *buf, union unwind_desc *uwd);
char *
unwind_decode_P9(char *buf, union unwind_desc *uwd);
char *
unwind_decode_P10(char *buf, union unwind_desc *uwd);
char *
unwind_decode_B1(char *buf, union unwind_desc *uwd);
char *
unwind_decode_B2(char *buf, union unwind_desc *uwd);
char *
unwind_decode_B3(char *buf, union unwind_desc *uwd);
char *
unwind_decode_B4(char *buf, union unwind_desc *uwd);
char *
unwind_decode_X1(char *buf, union unwind_desc *uwd);
char *
unwind_decode_X2(char *buf, union unwind_desc *uwd);
char *
unwind_decode_X3(char *buf, union unwind_desc *uwd);
char *
unwind_decode_X4(char *buf, union unwind_desc *uwd);






