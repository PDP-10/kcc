/*	CCGEN2.C - Generate code for parse-tree expressions
**
**	(c) Copyright Ken Harrenstien 1989
**		All changes after v.295, 14-May-1989
**	(c) Copyright Ken Harrenstien, SRI International 1985, 1986
**		All changes after v.79, 8-Aug-1985
**
**	Original version (C) 1981  K. Chen
*/
#define NEWTERN 1	/* Try new ternary code */

#include "cc.h"
#include "ccgen.h"
#include <string.h>

/* Imported functions */
extern SYMBOL *newlabel(void);		/* CCSYM */
extern int elembsize(TYPE *);			/* CCSYM */
extern INT sizetype(TYPE *), sizeptobj(TYPE *);
extern void				/* CCCODE */
	codek0(int, VREG *, VREG *), codek4(int, VREG *, VREG *),
	code4s(int, VREG *, VREG *, int, INT),
	code0(int, VREG *, VREG *), code1(int, VREG *, INT),
	codebp(int, int, INT, int, SYMBOL *, INT),
	code3(int, VREG *, SYMBOL *), code4(int, VREG *, VREG *),
	code5(int, VREG *), code6(int, VREG *, SYMBOL *),
	codemdx(int, int, SYMBOL *, INT, int), code8(int, VREG *, INT),
	code9(int, VREG *, double, int),
	code10(int, VREG *, SYMBOL *, INT, INT), code13(int, VREG *, INT),
	codestr(char *, int), codlabel(SYMBOL *), fixprev(void),
	flushcode(void);
extern void code4m(int, VREG *, VREG *, char *), code5m(int, VREG *, char *);
extern void code00(int, int, int), code40(int,int,int,INT); /* Reg linkage */
extern int codcreg(VREG *, VREG *);		/* CCCODE */
extern int sideffp(NODE *);		/* CCEVAL */
extern int vrispair(VREG *);
extern void folddiv(VREG *);
extern int unjump(SYMBOL *);
extern SYMBOL *cregupto(SYMBOL *);	/* CCCREG for gternary() */
extern PCODE *before(PCODE *);
extern VREG *vrdget(void);
extern void vrfree (VREG *);
extern VREG *vrget(void);
extern int vrispair(VREG *);
extern void vrnarrow (VREG *);
extern int vrreal (VREG *);
extern int vrstoreal (VREG *, VREG *);
extern int vrtoreal(VREG *);
extern void vrufcreg(VREG *);
extern void vralspill(void);
extern void vrlowiden (VREG *);
extern VREG *vrretdget(void);
extern VREG *vrretget(void);
extern void vrallspill(void);	/* VERY non-optimal!! */

/* Exported functions */
VREG *genexpr(NODE *);
void genxrelease(NODE *);
void relflush(VREG *reg);		/* Maybe move to CCOPT */
void gboolean(NODE *n, SYMBOL *false, int reverse);
VREG *getmem(VREG *reg, TYPE *t, int byte, int keep),
  *stomem(VREG *reg, VREG *ra, INT siz, int byteptr); /* CCGEN1 auto inits */
VREG *gmuuo(NODE *);

/* Internal functions */
static VREG *gexpr(NODE *),
	*gternary(NODE *);
static void gor(NODE *, SYMBOL *, int),
	gand(NODE *, SYMBOL *, int),
	gboolop(NODE *, int);
static VREG *gassign(NODE *),
	*gbinary(NODE *),
	*garithop(int, VREG *, VREG *, int),
	*gptrop(int, VREG *, VREG *, TYPE *, TYPE *),
	*gptraddend(TYPE *, NODE *),
	*glogical(NODE *),
	*gunary(NODE *),
	*gcast(NODE *),
	*gcastr(int, VREG *, TYPE *, TYPE *, NODE *),
	*gintwiden(VREG *, TYPE *, TYPE *, NODE *),
	*guintwiden(VREG *, int, NODE *),
	*gincdec(NODE *, int, int),
	*gprimary(NODE *),
	*gcall(NODE *);
static void emit_blissargs(NODE *);
static INT sizeargs(NODE *);
static void gfnarg(NODE *);
static VREG *gaddress(NODE *);
static void pitopc(VREG *, int, int, int);
static  int bptrref(NODE *);
static void gasm(NODE *);
#if 0
static VREG *rgetmem(VREG *reg, TYPE *t, int byte, int keep);
#else
static VREG *rgetmem(VREG *reg, TYPE *t, int keep);
#endif
#if 0
static VREG* rstomem(VREG *reg, int ra, INT siz, int byteptr);
static VREG *gexpr(), *gternary();
static void gor(), gand(), gboolop();
static VREG *gassign(), *gbinary(), *garithop(), *gptrop(), *gptraddend(),
       *glogical(), *gunary(), *gcast(), *gcastr(),
       *gintwiden(), *guintwiden(), *gincdec(),  *gprimary(),  *gcall();
static void emit_blissargs();
static INT sizeargs();
static void gfnarg();
static VREG *gaddress();
static void pitopc();
static int bptrref();
static void gasm();
#endif

/* GENEXPR - Main function for expression code generation.
**	Argument is pointer to a parse-tree node expression;
**	Result is pointer to a virtual register.
** NOTE: the result may be NULL if the expression was marked
**	to be discarded, or was cast to (void), or an error
**	was encountered.
*/
VREG *
genexpr(NODE *n)
{
    if (!n)
	return NULL;		/* Check for null exprs here */
    if (n->Nflag & NF_DISCARD)	/* If discarding result, */
	{
	relflush(gexpr(n));		/* flush any resulting register(s) */
	return NULL;
	}
    return gexpr(n);		/* Normal case, generate code & return reg */
}

/* GENXRELEASE - Auxiliary like genexpr but called when we want to make
**	sure that the resulting value is forced to be discarded.
*/
void
genxrelease(NODE *n)
{
    n->Nflag |= NF_DISCARD;	/* Will be discarding this value */
    genexpr(n);
}

/* GEXPR - workhorse routine for genexpr().  Note arg is guaranteed non-null.
*/
static VREG *
gexpr(NODE *n)
{
    switch (tok[n->Nop].tktype)
	{
	case TKTY_ASOP:
	    return gassign(n);
	case TKTY_TERNARY:
	    return gternary(n);
	case TKTY_BINOP:
	    return gbinary(n);
	case TKTY_BOOLOP:
	case TKTY_BOOLUN:
	    return glogical(n);
	case TKTY_UNOP:
	    return gunary(n);
	case TKTY_RWOP:				/* For now, RW's go below */
	case TKTY_PRIMARY:
	    return gprimary(n);
	case TKTY_SEQ:				/* Comma operator */
	    if (n->Nleft)
		genxrelease(n->Nleft);	/* Flush result of left */
	    return genexpr(n->Nright);	/* and return that of right */
	}

    int_error("gexpr: bad op %N", n);
    return NULL;
}

/*
** RELFLUSH - Flush no-longer-wanted register value
**	Mainly called by genexpr(); also called by gcastr() when casting
** a value to (void).
**	Note that the reg argument may be NULL.  This can happen for
** a generated value of type "void" (size 0).
*/
void
relflush (VREG *reg)
{
    int r;			/* get physical register */
    PCODE *p, *before();

    if (reg == NULL)
	return;
    r = vrtoreal(reg);		/* Save physical reg # */
    vrfree(reg);		/* Now release virtual reg or reg pair */

    /* This should be moved to someplace in CCOPT */
    if (optobj)
	for (p = previous; p != NULL; p = before (p))
	    {
	    if (p->Pop == P_ADJSP)
		continue; /* skip back across P_ADJSP */
	    else if (p->Pop != P_MOVE
		     || p->Preg != r
		     || prevskips(p))
		break; /* not flushable */
	    else 
	/* Do NOT discard preserved reg values.
	 * Later, add optimization at end of basic block to do it.
	 */ if (Register_Nopreserve(p->Preg))
		{
		p->Pop = P_NOP;		/* drop pointless NOP */
		fixprev();			/* fix up for drop */
		}
	    }
}

/* GTERNARY - generate code for ternary operators
**	Note: handles case where one or both result expression pointers may be
** NULL.  The overall value had better be "void" if so.
**
** There is some suboptimal code here, namely the call to "vrallspill()" to
** save any registers that are active at the time this operand is executed.
** This is necessary because we don't know at this point whether either the
** true or false path will require saving registers (eg if a function call is
** done), but control still has to merge back to the same place.  If one
** path saves regs and the other doesn't, the stack is going to be confused
** at the point where control merges (ie at end of ternary expression).
** The active registers have to be in the same state afterwards as they
** were before, and at the moment the only safe way of doing this is to
** bite the bullet and save them all prior to doing the ternary expression.
**	A similar problem exists for the logical operands; anything that
** branches during expression evaluation.  Conditional statements like "if"
** are not affected because there are never any registers active across a
** statement.
*/

static VREG *
gternary (NODE *n)
{
    SYMBOL *false, *done;
    int siz;
    NODE *nfirst, *nsecond;
    VREG *reg;
#if NEWTERN
    int uptof = 0;
    SYMBOL *savupto;
#endif

    /* find the pieces of code we're going to use */
    siz = sizetype(n->Ntype);		/* Find size of overall result */
    if (n->Nflag & NF_DISCARD)		/* If result being discarded, */
	siz = 0;			/* pretend size is 0 (void) */
    nfirst = n->Nright->Nleft;		/* First (if-true) result expr */
    nsecond = n->Nright->Nright;	/* Second (if-false) result expr */

    /* Check for (very unlikely) case of both results non-existent.
    ** CCEVAL's evaldiscard() should have substituted some other node op, 
    ** so this may be a bug.  We handle this mainly to ensure following
    ** code is guaranteed of having at least one result expr.
    */
    if (!nfirst && !nsecond)		/* Check for unlikely case */
	{
	if (siz != 0)			/* Overall result must be void! */
	    int_error("gternary: no operands %N", n);
	genxrelease(n->Nleft);		/* Generate condition */
	return NULL;
	}
    if (n->Ntype->Tspec == TS_ARRAY)	/* Another just-in-case check */
	{
	int_error("gternary: array type %N", n);
	siz = 0;
	}

#if 1
    /* Clean up previously allocated registers */
    if (siz == 2)
	vrfree(vrretdget());		/* Make sure ACs 1 & 2 free */
    else if (siz >= 1)
	vrfree(vrretget());	/* else just ensure AC1 free */
    /* Else void return value */
#endif

    false = (nfirst && nsecond)			/* If we'll need it, */
		 ? newlabel() : NULL;		/* get a new label for false */
    done = (n->Nendlab ?			/* Get overall end label */
		n->Nendlab : newlabel());
    if (nfirst)
	nfirst->Nendlab = done;		/* Make that be expr endlab */
    if (nsecond)
	nsecond->Nendlab = done;
    if (!false)				/* If don't have both exprs */
	false = done;			/* then use endlab as false jump */

    /* Now just before generating the code to test a condition and branch,
    ** we have to ensure that any active registers are saved.  See note at
    ** top of page.
    */
    vrallspill();

    /* There are three possible configurations:
    ** (1) Both nfirst and nsecond exist.  Failing test jumps to "false".
    **		Both results must be moved to a common register.
    ** (2) Only nfirst exists.  Failing test jumps to "done".
    **		We return the register nfirst gives, if any.
    ** (3) Only nsecond exists.  Test is REVERSED; if fails, jumps to "done".
    **		We return the register nsecond gives, if any.
    ** We've already set up the "false" label to be the same as "done"
    ** if either of the latter two cases holds.
    */
    reg = NULL;			/* Ensure no return reg initially */
    gboolean(n->Nleft, false,		/* Generate code to test condition */
		nfirst == NULL);	/* (reverse sense if 1st is gone) */

    if (nfirst)
	{
	if (siz > 0)
	    {
#if 0 /*NEWTERN*/
	    reg = genexpr(nfirst);	/* Just return what we get */
	    if (optgen && nsecond		/* If optimizing, and other */
	     && (nsecond->Nop == N_FNCALL	/* val is a function call, */
		|| (nsecond->Nop == N_CAST &&
		    nsecond->Nleft->Nop == N_FNCALL))
	     && vrtoreal(reg) != R_RETVAL)	/* and 1st val in diff reg, */
		{
		code0(siz == 2 ? P_DMOVE : P_MOVE,	/* then put 1st val */
				VR_RETVAL, reg);	/* into this reg! */
		reg = (siz == 2 ? vrdget() : vrget());
		}
	    else if (optgen)		/* One more optimization try */
		backreg(reg);		/* Flush a MOVE R,S as we don't care */
					/* at this point what phys reg is */
#else
	    if (Register_Id(nfirst))
		code00(P_MOVE, R_RETVAL, nfirst->Nid->Sreg);
	    else
		code0(siz == 2 ? P_DMOVE : P_MOVE, VR_RETVAL, genexpr(nfirst));
#endif
	    }
	else
	    genxrelease(nfirst);
	if (nsecond)
	    {
	    code6(P_JRST, (VREG *)NULL, done);	/* skip over the hard way */
	    codlabel(false);			/* now start second part */
	    }
	}

    if (nsecond)
	{
	if (siz > 0)
	    {
#if NEWTERN
	    if (nfirst)
		{
		savupto = cregupto(done);	/* Set fence for changereg */
		uptof++;			/* say fence set */
		}
	    if (Register_Id(nsecond))
		code00(P_MOVE, R_RETVAL, nsecond->Nid->Sreg);
	    else
		code0 (siz == 2 ? P_DMOVE : P_MOVE, VR_RETVAL, genexpr(nsecond));
#else
	    code0 (siz == 2 ? P_DMOVE : P_MOVE, VR_RETVAL, genexpr(nsecond));
#endif
	    }
	else
	    genxrelease(nsecond);
	}

    if (n->Nendlab == NULL)
	{
	codlabel(done);		/* second clause done here */
#if NEWTERN
	if (uptof)
	    {
	    cregupto(savupto);	/* Restore saved fence value */
	    uptof = 0;
	    }
#if 1
	/* If only one result register was used, try to make it a "normal"
	** (non-return) reg to avoid hogging reg 1 and interfering with
	** common sub-expression matching.
	*/
	if (siz > 0 && siz != 2		/* Only one register? */
		 && optobj)
	    {
	    reg = vrget();		/* Get normal reg */
	    reg->Vrtype = n->Ntype;	/* Set C type of result */
	    if (codcreg(reg, VR_RETVAL))
		return reg;
	    vrfree(reg);		/* didn't work, put back that reg. */
	    }
#endif
#endif
	}

#if NEWTERN
    if (uptof)
	cregupto(savupto);	/* Restore saved fence if one */
#if 0
    if (reg)
#else
    if (siz <= 0)
	return NULL;		/* Void */
    reg = (siz == 2 ? vrretdget() : vrretget());	/* One or two return regs */
#endif
#endif
    reg->Vrtype = n->Ntype;		/* Set C type of result obj */
    return reg;
}

/* GBOOLEAN - Generate code for boolean expressions
**	jump to false label if expr not true
**	reverse sense of test if reverse bit set
*/
void
gboolean(NODE *n, SYMBOL *false, int reverse)
{
    VREG *r;
    int op;

    if (n == NULL)		/* Paranoia: bug catcher */
	{
	int_error("gboolean: null arg");
	return;
	}

    /*
    ** The big switch.  Either we call some handler routine such as
    ** gor or gand, or we make a skip and then a jump.  If the former,
    ** we are done, and the call should tail recurse.  Otherwise, we
    ** need to add the jump to the given label, so we break from the switch.
    */

    switch (n->Nop)
	{
	case Q_NOT:
	    n->Nleft->Nendlab = n->Nendlab;	/* set up variables */
	    n = n->Nleft;			/* with parity switched */
	    reverse = !reverse;		/* for tail recursive call */
	    gboolean(n, false, reverse);	/* to self */
	    return;

	case Q_LAND:
	    if (reverse)
		gor(n, false, reverse); /* more tail recursion */
	    else
		gand(n, false, reverse);
	    return;

	case Q_LOR:
	    if (reverse)
		gand(n, false, reverse); /* still more */
	    else
		gor(n, false, reverse);
	    return;

	case Q_NEQ:
	case Q_LEQ:
	case Q_GEQ:
	case Q_LESS:
	case Q_GREAT:
	case Q_EQUAL:
	    gboolop(n, reverse);		/* comparison, make skip */
	    break;				/* followed by GOTO */

	case N_ICONST:
	case N_PCONST:
	    op = n->Niconst;		/* unconditional condition */
	    if (reverse && op)
		break;	/* jump when true and true? */
	    if (!reverse && !op)
		break;	/* jump when false and false? */
	    return;

	default:
	    n->Nendlab = NULL;	/* cond endlab is not expr endlab */
	    if ((r = genexpr(n)) != NULL)	/* get expression into reg (may be discarded)*/
		{
		int bits = tbitsize(n->Ntype);	/* Find # bits of value */
		if (bits < TGSIZ_WORD)		/* If not full wd, then */
		    r = guintwiden(r, bits, n);	/* widen it unsignedly! */
		code6(reverse? P_JUMP+POS_SKPN : P_JUMP+POS_SKPE,
			    r, false);	/* test and jump */
		vrfree(r);			/* now done with register */
		}
	    return;				/* don't make spurious P_JRST */
	}
    code6(P_JRST, (VREG *)NULL, false);	/* broke out, want a GOTO */
}

/* GOR - Generate || expression
*/
static void
gor(NODE *n, SYMBOL *false, int reverse)
{
    SYMBOL *lab;

    if ((lab = n->Nendlab) == 0)
	lab = newlabel(); /* get label */
    gboolean(n->Nleft, lab, !reverse);	/* output first clause */
    n->Nright->Nendlab = lab;		/* no more labels in second clause */
    gboolean(n->Nright, false, reverse);
    if (n->Nendlab == 0)
	codlabel(lab); /* send out made label */
}

/* GAND - Generate && expression
*/
static void
gand(NODE *n, SYMBOL *false, int reverse)
{
    n->Nright->Nendlab = n->Nendlab;
    gboolean(n->Nleft, false, reverse);
    gboolean(n->Nright, false, reverse);
}

/* GBOOLOP - Generate code for == > < <= >= !=
**
*/
static void
gboolop(NODE *n, int reverse)
{
    int op;
    VREG *r1, *r2;

    /*
    ** Generate operands and skip instruction for the test
    **
    ** Note that floating point can use the same comparison
    ** instructions as integers, so we don't have to test for them.
    */

    switch (n->Nop)
	{
	case Q_EQUAL:
	    op = P_CAM+POF_ISSKIP+POS_SKPE;
	    break;
	case Q_NEQ:
	    op = P_CAM+POF_ISSKIP+POS_SKPN;
	    break;
	case Q_LEQ:
	    op = P_CAM+POF_ISSKIP+POS_SKPLE;
	    break;
	case Q_GEQ:
	    op = P_CAM+POF_ISSKIP+POS_SKPGE;
	    break;
	case Q_LESS:
	    op = P_CAM+POF_ISSKIP+POS_SKPL;
	    break;
	case Q_GREAT:
	    op = P_CAM+POF_ISSKIP+POS_SKPG;
	    break;
	}

    /* May need to munch on char pointers to get into comparable form */
    switch (n->Nop)
	{
	case Q_LEQ:
	case Q_GEQ:
	case Q_LESS:
	case Q_GREAT:
	    if (tisunsign(n->Nleft->Ntype))	/* If operands are unsigned */
		{
		r1 = genexpr(n->Nleft);		/* Get operand 1 */
		code8(P_TLC, r1, 0400000L);	/* and flip sign bit */
		r2 = genexpr(n->Nright);	/* Ditto for operand 2 */
		code8(P_TLC, r2, 0400000L);
		break;
		}
	    else if (tisbytepointer(n->Nleft->Ntype))
		{
		/* If operands are byte pointers */
		/* Note that:
		** OWGBPs can omit the SKIP+TLC, or use this:
		**	MOVE R,PTR1
		**	SUB R,PTR2
		**	ROT R,6
		**	CAIx R,0
		** Local-fmt BPs can use the sequence:
		**	MOVE R,PTR1
		**	MOVE R+1,PTR2	; Needs double reg
		**	ROTC R,6	; Yes this really works!
		**	CAMx R,R+1
		*/
		r1 = genexpr(n->Nleft);		/* Get operand 1 */
		code0(P_SKIP+POF_ISSKIP+POS_SKPL, r1, r1);
		code8(P_TLC, r1, 0770000L);	/* Zap P bits if local */
		code8(P_ROT, r1, 6);		/* Get P or PS into low bits */

		/* Repeat for 2nd operand */
		r2 = genexpr(n->Nright);	/* Get operand 2 */
		code0(P_SKIP+POF_ISSKIP+POS_SKPL, r2, r2);
		code8(P_TLC, r2, 0770000L);	/* Zap P bits if local */
		code8(P_ROT, r2, 6);		/* Get P or PS into low bits */

		/* Now can compare the registers with normal CAM! */
		break;
		}
	    /* Else just fall through for normal expression evaluation */

	case Q_EQUAL:
	case Q_NEQ:
	    r1 = genexpr (n->Nleft);	/* calculate values to compare */
	    r2 = genexpr (n->Nright);
	    break;
	}

    if (reverse)
	op = revop (op);	/* maybe invert test */

    /*
    ** Generate and optimize the test.
    **
    ** If we are comparing double precision floating point we need
    ** to look at both pairs of words, so we use a cascaded pair or
    ** trio of comparisons.
    */

    if (   n->Nleft->Ntype->Tspec == TS_DOUBLE
	|| n->Nleft->Ntype->Tspec == TS_LNGDBL )
	{
	switch (op)
	    {
	    case P_CAM+POF_ISSKIP+POS_SKPL:
		flushcode();		/* don't confuse peepholer */
		code0(P_CAM+POF_ISSKIP+POS_SKPL, r1, r2);
		code0(P_CAM+POF_ISSKIP+POS_SKPGE, VR2(r1), VR2(r2));
		op = P_CAM+POF_ISSKIP+POS_SKPLE;
		break;
	    case P_CAM+POF_ISSKIP+POS_SKPLE:
		flushcode();		/* don't confuse peepholer */
		code0(P_CAM+POF_ISSKIP+POS_SKPL, r1, r2);
		code0(P_CAM+POF_ISSKIP+POS_SKPG, VR2(r1), VR2(r2));
		break;
	    case P_CAM+POF_ISSKIP+POS_SKPG:
		flushcode();		/* don't confuse peepholer */
		code0(P_CAM+POF_ISSKIP+POS_SKPG, r1, r2);
		code0(P_CAM+POF_ISSKIP+POS_SKPLE, VR2(r1), VR2(r2));
		op = P_CAM+POF_ISSKIP+POS_SKPGE;
		break;
	    case P_CAM+POF_ISSKIP+POS_SKPGE:
		flushcode();		/* don't confuse peepholer */
		code0(P_CAM+POF_ISSKIP+POS_SKPG, r1, r2);
		code0(P_CAM+POF_ISSKIP+POS_SKPL, VR2(r1), VR2(r2));
		break;
	    case P_CAM+POF_ISSKIP+POS_SKPE:
		code0(P_CAM+POF_ISSKIP+POS_SKPN, VR2(r1), VR2(r2));
		break;
	    case P_CAM+POF_ISSKIP+POS_SKPN:
		code0(P_CAM+POF_ISSKIP+POS_SKPN, VR2(r1), VR2(r2));
		code0(P_CAM+POF_ISSKIP+POS_SKPE, r1, r2);
		code5(P_TRN+POF_ISSKIP+POS_SKPA, 0);
		vrfree(r1);
		return;
	    }
	}
    code0(op, r1, r2);			/* generate and optimize test */
    vrfree(r1);
}

/* GASSIGN - Generate assignment expression.
**	Various tricky stuff involved.
** Note the hair needed for handling compound assignment, because the f*ed-up
** peepholer zaps index registers with wild abandon.  We have to compensate
** for this by being careful how we generate the address of the destination.
**
** Also note hair for storing into volatile objects!  This is the counterpart
** to the fetch checking in gprimary() and gunary().  The other store code is
** in gincdec().
*/

static VREG *
gassign(NODE *n)
{
    VREG *r1, *r2, *ra;
    int ptr, savaddr;
    INT siz;
    NODE *nod;		/* Points to lvalue (without conversion) */
    int lconv;		/* Holds lvalue conversion op if any */
    int volat;		/* True if obj is volatile */
    TYPE *fromt, *tot;

    nod = n->Nleft;
    if (nod->Nop == N_CAST)	/* If lvalue needs conversion before the op */
	{
	lconv = nod->Ncast;	/* Remember conversion op for lvalue */
	tot = nod->Ntype;	/* cast to this type */
	nod = nod->Nleft;	/* Then get ptr to real lvalue */
	fromt = nod->Ntype;	/* cast from this type */
	}
    else
	lconv = CAST_NONE;

    siz = sizetype(n->Ntype);	/* Get size of result type, in words */

    /* See if object will be referenced via a byte pointer */
    if ((ptr = bptrref(nod)) < 0)
	{
	int_error("gassign: bad op %N", nod);
	ptr = 0;
	}
    if ((volat = tisanyvolat(nod->Ntype)) != 0)
	flushcode();		/* Barf, foil peepholer if volatile obj */

    if (n->Nop == Q_ASGN)	/* Simple assignment? */
	{
	r1 = genexpr(n->Nright);	/* Generate value first */
	/* Special check for doing IDPB.  Safer to do here instead of
	** in peephole, at least until peepholer fixed to allow keeping
	** an index reg around!
	*/
#if 0
/* Later, add many cases here when new MACRO instructions are defined.
 * Imitate case N_PTR: in gunary() below.
 */
#endif
	if (optgen && ptr			/* If a byte ptr */
		&& nod->Nop == N_PTR		/* and op is "*++(exp)" */
		&& nod->Nleft->Nop == N_PREINC)
	    {
#if 0		/* Later, fix Reg linkage for pointers */
	    if (Register_Id(nod->Nleft->Nleft))
		code40(P_IDPB, r1->Vrloc, nod->Nleft->Nleft->Nid->Sreg, 0);
	    else
#endif
		code4(P_IDPB, r1, gaddress(nod->Nleft->Nleft));
	    return r1;
	    }
	else if (Register_Id(nod))	/* approximate stomem for registers */
	    {
	    r_preserve = nod->Nid->Sreg;
	    if (siz == 1)
		{
		ra = vrget();
		ra->Vrtype = nod->Ntype;
		code00(P_MOVE, ra->Vrloc, r1->Vrloc);
		}
	    else
		int_error ("gassign: reg argument size > 1");
	    vrfree (r1);
	    return ra;
	    }
	else
	    r1 = stomem(r1,		/* Store the value */
		gaddress(nod),		/* into address of lvalue */
		/* Operand and operation types are same, so siz is correct */
		siz,
		ptr);			/* and flag saying if addr is ptr */
	if (volat)
	    flushcode();
	return r1;
	}

    /* Some compound assignment type.
    ** First, generate the right operand, including any conversions.
    */
    r2 = (n->Ntype->Tspec == TS_PTR) ?		/* Doing pointer arith? */
	gptraddend(n->Nleft->Ntype, n->Nright)	/* Operand for ptr arith */
	: genexpr(n->Nright);			/* General-type operand */

    /* Then generate the left operand.  For the time being, the peephole
    ** optimizer is so screwed up that we can't keep the address around
    ** and have to generate it twice.
    */
    savaddr = sideffp(nod);     /* Warn user if we'll fail */

    if (Register_Id(nod))	/* approximate getmem() for registers */
	{
	r_preserve = nod->Nid->Sreg;
	ra = vrget();
	ra->Vrtype = nod->Ntype;
#if 0
	r1 = rgetmem(ra, nod->Ntype, ptr, savaddr);
#else
	r1 = rgetmem(ra, nod->Ntype, savaddr);
#endif
	}
    else
	r1 = getmem(ra=gaddress(nod), /* Get left operand, WITHOUT releasing addr!*/
	    nod->Ntype,	/* using its real type */
	    ptr,			/* addr may be a byte pointer */
	    savaddr);			/* Keep the address reg! */

    /* Now have left operand in R1.  Convert it for operation, if needed. */
    if (lconv != CAST_NONE)	/* Convert left operand if necessary */
	r1 = gcastr(lconv, r1, fromt, tot, nod);

    /* Apply the arithmetic operation, checking to make sure pointer
    ** arithmetic is handled properly.  r2 is released.
    */
    if (n->Ntype->Tspec == TS_PTR)	/* If doing pointer arith */
	r1 = gptrop(n->Nop, r1, r2, n->Ntype, n->Nright->Ntype);
    else
	r1 = garithop(n->Nop, r1, r2, n->Nleft->Ntype->Tspec);

    /* Now see if there's any assignment conversion to perform on
    ** the result of the operation.
    */
    if (n->Nascast != CAST_NONE)
	{
	r1 = gcastr(n->Nascast, r1, n->Nleft->Ntype, n->Ntype, (NODE *)NULL);
	}

    if (Register_Id(nod))	/* approximate stomem for registers */
	{
	if (siz == 1)
	    {
	    code00(P_MOVE, nod->Nid->Sreg, r1->Vrloc);
	    if (Register_Nopreserve(ra->Vrloc)) /* garithop changed it!? */
		vrfree (ra);
	    return r1;
	    }
	else
	    int_error ("gassign: reg argument size > 1");
	return ra;
	}
    else
	{
	/* Finally, can store the value back. We either use the
	** saved address, if one, or generate it all over again.
	*/
	if (!savaddr)
	    ra = gaddress(nod);		/* Else re-use saved addr */
	r1 = stomem(r1, ra, siz, ptr);
	}
    if (volat)
	flushcode();			/* Barf bletch */
    return r1;
}

/* GBINARY - Generate code for binary operators.
**
*/
static VREG *
gbinary(NODE *n)
{
    VREG *r1, *r2;

    /*
    ** First, check for pointer arithmetic.  Legal operations are:
    **	Operation	Result
    **	(1) num + ptr	ptr
    **	(2) ptr + num	ptr
    **	(3) ptr - num	ptr
    **	(4) ptr - ptr	int or long
    **
    **	If the pointer is a byte pointer, we always make the number first.
    ** This is only because the current optimizer is too stupid to recognize
    ** certain patterns any other way.
    */
    if (n->Ntype->Tspec == TS_PTR		/* Catch cases 1, 2, 3 */
	|| n->Nleft->Ntype->Tspec == TS_PTR)	/* Catch case 4 */
	{
	if (n->Nop == Q_MINUS)	/* Cases 3 and 4 */
	    {
	    if (n->Nright->Ntype->Tspec == TS_PTR)	/* Case 4: ptr-ptr */
		{
		r1 = genexpr(n->Nleft);		/* Make the left operand 1st */
		return gptrop(n->Nop, r1, genexpr(n->Nright),
			n->Nleft->Ntype, n->Nright->Ntype);
		}
	    else					/* Case 3: ptr-num */
		{
		r1 = genexpr(n->Nleft);				/* Make ptr */
		r2 = gptraddend(n->Nleft->Ntype, n->Nright);	/* Make num */
		return gptrop(n->Nop, r1, r2,
			n->Nleft->Ntype, n->Nright->Ntype);
		}
	    }
	/* Cases 1 and 2 */
	if (n->Nleft->Ntype->Tspec != TS_PTR)	/* Do case 1: num+ptr */
	    {
	    r2 = gptraddend(n->Nright->Ntype,n->Nleft);	/* Make num 1st */
	    return gptrop(n->Nop, genexpr(n->Nright), r2,
			n->Nright->Ntype, n->Nleft->Ntype);	/* reversed */
	    }
	else				/* Do case 2: ptr+num */
	    {
	    r1 = genexpr(n->Nleft);			/* Make ptr 1st */
	    r2 = gptraddend(n->Nleft->Ntype,n->Nright);	/* num 2nd */
	    return gptrop(n->Nop, r1, r2,
			n->Nleft->Ntype, n->Nright->Ntype);
	    }
	}

    /* No pointer arithmetic involved, can just generate arithmetic stuff.
    ** Normally we generate the left operand first, but if the right operand
    ** is a function call then we reverse the order so as to avoid
    ** saving/restoring registers across the call.
    ** Also, if using normal ordering, we check to see whether the left
    ** operand will need to be widened (since integer division requires
    ** a doubleword register), and if so widen it ahead of time so that
    ** the generation of the right operand won't suboptimally seize the
    ** 2nd register and then have to be shuffled around later.
    */
    if (n->Nright->Nop == N_FNCALL && optgen)
	{
	r2 = genexpr(n->Nright);	/* Do function call first */
	r1 = genexpr(n->Nleft);		/* then left operand */
	}
    else
	{
	r1 = genexpr(n->Nleft);		/* Normal order, left first */
	if ((n->Nop == Q_DIV || n->Nop == Q_MOD) && tisinteg(n->Ntype)
		&& optgen)
	    vrlowiden(r1);		/* Widen in preparation for div */
	r2 = genexpr(n->Nright);	/* Now generate right operand */
	}
    return garithop(n->Nop, r1, r2, n->Ntype->Tspec);
}

/* GARITHOP - Generate code for binary arithmetic operators
**	given values in registers.
** The only types permitted are:
**		TS_FLOAT, TS_DOUBLE, TS_LNGDBL
**		TS_INT, TS_UINT
**		TS_LONG, TS_ULONG
**	Note that types "char" and "short" should already have been converted
** (via usual unary/binary conversions) to "int" before the operation
** is performed.
*/

static VREG *
garithop(op, r1, r2, ts)
int op;			/* Operation to generate code for */
int ts;			/* Type of the operands (TS_ value) */
VREG *r1, *r2;		/* Registers operands are in (r2 is released) */
{
    switch(op)
	{
	case Q_ASPLUS:
	case Q_PLUS:
	    switch (ts)
		{
		default:
		    int_error("garithop: bad +");
		case TS_INT:
		case TS_UINT:
		case TS_LONG:
		case TS_ULONG:
		    code0(P_ADD,  r1, r2);
		    break;
		case TS_FLOAT:
		    code0(P_FADR, r1, r2);
		    break;
		case TS_DOUBLE:
		case TS_LNGDBL:
		    code0(P_DFAD, r1, r2);
		    break;
		}
	    break;

	case Q_ASMINUS:
	case Q_MINUS:
	    switch (ts)
		{
		default:
		    int_error("garithop: bad -");
		case TS_INT:
		case TS_UINT:
		case TS_LONG:
		case TS_ULONG:
		    code0(P_SUB,  r1, r2);
		    break;
		case TS_FLOAT:
		    code0(P_FSBR, r1, r2);
		    break;
		case TS_DOUBLE:
		case TS_LNGDBL:
		    code0(P_DFSB, r1, r2);
		    break;
		}
	    break;

	/*	* Unsigned Multiplication
	**	MUL R,E
	**	TRNE R,1	or	LSH R+1,1	or	LSH R+1,1
	**	 TLOA R+1,400000	LSHC R,-1		LSHC R,-35.
	**	  TLZ R+1,400000
	**	result in R+1		result in R+1		result in R
	*/
	case Q_ASMPLY:
	case Q_MPLY:
	    switch (ts)
		{
		default:
		    int_error("garithop: bad *");
		case TS_UINT:
		case TS_ULONG:
		    if (!vrispair(r1))	/* Unless already widened, */
			vrlowiden(r1);	/* grab two words for the multiply */
		    code0(P_MUL, r1, r2);
		    code8(P_TRN+POF_ISSKIP+POS_SKPE, r1, 1);
		    code8(P_TLO+POF_ISSKIP+POS_SKPA, VR2(r1), 0400000L);
		    code8(P_TLZ, VR2(r1), 0400000L);
		    vrnarrow(r1 = VR2(r1));	/* Narrow back, keep 2nd wd */
		    break;
		case TS_INT:
		case TS_LONG:
		    code0(P_IMUL, r1, r2);
		    break;
		case TS_FLOAT:
		    code0(P_FMPR, r1, r2);
		    break;
		case TS_DOUBLE:
		case TS_LNGDBL:
		    code0(P_DFMP, r1, r2);
		    break;
		}
	    break;

    /* Integer division is done differently from other integer operations
    ** because the IDIV instruction produces a doubleword result.
    ** Note that we can't do the apparent optimization of using ASH or AND
    ** when the divisor is a constant power of two, because they perform
    ** inconsistently with IDIV on negative numbers.
    */
	case Q_ASDIV:
	case Q_DIV:
	    switch (ts)
		{
		default:
		    int_error("garithop: bad /");
		case TS_INT:
		case TS_UINT:
		case TS_LONG:
		case TS_ULONG: /* Hair for integer division */
			{
			int save_reg = r1->Vrloc;
			if (!vrispair(r1))	/*Unless already widened by gbinary,*/
			    vrlowiden(r1);	/* grab two words for the division. */
			code0((tspisunsigned(ts) ? P_UIDIV : P_IDIV), r1, r2);
			vrnarrow(r1);		/* Narrow back, keep 1st word */
			if (Register_Preserve(save_reg) && save_reg != r1->Vrloc)
			    {
			    code00(P_MOVE, save_reg, r1->Vrloc);
			    r1->Vrloc = save_reg;
			    }
			folddiv(r1);		/* Do cse on result */
			break;
			}
		case TS_FLOAT:
		    code0(P_FDVR, r1, r2);
		    break;
		case TS_DOUBLE:
		case TS_LNGDBL:
		    code0(P_DFDV, r1, r2);
		    break;
		}
	    break;

	case Q_ASMOD:
	case Q_MOD:
	    switch (ts)
		{
		default:
		    int_error("garithop: bad %%");
		case TS_INT:
		case TS_UINT:
		case TS_LONG:
		case TS_ULONG:
					/* Hair for integer remainder */
		    if (!vrispair(r1))	/* Unless already widened by gbinary,*/
			vrlowiden(r1);	/* grab two words for the division. */
		    code0((tspisunsigned(ts) ? P_UIDIV : P_IDIV), r1, r2);
		    vrnarrow(r1 = VR2(r1)); /* Narrow back, keep 2nd word */
		    folddiv(r1);		/* Do cse on result */
		    break;
		}
	    break;

	case Q_ASRSH:
	case Q_RSHFT:
	    code0(P_MOVN, r2, r2);		/* negate arg to make right shift */
					/* Then drop through to do shift */
	case Q_ASLSH:
	case Q_LSHFT:
	    switch (ts)
		{
		default:
		    int_error("garithop: bad shift");

		case TS_INT:	/* Signed values use arith shift for >> */
		case TS_LONG:
		    if (op == Q_ASRSH || op == Q_RSHFT)
			{
			code4(P_ASH, r1, r2);
			break;
			}
		/* Drop thru if <<, for logical shift. */
		/* According to CARM, << is always logical even if signed */

		case TS_UINT:		/* Unsigned values use logical shift */
		case TS_ULONG:
		    code4(P_LSH, r1, r2);	/* this takes arg as if PTA_RCONST */
		    break;
		}
	    break;

	case Q_ASOR:
	case Q_OR:
	    switch (ts)
		{
		default:
		    int_error("garithop: bad |");
		case TS_INT:
		case TS_UINT:
		case TS_LONG:
		case TS_ULONG:
		    code0 (P_IOR, r1, r2);
		    break;
		}
	    break;

	case Q_ASAND:
	case Q_ANDT:
	    switch (ts)
		{
		default:
		    int_error("garithop: bad &");
		case TS_INT:
		case TS_UINT:
		case TS_LONG:
		case TS_ULONG:
		    code0 (P_AND, r1, r2);
		    break;
		}
	    break;

	case Q_ASXOR:
	case Q_XORT:
	    switch (ts)
		{
		default:
		    int_error("garithop: bad ^");
		case TS_INT:
		case TS_UINT:
		case TS_LONG:
		case TS_ULONG:
		    code0 (P_XOR, r1, r2);
		    break;
		}
	    break;

	default:
	    int_error("garithop: bad op %d", op);
	    vrfree(r2);
	}
    return r1;
}

/* GPTROP - Generate code for pointer arithmetic operations.
**	Legal pointer arithmetic operations are:
**		Operation	Result
**		* (1) num + ptr	ptr
**		(2) ptr + num	ptr
**		(3) ptr - num	ptr	
**		(4) ptr - ptr	int or long
**
** NOTE: It is the caller's responsibility to swap the operands of case 1 to
** transform it into case 2.  It is up to the caller to decide which one
** to generate first; however, for case 4 it is probably best to do the
** left operand first.
**	If the 2nd operand is a number it must have been generated by
** gptraddend (rather than genexpr).  In this case, r2 may be NULL if
** gptraddend has determined that the number is zero and nothing needs
** to be added or subtracted.
*/
static VREG *
gptrop(op, r1, r2, lt, rt)
int op;			/* Q_PLUS, Q_MINUS, Q_ASPLUS, Q_ASMINUS */
VREG *r1, *r2;		/* Registers holding left and right operands */
TYPE *lt, *rt;		/* Types of left and right operands */
{
    INT size;

    switch (op)
	{
	case Q_ASMINUS:
	case Q_MINUS:
	    if (rt->Tspec == TS_PTR)	/* Handle case 4 */
	    /* Handle case 4: ptr-ptr (make left operand first) */
		{
		if (tisbytepointer(lt))
		    {
		    vrlowiden(r1);			/* Must widen */
		    code0(P_SUBBP, r1, r2);		/* Do the sub */
		    vrnarrow(r1 = VR2(r1));		/* Result in 2nd word */
		    }
		else
		    code0(P_SUB, r1, r2);
		if ((size = sizeptobj(lt)) > 1)
		    {
		    vrlowiden(r1);		/* Ugh, must adjust result */
		    code1(P_IDIV, r1, size);
		    vrnarrow(r1);		/* Narrow to get result in 1st wd */
		    folddiv(r1);
		    }
		break;
		}

	/* Handle case 3: ptr-num.  Num must be generated by gptraddend. */
	    if (r2 == NULL)
		return r1;	/* Ensure have something to subtract */
	    if (tisbytepointer(lt))
		{
		code0(P_MOVN, r2, r2);
		code0(P_ADJBP, r2, r1);	/* Adjust char pointer */
		return r2;
		}
	    code0(P_SUB, r1, r2);		/* Adjust word pointer */
	    break;

	case Q_ASPLUS:
	case Q_PLUS:
	/* Handle case 2: ptr+num.  Num must be generated by gptraddend. */
	/* Note that case 1 should be transformed into case 2 by caller. */
	    if (r2 == NULL)
		return r1;	/* Ensure something to add */
	    if (tisbytepointer(lt))		/* If ptr is a char ptr */
		{
		code0(P_ADJBP, r2, r1);	/* Adjust char pointer */
		return r2;
		}
	    code0(P_ADD, r2, r1);		/* Adjust word pointer */
	    return r2;

	default:
	    int_error("gptrop: bad op %d", op);
	}
    return r1;
}

/* GPTRADDEND - Auxiliary to GPTROP.  This routine generates the
**	proper value for adding or subtracting from a pointer.
**	Note that it may return NULL if it determines that the value
**	is zero; that is, no value (and no operation) is necessary.
*/
static VREG *
gptraddend(TYPE *t, NODE *n)
/* Type of the pointer this value is being added to and Addend(or subtrahend)
 * expression */
{
    VREG *r;
    INT size;

    if (n->Nop == N_ICONST && optgen)		/* Do optimization */
	{
	size = sizeptobj(t) * n->Niconst;	/* If num is a constant */
	if (size == 0)
	    return NULL;		/* Zero value, gen nothing! */
	r = vrget();
	code1(P_MOVE, r, size);
	r->Vrtype = n->Ntype;		/* Set C type of object in reg */
	return r;
	}
    r = genexpr(n);			/* First generate value as given */
    if ((size = sizeptobj(t)) > 1)	/* Then check to see if mult needed */
	code1(P_IMUL, r, size);	/* Yeah, multiply it by size of obj */
    return r;
}

/* GLOGICAL - Generate code for boolean binary & unary operators
*/

static VREG *
glogical(NODE *n)
{
    VREG *reg;
    SYMBOL *false, *true, *temp;
    int reverse;

    reverse = (optgen && n->Nop == Q_LOR);
    n->Nendlab = true = newlabel();	/* get label for true case */
    false = newlabel();			/* get label for false case */

    /*
    ** See gternary() for an explanation of why this call is needed.
    */
    vrallspill();

    gboolean (n, false, reverse);	/* make the boolean code */
    if (optgen && unjump (false))	/* can put false case first? */
	{
	temp = false;			/* yes, swap meaning of false */
	false = true;			/* and true, so labels go out */
	true = temp;			/* in correct order. */
	reverse = !reverse;		/* also invert reversal switch */
	}

    if (n->Nflag & NF_RETEXPR)
	reg = vrretget(); /* get value in return reg */
    else
	reg = vrget();		/* not for return, use normal reg */
    reg->Vrtype = n->Ntype;		/* Set C type of object in reg */
    codlabel(true);			/* true label goes here */
    if (reverse)
	code0(P_TDZ+POF_ISSKIP+POS_SKPA, reg, reg); /* make zero, skip */
    else
	code1(P_SKIP+POF_ISSKIP+POS_SKPA, reg, 1); /* make one, skip */

    codlabel(false);			/* now make false label */
    if (reverse)
	code1(P_MOVE, reg, 1);	/* reversed false makes one */
    else
	code5(P_SETZ, reg);		/* normal false makes zero */
    return reg;				/* return the register */
}

extern int _chnl;

/* GUNARY - Generate code for unary operators
*/

static VREG *
gunary(NODE *n)
{
    VREG *r;
    int volat;

    switch (n->Nop)
	{
	case N_PREINC:
	    return gincdec(n,  1, 1);
	case N_PREDEC:
	    return gincdec(n, -1, 1);
	case N_POSTINC:
	    return gincdec(n,  1, 0);
	case N_POSTDEC:
	    return gincdec(n, -1, 0);

	case N_CAST:
	    return gcast(n);
	case N_ADDR:
	    return gaddress(n->Nleft);

	case N_PTR:
	/* See comments at gprimary() about volatile objects. */
	    if ((volat = tisvolatile(n->Ntype)) != 0)
		flushcode();		/* Obj is volatile, avoid optimiz */

	    if (debcsi == KCC_DBG_NULL)
		{
		_chnl = n->sfline;
		switch (n->Nleft->Nop)
		    {
		    case Q_IDENT:
			code4 (P_NULPTR, (VREG *) NULL, gaddress (n->Nleft));
			break;
		    case N_PREINC:
		    case N_PREDEC:
		    case N_POSTINC:
		    case N_POSTDEC:
			code4 (P_NULPTR, (VREG *) NULL, gaddress (n->Nleft->Nleft));
			break;
		    default:
			_chnl = -1;
			break;
		    }
		}

	/* Special check for doing ILDB.  Safer to do here instead of
	** in peephole, at least until peepholer fixed to allow keeping
	** an index reg around!
	*/
	    if (Register_Id(n->Nleft))
		{
#if 0	/* Reg linkage */
		if (optgen && tisbytepointer(n->Nleft->Ntype)) /* if byte ptr */
		    {
		    r = vrget();
		    r->Vrtype = n->Ntype; /* Set C type of object in reg */
		    if (n->Nleft->Nop == N_PREINC)	/* "*++(reg)" */
			code0 (P_ILDB, r, gaddress (n->Nleft->Nleft));
#if 0	/* add when LDBI, LDBD, and DLDB are defined here, below, and in 
	 * gassign() for case N_PTR:
	 */
		    else if (n->Nleft->Nop == N_PREDEC)	/* "*--(reg) */
			code0 (P_DLDB, r, gaddress (n->Nleft->Nleft));
		    else if (n->Nleft->Nop == N_POSTINC)	/* "*(reg)++" */
			code0 (P_LDBI, r, gaddress (n->Nleft->Nleft));
		    else if (n->Nleft->Nop == N_POSTDEC)	/* "*(reg)--" */
			code0 (P_LDBD, r, gaddress (n->Nleft->Nleft));
		    else if (n->Nleft->Nop == NULL)
			code0 (P_LDB, r, gaddress (n->Nleft->Nleft));
#endif
		    else	/* approximate getmem() for registers */
#if 0
			r = rgetmem(genexpr(n->Nleft), n->Ntype,
				    tisbytepointer(n->Nleft->Ntype), 0);
#else
			r = rgetmem(genexpr(n->Nleft), n->Ntype, 0);
#endif
		    }
		else		/* approximate getmem() for registers */
#endif
#if 0
		    r = rgetmem(genexpr(n->Nleft), n->Ntype,
			    tisbytepointer(n->Nleft->Ntype), 0);
#else
		    r = rgetmem(genexpr(n->Nleft), n->Ntype, 0);
#endif
		}
	    else 
	/* if "*++(exp)" of a byte pointer */ if (optgen && tisbytepointer(n->Nleft->Ntype) &&
		    n->Nleft->Nop == N_PREINC)
		{
		r = vrget();
		r->Vrtype = n->Ntype; /* Set C type of object in reg */
		code4 (P_ILDB, r, gaddress(n->Nleft->Nleft));
		}
	    else
		r = getmem(genexpr(n->Nleft), n->Ntype,
				    tisbytepointer(n->Nleft->Ntype), 0);
	    if (volat)
		flushcode();
	    return r;

	case Q_MUUO:
	    return gmuuo(n);

	case N_NEG:
	    if (Register_Id(n->Nleft))
		{
#if 0
		if ( n->Ntype->Tspec == TS_DOUBLE
			|| n->Ntype->Tspec == TS_LNGDBL)
		    {
		    r = vrdget();
		    r->Vrtype = n->Nleft->Ntype;
		    code00(P_DMOVN, r->Vrloc, n->Nleft->Nid->Sreg);
		    }
		else
#endif
		    {
		    r = vrget();
		    r->Vrtype = n->Nleft->Ntype;
		    code00(P_MOVN, r->Vrloc, n->Nleft->Nid->Sreg);
		    }
		return r;
		}
	    r = genexpr(n->Nleft);
	    if ( n->Ntype->Tspec == TS_DOUBLE
		    || n->Ntype->Tspec == TS_LNGDBL)
		code0(P_DMOVN, r, r);
	    else
		code0(P_MOVN, r, r);
	    return r;

	case Q_COMPL:
	    if (Register_Id(n->Nleft))
		{
		r = vrget();
		r->Vrtype = n->Nleft->Ntype;
		code00(P_SETCM, r->Vrloc, n->Nleft->Nid->Sreg);
		return r;
		}

	    r = genexpr(n->Nleft);
	    code0(P_SETCM, r, r);
	    return r;

	default:
	    int_error("gunary: bad op %N", n);
	    return 0;
	}
}

/* GCAST - Generate code for type conversion (cast)
**
**	Note that the way we manage the task of keeping char values
** masked off is NOT by implementing a mask for casts to (char) type.
** Rather, we mask the register value only when widening.  This works
** because a value of type (char) is always either assigned to a (char) object
** (in which case a byte pointer is used and the mask is automatic) or
** it is used in an expression -- and always promoted to an int or u_int.
** The masking would be wasteful and unnecessary for the first case, and
** the second case will always have an explicit N_CAST to widen the integer.
** See the INTERN.DOC file for a better explanation.
*/

static VREG *
gcast(NODE *n)
{
    VREG *r;
    /* If this expression is a return value, see if we can pass on
    ** the flag which marks it thusly.  This basically benefits
    ** gcall() which uses the flag to do tail recursion; we want to ensure
    ** that a no-op cast won't prevent this optimization.
    */
    if ((n->Nflag & NF_RETEXPR)		/* This expr is a return val? */
	&& gcastr(n->Ncast, (VREG *)NULL,	/* and cast is a no-op? */
			n->Nleft->Ntype, n->Ntype, n->Nleft) == NULL)
	{
	n->Nleft->Nflag |= NF_RETEXPR;	/* Yes, pass flag on! */
	if ((r = genexpr(n->Nleft)) != NULL) /* No cast, just generate expr */
	    r->Vrtype = n->Ntype;	/* and reflect correct type */
	return r;
	}

    return gcastr(n->Ncast, genexpr(n->Nleft),
			n->Nleft->Ntype, n->Ntype, n->Nleft);
}

static VREG *
gcastr(cop, r, tfrom, tto, ln)
int cop;	/* Cast op (a CAST_ value) */
VREG *r;	/* Virtual reg holding value to cast.
		** NOTE NOTE NOTE!!!  If this is NULL, we are merely testing
		** to see whether a cast would be produced.  If there is
		** no cast, NULL will be returned, else (VREG *)-1.
		*/
TYPE *tfrom, *tto;
NODE *ln;	/* If non-null, is node that R was generated from. */
{
    switch (cop)
	{
	case CAST_NONE:		/* No actual action required */
	    break;

	case CAST_VOID:		/* Throwing away the value */
	    if (r)
		relflush(r);	/* Release the register */
	    return NULL;

	case CAST_IT_PT:
	    if (!r)					/* Just checking? */
		return gintwiden(r, tfrom, uinttype, ln);
	    else
		r = gintwiden(r, tfrom, uinttype, ln); /* Widen int to uint */
	    break;

	case CAST_IT_EN:
	case CAST_IT_IT:
	    if (!r)					/* Just checking? */
		return gintwiden(r, tfrom, tto, ln);
	    else
#if 0	/* Later, Reg linkage */	
	    if (Register_Nopreserve (r->Vrloc) && unsigned)
#endif
		r = gintwiden(r, tfrom, tto, ln); /*Widen integer if needed */
	    break;

	case CAST_EN_EN:
	case CAST_EN_IT:
	case CAST_PT_IT:			/* No representation change needed */
	    break;

	case CAST_PT_PT:			/* General ptr to ptr conversion */
	    if (tisbytepointer(tfrom))
		{
		if (tisbytepointer(tto))
		    {
		/* Byte pointer to byte pointer, check sizes */
		    int fsiz = elembsize(tfrom);
		    int tsiz = elembsize(tto);
		    if (!fsiz)
			{
		    /* (void *) to byte pointer. */
			if (tischarpointer(tto))	/* If any kind of char obj, */
			    break;			/* do no conversion. */
			fsiz = TGSIZ_CHAR;		/* Else cvt as if (char *) */
			}
		    if (!tsiz)
			{
		    /* Byte pointer to (void *) */
			if (tischarpointer(tfrom))	/* If any kind of char obj, */
			    break;			/* do no conversion. */
			tsiz = TGSIZ_CHAR;		/* Else cvt as if (char *) */
			}
		    if (fsiz == tsiz)
			break;	/* No conversion needed? */

		    if (!r)
			return (VREG *)-1;	/* Need, stop if just chking */

		/* If converting between char and short
		** (9 and 18 bit bytes), use special op.
		*/
		    if (   (fsiz == TGSIZ_CHAR && tsiz == TGSIZ_SHORT)
			|| (fsiz == TGSIZ_SHORT && tsiz == TGSIZ_CHAR))
			{
			code10(P_PTRCNV, r, (SYMBOL *)NULL, tsiz, fsiz);
			break;
			}

		/* Odd size, convert to word pointer, then to byte pointer. */
		    code10(P_TDZ+POF_ISSKIP+POS_SKPE,	/* Check for NULL */
			r, (SYMBOL *)NULL, -1, 0);	/* Mask off P+S */
		    code10(P_IOR, r, (SYMBOL *) NULL, tsiz, 0);	/* make BP */
		    }
		else
		    {
		/* Byte pointer (any kind!) to word pointer */
		    if (!r)
			return (VREG *)-1;	/* Stop if just checking. */
		    code10(P_TDZ, r, (SYMBOL *) NULL, -1, 0); /* Mask off P+S */
		    }
		}
	    else if (tisbytepointer(tto))
		{
		int tsiz;
		/* Word pointer to byte pointer */
		if (!r)
		    return (VREG *)-1;	/* Stop if just checking. */
		if ((tsiz = elembsize(tto)) == 0)   /* Check for (void *) */
		    tsiz = TGSIZ_CHAR;
		pitopc(r, tsiz, 0, 0);
		}
	    break;

	case CAST_FP_IT:
	    if (!r)
		return (VREG *)-1;	/* Stop if just checking. */
	    switch (tfrom->Tspec)
		{
		case TS_FLOAT:
		    code0(P_FIX, r, r);	/* just use that! */
		    break;
		case TS_DOUBLE:
		case TS_LNGDBL:
		    code0(P_DFIX, r, r);	/* r must be a register pair */
		    vrnarrow(r);		/* Use 1st AC as result */
		    break;
		}
	/* Narrow the int here if needed */
	    break;

	case CAST_FP_FP:
	    switch (castidx(tfrom->Tspec,tto->Tspec))
		{
		case castidx(TS_DOUBLE,TS_FLOAT):
		case castidx(TS_LNGDBL,TS_FLOAT):
		    if (!r)
			return (VREG *)-1;	/* Stop if just checking. */
		    code0(P_DSNGL, r, r);	/* r must be a register pair! */
		    vrnarrow(r);		/* Forget about the second word */
		    break;
		case castidx(TS_FLOAT,TS_DOUBLE):
		case castidx(TS_FLOAT,TS_LNGDBL):
		    if (!r)
			return (VREG *)-1;	/* Stop if just checking. */
		    vrlowiden(r);
		    code5(P_SETZ, VR2(r));
		    break;
		case castidx(TS_LNGDBL,TS_DOUBLE):
		case castidx(TS_DOUBLE,TS_LNGDBL):
		    break;
		}
	    break;

	case CAST_IT_FP:
	    if (!r)
		return (VREG *)-1;	/* Stop if just checking. */
	    r = gintwiden(r, tfrom,		/* Ensure widened to int or unsigned */
		    tissigned(tfrom) ? inttype : uinttype,
		    ln);
	    switch (tto->Tspec)
		{
		case TS_FLOAT:
		/* Although FLTR and UFLTR are always supported by CCOUT,
		** on KA-10s they are inefficient enough that it is worth
		** checking for the opportunity to use a simple FSC, which
		** is limited to integers of 27 bits or less.
		*/
		    if (tissigned(tfrom) || tbitsize(tfrom) < TGSIZ_WORD)
			{
		    /* Signed or known positive */
			code0(P_FLTR, r, r); /* Use FLTR instr or macro */
			break;
			}
		/* Ugh, unsigned full word value, must use hairy UFLTR. */
		    code0(P_UFLTR, r, r); /* Use UFLTR simulated op */
		    break;

		case TS_DOUBLE:
		case TS_LNGDBL:
		    vrlowiden(r);	/* Make into register pair */
		    code5(P_SETZ, VR2(r)); /* zero the next reg */
		    if (tissigned(tfrom) || tbitsize(tfrom) < TGSIZ_WORD)
			{
			code8(P_ASHC, r, -8); /* shift out mantissa*/
			code8(P_TLC, r, 0243000L); /* put exponent in */
			}
		    else			/* Unsigned conversion */
			{
			code8(P_LSHC, r, -9);		/* Shift unsigned */
			code8(P_LSH, VR2(r), -1); /* Fix up lo wd */
			code8(P_TLC, r, 0244000L); /* exp (note 1 bigger!) */
			}
		    code9(P_DFAD, r, 0.0, 1);	/* Normalize the result */
		    break;
		}
	    break;

	default:
	    int_error("gcastr: bad cast %d", cop);
	    return NULL;
	}

    /* Cast done, now set new type of object in virtual register! */
    if (r)
	r->Vrtype = tto;
    return r;
}

/* GINTWIDEN and GUINTWIDEN - Auxiliaries for GCAST to widen integral values.
**	Always widens to full word even if new type is smaller, because
**	it's just as easy and makes no difference to handling of new type.
** NOTE: treats a VREG arg of NULL just as gcastr() does, i.e. only checks
**	to see whether a conversion would be necessary or not.
** GUINTWIDEN is a subroutine just so gboolean() can invoke it to force
**	an unsigned-type widen.
*/
static VREG *
gintwiden(VREG *r, TYPE *tfrom, TYPE *tto, NODE *n)		
/* Node that R was generated from (if any) */
{
    if (tbitsize(tto) > tbitsize(tfrom))
	{
	if (tisunsign(tfrom))	/* Handle unsigned.  Easy, just mask off */
	    {
	    r = guintwiden(r, tbitsize(tfrom), n);
	    }
	else		/* Handle signed.  Harder, must test bit. */
	    {
	    if (!r)
		return (VREG *)-1;		/* Stop if just checking. */
	    if (tbitsize(tfrom) == TGSIZ_HALFWD)	/* Special case */
		{
		code0(P_HRRE, r, r);		/* Extend sign of halfwd */
		return r;
		}
	    code8(P_TRN+POF_ISSKIP+POS_SKPE, r, (1<<(tbitsize(tfrom)-1)));
	    code8(P_TRO+POF_ISSKIP+POS_SKPA, r, -(1 << tbitsize(tfrom)));
	    code1(P_AND, r, (1 << tbitsize(tfrom))-1);	/* Positive, zap! */
	    }
	}
    return r;
}

static VREG *
guintwiden(VREG *r, int fbitsize, NODE *n)
/* # bits of value in R and Node that R was generated from (if any) */
{
    /* Must zap high-order bits.  Try to avoid doing this by
    ** seeing whether those bits are known to already be zero.
    ** Primary case is that of an LDB data fetch.
    */
    if (!(n &&
      (bptrref(n) > 0			/* Win if LDB fetch */
      || (n->Nop == Q_ASGN		/* Or if an assignment of a */
	&& bptrref(n->Nright) > 0	/* LDB also of safe size */
	&& tbitsize(n->Nright->Ntype) <= fbitsize))) )
	{
	if (!r)
	    return (VREG *)-1;		/* Stop if just checking. */
	code1(P_AND, r, ((INT) 1 << fbitsize)-1);	/* Zap! */
	}
    return r;
}

/* GINCDEC - Generate code for prefix/postfix increment/decrement.
**	This is special-cased (instead of being handled by general
**	arith code) both for efficiency and because the address is
**	only supposed to be evaluated once.  The code also checks
**	for NF_DISCARD to see whether the result value is needed or not;
**	if not, it forces the operation to be prefix instead of postfix,
**	so that all fixup work can be avoided!
*/

static VREG *
gincdec(NODE *n, int inc, int pre)
/* The inc/dec expression node, +1 for increment, -1 for decrement, and 
 * True if prefix, else postfix.
 */
{
    VREG *r, *ra, *r2;
    INT size = 1;		/* Default size for most common case */
    int savaddr;
    int volat;

    if (n->Nflag & NF_DISCARD)	/* Will result be discarded? */
	pre = 1;		/* If so, prefix form is always better! */
    n = n->Nleft;		/* Mainly interested in operand */
    if ((volat = tisvolatile(n->Ntype)) != 0)
	flushcode();		/* Barfo, avoid optimiz of volatile obj */

    if (Register_Id(n))
	{
	void codr1(int, int, INT);
	if (pre)	/* r->Vrloc = n->Nid->Sreg, if preserve reg */
	    r_preserve = n->Nid->Sreg;

	switch(n->Ntype->Tspec)
	    {
	    case TS_FLOAT:
		r = vrget();
		r->Vrtype = n->Ntype;	/* Set C type of object in reg */
		code00(P_MOVE, r->Vrloc, n->Nid->Sreg);
		codr1(P_FADR, n->Nid->Sreg,(INT) ((inc > 0)? 1.0 : -1.0)); // FW KCC-NT
		break;
#if 0	/* for next version of KCC regs */
	    case TS_DOUBLE:
	    case TS_LNGDBL:
		r = vrdget();
		r->Vrtype = n->Ntype;	/* Set C type of object in reg */
		if (!pre)
		    {
		    code00(P_DMOVE, r->Vrloc, n->Nid->Sreg);
		    r_preserve = n->Nid->Sreg;
		    ra = vrdget();
		    ra->Vrtype = n->Ntype;/* Set C type of object in reg */
		    code9(P_DFAD, ra, ((inc > 0)? 1.0 : -1.0), 1);
		    vrfree(ra);
		    }
		else
		    code9(P_DFAD, r, ((inc > 0)? 1.0 : -1.0), 1);
		break;
	    case TS_PTR:			/* Hacking pointer? */
	    case TS_ENUM:
	    case TS_BITF:
	    case TS_UBITF:
#endif
	    case TS_INT:
	    case TS_UINT:
	    case TS_LONG:
	    case TS_ULONG:
	    case TS_CHAR:
	    case TS_UCHAR:
	    case TS_SHORT:
	    case TS_USHORT:
		r = vrget();
		r->Vrtype = n->Ntype;	/* Set C type of object in reg */
		code00(P_MOVE, r->Vrloc, n->Nid->Sreg);
		codr1(P_ADD, n->Nid->Sreg,((inc > 0)? 1 : -1));
		break;
	    default:
		int_error("gincdec: bad reg type %N", n);
		return NULL;
	    }
	}
    else
	switch (n->Ntype->Tspec)
	    {
	    case TS_FLOAT:
		r = vrget();
		r->Vrtype = n->Ntype;	/* Set C type of object in reg */
		code9(P_MOVE, r, (inc > 0 ? 1.0 : -1.0), 0);
		code4(P_FADR+POF_BOTH, r, gaddress(n));
		if (!pre)
		    code9(P_FSBR, r, (inc > 0 ? 1.0 : -1.0), 0);
		break;

	    case TS_DOUBLE:
	    case TS_LNGDBL:
		r = vrdget();
		r->Vrtype = n->Ntype;	/* Set C type of object in reg */
		if ((savaddr = sideffp(n)) != 0) /* See if lvalue has side effects */
		    {
		    ra = gaddress(n);		/* Yes, make address first */
		    code9(P_DMOVE, r, (inc > 0 ? 1.0 : -1.0), 1);
		    codek4(P_DFAD, r, ra);	/* Do op, keep address reg around */
		    code4(P_DMOVEM, r, ra);
		    }
		else
		    {
		    code9(P_DMOVE, r, (inc > 0 ? 1.0 : -1.0), 1);
		    code4(P_DFAD, r, gaddress(n));
		    code4(P_DMOVEM, r, gaddress(n));
		    }
		if (!pre)
		    code9(P_DFSB, r, (inc > 0 ? 1.0 : -1.0), 1);
		break;

	    case TS_PTR:			/* Hacking pointer? */
		size = sizeptobj(n->Ntype);	/* Find size of obj */
		if (!size)
		    int_error("gincdec: 0-size obj %N", n);
		if (tisbytepointer(n->Ntype))	/* Special if a (char *) */
		    {
		    if (inc < 0)
			size = -size;
		    if ((savaddr = sideffp(n)) != 0) /* See addr has side effs */
			ra = gaddress(n);		/* Ugh, find & save it */
		    r = vrget();
		    r->Vrtype = n->Ntype;		/* Set C type of obj in reg */

		/* If doing post-increment, save orig pointer value */
		    if (!pre)
			{
			r2 = vrget();
			r->Vrtype = n->Ntype;	/* Set C type of obj in reg */
			if (savaddr)
			    codek4(P_MOVE, r2, ra);	/* Save ptr */
			else
			    code4(P_MOVE, r2, gaddress(n));
			}

		/* Now perform the increment.  If the address of the pointer
		** was saved in ra, it is released in this process.  r has
		** a copy of the new pointer value.
		*/
		    if (size == 1)		/* Special case */
			{
			if (savaddr)
			    codek4(P_IBP, 0, ra);
			else
			    code4(P_IBP, (VREG *)NULL, gaddress(n));
			if (pre)			/* If will need val, get it. */
			    code4(P_MOVE, r, (savaddr ? ra : gaddress(n)));
			}
		    else			/* General case */
			{
			code1(P_MOVE, r, size);	/* get how much */
			if (savaddr)
			    codek4(P_ADJBP, r, ra);
			else
			    code4(P_ADJBP, r, gaddress(n));
			code4(P_MOVEM, r,		/* store back in memory */
				    (savaddr ? ra : gaddress(n)));
			}

		/* Now, if doing postincrement, flush r and use r2 instead */
		    if (!pre)
			{
			vrfree(r);
			r = r2;
			}
		    break;		/* Break out to return R */
		    }
	    /* Drop through to handle non-char pointer as integer */


	    case TS_ENUM:
	    case TS_INT:
	    case TS_UINT:
	    case TS_LONG:
	    case TS_ULONG:
		r = vrget();
		r->Vrtype = n->Ntype;	/* Set C type of obj in reg */
		if (size == 1)
		    code4((inc > 0 ? P_AOS : P_SOS), r, gaddress(n));
		else				/* inc/dec by non-1 integer */
		    {
		    code1(P_MOVE, r, (inc > 0 ? size : -size));
		    code4(P_ADD+POF_BOTH, r, gaddress(n));
		    }
		if (!pre)			/* For postincrement, undo reg */
		    code1((inc > 0 ? P_SUB : P_ADD), r, size); /* undo change */
		break;

	    case TS_BITF:
	    case TS_UBITF:
	    case TS_CHAR:
	    case TS_UCHAR:
	    case TS_SHORT:
	    case TS_USHORT:
		if (inc < 0)
		    size = -size;
		savaddr = sideffp(n);       /* See if addr has side effs */
		ra = gaddress(n);		/* Ugh, find & save it */
	    /* Fetch byte, save addr if savaddr != 0 */
		r = getmem(ra, n->Ntype, 1, savaddr);

		code1(P_ADD, r, size);	/* Add inc/dec value */

	    /* Now store byte back */
		if (!savaddr)
		    ra = gaddress(n);	/* else, re-use ra */
		stomem(r, ra, 1, 1);

		if (!pre)				/* For postfix, undo reg */
		    code1(P_SUB, r, size);		/* undo change */
		break;

	    default:
		int_error("gincdec: bad type %N", n);
		return NULL;
	    }
    if (volat)
	flushcode();		/* Finish up after volatile obj */
    return r;
}

/* GPRIMARY - Generate primary expression.
**
** This handles all primary expressions, which are composed of node ops
**	N_FNCALL,
**	Q_DOT,		(may be lvalue)
**	Q_MEMBER,	(always lvalue)
**	Q_IDENT,	(always lvalue)
**	N_ICONST, N_FCONST, N_PCONST, N_SCONST, N_VCONST, Q_ASM.
** The first three of those are not terminal nodes and may have further
** sub-expressions.
** Note that array subscripting is done as pointer arithmetic rather than
** using a specific operator.  Similarly, parenthesized expressions have
** no specific op since the parse tree structure reflects any parenthesizing.
**	This is where array and function names are caught and turned into
** pointers instead.  Arrays and functions are the only Q_IDENTs for which
** the node type (Ntype) is different from the symbol type (Stype)!  The
** symbol type will have the actual type of the name, whereas the node type
** will be that of "pointer to <Stype>".
**	Note special checking for fetching a value from "volatile"-qualified
** lvalues.  There are only four nodes that can be lvalues -- the three above,
** plus N_PTR which is handled in gunary().  Storing into those lvalues is
** handled by gassign() and gincdec().  Because the peephole optimizer is
** such a mess, we can't easily tell it to avoid volatile objects; instead
** we simply flush out all peephole code before and after generating the
** fetch from (or store into) a volatile object!  Crude, but should work.
*/
static VREG *
gprimary(NODE *n)
{
    VREG *q, *r;
    INT siz;
    int volat, t;

    switch (n->Nop)
	{

	case Q_IDENT:		/* Variable name */
	    if ((t = n->Nid->Stype->Tspec) == TS_FUNCT || t == TS_ARRAY )
		{
	   /* Check for funct/array. Make sure Ntype is ptr */
		if (n->Ntype->Tspec != TS_PTR)
/* Later make this error again */
		    int_warn("gprimary: array/funct %N", n);
		return gaddress(n);	/* Yup, just return ptr to object */
		}
	/* Normal variable or structure/union */
	    if ((volat = tisanyvolat(n->Ntype)) != 0)
		flushcode();		/* If volatile, avoid optimization */

	    if (Register_Id(n))	/* approximate getmem() for registers */
#if 0
		r = rgetmem(gaddress(n), n->Ntype, tisbyte(n->Ntype), 0);
#else
		r = rgetmem(gaddress(n), n->Ntype, 0);
#endif
	    else
		r = getmem(gaddress(n), n->Ntype, tisbyte(n->Ntype), 0);

	    if (volat)
		flushcode();
	    return r;

	case N_SCONST:		/* Literal string - get char pointer to it */
	    n->Nsclab = newlabel();
	    n->Nscnext = litstrings;	/* link on string stack */
	    litstrings = n;			/* include this one */
	    r = vrget();
	    r->Vrtype = n->Ntype;		/* Set C type of object in reg */
	/* Get byte ptr to str, using given bytesize of type! */
	    code10(P_MOVE, r, n->Nsclab, elembsize(n->Ntype), 0);
	    return r;

	case N_VCONST:		/* Void "constant" */
	    return NULL;		/* No register used! */
	case N_ICONST:		/* Integer constant */
	case N_PCONST:		/* Pointer constant uses same cell etc */
	    r = vrget();
	    r->Vrtype = n->Ntype;	/* Set C type of object in reg */
	    code1(P_MOVE, r, n->Niconst);
	    return r;

	case N_FCONST:		/* Floating-point constant */
	    switch (n->Ntype->Tspec)
		{
		case TS_FLOAT:
		    r = vrget();
		    r->Vrtype = n->Ntype;	/* Set C type of object in reg */
		    code9(P_MOVE, r, n->Nfconst, 0);
		    break;
		case TS_DOUBLE:
		case TS_LNGDBL:
		    r = vrdget();
		    r->Vrtype = n->Ntype;	/* Set C type of object in reg */
		    code9(P_DMOVE, r, n->Nfconst, 1);
		    break;
		}
	    return r;

	case Q_ASM:
	    gasm(n);
	    return NULL;		/* Currently never returns anything */

	case N_FNCALL:		/* Function call */
	    return gcall(n);

	case Q_DOT:			/* (). direct component selection */
	    if (!(n->Nleft->Nflag & NF_LVALUE))
		break;		/* Ugh, do hairy stuff if not lvalue! */

	    if ((debcsi == KCC_DBG_NULL) && (n->Nleft->Nop == Q_MEMBER))
		{
		_chnl = n->Nleft->sfline;
		if (n->Nleft->Nleft->Nop == N_CAST)
		    code4 (P_NULPTR, (VREG *) NULL, gaddress (n->Nleft->Nleft->Nleft));
		else
		    code4 (P_NULPTR, (VREG *) NULL, gaddress (n->Nleft->Nleft));
		}

	/* OK, fall thru to handle like Q_MEMBER */

	case Q_MEMBER:		/* ()-> indirect component selection */
	    if ((volat = tisanyvolat(n->Ntype)) != 0)
		flushcode();		/* Ugh, avoid optimiz of volatile */

#if 0 /* KAR-1/92, leaving this as #if 0 in case I need it later */
	    if ((debcsi == KCC_DBG_NULL) && (n->Nop == Q_MEMBER))
		{
		_chnl = n->sfline;
		code4 (P_NULPTR, (VREG *) NULL, gaddress (n->Nleft));
		}
#endif

	    if (Register_Id(n))	/* approximate getmem() for registers */
#if 0
		r = rgetmem(gaddress(n), n->Ntype,
			    (n->Nxoff < 0) || tisbyte(n->Ntype), 0);
#else
		r = rgetmem(gaddress(n), n->Ntype, 0);
#endif
	    else
		r = getmem(gaddress(n), n->Ntype,
			    (n->Nxoff < 0) || tisbyte(n->Ntype), 0);
	    if (volat)
		flushcode();
	    return r;

	case Q_MUUO:
	    return gmuuo(n);

	default:
	    int_error("gprimary: bad op %N", n);
	    return NULL;
	}

    /* Hairy stuff for Q_DOT of something that isn't an lvalue.
    ** This can only happen for a struct returned from a function call.
    ** The structure resulting from the expression will either be
    ** completely contained in the registers (if size <= 2) or the register
    ** will contain the structure address.
    */
    if ((siz = sizetype(n->Nleft->Ntype)) > 2)	/* Find # wds in it */
	/* Fake out gaddress into using genexpr instead of another gaddress
	** when evaluating the structure expression, since result will
	** be a pointer.
	*/
	{
	n->Nop = Q_MEMBER;

	if (Register_Id(n))	/* approximate getmem() for registers */
#if 0
	    return rgetmem(gaddress(n), n->Ntype,
			(n->Nxoff < 0) || tisbyte(n->Ntype), 0);
#else
	    return rgetmem(gaddress(n), n->Ntype, 0);
#endif
	else
	    return getmem(gaddress(n), n->Ntype,
			(n->Nxoff < 0) || tisbyte(n->Ntype), 0);
	}

    /* Pull component out of structure in 1- or 2-word register */
    r = genexpr(n->Nleft);	/* Get the structure */
    switch (n->Nxoff)		/* See which part of it we want */
	{
	case 0:			/* Want first word? */
	    if (siz == 2 && sizetype(n->Ntype) == 1)
		vrnarrow(r);	/* Keep 1st word of a 2-word value */
	    return r;
	case 1:			/* Want second word? */
	    vrnarrow(r = VR2(r)); /* Keep second word of a 2-word value */
	    return r;

	default:			/* Bitfield of some kind */
	/* NOTE: This generates a very uncommon use of PTA_BYTEPOINT
	** wherein the E field of the byte pointer is actually a register
	** address.  This is why vrreal() is called, to get the
	** actual register number.  As long as this is one of the return-value
	** registers as it should be, this usage is probably safe from
	** the peephole optimizer.
	*/
	    q = vrget();		/* Get another register */
	    q->Vrtype = n->Ntype;	/* Set C type of object in reg */
	    (void) vrstoreal(q, r);	/* Make sure both regs are active! */
	    codebp(P_LDB, vrreal(q), (unsigned)((- (n->Nxoff)) & 07777) << 6,
		   0, NULL, vrreal(r) + ((unsigned)(-(n->Nxoff)) >> 12));
	    vrfree(r);		/* don't need rest of struct */
	    return q;
	}
}

/* GCALL - Generate function call
*/

static
VREG*
gcall (NODE* n)
    {
    NODE*	l;
    INT		narg,
		siz;
    VREG*	r;
    SYMBOL*	arg;


    if (n->Nleft->Ntype->Tspec != TS_FUNCT)
	int_error ("gcall: non-function %N", n);

    /* Check to see if OK to try for tail recursion */

    if (!optgen				/* Not optimizing? */
	|| stkgoto			/* Function contains a setjmp call? */
	|| stackrefs)			/* Fn makes addr refs to stack? */
	n->Nflag &=~ NF_RETEXPR;	/* If any of the above, forget it. */

    /* Check for args in same order - if ok, can tail recurse */

    l = n->Nright;
    siz = sizetype(n->Ntype);		/* calculate size of return value */

    if (n->Ntype->Tspec == TS_ARRAY)	/* Someday flush this, I hope */
	{
	int_error ("gcall: array type %N", n);
	siz = 0;
	}

    narg = -1;

    while ((n->Nflag & NF_RETEXPR) && l != NULL)
	{
	if (l->Nop == N_EXPRLIST)
	    {
	    arg = (l->Nright->Nop == Q_IDENT? l->Nright->Nid : NULL);
	    l = l->Nleft;
	    }
	else
	    {
	    arg = (l->Nop == Q_IDENT? l->Nid : NULL);
	    l = NULL;
	    }

	if (arg == NULL || (arg->Sclass != SC_ARG && arg->Sclass != SC_RARG))
	    n->Nflag &=~ NF_RETEXPR;
	else
	    {
	    if (narg == -1)
		narg = arg->Svalue;
	    else if (narg != arg->Svalue)
		n->Nflag &=~ NF_RETEXPR;

	    narg -= sizetype(arg->Stype);

	    if (narg < 0)
		n->Nflag &=~ NF_RETEXPR;
	    }
	}

    if (siz > 2)
	narg -= 1;		/* account for retval (struct *) */

    if (n->Nright == NULL)
	narg = 0;	/* no args always matches */

    /*
     * If we still think we can tail recurse, do it.
     *
     * NOTE: profiling precludes tail recursion: MVS 09/20/89
     */

    if (!profbliss)			/* for BLISS profiler */
	{
	if (narg == 0 && (n->Nflag & NF_RETEXPR))
	    {
	    r = gaddress (n->Nleft);	/* get address of function first */
	    code8(P_ADJSP, VR_SP, -stackoffset); /* before we lose marbles */
	    code4(P_JRST, (VREG *)NULL, r); /* now we can jump to it */
	    return NULL;		/* can't want a return value */
	    }
	}

    if (n->Nleft->Nid->Sflags & TF_FORTRAN)	    /* FORTRAN fn */
	XF4_call_spill = (char) ~0;		/* spill preserved regs if XF4 call */ // FW KCC-NT

    vrallspill();			/* save active non-preserved regs */

    XF4_call_spill = 0;

    /*
     * Next push function arguments
     */

    l = n->Nright;
    narg = stackoffset;			/* remember argument block start */

    /*
     * Choose bliss, fortran, interrupt, or normal C function argument linkage
     */

    if (n->Nleft->Nid->Sflags & TF_BLISS)
	emit_blissargs (l);		/* bliss linkage */
    else if (n->Nleft->Nid->Sflags & TF_FORTRAN)
	{
	/* fortran linkage */

	code1(P_MOVS, (r = vrget()), (- sizeargs(l)) & 0777777L);
	code0(P_PUSH, VR_SP, r);	/* Start with -<# arg wds>,,0 */
	stackoffset++;
	emit_blissargs(l);		/* now push args in BLISS order */
	}
    else if (n->Nleft->Nid->Sflags & TF_INTERRUPT)
	{
	/*
	 * FW 2A(52)
	 *
	 * This is an interrupt function.  As such, it needs a special
	 * prolog and epilog.
	 */

	}
    else				/* ...No, it's a C fn */
	while (l != NULL)
	    {
	    if (l->Nop == N_EXPRLIST)
		{
		gfnarg(l->Nright);
		l = l->Nleft;
		}
	    else
		{
		gfnarg(l);
		break;
		}
	    }

    if (siz > 2)		/* Push struct addr on stack as 1st arg */
	{
	r = vrget();
	code13(P_MOVE, r, (n->Nretstruct->Svalue + 1) - stackoffset);
	code0(P_PUSH, VR_SP, r);
	stackoffset++;
	}

    narg -= stackoffset;	/* calculate neg number of arg words */

    if (n->Nleft->Nid->Sflags & TF_FORTRAN)	/* for a FORTRAN fn */
	{
	/*
	 * Do FORTRAN call.  Must get function address first, in case it is
	 * an expression that might possibly clobber AC16, and then point
	 * AC16 (R_FAP) to the start of our args on stack.
	 */

	r = gaddress(n->Nleft);		/* Get function addr first */
	code13(P_MOVE, VR_FAP, narg+2); /* Point to just after count */
	code4(P_PUSHJ, VR_SP, r);	/* Call function */
	}
    else if (n->Nleft->Nop == Q_IDENT)
	code6(P_PUSHJ, VR_SP, n->Nleft->Nid);	/* optimization */
    else
	code4(P_PUSHJ, VR_SP, gaddress(n->Nleft)); /* call function or expr */

    /*
     * flush args off stack
     */

    if (narg)
	{
	code8 (P_ADJSP, VR_SP, narg);
	stackoffset += narg;
	}

    if (siz == 1)
	r = vrretget ();		/* one return register */
    else if (siz == 2)
	r = vrretdget ();		/* two */
    else if (siz > 2)
	{
	/* Can optimize better if we re-generate the addr we gave as arg. */

	code13 (P_MOVE, (r = vrretget ()),
		(n->Nretstruct->Svalue + 1) - stackoffset);
	}
    else
	return NULL;			/* Returning void */

    if (n->Nleft->Nid->Sflags & TF_FORTRAN)	/* for a FORTRAN fn */
	{
	/* FORTRAN functions return values in regs 0+1 instead of 1+2 */

	code0((siz == 2) ? P_DMOVE : P_MOVE, r, VR_ZERO);
	code5(P_SETZ, VR_ZERO);	/* This may not be necessary */
	}

    r->Vrtype = n->Ntype;		/* Set C type of result obj */
    return r;
    }

/*
 * void emit_blissargs (NODE*)
 *
 * This little recursive function traverses a NODE tree
 * and generates calles to gfnarg () such as to emit pushes
 * for a function's arguments in the reverse of the usual order.
 */

static
void
emit_blissargs (NODE *l)
    {
    if (l)
	{
	if (l->Nop == N_EXPRLIST)
	    {
	    emit_blissargs(l->Nleft);
	    gfnarg(l->Nright);
	    }
	else
	    gfnarg(l);
	}
    }

/* Count # words needed by all args ahead of time, so FORTRAN linkage
** can use it without backpatching.
*/

static INT
sizeargs(NODE *l)
{
    INT size = 0;
    for (; l; l = l->Nleft)
	{
	if (l->Nop == N_EXPRLIST)
	    size += sizetype(l->Nright->Ntype);
	else
	    {
	    size += sizetype(l->Ntype);
	    break;
	    }
	}
    return size;
}

/* GFNARG - generate function argument value and push on stack
**
*/
static void
gfnarg(NODE *n)
{
    VREG *reg;
    INT siz;

    siz = sizetype(n->Ntype);
    if (n->Ntype->Tspec == TS_ARRAY)
	{
	int_error("gfnarg: array type %N", n);
	siz = 0;
	}

#if 0		/* Reg linkage */
    if (Register_Id(n))
	{
	if (siz == 1)
	    {
	    code00(P_PUSH, R_SP, n->Nid->Sreg);
	    stackoffset++;
	    }
	else
	    int_error ("gfnarg: size of reg var %s > 1", n->Nid->Sname);
	return;
	}
#endif

    switch (siz)
	{
	case 1:
	    code0(P_PUSH, VR_SP, genexpr(n));
	    stackoffset++;
	    break;
	case 2:
	    reg = genexpr(n);
	    code0(P_PUSH, VR_SP, reg);
	    code0(P_PUSH, VR_SP, VR2(reg));
	    vrfree(reg);
	    stackoffset += 2;
	    break;

	default:
	    reg = vrget();
	    code8(P_ADJSP, VR_SP, siz);	/* Make space on the stack */
	    stackoffset += siz;		/* remember where we are on stack */
	    code13(P_MOVE, reg, -(siz-1));	/* Get pointer to the space */
	    code4s(P_SMOVE, reg, genexpr(n), 0, siz);	/* Copy, release reg */
	    vrfree(reg);
	}
}

/* GADDRESS - Generate address of object or function.
**	Will set up as byte pointer if necessary
*/
static VREG *
gaddress(NODE *n)
{
    int boff, bsiz;
    INT offset;
    VREG *r, *p;
    SYMBOL *s;

    switch (n->Nop)
	{
	case Q_ASPLUS:	/* ptr += &a[] */
	case Q_PLUS:
	case Q_ASMINUS:	/* ptr -= &a[] */
	case Q_MINUS:
	case N_PTR:
	    return genexpr(n->Nleft);

	case Q_DOT:
	case Q_MEMBER:

	    if (n->Nop == Q_MEMBER)
		{
		if (debcsi == KCC_DBG_NULL)
		    {
		    _chnl = n->sfline;
		    switch (n->Nleft->Nop)
			{
			case N_CAST:
			case N_ADDR:
			case Q_ASGN:
			case N_FNCALL:
			    if (n->Nleft->Nleft->Nop == Q_IDENT)
				code4 (P_NULPTR, (VREG *) NULL, gaddress (n->Nleft->Nleft));
			    else
				_chnl = -1;
			    break;
			default:
			    code4 (P_NULPTR, (VREG *) NULL, gaddress (n->Nleft));
			    break;
			}
		    }
		r = genexpr (n->Nleft);
		}
	    else
		r = gaddress (n->Nleft);

	    offset = n->Nxoff;		/* calculate offset */

	/* Check for attempt to get address of object within a word. */
	    if (offset < 0)		/* bitfield or byte? */
		{
		offset = -offset;		/* Get back original encoding */
		if (!tisbitf(n->Ntype))	/* If not bitfield, assume byte */
		    {
		    bsiz = (int) (offset & 077);	/* Get byte size of object */
		    boff = (int) (((TGSIZ_WORD	/* Get offset in bytes */
			      - ((offset & 07700) >> 6)
			    ) / bsiz) - 1);
		    offset = (unsigned INT)offset >> 12;	/* Get wd offset */
		    if (offset > 0)
			code1(P_ADD, r, offset);	/* Do word offset */
		    pitopc(r, bsiz, boff, 1);	/* Turn addr into byte ptr */
		    return r;
		    }

	    /* True bitfield.
	    ** Note that although C does not allow pointers to bitfields, we
	    ** still want to generate bitfield "addresses" for internal use
	    ** so that the code generation can avoid lots of special-casing.
	    */
		p = vrget();		/* Need another reg */
		p->Vrtype = n->Ntype;	/* Set C type of object in reg */
		(void) vrstoreal(r, p);	/* Ensure both regs active! */
		codebp(P_MOVE, vrreal(p),	/* Construct local byte pointer */
			    (unsigned INT)(offset&07777) << 6,
			    vrreal(r), NULL, (unsigned INT)offset >> 12);

	    /* Now we release the struct address, even though it is still
	    ** needed as index by the byte pointer we created!  This is
	    ** should be safe as long as the resulting address is used
	    ** immediately -- for bitfields this should always be true since
	    ** bitfield addresses cannot have an independent existence.
	    */
		vrfree(r);
		return p;
		}

	    if (offset > 0)
		code1(P_ADD, r, offset);	/* perform offset */
	    if (tisbytearray(n->Ntype))	/* If addr of byte array, */
		pitopc(r, elembsize(n->Ntype), 0, 1);	/* make BP to start */
	    else if (tisbyte(n->Ntype))	/* If addr of single byte, */
		pitopc(r, tbitsize(n->Ntype),	/* point to low byte */
			    (TGSIZ_WORD/tbitsize(n->Ntype))-1, 1);
	    return r;

	case Q_IDENT:
	/* Note type checked is that of the symbol's, not that of the
	** node's.  This ensures we do the right thing when the ident
	** is that of a function or array.
	*/
	    r = vrget();
	    r->Vrtype = n->Ntype;		/* Set C type of object in reg */
	    s = n->Nid;
	    if (tisbytearray(s->Stype))	/* If ident is byte array, */
		{
		bsiz = elembsize(s->Stype);		/* set byte params */
		offset = 0;				/* with left-justified byte */
		}
	    else if (tisbyte(n->Ntype))	/* If it's a single byte, */
		{
		bsiz = tbitsize(n->Ntype);		/* also set them */
		offset = (TGSIZ_WORD/bsiz)-1;	/* with right-justified byte */
		}
	    else
		bsiz = 0;

	    switch (s->Sclass)
		{
		case SC_RAUTO:		/* value already moved to reg */
		    code00(P_MOVE, r->Vrloc, s->Sreg);
		    return r;

		case SC_AUTO:		/* Local variables */
		    code13(P_MOVE, r, (s->Svalue + 1) - stackoffset);
		    break;

		case SC_RARG:		/* Function parameters */
		    code00(P_MOVE, r->Vrloc, s->Sreg);
		    return r;

		case SC_ARG:
		    code13(P_MOVE, r, (- s->Svalue) - stackoffset);
		    break;

		case SC_ENUM:
		    int_error("gaddress: enum tag: %S %N", s, n);
		    return r;

		case SC_ISTATIC:	/* Internal static */
		    s = s->Ssym;	/* uses internal label instead */

		case SC_XEXTREF:
		case SC_EXLINK:	/* Anything with linkage */
		case SC_EXTDEF:
		case SC_EXTREF:
		case SC_INTDEF:
		case SC_INTREF:
		case SC_INLINK:
		    if (bsiz)				/* If byte pointer is addr, */
			code10(P_MOVE, r, s, bsiz, offset);	/* make BP */
		    else
			code3(P_MOVE, r, s);	/* else just make addr */
		    return r;

		default:
		    int_error("gaddress: bad Sclass %d %N", s->Sclass, n);
		    return r;
		}

	    if (bsiz)			/* If addr is a byte addr, */
		pitopc(r, bsiz, offset, 1);	/* make BP */
	    return r;

	default:
	    int_error("gaddress: bad op %N", n);
	    return 0;
	}
}

/* GETMEM - Get object from memory, given address in register.
**	Releases the register unless the "keep" flag is set.
*/

VREG *
getmem(VREG *reg, TYPE *t, int byte, int keep)
{
    VREG *q;

    switch (sizetype(t))
	{
	case 1:
	    q = vrget();
	    q->Vrtype = t;		/* Set C type of object in reg */
	    if (byte)
		(keep ? codek0(P_LDB, q, reg) : code0(P_LDB, q, reg));
	    else
		(keep ? codek4(P_MOVE, q, reg) : code4(P_MOVE, q, reg));
	    return q;

	case 2:
	    q = vrdget();
	    q->Vrtype = t;		/* Set C type of object in reg */
	    (keep ? codek4(P_DMOVE, q, reg) : code4(P_DMOVE, q, reg));
	    return q;

	default:
	    return reg;
	}
}


/* STOMEM - Store register into memory; inverse of GETMEM.
**	Releases the address register, returns the value register
**	for possible further processing.
*/
VREG *
stomem(VREG *reg, VREG *ra, INT siz, int byteptr)		
/* Reg w/value to store (NULL if stacked struct), Reg w/address to 
 * store into, Size of object in words, True if "address" is a byte pointer 
 */
{
    switch (siz)
	{
	case 1:			/* Store single word or byte */
	    if (byteptr)
		code0(P_DPB, reg, ra);
	    else
		code4(P_MOVEM, reg, ra);
	    break;

	case 2:			/* Store doubleword */
	    code4(P_DMOVEM, reg, ra);
	    break;

	default:			/* Store a stacked structure */
	/* ra has dest addr, reg has source addr */
	    code4s(P_SMOVE, ra, reg, 0, siz);	/* Copy, release reg */
	    return ra;
	}
    return reg;
}

/* RGETMEM - Get object from register and do not release register,
**	     imitating GETMEM for registers.
*/

static
VREG *
#if 0
rgetmem(VREG *reg, TYPE *t, int byte, int keep)
#else
rgetmem(VREG *reg, TYPE *t, int keep)
#endif
{
    VREG *q;

    if (sizetype(t) == 1)
	{
	q = vrget();
	q->Vrtype = t;		/* Set C type of object in reg */
#if 0
	if (byte)
	    codek0(P_LDB, q, reg);
	else
	    codek0(P_MOVE, q, reg);

	if (byte)
	    (keep ? codek0(P_LDB, q, reg) : code0(P_LDB, q, reg));
	else
#else
	if (keep)
	    codek0(P_MOVE, q, reg);
	else
	    code0 (P_MOVE, q, reg);
#endif
	return q;
	}
    else
	{
	int_error("rgetmem: register var of non-arithmetic type");
	return reg;
	}
}
#if 0
/* RSTOMEM - Store register into register; inverse of RGETMEM.
**	     Do not release register, imitating STOMEM for registers.
**	
*/
VREG *
rstomem(VREG *reg, int ra, INT siz, int byteptr)
/* Reg w/value to store (NULL if stacked struct), Real reg to 
 * store into, Size of object in words, True if "address" is a byte pointer 
 */
{
    if (siz == 1)
	code00(P_MOVE, ra, reg->Vrloc);
    else	/* reg not defined for arrays or stacked structures */
	{
	int_error("rstomem: register var of non-arithmetic type");
	}
    return reg;
}
#endif

/* PITOPC - Construct byte pointer from word pointer.
**	Currently the only offsets used are either 0 (for left justified byte)
** or <# bytes-per-word>-1 (for right justified byte).  Note that the latter
** can result in unused low-order bits if the byte size does not completely
** fill the word.
**
** Turn a word pointer into a byte pointer.  So that our programs
** should run in extended addressing as well as in section 0, we
** must be able to create either local or global byte pointers,
** so we add in our P/S fields from a table instead of literally.
**
** Even if we know that the pointer will point to the same section
** that the code is in, we cannot use a local byte pointer, because
** pointers are local to where they are stored rather than to where
** the PC currently is.
**
** If the pointer is merely going to be used to load or deposit
** a byte, it will get turned into a local byte pointer later by
** the peepholer (see localbyte() in CCOPT).
*/
static void
pitopc(VREG *r, int bsiz, int offset, int safe)	
/* byte size in bits,  # bytes offset from start of word addr in R, 
 * and Set if pointer known to be non-NULL, needn't test for 0.
 */
{
    if (!safe)				/* Unless we already know not NULL */
	code0(P_SKIP+POF_ISSKIP+POS_SKPE, r, r);	/* NULL stays NULL */
    code10(P_IOR, r, (SYMBOL *)NULL, bsiz, offset);	/* Make it a pointer */
}

/* BPTRREF - sees if expression value consists of a byte pointer
**	reference.
** Returns:
**	1 if expression is a legal lvalue referenced via byte ptr.
**	0 if expression is a legal lvalue referenced via word address.
**	-1 if expression is not an lvalue operand.
*/
static int
bptrref(NODE *n)
{
    switch (n->Nop)
	{
	case Q_DOT:
	case Q_MEMBER:
	    return (n->Nxoff < 0			/* bitfield? */
		    || tisbyte(n->Ntype));	/* or char? */

	case N_PTR:
	    return tisbytepointer(n->Nleft->Ntype);	/* byte pointer deposit */

	case Q_IDENT:
	    return tisbyte(n->Ntype);

	default:
	    return -1;
	}
}

/* GASM - Generate direct assembly language constructs
**
*/
static void
gasm(NODE *n)
{
    NODE *arg;

    if ((arg = n->Nleft) == 0)
	{
	int_error("gasm: no arg %N", n);
	return;
	}
    if (arg->Nop != N_SCONST)
	{
	int_error("gasm: non-string arg %N", n);
	return;
	}
    /* Output the string, minus the terminating null char */
    codestr(arg->Nsconst, arg->Nsclen-1);
}


/* KAR-6/91, Changed sentinal value for _chnl to -1 from 0 */
int		_chnl = -1;
#define mnem_param	temp->Nleft->Nleft->Nleft
#define ac_param	temp->Nleft->Nleft
#define ret_param	temp->Nleft->Nright
#define ea_param	temp->Nright
#define ch_sig		n->Nleft

/* --------------------------- */
/*	imuuo statement	       */
/*		-by KAR 1/91   */
/* --------------------------- */
VREG *
gmuuo(n)
NODE *n;
{
    VREG    *ac, *ret_ac;
    int	    p3_omitted = 0, p4_omitted = 0;
    NODE    *temp = n->Nleft->Nleft;	/* else, BC++ fails!? */

    if ((ret_param->Nop == N_ICONST) && (ret_param->Niconst == 0))
	p3_omitted = 1;
    if ((ea_param->Nop == N_ICONST) && (ea_param->Niconst == 0))
	p4_omitted = 1;
	
	/* The following code is outputed:
	 *	MOVE   ac,ac_contents
         *      muuo   ac,
         *      TDZA   1,1
         *      MOVEI  1,1
	 *      MOVEM  ac,ret_val	; if a return address is specified
	 */
    ac = vrget();
    if (ch_sig->Nright->Niconst == 0)
	code0(P_MOVE, ac, genexpr(ac_param->Nright));
    else
	_chnl = ac_param->Nright->Niconst;
    if (p4_omitted != 1)
	code4m(P_MUUO, ac, genexpr(ea_param),mnem_param->Nright->Nsconst);
    else
	code5m(P_MUUO, ac, mnem_param->Nright->Nsconst);
    ret_ac = vrget();
    code0(P_TDZ+POF_ISSKIP+POS_SKPA, ret_ac, ret_ac);
    code1(P_MOVE, ret_ac, 1);
    if (p3_omitted != 1)
	switch (ret_param->Nid->Sclass)
	    {
	    case SC_ARG:
	    case SC_AUTO:
		code4(P_MOVEM, ac, gaddress(ret_param));
		break;
	    case SC_RARG:
	    case SC_RAUTO:
	    case SC_REGISTER:
		code0(P_MOVEM, ac, gaddress(ret_param));
		break;
	    default:
		code6(P_MOVEM, ac, ret_param->Nid);
		break;
	    } /* switch */
    vrfree(ac);
    return(ret_ac);
}
