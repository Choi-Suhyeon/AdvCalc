#include <memory.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "2-AdvCalc.h"

void exitwitherr(char * err) {
    yyerror(err);
    exit(1);
}

/* symbol table */
/* hash a symbol */

static unsigned symhash(char * sym) {
    unsigned c;

    unsigned hash = 0;

    while ((c = *sym++)) hash += (hash << 3) + c;

    return hash;
}

struct symbol * lookup(char * sym) {
    struct symbol * sp = symtab + symhash(sym) % NHASH;
    int           scount = NHASH;

    while (--scount >= 0) {
        if (sp->name && !strcmp(sp->name, sym)) return sp;

        if (!sp->name) return memmove(sp, &((struct symbol) { .name = strdup(sym), .value = 0, .func = NULL, .syms = NULL }), sizeof(struct symbol));

        if (++sp >= symtab + NHASH) sp = symtab;
    }

    yyerror("symbol table overflow\n");
    abort();
}

struct ast * newast(int type, struct ast * l, struct ast * r) {
    struct ast * a = (struct ast *)malloc(sizeof(struct ast));

    if (!a) exitwitherr("out of space");

    return memmove(a, &((struct ast) { .nodetype = type, .l = l, .r = r }), sizeof(struct ast));
}

struct ast * newnum(double d) {
    struct numval * a = (struct numval *)malloc(sizeof(struct numval));

    if (!a) exitwitherr("out of space");

    return (struct ast *)memmove(a, &((struct numval) { .nodetype = 'K', .number = d }), sizeof(struct numval));
}

struct ast * newcmp(int type, struct ast * l, struct ast * r) {
    struct ast * a = (struct ast *)malloc(sizeof(struct ast));

    if (!a) exitwitherr("out of space");

    return memmove(a, &((struct ast) { .nodetype = '0' + type, .l = l, .r = r }), sizeof(struct ast));
}

struct ast * newfnc(int type, struct ast * l) {
    struct fncall * a = (struct fncall *)malloc(sizeof(struct fncall));

    if (!a) exitwitherr("out of space");

    return (struct ast *)memmove(a, &((struct fncall) { .nodetype = 'F', .l = l, .functype = type }), sizeof(struct fncall));
}

struct ast * newcall(struct symbol * s, struct ast * l) {
    struct ufncall * a = (struct ufncall *)malloc(sizeof(struct ufncall));

    if (!a) exitwitherr("out of space");

    return (struct ast *)memmove(a, &((struct ufncall) { .nodetype = 'C', .l = l, .s = s }), sizeof(struct ufncall));
}

struct ast * newref(struct symbol * s) {
    struct symref * a = (struct symref *)malloc(sizeof(struct symref));

    if (!a) exitwitherr("out of space");

    return (struct ast *)memmove(a, &((struct symref) { .nodetype = 'N', .s = s }), sizeof(struct symref));
}

struct ast * newasgn(struct symbol * s, struct ast * v) {
    struct symasgn * a = (struct symasgn *)malloc(sizeof(struct symasgn));

    if (!a) exitwitherr("out of space");

    return (struct ast *)memmove(a, &((struct symasgn) { .nodetype = '=', .s = s, .v = v }), sizeof(struct symasgn));
}

struct ast * newflow(int type, struct ast * cond, struct ast * tl, struct ast * el) {
    struct flow * a = (struct flow *)malloc(sizeof(struct flow));

    if (!a) exitwitherr("out of space");

    return (struct ast *)memmove(a, &((struct flow) { .nodetype = type, .cond = cond, .tl = tl, .el = el }), sizeof(struct flow));
}

void treefree(struct ast * a) {
    switch (a->nodetype) {
        case '+':
        case '-':
        case '*':
        case '/':
        case 'L':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
            treefree(a->r);
        case '|':
        case 'M':
        case 'C':
        case 'F':
            treefree(a->l);
        case 'K':
        case 'N':
            break;
        case '=':
            free(((struct symasgn *)a)->v);
            break;
        case 'I':
        case 'W':
            free(((struct flow *)a)->cond);
            if (((struct flow *)a)->tl) treefree(((struct flow *)a)->tl);
            if (((struct flow *)a)->el) treefree(((struct flow *)a)->el);
            break;
        default:    
            printf("internal error: free bad node %c\n", a->nodetype);
    }

    free(a);
}

struct symlist * newsymlist(struct symbol * sym, struct symlist * next) {
    struct symlist * sl = (struct symlist *)malloc(sizeof(struct symlist));

    if (!sl) exitwitherr("out of space");

    return memmove(sl, &((struct symlist) { .sym = sym, .next = next }), sizeof(struct symlist));
}

void symlistfree(struct symlist * sl) {
    while (sl) {
        struct symlist * nsl = sl->next;

        free(sl);
        sl = nsl;
    }
}

static double callbuiltin(struct fncall *);
static double calluser(struct ufncall *);

double eval(struct ast * a) {
    double v;

    if (!a) {
        yyerror("internal error, null eval");

        return 0.0;
    }

    switch (a->nodetype) {
        /* constant */
        case 'K': v = ((struct numval *)a)->number; break;

        /* name reference */
        case 'N': v = ((struct symref *)a)->s->value; break;
        
        /* assignment */
        case '=': v = ((struct symasgn *)a)->s->value = eval(((struct symasgn *)a)->v); break;

        /* expression */
        case '+': v = eval(a->l) + eval(a->r); break;
        case '-': v = eval(a->l) - eval(a->r); break;
        case '*': v = eval(a->l) * eval(a->r); break;
        case '/': v = eval(a->l) / eval(a->r); break;
        case '|': v = (v = eval(a->l)) >= 0 ? v : -v; break;
        case 'M': v = -eval(a->l); break;

        /* comparisons */
        case '1': v = eval(a->l) >  eval(a->r); break;
        case '2': v = eval(a->l) <  eval(a->r); break;
        case '3': v = eval(a->l) != eval(a->r); break;
        case '4': v = eval(a->l) == eval(a->r); break;
        case '5': v = eval(a->l) >= eval(a->r); break;
        case '6': v = eval(a->l) <= eval(a->r); break;

        /* control flow */
        /* if/then/else */
        case 'I':
            v = eval(((struct flow *)a)->cond)
                ? (((struct flow *)a)->tl ? eval(((struct flow *)a)->tl) : 0.) 
                : (((struct flow *)a)->el ? eval(((struct flow *)a)->el) : 0.);
            break; 
        /* while/do */
        case 'W':
            v = 0.;

            if (((struct flow *)a)->tl) {
                while (eval(((struct flow *)a)->cond)) {
                    v = eval(((struct flow *)a)->tl);
                }
            }

            break;

        /* list of statements */
        case 'L': eval(a->l); v = eval(a->r); break;
        case 'F': v = callbuiltin((struct fncall *)a); break;
        case 'C': v = calluser((struct ufncall *)a); break;

        default: printf("internal error: bad node %c\n", a->nodetype);
    }

    return v;
}

void dodef(struct symbol * name, struct symlist * syms, struct ast * func) {
    if (name->syms) symlistfree(name->syms);
    if (name->func) treefree(name->func);

    name->syms = syms;
    name->func = func;
}

static double callbuiltin(struct fncall * f) {
    enum bifs functype = f->functype;
    double v = eval(f->l);

    switch (functype) {
        case B_sqrt: return sqrt(v);
        case B_exp:  return exp(v);
        case B_log:  return log(v);
        case B_print: printf("= %4.4lf\n", v); return v;
        default:
            yyerror("Unknown built-in function %d\n", functype);
            return 0.;
    }
}

static double calluser(struct ufncall * f) {
    int nargs;

    struct symbol * fn = f->s;
    struct ast * args = f->l;

    if (!fn->func) {
        yyerror("call to undefined function %s", fn->name);
        return 0.;
    }

    struct symlist * sl = fn->syms;

    for (nargs = 0; sl; sl = sl->next) nargs++;

    double * oldval = (double *)malloc(nargs * sizeof(double)),
           * newval = (double *)malloc(nargs * sizeof(double));

    if (!(oldval && newval)) {
        yyerror("Out of space in %s", fn->name);
        return 0.;
    }

    for (int i = 0; i < nargs; i++) {
        if (!args) {
            yyerror("too few args in call to %s", fn->name);
            free(oldval);
            free(newval);

            return 0.;
        }

        if (args->nodetype == 'L') {
            newval[i] = eval(args->l);
            args = args->r;
        }
        else {
            newval[i] = eval(args);
            args = NULL;
        }
    }

    sl = fn->syms;

    for (int i = 0; i < nargs; i++) {
        struct symbol * s = sl->sym;

        oldval[i] = s->value;
        s->value = newval[i];
        sl = sl->next;
    }

    free(newval);

    double v = eval(fn->func);

    sl = fn->syms;

    for (int i = 0; i < nargs; i++) {
        struct symbol * s = sl->sym;

        s->value = oldval[i];
        sl = sl->next;
    }

    free(oldval);

    return v;
}

void yyerror(char * s, ...) {
	va_list ap;

	va_start(ap, s);
	fprintf(stderr, "%d: error: ", yylineno);
	vfprintf(stderr, s, ap);
	fprintf(stderr, "\n");
}

int main(void) {
	printf("> ");

	return yyparse();
}