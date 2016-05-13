
/*	CCERR.C - Error Handling
**
**	(c) Copyright Ken Harrenstien 1989
**		All changes after v.57, 15-Apr-1988
**	(c) Copyright Ken Harrenstien, SRI International 1985, 1986
**		All changes after v.23, 8-Aug-1985
**
**	Original version (C) 1981  K. Chen
*/

#include "cc.h"
#include "cclex.h"	/* For access to error line buffer */
#include "ccchar.h"	/* For isprint() */
#include <stdarg.h>	/* For var args */
#include <stdlib.h>	/* calloc(), EXIT_SUCCESS, EXIT_FAILURE */
#include <string.h>

/* Exported functions declared in ccerr.h */
void errfopen (char *desc, char *fnam);
int expect (int t);
void note (char *fmt, ...);
void advise (char *fmt, ...);
void warn (char *fmt, ...);
void int_warn (char *fmt, ...);
void error (char *fmt, ...);
void int_error (char *fmt, ...);
void efatal (char *fmt, ...);

/* Imported functions */
extern char *estrcpy(char *, char *);	/* CCASMB for string hacking */
extern int nextoken(void);		/* CCLEX */

/* Internal functions */
static char *errmak(char *fmt, va_list ap);
static void buf_errmsg(char *);
static void context(char *etype, char *fmt, va_list ap);
static void ectran(char *to, char *from, int cnt);
static int evsprintf(char *cp, char *fmt, va_list *aap);
static char *tokname(int tok);
static int edefarg(char *cp, char **afmt, va_list *aap);
static void recover(int n);

#if 0
  static char *errmak();
  static void buf_errmsg();
  static void context();
  static void ectran();
  static int evsprintf();
  static char *tokname();
  static int edefarg();
  static void recover();

  #if __STDC__
    #define PRFUN(f) f(char *fmt, ...) { va_list ap; va_start(ap, fmt);
  #else
    #define PRFUN(f) f(fmt) char *fmt; { va_list ap; va_start(ap, fmt);
  #endif
#endif

/* NOTE - print notification message
*/
void 
note (char *fmt, ...)
{ 
    va_list ap; 

    va_start(ap, fmt);
    ++nwarns;
    if (wrnlev < WLEV_NOTE)
	context("[Note] ", fmt, ap);
}

/* ADVISE - print advisory message
*/
void 
advise (char *fmt, ...)
{
    va_list ap; 
    
    va_start(ap, fmt);
    ++nwarns;
    if (wrnlev < WLEV_ADVISE)
	context("[Advisory] ", fmt, ap);
}

/* WARN - print warning message
*/
void 
warn (char *fmt, ...)
{
    va_list ap; 

    va_start(ap, fmt);
    ++nwarns;
    if (wrnlev < WLEV_WARN)
	context("[Warning] ", fmt, ap);
}

void 
int_warn (char *fmt, ...)
{
    va_list ap; 
    
    va_start(ap, fmt);
    ++nwarns;
    context("[Warning][Internal error] ", fmt, ap);
}

/* ERROR - print error message
** INT_ERROR - same, but prefixes with "Internal error - "
*/
void 
error (char *fmt, ...)
{
    va_list ap; 

    va_start(ap, fmt);
    ++nerrors;				/* Mark this as an error */
    context("", fmt, ap);		/* Show context */
}

void 
int_error (char *fmt, ...)
{
    va_list ap; 

    va_start(ap, fmt);
    ++nerrors;					/* Mark this as an error */
    context("[Internal error] ", fmt, ap);	/* Show context */
}

/* EFATAL - print fatal error message, with context.
*/
void 
efatal (char *fmt, ...)
{
    va_list ap; 

    va_start(ap, fmt);
    context("[FATAL] ", fmt, ap);		/* Show context */
    exit(EXIT_FAILURE);
}

/* JMSG - print job error message (no context).
** JWARN - print job warning message ("% ...").  FW 2A(47)
** JERR - print job error message and bump error count for current module.
** FATAL - print fatal job error message and die.
*/

static char jerrhdr[] = "? KCC";
static char jwarnhdr[] = "% KCC";	/* FW 2A(47) */

void 
jmsg (char *fmt, ...)
{
    va_list ap; 


    va_start(ap, fmt);

    if (outmsgs == stdout)
        fprintf(outmsgs, "%s - %s\n", jerrhdr, errmak(fmt, ap));

    fprintf(stderr, "%s - %s\n", jerrhdr, errmak(fmt, ap));
}

/* FW 2A(47) */
void 
jwarn (char *fmt, ...)
    {
    va_list ap; 

    
    va_start (ap, fmt);
    fprintf (outmsgs, "%s - %s\n", jwarnhdr, errmak (fmt, ap));
    }

void 
jerr (char *fmt, ...)
{
    va_list ap; 

    
    va_start(ap, fmt);
    ++nerrors;
    fprintf(outmsgs, "%s - %s\n", jerrhdr, errmak(fmt, ap));
}

void fatal (char *fmt, ...)
{
    va_list ap; 

    va_start(ap, fmt);
    fprintf(outmsgs, "%s - Fatal error: %s\n", jerrhdr, errmak(fmt, ap));
    exit(EXIT_FAILURE);				/* stop program */
}

/* ERRFOPEN - Auxiliary for CC and CCOUT, invoked after failing fopen()s
*/
void
errfopen(char *desc, char *fnam)
{
    jerr("Could not open %s file \"%s\"", desc, fnam);
}

#if 0	/* 5/91 KCC size */
/* ERRNOMEM - Auxiliary invoked when a calloc fails.
*/
void
errnomem(char *s)
{
    efatal("Out of memory %s", s);
}
#endif

/* ERRMAK - Return pointer to a static buffer containing error msg text.
**	This does not contain a newline.
*/
static char *
errmak(char *fmt, va_list ap)
{
#if SYS_CSI		/* 9/91 shrink KCC */
    static char *emsgbuf = NULL;
    if (emsgbuf == NULL)
	emsgbuf = (char *) calloc (1, 1024);
        if (emsgbuf == NULL)
            jerr("Out of memory for huge error message buffer\n");
#else
    static char emsgbuf[2000];	/* Want lots of room to be real safe */
#endif

    evsprintf(emsgbuf, fmt, &ap);
    va_end(ap);
    return emsgbuf;
}

#if SYS_CSI 
/* BUF_ERRMSG - buffer current error message for printing later
**		in the mixed-listing file.
**
*/
static void 
buf_errmsg (char *errmsg)
{
	if (errbuf == NULL) {
		errbuf = (char *) calloc (1, strlen (errmsg) + 1);
                if (errbuf == NULL)
                    jerr("Out of memory for error message buffer\n");
		strcpy (errbuf, errmsg);
		err_waiting = 1;
		} /* if-part */
	else {
		errbuf = (char *) realloc((void *) errbuf,
			   (strlen (errbuf) + strlen (errmsg)));
                if (errbuf == NULL)
                    jerr("Out of memory for error message realloc\n");
		strcat (errbuf, errmsg);
		err_waiting++;
	} /* else-part */
}
#endif

/* CONTEXT - print context of error
**	If "fline" is set 0 (normally never, since it starts at 1)
**	the input buffer context will not be shown.
**	This feature is used when emitting warnings after a file has been
**	completely compiled.
*/

static void
context(char *etype, char *fmt, va_list ap)
{
    char *estr;
    char *cp, *ep;
    char conbuf[ERRLSIZE*6];	/* Allow for lots of "big" chars */
#if SYS_CSI
    char errstor[ERRLSIZE*6];   /* Buffer to keep entire error msg */
    char *esp = errstor;
#endif
    int cnt, colcnt;
    int here = line;		/* Line # on page */

    estr = errmak(fmt, ap);	/* Build error message */

    if (erptr != errlin && erptr[-1] == '\n')
	--here;			/* Find right line # on current page */
				/* (KLH: but probably not worth the trouble) */

#if 1	/* New version */
    fprintf(outmsgs, "\"%s\", line %d: %s%s\n",
			inpfname, fline, etype, estr);

#if SYS_CSI
    if (mlist)
	    sprintf(esp, "; \"%s\", line %d: %s%s\n; ",
			inpfname, fline, etype, estr);
#endif
		
    /* Someday may wish to make further context optional (runtime switch) */
    if (!fline || 0) return;	/* Omit buffer context if at EOF */

    cp = estrcpy(conbuf, "       (");	/* Indented by 6 */
    if (curfn != NULL) {		/* are we in some function? */
	cp = estrcpy(cp, curfn->Sname);	/* yes, give its name */
	if (fline > curfnloc) {
	    sprintf(cp, "+%d", fline - curfnloc);
	    cp += strlen(cp);
	}
	cp = estrcpy(cp, ", ");		/* separate from page/line info */
    }

    sprintf(cp, "p.%d l.%d): ", page, here);
    colcnt = strlen(conbuf);	/* # cols so far */
    fputs(conbuf, outmsgs);
#if SYS_CSI
    if (mlist)
	strcat(esp, conbuf);
#endif

    /* Show current input context */
#if 1
    /* Unroll circular buffer */
    if (!ercsiz) ercsiz = 79;	/* Set default if needed (# cols of context) */
    colcnt = ercsiz - colcnt;	/* Get # columns available for input context */
    cp = conbuf;
    ep = erptr;
    cnt = erpleft;
    while (*ep == 0 && --cnt > 0) ++ep;	/* Scan to find first non-null */
    if (cnt > 0) {
	ectran(conbuf, ep, cnt);	/* Translate cnt chars from ep to buf*/
	ectran(conbuf+strlen(conbuf),	/* then initial part */
		errlin, ERRLSIZE - erpleft);
    } else {
	ep = errlin;
	cnt = ERRLSIZE - erpleft;
	while (*ep == 0 && --cnt > 0) ++ep;
	ectran(conbuf, ep, cnt);
    }
    if ((cnt = strlen(cp = conbuf)) > colcnt)
	cp += cnt - colcnt;	/* If too long, show only last N chars */

    fputs(cp, outmsgs);		/* Output the context string! */
    fputc('\n', outmsgs);
    fputc('\n', outmsgs);	/* Extra newline between msgs for clarity */
#if SYS_CSI
    if (mlist) {
	strcat(esp, cp);
	strcat(esp, "\n");
	buf_errmsg(esp);
    } /* if mlist */
#endif /* SYS_CSI */

#else
    if (erptr != errlin) *erptr = 0;	/* terminate line for printf */
    fprintf(outmsgs, "%s\n", errlin);	/* print where we were */
#endif

#else	/* Old version */
    fprintf(outmsgs, "\n%s at ", etype);	/* start error message */
    if (curfn != NULL) {		/* are we in some function? */
	fputs(curfn->Sname, outmsgs);	/* yes, give its name */
	if (fline > curfnloc) fprintf(outmsgs, "+%d", fline - curfnloc);
	fputs(", ", outmsgs);		/* separate from absolute loc */
    }
    if (page > 1) fprintf(outmsgs, "page %d ", page); /* page number */
    fprintf(outmsgs,"line %d of %s:\n", here, inpfname);
    if (erptr != errlin) *erptr = 0;	/* terminate line for printf */
    fputs(errlin, outmsgs);		/* print where we were */
#endif
}

/* ECTRAN - translate file input string to something nice for
**	error message output.  Always adds a NUL after "cnt" chars.
*/
static void
ectran(char *to, char *from, int cnt)
{
    int c;
    char *exp;
    char expbuf[8];

    while (--cnt >= 0) {
	if (isprint(c = *from++)) {
	    *to++ = c;
	    continue;
	} else switch (c) {
	case (char)-1:	exp = "<EOF>"; break;
	case '\b':	exp = "<\\b>"; break;	/* Show unusual whitespace */
	case '\f':	exp = "<\\f>"; break;
	case '\v':	exp = "<\\v>"; break;
	case '\r':	exp = "<\\r>"; break;
#if 1
	case '\t':
	case '\n':	exp = " "; break;	/* Just use whitespace */
#else
	case '\t':	exp = "<\\t>"; break;
	case '\n':	exp = "<\\n>"; break;
#endif
	default: sprintf(exp = expbuf,"<\\%o>", c); break;
	}
	to = estrcpy(to, exp);
    }
    *to = '\0';		/* Ensure string ends with null. */
}

static int
evsprintf(char *cp, char *fmt, va_list *aap)
{
    int i, max;
    char *str;
    NODE *n;
    SYMBOL *s;
    int cnt = 0;
    va_list ap = *aap;
    
    for (*cp = *fmt; *cp; *++cp = *++fmt) {
	if (*cp != '%') { ++cnt; continue; }
	switch (*++fmt) {
	case '%': continue;
	case 'E':		/* Substitute new format string */
	    fmt = va_arg(ap, char *);
	    --fmt;
	    i = 0;
	    break;
	case 'N':		/* Node op printout */
	    n = va_arg(ap, NODE *);
	    i = sprintf(cp, "(node %d: %d=%s)", nodeidx(n), n->Nop,
						tokname(n->Nop));
	    break;
	case 'Q':		/* Token printout */
	    i = va_arg(ap, int);
	    i = sprintf(cp, "(token %d=%s)", i, tokname(i));
	    break;
	case 'S':		/* Symbol printout */
	    s = va_arg(ap, SYMBOL *);
	    str = s->Sname;
	    i = '"';			/* Default quoting char */
	    max = IDENTSIZE-1;		/* Max ident length */
	    switch (str[0]) {
	    case SPC_IDQUOT:
		i = '`';	/* Different quote char, then drop thru */
	    case SPC_SMEM:
	    case SPC_TAG:
	    case SPC_LABEL:	/* Don't show prefix char */
		--max;
		++str;
	    default:
	        ;	/* do nothing */
	    }
	    i = sprintf(cp, "%c%.*s%c", i, max, str, i);
	    break;
	default:
	    i = edefarg(cp, &fmt, &ap);
	    break;
	}
	cnt += i;
	cp += i-1;
    }
    *aap = ap;		/* Update arglist pointer */
    return cnt;
}

static char *
tokname(int tok)
{
    return (0 < tok && tok < NTOKDEFS) ? nopname[tok] : "??";
}

/* EDEFARG - auxiliary to handle default %-specifications.
**	Simple-mindedly copies anything that looks like a sprintf format
**	spec, and invokes sprintf on it with an appropriate argument
**	plucked from the arglist.
** Return value is # chars written, and
** FMT is updated to point at last char read.
*/
static int
edefarg(char *cp, char **afmt, va_list *aap)
/* cp Points to place to deposit, afmt Points to 1st char after % */
{
    int c;
    char fmtbuf[50];
    char *fmt = *afmt;
    char *bp = fmtbuf;
    int typ = 0;

    *bp = '%';
    for (c = *fmt; ; c = *++fmt) {
	switch (*++bp = c) {
	case '*':		/* Can't handle this */
	default:		/* Or end-of-string or anything unknown */
	    *++cp = **afmt;
	    return 2;
	case '-': case '+': case '0':	/* Prefix flags */
	case ' ': case '#': case '.':
	case '1': case '2': case '3':	/* Width or precision specs */
	case '4': case '5': case '6':
	case '7': case '8': case '9':
	    continue;
	case 'h': case 'l': case 'L':	/* Type size flags */
	    typ = c;
	    continue;
	case 's':
	    typ = c;
	    break;
	case 'f': case 'e': case 'E': case 'g': case 'G':
	    if (!typ) typ = 'd';
	    break;
	case 'c':		/* Types that take (int) unless h or l seen */
	case 'i': case 'd': case 'o':  case 'u': case 'x': case 'X':
	    break;
	}
	/* Done, assume last char is conversion specifier */
	*++bp = 0;		/* Tie off format string */
	*afmt = fmt;		/* Won, update format ptr */
	switch (typ) {		/* Default type size is (int) */
	default:  return sprintf(cp, fmtbuf, va_arg(*aap, int));
	case 'l': return sprintf(cp, fmtbuf, va_arg(*aap, long));
	case 'h': return sprintf(cp, fmtbuf, va_arg(*aap, short));
	case 's': return sprintf(cp, fmtbuf, va_arg(*aap, char *));
	case 'd': return sprintf(cp, fmtbuf, va_arg(*aap, double));
	case 'L': return sprintf(cp, fmtbuf, va_arg(*aap, long double));
	}
    }
}

/* ---------------------- */
/*	expect token      */
/* ---------------------- */
int
expect(int t)
{
    char *s, str[32];

    if (t == token) {
	nextoken();
	return 1;
    }
    switch (t) {
    case T_LPAREN:	s = "left parenthesis"; 	break;
    case T_RPAREN:	s = "right parenthesis"; 	break;
    case T_LBRACK:	s = "left bracket"; 		break;
    case T_RBRACK:	s = "right bracket"; 		break;
    case T_SCOLON:	s = "semicolon";		break;
    case T_COMMA:	s = "comma";			break;
    case T_COLON:	s = "colon";			break;
    case Q_IDENT:	s = "identifier";		break;
    case T_LBRACE:	s = "open brace";		break;
    case T_RBRACE:	s = "close brace";		break;
    case Q_WHILE:	s = "\"while\" keyword";	break;
    default:		sprintf(s = str, "[token %d]", t);	break;
    }
    error("Expected token (%s) not found", s);
    recover(t);
    return 0;
}

/* ------------------------ */
/*	error recovery      */
/* ------------------------ */
/* KLH: this is pretty poor; someday work on it. */

static void
recover(int n)
{
    if (n == T_SCOLON) {
	while (!eof &&  token != T_SCOLON && 
			token != T_RBRACE &&
			token != T_LBRACE)
	    nextoken();
	if (token == T_SCOLON) nextoken();
	return;
    }
/*  tokpush(token, csymbol); */
/*  token = n;		     */
}

#if 0	/* 5/91 KCC size */
int
errflush(void)
{
    for(;;) switch (token) {
	case T_EOF: case T_SCOLON: case T_RBRACE:
	    return nextoken();
	default:
	    nextoken();
    }
}
#endif
