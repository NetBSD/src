/*
 * fgen.h -- stuff for the fcode tokenizer.
 */

/* Type of a Cell */
typedef long Cell;

/* Token from the scanner. */
struct tok {
	int type;
	char *text;
};

#define TOKEN struct tok
#define YY_DECL TOKEN* yylex __P((void))

#define FCODE	0xF00DBABE
#define MACRO	0xFEEDBABE

/* Defined fcode and string. */
struct fcode {
	char *name;
	long num;
	int type;
	struct fcode *l;
	struct fcode *r;
};

/* macro instruction as separate words */
struct macro {
	char *name;
	char *equiv;
	int type;
	struct macro *l;
	struct macro *r;
};

/*
 * FCode header -- assumes big-endian machine, 
 *	otherwise the bits need twiddling.
 */
struct fcode_header {
	char	header;
	char	format;
	short	checksum;
	int	length;
};

/* Tokenizer tokens */
enum toktypes { 
	TOK_OCTAL = 8,
	TOK_DECIMAL = 10,
	TOK_HEX = 16,

	TOK_NUMBER, 
	TOK_STRING_LIT, 
	TOK_C_LIT,
	TOK_PSTRING, 
	TOK_TOKENIZE,
	TOK_COMMENT, 
	TOK_ENDCOMMENT,
	TOK_COLON, 
	TOK_SEMICOLON, 
	TOK_TOSTRING,
	
	/* These are special */
	TOK_AGAIN,
	TOK_ALIAS,
	TOK_GETTOKEN,
	TOK_ASCII,
	TOK_BEGIN,
	TOK_BUFFER,
	TOK_CASE,
	TOK_CONSTANT,
	TOK_CONTROL,
	TOK_CREATE,
	TOK_DEFER,
	TOK_DO,
	TOK_ELSE,
	TOK_ENDCASE,
	TOK_ENDOF,
	TOK_EXTERNAL,
	TOK_FIELD,
	TOK_HEADERLESS,
	TOK_HEADERS,
	TOK_IF,
	TOK_LEAVE,
	TOK_LOOP,
	TOK_OF,
	TOK_REPEAT,
	TOK_THEN,
	TOK_TO,
	TOK_UNTIL,
	TOK_VALUE,
	TOK_VARIABLE,
	TOK_WHILE,
	TOK_OFFSET16,

	/* Tokenizer directives */
	TOK_BEGTOK,
	TOK_EMIT_BYTE,
	TOK_ENDTOK,
	TOK_FLOAD,

	TOK_OTHER
};
