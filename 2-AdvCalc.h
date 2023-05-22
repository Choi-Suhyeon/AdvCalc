/* interface to the lexer */
#include "2-AdvCalc.tab.h"

extern int yylineno;
void yyerror(char *, ...);

/* symbol table */
struct symbol {
	char * name;
	double value;
	struct ast * func;
	struct symlist * syms;
};

#define NHASH 9997
struct symbol symtab[NHASH];

struct symbol * lookup(char *);

/* list of symbols for an argument list */
struct symlist {
	struct symbol * sym;
	struct symlist * next;
};

struct symlist * newsymlist(struct symbol * sym, struct symlist * next); 
void symlistfree(struct symlist * sl);

enum bifs { /* built-in functions */
	B_sqrt = 1,
	B_exp,
	B_log,
	B_print,
};

/* nodes in the abstract syntax tree */
/* all have common initial nodetype */
struct ast {
	int nodetype;
	struct ast * l,
	           * r;
};

struct fncall {
	int nodetype; /* type F */
	struct ast * l;
	enum bifs functype;
};

struct ufncall {
	int nodetype; /* type C */
	struct ast * l;
	struct symbol * s;
};

struct flow {
	int nodetype; /* type I or W */
	struct ast * cond, /* condition */
			   * tl,   /* then branch */
			   * el;   /* else branch, optional */
};

struct numval {
	int nodetype; /* type K */
	double number;
};

struct symref {
	int nodetype; /* type N */
	struct symbol * s;
};

struct symasgn {
	int nodetype; /* type = */
	struct symbol * s;
	struct ast * v;
};

/* build an AST */
struct ast * newast(int, struct ast *, struct ast *);
struct ast * newcmp(int, struct ast *, struct ast *);
struct ast * newfnc(int, struct ast *);
struct ast * newcall(struct symbol *, struct ast *);
struct ast * newref(struct symbol *);
struct ast * newasgn(struct symbol *, struct ast *);
struct ast * newnum(double);
struct ast * newflow(int, struct ast *, struct ast *, struct ast *);

/* define a function */
void dodef(struct symbol * name, struct symlist * syms, struct ast * stmts);

/* evaluate an AST */
double eval(struct ast *);

/* delete & free an AST */
void treefree(struct ast *);

