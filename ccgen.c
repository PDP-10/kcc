/*	CCGEN.C - Generate code for parse-tree data declarations
**
**	(c) Copyright Ken Harrenstien 1989
**		All changes after v.221, 25-Apr-1988
**	(c) Copyright Ken Harrenstien, SRI International 1985, 1986
**		All changes after v.84, 8-Aug-1985
**
**	Original version (C) 1981  K. Chen
*/

#include "cc.h"
#include "ccgen.h"
#include "ccchar.h"
#include <string.h>

/* Imported (external) functions used herein */
extern void ridlsym(SYMBOL *);				/* CCSYM */
extern SYMBOL *newlabel(void);
extern void freelabel(SYMBOL *);
extern INT sizearray(TYPE *), sizetype(TYPE *);		/* CCSYM */
extern int elembsize(TYPE *);
extern void code5(int, VREG *), 
	code6(int, VREG *, SYMBOL *), codemdx(int, int, SYMBOL *, INT, int), 
        code00 (int, int, int), code12 (int, VREG *, INT),
	code8(int, VREG *, INT), flushcode(void);	/* CCCODE */
extern VREG *vrget(void);
extern void genstmt(NODE *);
extern void genxrelease(NODE *);
extern void				/* CCOUT */
	outmidef(SYMBOL *), outmiref(SYMBOL *), outid(char *), 
	outptr(SYMBOL *, int, INT), outscon(char *, int, int), 
	outlab(SYMBOL *), outnum(INT), outnl(void), 
	outstr(char *);
extern int				/* CCOUT */
	outflt(int, INT *, int), codeseg(void), dataseg(void), prevseg(int);
extern void vrinit(void);			/* CCREG.C */
extern void vrendchk (void);
extern void vrfree (VREG *);

extern
void	    outiprolog (void);		/* FW 2A(52) */

#if SYS_CSI		
extern void outpghdr(void), outprolog(SYMBOL *);		/* CCOUT */
#endif

/* Exported functions defined here */
void gencode(NODE *n);		/* Called by CC mainline */

/* Internal Functions */
static void inicode(void), 
	    endcode(void), 
	    gendata(NODE *), 
	    genfunct(NODE *), 
	    gliterals(void), 
	    giz(NODE *, TYPE *, SYMBOL *), 
	    gizword(NODE *, TYPE *, SYMBOL *), 
	    giznull(TYPE *), 
	    gizexpr(NODE *, TYPE *);
static int  gizptr(NODE *); 
static INT  gizconst(NODE *);

static void gizlist(NODE *, TYPE *, SYMBOL *);
static void gizbytes(NODE *, TYPE *, SYMBOL *, int);
static void bytbeg(int), 
	    wdalign(void), 
	    outval(long), 
	    outbyte(long, int), 
	    outzbs(INT), 
	    outzwds(INT);
static INT  bytend(void);
#if 0
static void inicode(), endcode(), gendata(), genfunct(), gliterals();
static void giz(), gizword(), giznull(), gizexpr();
static int gizptr(), gizconst();

static void gizlist();
static void gizbytes();
static void bytbeg(), wdalign(), outval(), outbyte(),
	outzbs(), outzwds();
static INT bytend();
#endif

/* OUT data emission vars. */
static int bsiz;	/* 0 if in word mode, else byte size in bits */
static int bpw;		/* # bytes per word */
static int bpos;	/* P of last byte deposited (TGSIZ_WORD at beg of wd)*/
static INT savlct;	/* Saved loc ctr at start of byte mode */
static INT locctr;	/* Location counter (only for tracking # wds output) */

/* GENCODE - Generate code/data from parse-tree node
*/
void
gencode(NODE *n)
{
    if (n)				/* Ignore null stmts/defs */
	switch (n->Nop) {
	    case N_DATA:
		if (!nerrors) gendata(n); 	/* Generate data definition */
		break;
	    case N_FUNCTION:
		if (!nerrors) genfunct(n);	/* Generate function instrs */
		ridlsym((SYMBOL *)NULL);	/* Flush any local symbols */
		break;
	    default:
		int_error("gencode: bad node %N", n);
 	}
}

/*
 * genfunct ()
 *
 * - Generate machine instruction code for function
 */

static
void
genfunct (NODE* n)
    {
    extern
    int		_word_cnt;	/* KAR-2/91, from CCOUT # wds in function */
    int		i;
    VREG*	r;


    isr = 0;

    if (n->Nleft->Nright)		/* Any local-scope static data defs? */
	gendata(n->Nleft->Nright);	/* Yes, generate them first */

    codeseg ();				/* Ensure in code segment */
    inicode ();				/* Start making code */

    if (mlist)
	{
	/* Emit definitions for register variables. */

	for (i = 0; i < _reg_count; i++)
	    {
	    putc ('\t', out);
	    outid (Reg_Id[i]->Sname);
	    putc ('=', out);
	    outnum (i + r_maxnopreserve + 1); /* FW 2A(47) */
	    putc ('\n', out);
	    }
	}

    outmidef(n->Nleft->Nleft->Nid);	/* Output function label */

    if (curfn != n->Nleft->Nleft->Nid)
	int_error("Bad funct Nid \"%S\"", n->Nleft->Nleft->Nid);

    if (n->Nleft->Nleft->Nid->Sflags & TF_INTERRUPT)
	{				/* FW 2A(52) */
	isr = 1;
	outiprolog ();
	}

    if (profbliss)			/* for BLISS profiler */
	outprolog (curfn);		/* added 09/15/89 by MVS */

    if (n->Nreg <= R_PRESERVE_COUNT)	/* Reg linkage */
	{
        for (i = 0; i < n->Nreg;  i++)
	    {
	    /* Push preserved registers for all functions except main () */   

	    if (!fn_main)
		{	
		code00 (P_PUSH, R_SP, i + r_maxnopreserve + 1); /* FW 2A(47) */
	        ++stackoffset;
	        oline++;
	        }
		
	    /* Initialize all reg parameters (often optimized away later) */

	    if (Reg_Id[i]->Sclass == SC_RARG)
	        {
		r_preserve = i + r_maxnopreserve + 1; /* FW 2A(47) */
		r = vrget();

		/* ex:  MOVE argc,-1(SP) */

		code12(P_MOVE, r, -(Reg_Id[i]->Svalue) - stackoffset);
		vrfree(r);
	        }
	    }
	}
    
    if (maxauto)
	{				/* If any auto vars, */
	code8(P_ADJSP, VR_SP, maxauto);	/* make room for them on stack */
	stackoffset += maxauto;		/* and remember stack bumped */
        }

    genstmt (n->Nright);		/* Generate code for body */
    endcode ();				/* Wrap up code */

    if (vrbfun) 
	fprintf (outmsgs, "%d words\n", _word_cnt);

    _word_cnt = 0;

    if (mlist && (oline > HDR_LINES))
	outpghdr();
}

/* INICODE - Common code generation inits
*/
static void
inicode(void)
{
    previous = NULL;
    litstrings = NULL;
    litnodes = NULL;
    looplabel = brklabel = NULL;
    stackoffset = maxcode = mincode = 0;
    vrinit();
}

/* ENDCODE - Common code generation wrap-ups
*/

static void
endcode(void)
{
    flushcode();	/* Flush out peephole buffer */
    gliterals();	/* Generate any accumulated literals */
    vrendchk();		/* Check to make sure no regs active */
}

/* GENDATA - Generate data definitions
**
** This routine is only called to process static-extent data definitions
** of global or local scope, as opposed to local-extent (automatic) defs
** which are generated by genadata() in CCGEN1.
** Note that the Ntype of the symbol's Q_IDENT node is never examined here;
** the symbol's Stype is used instead.  They are identical except for
** array and function names, when the Ntype is "pointer to <Stype>".
*/

static void
gendata(NODE *n)
{
    NODE *var;
    SYMBOL *s;

    for (; n != NULL; n = n->Nright) {
	if (n->Nop != N_DATA) {
	    int_error("gendata: bad N_DATA %N", n);
	    break;
	}
	if ((var = n->Nleft) != NULL) {	/* For each item on N_DATA list */
	    TYPE *t;
	    if (var->Nop != N_IZ) {
		int_error("gendata: bad datum %N", n);
		break;
	    }
	    s = var->Nleft->Nid;		/* get symbol */
	    for (t = s->Stype; t->Tspec == TS_ARRAY; t = t->Tsubt)
		 ; /* Get to bottom of array */
	    if (!tisanyvolat(t) && tisconst(t))	/* If obj can be pure, */
		codeseg();			/* put it in pure code seg */
	    else dataseg();			/* Else ensure in data seg */
	    outmidef(s);			/* make label for variable */
	    giz(var->Nright, s->Stype, s);	/* do the initialization */
	}
    }
    gliterals();		/* Put literals into code (pure) segment */
}

/* GLITERALS - Emit all accumulated literals
**	Forces use of code segment as literals are expected to be pure,
**	although this is not mandatory.
*/
static void
gliterals(void)
{
    if (litstrings || litnodes) {		
	codeseg();
	flushcode();			/* Make sure all code forced out */
    }
    /* Do node literals first since they may generate more string literals! */
    while (litnodes != NULL) {
	outlab(litnodes->Nendlab);	/* Emit internal label */
	giz(litnodes->Nleft, litnodes->Nleft->Ntype, litnodes->Nendlab);
	freelabel(litnodes->Nendlab);
	litnodes = litnodes->Nright;
    }
    while (litstrings != NULL) {	/* Output literal strings */
	outlab(litstrings->Nsclab);	/* Emit generated label */
	freelabel(litstrings->Nsclab);	/* and then can free it. */
	outtab();			/* spaced out from string. */
	outscon(litstrings->Nsconst,	/* Output string literal, */
		    litstrings->Nsclen,	/* this long */
		    elembsize(litstrings->Ntype));	/* of this bytesize. */
	outnl();				/* End with final newline */
	litstrings = litstrings->Nscnext;	/* chain through list */
    }
}

/* GIZ - Generate initialization value for an object
*/
static void
giz(NODE *n, TYPE *t, SYMBOL *s)
{
    if (n == NULL) {
	giznull(t);	/* nothing there, just make block */
	return;
    }

    switch (t->Tspec) {
    case TS_ARRAY:
    case TS_STRUCT:
    case TS_UNION:
	gizlist(n, t, s);
	return;

    default:				/* initializing simple object */
	gizword(n, t, s);		/* make just one or two words */
	return;
    }
}

/* GIZWORD - emit initialization for a simple var (not array or structure)
**	This closely follows the nisconst() routine in CCDECL which
**	checked for legality while parsing.
*/
static void
gizword(NODE *n, TYPE *t, SYMBOL *s)
{
    if (n->Nop == N_IZLIST) {		/* something in brackets? */
	if (n->Nright != NULL)		/* no more than one allowed */
	    int_error("gizword: izer mismatch for %S %N", s, n);
	gizword(n->Nleft, t, s);	/* Just use inner part */
    } else
	if (!gizconst(n))		/* Try new stuff.  If not constant, */
	    gizexpr(n, t);		/* sigh, make at runtime. */
}

/* GIZCONST - Returns true if expression is an allowable initializer constant,
**	with appropriate code generated.  Otherwise, caller must generate.
*/
/* Return value indicates something about the type of constant: */
#define CT_NOTCON 0	/* not a constant, caller must generate. */
#define CT_CON	1	/* definitely a constant (arith, or a cast pointer) */
#define CT_ADDR	2	/* address of some kind */
#define CT_FUNC	3	/* function address (cannot add or sub from this) */

static struct pointerval {
	SYMBOL *pv_id;		/* Identifier (if any) */
	INT pv_off;		/* Offset from identifier (words or bytes) */
	int pv_bsize;		/* Byte size of pointer (0 = word) */
} pv;

static INT
gizconst(NODE *e)
{
    INT res;

    switch(e->Nop) {

	case N_ICONST:
	    if (tisbyte(e->Ntype)) {	/* Special handling for byte vals */
		res = e->Niconst & ((1<<tbitsize(e->Ntype))-1);	/* Mask off */
		res <<= (TGSIZ_WORD % tbitsize(e->Ntype));	/* Shift */
		outval(res);
		return CT_CON;
	    }
	    /* Normal word value, drop through */
	case N_PCONST:
#if __MSDOS__
	    if (e->Ntype->Tflag & TF_UNSIGN)
		unsign_int = 1;	   /* used in outnum() called by outval() */
	    outval(e->Niconst);		/* Just emit integer constant */
	    unsign_int = 0;
#else    
	    outval(e->Niconst);		/* Just emit integer constant */
#endif
	    return CT_CON;		/* Say simple constant generated */

	case N_FCONST:			/* Invoke rtn from CCOUT */
	    locctr += outflt(e->Ntype->Tspec, (INT *)&e->Nfconst, 0);
	    return CT_CON;		/* Say simple constant generated */


	/* Only the most likely cast conversions are supported here,
	** the others aren't common enough to be worth the
	** extra trouble.
	*/
	case N_CAST:
	    if (e->Ncast == CAST_NONE)
		return gizconst(e->Nleft); /*Most trivial cast just pass on */
	    else if (e->Ncast != CAST_PT_PT)
		return CT_NOTCON;	/* Not a constant */
	    /* Drop through to check for ptr (most likely cast) */

	default:
	    if (e->Ntype->Tspec == TS_PTR) {	/* Is this a pointer? */
		pv.pv_id = NULL;		/* Initialize arg struct */
		pv.pv_off = pv.pv_bsize = 0;
		if ((res = gizptr(e)) != 0) {	/* Fill in the struct */
		    /* Won, output pointer word. */
		    outtab();			/* Won, output it. */
		    outptr(pv.pv_id, pv.pv_bsize, pv.pv_off);
		    outnl();
		    ++locctr;
		    return res;
		}
	    }
    }
    return CT_NOTCON;		/* Must generate instructions */
}

/* GIZPTR - auxiliary for GIZCONST */
static int
gizptr(NODE *n)
{
    INT addoff, off;
    int i;
    TYPE *t;

    switch (n->Nop) {
	case N_CAST:
	    switch ((int) n->Ncast) {
		case CAST_PT_PT:	/* Only ptr-ptr supported */
		    i = gizptr(n->Nleft);	/* Get values for operand */
		    if (i == CT_FUNC		/* Function addr?  If so, */
		      && n->Ntype->Tspec == TS_PTR	/* and converting to */
		      && n->Ntype->Tsubt->Tspec == TS_FUNCT)	/* same, */
			return CT_FUNC;		/* No further conv needed! */

		    if (i != 2)		/* Only normal addrs allowed now */
			return CT_NOTCON;

		    /* First see whether a conversion is actually needed */
		    i = elembsize(n->Ntype);	/* Desired bytesize of ptr */
		    if (i == 0) {		/* Casting to (void *)? */
			if (tischarpointer(n->Nleft->Ntype)) /* from (char*)?*/
			    return CT_ADDR;	/* Yes, no change */
			i = TGSIZ_CHAR;		/* Else cvt to this bsize */
		    } else if (!elembsize(n->Nleft->Ntype)) { /* fm (void*)? */
			if (tischarpointer(n->Ntype))	      /* to (char*)?*/
			    return CT_ADDR;	/* Yes, no change */
		    }
		    if (i >= TGSIZ_WORD) i = 0;
		    if (i == pv.pv_bsize)	/* If already OK, */
			return CT_ADDR;		/* just return success */

		    /* Different sizes.  Check to see if boundaries match.
		    ** This takes care of 9<->18 bit conversions
		    ** (as well as any others)
		    */
		    if (i && pv.pv_bsize) {	/* Both are byte ptrs? */
			if (pv.pv_bsize < i && (i%pv.pv_bsize == 0)) {
			    pv.pv_off /= (i/pv.pv_bsize);
			    pv.pv_bsize = i;
			    return CT_ADDR;
			}
			if (i < pv.pv_bsize && (pv.pv_bsize%i == 0)) {
			    pv.pv_off *= (pv.pv_bsize/i);
			    pv.pv_bsize = i;
			    return CT_ADDR;
			}
		    }

		    /* Odd sizes.  First must always cvt to a word pointer */
		    if (pv.pv_bsize) {
			pv.pv_off /= (TGSIZ_WORD/pv.pv_bsize);
			pv.pv_bsize = 0;
		    }
		    /* Casting to byte ptr of some kind? */
		    if (i && (i < TGSIZ_WORD)) {
			pv.pv_off *= (TGSIZ_WORD/i);
			pv.pv_bsize = i;
		    }
		    return CT_ADDR;

		default:		/* Only ptr-to-ptr supported for now */
		    break;
	    }
	    return CT_NOTCON;


	case N_SCONST:
	    pv.pv_id = n->Nsclab = newlabel();	/* Get fwd lab for later use */
	    n->Nscnext = litstrings;	/* Link on string stack */
	    litstrings = n;		/* Now on stack */
	    pv.pv_bsize = elembsize(n->Ntype);	/* Set bsize */
	    return CT_ADDR;			/* Say address generated */

	case Q_IDENT:
		/* Identifier.  See documentation for Q_IDENT in cctoks.h
		** for explanation of this method of testing.
		*/
	    pv.pv_id = n->Nid;			/* Remember it */
	    switch (n->Nid->Stype->Tspec) {
		case TS_FUNCT:			/* Function address */
		    return CT_FUNC;		/* Say function address */
		case TS_ARRAY:			/* Array address */
		    if (tisbytearray(n->Nid->Stype))	/* If byte array, */
			pv.pv_bsize = elembsize(n->Nid->Stype);	/* set size */
		    return CT_ADDR;		/* Say array address */
	        default:
	            ;	/* do nothing */
	    }
	    return CT_NOTCON;			/* Barf */

	case N_ADDR:
	    switch (n->Nleft->Nop) {
		case N_PTR:			/* &(*()) is no-op */
		    return gizptr(n->Nleft->Nleft);

#if 0
		/* Allow for conversion of arrays generated by subscripting */
		case Q_PLUS:
		    if (n->Nleft->Ntype->Tspec == TS_ARRAY)
			return gizptr(n->Nleft);	/* OK, continue */
		    return CT_NOTCON;			/* Not array, fail */
#endif

		/* Structure hair.
		** For MEMBER (->) the Nleft must be a constant address.
		**	Can just apply nisconst to this.
		** For DOT (.) the Nleft can be anything that evaluates into
		**	a static structure.  We assume this is only possible
		**	with either Q_IDENT, or N_PTR of a struct addr.
		*/
		case Q_DOT:
		    if (tisbitf(n->Nleft->Ntype))	/* No bitfield ptrs */
			return CT_NOTCON;
		    switch (n->Nleft->Nleft->Nop) {
			case Q_IDENT:
			    switch (n->Nleft->Nleft->Nid->Sclass) {
				case SC_XEXTREF: case SC_EXLINK:
				case SC_EXTDEF: case SC_EXTREF:
				case SC_INTDEF: case SC_INTREF:
				case SC_INLINK: case SC_ISTATIC:
				    pv.pv_id = n->Nleft->Nleft->Nid;
				    goto dostruct; /* Good address of object */
				default:
				    ;	/* do nothing */
			    }
			    break;
			case N_PTR:
			    if (gizptr(n->Nleft->Nleft->Nleft) == CT_ADDR)
				goto dostruct;
			    break;
			default:
			    ;	/* do nothing */
		    }
		    return CT_NOTCON;			/* Otherwise fail. */

		case Q_MEMBER:
		    if (tisbitf(n->Nleft->Ntype)	/* No bitfield ptrs */
		      || gizptr(n->Nleft->Nleft) != CT_ADDR)
			return CT_NOTCON;
		dostruct:
		    /* If struct addr is OK, then we're OK */
		    if (pv.pv_bsize)	/* Structaddr never a byteptr */
			return CT_NOTCON;
		    if ((off = n->Nleft->Nxoff) < 0) {	/* Byte object? */
    /* Bug fix 'off' to '-off' by TEA, KAR 12/90 see fldsize() in ccdecl.c */
			pv.pv_bsize = (int) -off & 077;	/* Get byte size */
			pv.pv_off += (-off >> 12);	/* Add wd offset */
			pv.pv_off *= TGSIZ_WORD/pv.pv_bsize;
			pv.pv_off += (((-off)>>6)&077) / pv.pv_bsize;
		    } else if (tisbytearray(n->Nleft->Ntype)) {
			pv.pv_bsize = elembsize(n->Nleft->Ntype);
			pv.pv_off += off;	/* # of words offset */
			pv.pv_off *= TGSIZ_WORD/pv.pv_bsize;

		    } else {
			pv.pv_off += off;	/* # of words offset */
		    }
		    return CT_ADDR;

		case Q_IDENT:	/* Addr OK if of external or static */
			/* Needn't test type since parser checks it while
			** parsing "&" to verify not function or array.
			*/
		    switch (n->Nleft->Nid->Sclass) {
			case SC_XEXTREF: case SC_EXLINK:
			case SC_EXTDEF: case SC_EXTREF:
			case SC_INTDEF: case SC_INTREF:
			case SC_INLINK: case SC_ISTATIC:
			    pv.pv_id = n->Nleft->Nid;	/* Remember ident */
			    if (tisbyte(n->Nleft->Ntype)) {
				/* Single bytes are right-justified */
				pv.pv_bsize = (int) tbitsize(n->Nleft->Ntype);
				pv.pv_off = (TGSIZ_WORD/pv.pv_bsize) - 1;
			    }
			    return CT_ADDR;	/* Good address of object */
		    default:
			;	/* do nothing */
		    }
		    return CT_NOTCON;		/* Bad storage class */
	    default:
	        ;	/* do nothing */
	    }
	    return CT_NOTCON;			/* Bad use of & */

	/* Non-atomic expression checks, for plus and minus. */
	case Q_PLUS:
	    if (n->Nleft->Nop == N_ICONST		/* Integ constant */
		&& gizptr(n->Nright) == CT_ADDR) {	/* + address */
		    addoff = n->Nleft->Niconst;
		    t = n->Nright->Ntype;		/* Ptr has this type */
	     } else if (n->Nright->Nop == N_ICONST	/* Integ constant */
		&& gizptr(n->Nleft) == CT_ADDR) {	/* Address */
		    addoff = n->Nright->Niconst;
		    t = n->Nleft->Ntype;
	    } else return CT_NOTCON;

	    /* See comments for sizeptobj in CCSYM.  Only reason code is
	    ** duplicated here is to handle funny byte sizes right.  Puke!
	    */
	doadd:
#if 1
	    if (tisbytepointer(t)) {
		if (!pv.pv_bsize) pv.pv_bsize = elembsize(t);
		addoff *= sizearray(t->Tsubt);	/* Mult by # bytes in obj */
	    } else
		 addoff *= sizetype(t->Tsubt);	/* Mult by obj size in wds */
#else /* Old buggy code */
	    addoff *= sizetype(t->Tsubt);	/* Mult by obj size in wds */
	    if (tisbytepointer(t) && !tisbyte(t->Tsubt)) {
		if (!pv.pv_bsize) pv.pv_bsize = elembsize(t);
		addoff *= (TGSIZ_WORD / pv.pv_bsize);
	    }
#endif
	    pv.pv_off += addoff;
	    return CT_ADDR;

	case Q_MINUS:
	    if (n->Nright->Nop == N_ICONST	/* minus integ constant */
		&& gizptr(n->Nleft) == CT_ADDR) {	/* Address */
		addoff = - n->Nright->Niconst;
		t = n->Nleft->Ntype;
		goto doadd;
	    }
	    break;

	default:		/* Anything else just fails */
	    break;
    }
    return CT_NOTCON;
}

/* GIZNULL - Initialize an object to nothing.
*/
static void
giznull(TYPE *t)
{
    INT i;

    if ((i = sizetype(t)) <= 0)
#if SYS_CSI		/* 9/91, detect int a[]; that's never completed */
        if (i == 0)
            error("Missing size for definition of global non-external array");
        else
#endif
	    int_error("giznull: Bad BLOCK: %ld", (INT) i);
    else outzwds(i);
}

/* GIZEXPR - Generate code to initialize a static object at runtime.
**	Normally this should never be needed, but the capability is
**	kept here in case the need for cross-compiling ever comes up.
** Note: This will not work for initializing code-segment objects
** that are part of a larger object.  To work right, the init code gen
** needs to be deferred until the top-level object is done.  Don't bother
** fixing this unless it turns out we someday need it.
*/
static void
gizexpr(NODE *n, TYPE *t)
{
    static SYMBOL s;			/* Static to avoid re-initialization */
    SYMBOL *lnk;
    extern NODE *ndef(), *ndefop();
    int oseg;

    s.Sclass = SC_ISTATIC;		/* Set up temp sym for loc to init */
    s.Stype = t;			/* Type for gaddress() & ndefident() */
    s.Ssym = newlabel();		/* Get an internal sym */
    outlab(s.Ssym);			/* and emit it directly */
    strcpy(s.Sname, s.Ssym->Sname);	/* In case of debugging, copy name */
    giznull(t);				/* Emit space for the stuff to init */

    oseg = codeseg();			/* Switch to code segment */
    inicode();				/* Initialize for code generation */
    lnk = newlabel();			/* Get a label for linkage */
    outlab(lnk);			/* and emit it directly */
    outstr("\tBLOCK\t1\n");		/* Make space for linkage */

    /* Fake up an assignment expression setting this symbol */
    n = ndef(Q_ASGN, t, 0, ndefident(&s), n);	/* Use temp for Q_IDENT sym */
    genxrelease(n);			/* Generate code for assignment */

    code6(P_SKIP+POF_ISSKIP+POS_SKPE, VR_RETVAL, lnk); /* see if more inits */
    codemdx(P_JRST, 0, NULL, 1, R_RETVAL);	/* yes, chain to the next */
    code5(P_POPJ, VR_SP);			/* no, back to runtime init */
    endcode();				/* emit literals if any */

    outstr("\t.LINK\t1,");		/* start making link pseudo-op */
    outmiref(lnk);			/* linking through top of routine */
    outnl();				/* finish it off */
    prevseg(oseg);			/* back to previous segment */

    freelabel(s.Ssym);			/* no longer need labels */
    freelabel(lnk);			/* so give them back to freelist */
}

/* GIZLIST - initialize static (not auto) array/struct/union from list
*/
static void
gizlist(NODE *n, TYPE *t, SYMBOL *s)	/* N_IZLIST to initialize from */
{
    SYMBOL *sm;
    INT nelts, elwds;
    INT wdsleft;
    INT savloc;

    if ((wdsleft = sizetype(t)) <= 0) {		/* Paranoia */
	int_error("gizlist: bad size: %ld %N", (INT) wdsleft, n);
	return;					/* Don't try to fill out */
    }
    if (n->Nop != N_IZLIST) {			/* More paranoia */
	int_error("gizlist: not N_IZLIST %N", n);
	gizword(n, t, s);			/* Emit object expr anyway */
	return;					/* Nothing left on list */
    }

    switch (t->Tspec) {
    case TS_ARRAY:
	if (tisbytearray(t)) {		/* Array of bytes? */
	    gizbytes(n, t, s, 0);	/* Yep, go handle top-lev bytearray */
	    return;			/* Nothing left */
	}
	nelts = t->Tsize;		/* Get # elements in array */
	t = t->Tsubt;			/* Use member type from now on */
	elwds = sizetype(t);		/* Find # wds per element */
	for (; n && --nelts >= 0; n = n->Nright, wdsleft -= elwds)
	    giz(n->Nleft, t, s);	/* Initialize the element */
	break;

    case TS_UNION:
	if (n->Nright) {	/* Union izer should have only 1 element! */
	    int_error("gizlist: > 1 union izer %N", n);
	    n->Nright = NULL;	/* Merciless clobberage to recover */
	}
	/* Then drop thru to handle exactly like struct! */
    case TS_STRUCT:
	sm = t->Tsmtag->Ssmnext;	/* Struct & union have tag */
	savloc = locctr;		/* Remember current loc ctr */
	for (; n && sm; n = n->Nright, sm = sm->Ssmnext) {
	    INT w, o, woff;
	    int p, s, gap;

	    /* First ensure ready to emit right word for this object */
	    if ((o = sm->Ssmoff) < 0) {	/* Byte or bitf object? */
		w = (-o) >> 12;		/* Decode word offset */
		p = (int) (((-o)&07700) >> 6);/* Byte pos within word, in bits */
		s = (int) ((-o) & 077);		/* Size of object, in bits */
	    } else bytend(), w = o;	/* Word object, leave byte mode */
	    woff = locctr - savloc;	/* Find current offset */
	    if (w != woff) {
		bytend();		/* Align again in case byte mode */
		woff = locctr - savloc;	/* Current offset may have changed */
		if (woff > w)		/* Offset mustn't go backwards!!! */
		    int_error("gizlist: offset clash for %S", sm);
		else outzwds(w - woff);
	    }
	    /* Right word offset, now see if word or byte/bit object */
	    if (o >= 0) {		/* If word object, */
		giz(n->Nleft, sm->Stype, sm);	/* Simply initialize it */
		continue;
	    }

	    /* Handle bitfield (or byte) objects differently from word objs */
	    if (!bsiz) bytbeg(1);	/* Ensure in byte mode */
	    gap = bpos - (p + s);	/* Get space between */
	    if (gap) {
		if (gap < 0) int_error("gizlist: -gap for %S", sm);
		else outbyte(0L, gap);	/* Space out to right place */
	    }
	    if (tisbytearray(sm->Stype))
		gizbytes(n->Nleft, sm->Stype, sm, 1);
	    else {
		if (n->Nleft->Nop != N_ICONST) /* not const? */
		    int_error("gizlist: bitf izer not iconst %N", n);
		outbyte(n->Nleft->Niconst, s);
	    }
	}
	bytend();			/* Done, ensure out of byte mode */
	wdsleft -= (locctr - savloc);	/* Find # words left if any */
	break;

    default:
	int_error("gizlist: bad izer type: %d %N", t->Tspec, n);
	return;
    }

    /*
    ** Fill out remains of initializer.
    **
    ** We might have run off the end of our initializer before coming to
    ** the end of the array or structure we were initializing.  In that
    ** case, we are supposed to fill the rest with zeros; this is done
    ** by counting how much space we have and making a BLOCK that long.
    */
    if (n || wdsleft < 0) {
	int_error("gizlist: too many izers (wlft: %ld) %N", (INT) wdsleft, n);
	return;
    }
    if (wdsleft)
	outzwds(wdsleft);
}

/* GIZBYTES - Initialize byte array
**	May already be in byte mode.
*/
static void
gizbytes(NODE *izl, TYPE *t, SYMBOL *s, int lev) /* Level (0 is top level) */
{
    register NODE *n = izl;
    int savmode = bsiz;		/* Remember initial mode */
    INT nbs = sizearray(t);	/* # of bottom elements (bytes) in array */
    INT i;
    char *cp;

    if (n->Nop != N_IZLIST) {
	int_error("gizbytes: izer not list %N", n);
	return;
    }
    if (lev == 0)
	bytbeg(elembsize(t));		/* Get into byte mode with this size */
    for (n = izl; n; n = n->Nright) {
	switch (n->Nleft->Nop) {
	    case N_ICONST:		/* Single byte */
		outval(n->Nleft->Niconst);
		nbs--;			/* count off */
		break;
	    case N_SCONST:		/* String literal */
		if (izl != n || n->Nright) {	/* Must be only thing! */
		    int_error("gizbytes: str not sole node %N", n);
		}
		cp = n->Nleft->Nsconst;
		i = nbs > n->Nleft->Nsclen ? n->Nleft->Nsclen : nbs;
		nbs -= i;
		if (i > 0) do {
		    if (bsiz == 6) outval((long)tosixbit(*cp));
		    else outval((long)*cp);
		    ++cp;
		} while (--i > 0);
		break;
	    case N_IZLIST:		/* Subarray */
		gizbytes(n->Nleft, t->Tsubt,s, lev+1);	/* Do recursively */
		nbs -= sizearray(t->Tsubt);	/* Done with subarray bytes */
		break;
	    default:
		int_error("gizbytes: bad izer for %S %N", s, n);
	}
    }

    /*
    ** Initialization done, fill out rest of array.
    ** Our array might be a subarray of some other char array,
    ** so we must be prepared to leave a ragged end.
    */
    if (nbs > 0)		/* Not enough elements? */
	outzbs(nbs);		/* Fill up this many zero bytes */
    else if (nbs < 0)
	int_error("gizbytes: too many izers, %S", s);

    if (lev == 0 && !savmode)	/* If at top level, leave byte mode now */
	bytend();
    return;
}

/* OUT Data emission stuff.  Tracks whether in byte or word mode,
**	plus count of # words emitted so far.
*/

/* BYTBEG - Initializes to output bytes of given size.
**	If already in byte mode, does nothing except change bytesize.
*/
static void
bytbeg(int siz)
{
    if (!bsiz) {
	bpos = TGSIZ_WORD;
	bpw = TGSIZ_WORD/siz;
	savlct = locctr;
    }
    bsiz = siz;
}

/* BYTEND - Leaves byte mode, returns # words output
**	since byte mode was entered (0 if never entered)
*/
static INT
bytend(void)
{
    if (!bsiz) return 0;
    wdalign();			/* Force output to word boundary */
    bsiz = 0;			/* Leave byte mode */
    return locctr - savlct;	/* And return # words done so far */
}

/* WDALIGN - Aligns output to word boundary, doesn't change mode.
*/
static void
wdalign(void)
{
    if (bsiz && bpos != TGSIZ_WORD) {
	outnl();		/* Force out current word */
	++locctr;		/* and account for it */
	bpos = TGSIZ_WORD;	/* Now at start of new word */
    }
}

/* OUTVAL - Output value in either byte or word mode.
*/
static void
outval(long v)
{
    if (!bsiz) {
	outtab();
	outnum(v);
	outnl();
	++locctr;
    } else outbyte(v, bsiz);
}

/* OUTBYTE - like OUTVAL but byte size is specified (changes default).
**	Must already be in byte mode.
*/
static void
outbyte(long v, int siz)
{
    v &= ((unsigned long)1 << siz) - 1;	/* Ensure value masked off */
    if (bpos < siz)
	wdalign();		/* If not enough room, get new wd */
    if (bpos == TGSIZ_WORD)	/* If at start of word, */
	fprintf(out, "\tBYTE (%d) %lo", siz, v);	/* do specially */
    else if (siz == bsiz)		/* can skip size if no change */
	fprintf(out, ",%lo", v);
    else {
	fprintf(out, " (%d) %lo", siz, v);	/* Else just output it */
	bsiz = siz;
    }
    bpos -= siz;
}

#if 0	/* 5/91 KCC size */
/* OUTZVALS - Output zero values to fill up space.
*/
static void
outzvals(INT zeros)
{
    if (bsiz) outzbs(zeros);
    else outzwds(zeros);
}
#endif

/* OUTZBS - Output zero bytes to fill up space.
*/
static void
outzbs(INT zeros)
{
    if (zeros <= 0) return;
    while (bpos != TGSIZ_WORD && bpos >= bsiz && --zeros >= 0)
	outval(0L);		/* Add filler til at word boundary */
    if (zeros >= (bpw = (TGSIZ_WORD/bsiz))) {	/* Full words left? */
	wdalign();			/* Ensure properly aligned */
	outzwds(zeros/bpw);		/* Zap those words */
	zeros %= bpw;			/* Get remaining # bytes */
    }
    while (--zeros >= 0)		/* Finish off */
	outval(0L);
}

/* OUTZWDS - Output zero words to fill up space.
*/
static void
outzwds(INT nwds)
{
    if (nwds > 0) {
	fprintf(out, "\tBLOCK %lo\n", (INT) nwds); /* This many zero wds */
	locctr += nwds;
    }
}
