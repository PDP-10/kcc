/*	CCREG.C - Register management
**
**	(c) Copyright Ken Harrenstien 1989
**		All changes after v.45, 6-Apr-1988
**	(c) Copyright Ken Harrenstien, SRI International 1985, 1986
**		All changes after v.26, 8-Aug-1985
**
** Original version by David Eppstein / Stanford University / 8 Mar 1985
*/
#include <stdlib.h>	/* calloc() */
#include "cc.h"
#include "ccgen.h"

/* Exported functions */
void vrinit(void);		/* Init regs for new routine */
void vrendchk(void);	/* Check regs at end of routine */
VREG *vrget(void), *vrdget(void); /* Allocate virtual register or pair */
VREG *vrretget(void), *vrretdget(void);	/* Same but use return-value regs */
void vrfree(VREG *);		/* Release vreg */
void vrallspill(void);	/* Spill all active regs onto stack */
VREG *vrwiden(VREG *reg, int low);	/* Widen a vreg into a pair */
void vrlowiden(VREG *);	/* Common case: widen vreg in low direction */
void vrnarrow(VREG *);	/* Narrow a vreg pair into single vreg */
int vrreal(VREG *);		/* Get real reg # for active virtual reg */
int vrtoreal(VREG *);		/* Ensure reg is active, return real # */
int vrstoreal(VREG *, VREG *);	/* Same for 2 regs, return # of 1st */
int vrispair(VREG *);		/* TRUE if vreg is 1st of a vreg pair */
void vrufcreg(VREG *);	/* Undo MOVE of failed changereg w/o freeing */
int rfree(int);		/* TRUE if real register is assigned */
#if 0
int rhasval();		/* TRUE if real register has a value */
#endif


/* Imported functions */
extern PCODE *before(PCODE *);				/* CCCODE */
extern void code00(int, int, int), code8(int, VREG *, INT),
	code12(int, VREG *, INT);	/* CCCODE */
extern int ufcreg(int);				/* CCCREG */

/* Internal functions */
static void vr1free(VREG *), vrspill(VREG *), vr1spill(VREG *);
static int rrfind(void);
static void updrfree(void);
static int rrdfind(void);
static VREG *vrsetrr(VREG *, int), *vr1setrr(VREG *, int);
static void vrunlink(VREG *), vr1unlink(VREG *);
static VREG *vrlink(VREG *, VREG *), *vr1link(VREG *, VREG *),
       *vralloc(void), *vrdalloc(void);
#if	DEBUG_KCC
static void vrdumplists (void);
static char *vrdumpflags (int);
#endif

#define empty(vr) ((vr)->Vrnext == (vr))	/* TRUE if list is empty */

/* virtual regs not now in use */
static VREG freelist = {0, 0, (TYPE *)NULL, 0, &freelist, &freelist};

/* regs associated with real regs */
static VREG reglist = {0, 0, (TYPE *)NULL, 0, &reglist, &reglist};

/* regs spilled onto the stack */
static VREG spillist = {0, 0, (TYPE *)NULL, 0, &spillist, &spillist};

static VREG *regis[NREGS];	/* who is using which registers */
static int regfree[NREGS];	/* what regs are used in code */

/* Special vregs which the macros VR_RETVAL and VR_SP point to.  These
** exist only so routines can force the use of certain physical regs in
** special places, by providing the appropriate phys reg value.  These
** vregs are never strung on a list or used in any other way.
*/
VREG vr_retval	= { VRF_SPECIAL|VRF_LOCK, R_RETVAL };
VREG vr_sp	= { VRF_SPECIAL|VRF_LOCK, R_SP };

#if SYS_CSI	/* For FORTRAN linkage */
VREG vr_fap	= { VRF_SPECIAL|VRF_LOCK, R_FAP  };
VREG vr_zero	= { VRF_SPECIAL|VRF_LOCK, R_ZERO };
#endif

/*
** VRINIT - Initialize for start of new code
**	Called at the start of each function or code initializer by inicode()
** VRENDCHK - Perform wrap-up checks at end of code.
**	Called at end of each function or initializer by gend().
*/

void
vrinit(void)
{
    int i;


#if	DEBUG_KCC
    puts ("vrinit ()");
#endif

    vrendchk();
    for (i = 0; i < NREGS; i++)
	regis[i] = NULL;
}

void
vrendchk(void)
{
#if	DEBUG_KCC
    puts ("vrendchk ()");
#endif

    if (!empty(&reglist) || !empty(&spillist))
	{
	int_warn("vrendchk: leftover regs");
	/* Try to release all regs */

	while (!empty(&reglist))
	    {
#if	DEBUG_KCC
	    printf ("reglist=%d\t", reglist.Vrnext->Vrloc);
#endif
	    vrfree(reglist.Vrnext);
	    }

	while (!empty(&spillist))
	    {
#if	DEBUG_KCC
	    printf ("spillist=%d\t", spillist.Vrnext->Vrloc);
#endif
	    vrfree(spillist.Vrnext);
	    }
	}
}




/* VRGET -  Assign a new virtual register with corresponding real register.
*/
VREG *
vrget(void)
{
#if	DEBUG_KCC
    puts ("vrget ()");
#endif

    return vr1link(vr1setrr(vralloc(), rrfind()), &reglist);
}

/* VRDGET - Same as VRGET but assigns a double-word register.
*/
VREG *
vrdget(void)
{
#if	DEBUG_KCC
    puts ("vrdget ()");
#endif

    return vrlink(vrsetrr(vrdalloc(), rrdfind()), &reglist);
}

/* VRRETGET - Get a register for holding a return value
*/
VREG *
vrretget(void)
{
#if	DEBUG_KCC
    puts ("vrretget ()");
#endif

    if (regis[R_RETVAL] != NULL)	/* Ensure return reg is free */
	vrspill(regis[R_RETVAL]);
    return vr1link(vr1setrr(vralloc(), R_RETVAL), &reglist);
}

/* VRRETDGET - Get a double register for holding return value
*/
VREG *
vrretdget(void)
{
#if	DEBUG_KCC
    puts ("vrretdget ()");
#endif

    if (regis[R_RETVAL] != NULL)
	vrspill(regis[R_RETVAL]);
    if (regis[R_RETDBL] != NULL)
	vrspill(regis[R_RETDBL]);
    return vrlink(vrsetrr(vrdalloc(), R_RETVAL), &reglist);
}

/* VRFREE - Forget about a no-longer-in-use register
**
*/
void
vrfree(VREG *vr)
{
#if	DEBUG_KCC
    printf ("vrfree (%%%o)\n", vr);
#endif

    if (vr->Vrflags & VRF_REGPAIR)	/* If 1st of a pair, */
	vr1free(vr->Vrmate);		/* free 2nd first */
    vr1free(vr);
}

static void
vr1free(VREG *vr)
{
#if	DEBUG_KCC
    printf ("vr1free (%%%o)\n", vr);
#endif

    if (vr->Vrflags & VRF_SPECIAL)
	return;
    if (vr->Vrflags & VRF_SPILLED)
	int_warn("vr1free: spilled reg");
    else
	regis[vr->Vrloc] = NULL;	/* Say real reg now free */
    vr1unlink(vr);			/* Unlink reg(s), move to freelist */
    vr1link(vr, &freelist);
    /* No need to clear flags as vralloc() will do this. */
}




/* VRALLSPILL - Spill all registers
**	This is needed to save values over subr calls and conditional exprs.
*/
void
vrallspill(void)
{
#if	DEBUG_KCC
    puts ("vrallspill ()");
#endif

    while (!empty(&reglist))
	vrspill(reglist.Vrnext);
}

/* VRSPILL - Spill a virtual register.
**	Either we needed to reallocate it or we are calling a function.
**	In either case the register moves onto the stack.
*/
static void
vrspill(VREG *vr)
{
#if	DEBUG_KCC
    printf ("vrspill (%%%o)\n", vr);
#endif

    vr1spill(vr);		/* Must ALWAYS push 1st reg first!!!! */
    if (vr->Vrflags & VRF_REGPAIR)
	vr1spill(vr->Vrmate);	/* then 2nd if a pair */
}

static void
vr1spill(VREG *vr)
{
#if	DEBUG_KCC
    printf ("vr1spill (%%%o)\n", vr);
#endif

    if (vr->Vrflags & VRF_SPILLED)
	efatal("vr1spill: reg already spilled");
    vr1unlink(vr);			/* remove from assigned list */

    if (Register_Nopreserve(vr->Vrloc) || XF4_call_spill)
	{
	spillist.Vrnext->Vroldstk = stackoffset;/* remember where we are */

#if	DEBUG_KCC
	printf (">>> calling code00 ();\n");
#endif

	code00(P_PUSH, R_SP, vr->Vrloc);  /* push on stack, don't release */

#if	DEBUG_KCC
	printf (">>> returned from code00 ();\n");
#endif

	regis[vr->Vrloc] = NULL;	  /* no longer here */
	vr->Vrloc = (int) ++stackoffset;  /* Stack bigger now, note new loc */
	vr->Vrflags |= VRF_SPILLED;
	vr1link(vr, &spillist);		/* it's now spilled */
	}
}

/* VRWIDEN - Make single register into a register pair and returns that.
**	If "low" argument non-zero, register becomes the low word.
**	Contents of additional word are indeterminate.
*/
VREG *
vrwiden(VREG *reg, int low)			/* Old existing register */
{
    VREG *nreg;			/* New added register */
    VREG *vr1, *vr2;		/* 1st and 2nd regs of pair */
    int rr;

#if	DEBUG_KCC
    printf ("vrwiden (%%%o, %s)\n", reg, (low ? "TRUE" : "FALSE"));
#endif

    if (reg->Vrflags & (VRF_REGPAIR|VRF_REG2ND))
	{
	efatal("vrwiden: reg already wide");
	return reg;
	}

    /* Turn a single vreg into a vreg pair */
    reg->Vrmate = nreg = vralloc();	/* Get another vreg to make a pair */
    nreg->Vrmate = reg;			/* Link them together */
    vr1link(nreg, reg);			/* Add new to same list after old */
    if (low)
	{
	vr1 = nreg, vr2 = reg;
	vr1->Vrloc = reg->Vrloc-1;
	}
    else
	{
	vr1 = reg, vr2 = nreg;
	vr2->Vrloc = reg->Vrloc+1;
	}
    vr1->Vrflags |= VRF_REGPAIR;
    vr2->Vrflags |= VRF_REG2ND;

    if (reg->Vrflags & VRF_SPILLED)
	{
	/* On stack, don't need to find reg */
	nreg->Vrflags |= VRF_SPILLED;	/* Pretend new reg also spilled */
	nreg->Vroldstk = reg->Vroldstk;	/* Preserve this just in case */
	return vr1;			/* (note reg not really on stk!) */
	}

    /* Existing reg is in a real reg, see if neighbor is free */
    if (regis[nreg->Vrloc] == NULL	/* New reg available? */
      && vr1->Vrloc >= R_RETVAL		/* And pair is within range of */
      && vr2->Vrloc <= r_maxnopreserve)	/* useable registers? FW 2A(47) */
	{
	regis[nreg->Vrloc] = nreg;	/* Win!  Take the new reg */
	return vr1;			/* and return */
	}

    /* Can't use neighboring real reg, find another pair of real regs.
    ** We lock old reg while hunting to avoid unpleasant surprises.
    */
    if (reg->Vrflags & VRF_LOCK)
	{
	int_warn("vrwiden: virtual register with rr=%d locked\n", reg->Vrloc);
	rr = rrdfind();
	}
    else
	{
	reg->Vrflags |= VRF_LOCK;
	rr = rrdfind();
	reg->Vrflags &= ~VRF_LOCK;
	}

#if	DEBUG_KCC
    printf (">>> calling code00 ();\n");
#endif

    code00(P_MOVE, (low ? rr+1 : rr), reg->Vrloc);	/* Copy old reg */

#if	DEBUG_KCC
    printf (">>> returned from code00 ();\n");
#endif

    regis[reg->Vrloc] = NULL;		/* Free old real reg, and */
    return vrsetrr(vr1, rr);		/* set up new ones instead */
}


/* VRLOWIDEN - Widen a single virtual reg in low direction.
**	No return value is needed since the vreg pointer doesn't change.
*/
void
vrlowiden(VREG *vr)
{
#if	DEBUG_KCC
    printf ("vrlowiden (%%%o)\n", vr);
#endif

    (void) vrwiden(vr, 0);	/* Widen, existing word becomes 1st */
}

/* VRNARROW - Extract one word of a doubleword register
**	The pointer furnished as arg must point to the 1st or 2nd reg of
**	the pair, and that one is retained while the other is flushed.
*/
void
vrnarrow(VREG *vr)
{
#if	DEBUG_KCC
    printf ("vrnarrow (%%%o)\n", vr);
#endif

    if (vr->Vrflags & (VRF_REGPAIR|VRF_REG2ND))
	{
	/* Flush flags and ensure that vr1free doesn't complain if
	** the vreg happens to be spilled at moment.
	*/
	vr->Vrflags &= ~(VRF_REGPAIR|VRF_REG2ND|VRF_SPILLED);
	vr1free(vr->Vrmate);		/* Flush other reg */
	}
    else
	efatal("vrnarrow: already narrow");
}

/* VRREAL - return real (physical) register # for an active virtual reg.
**	If reg is not active (is on stack) then generates internal error,
**	but tries to recover by getting it anyway.
*/
int
vrreal(VREG *vr)
{
#if	DEBUG_KCC
    printf ("vrreal (%%%o)\n", vr);
#endif

    if (!(vr->Vrflags & VRF_SPILLED))
	return vr->Vrloc;
    int_error("vrreal: using spilled reg");
    return vrtoreal(vr);
}

/* VRTOREAL - Return a physical register number for some virtual register
** Pulls it back off the stack if necessary
**
** FW, 2A(41) : now it may happen, e.g. in a comparison operation involving
** two doubles, that vrtoreal () is called from one of the code* () routines,
** and its argument is the _second_half_ of a double virtreg that is on the
** spilled list.  Formerly, this case was handled poorly; the second half
** would be unspilled, but the first half would remain spilled, and the
** lists would get out of sync, causing a failure later on, typically from
** rrdfind ().  The appropriate action is to unspill the double and return
** the realreg for its second half.
*/

int
vrtoreal (VREG *reg)
{
#if	DEBUG_KCC
    printf ("vrtoreal (%%%o)\n", reg);
#endif

    /* Check for spilled register now somewhere on stack */

    if (reg->Vrflags & VRF_SPILLED)
	{
	int	stkloc;
	VREG	*vr;


	/* Unspill.  First check for the second-half case, FW 2A(41) */

	vr = (reg->Vrflags & VRF_REG2ND) ? reg->Vrmate : reg; /* FW 2A(41) */
		
	vr->Vrflags &= ~VRF_SPILLED;
	vrunlink (vr);			/* Unlink from spill list */
	stkloc = vr->Vrloc;		/* Note location on stack */

	if (vr->Vrflags & VRF_REGPAIR)
	    {
	    vr->Vrmate->Vrflags &= ~VRF_SPILLED;
	    vrsetrr (vr, rrdfind());
	    code12 (P_DMOVE, vr, stkloc - stackoffset);
	    }
	else
	    {
	    vr1setrr (vr, rrfind());
	    code12 (P_MOVE, vr, stkloc - stackoffset);
	    }

	/* drop stack to top remaining spilled reg */

	if (vr->Vrnext == spillist.Vrnext)
	    {
	    code8 (P_ADJSP, VR_SP, spillist.Vrnext->Vroldstk - stackoffset);
	    stackoffset = spillist.Vrnext->Vroldstk;
	    }

	vrlink(vr, &reglist);
	}

    /*
     * In a realreg now no matter what, so just return it.
     * This works whether the given reg was a single or either half
     * of a double, FW 2A(41)
     */

    return reg->Vrloc;
}

/* VRSTOREAL - Same as vrtoreal but for two registers at once.
**	This ensures that we don't mistakenly spill one of the regs that
**	a two-reg instruction needs.
*/
int
vrstoreal(VREG *vr, VREG *vr2)
{
#if	DEBUG_KCC
    printf ("vrstoreal (%%%o, %%%o)\n", vr, vr2);
#endif

    if (vr->Vrflags & VRF_LOCK)	/* If 1st reg already locked, */
	{
	(void) vrtoreal(vr2);		/* use fast method that avoids */
	return vr->Vrloc;		/* unlocking the 1st reg when done. */
	}
    if (vr->Vrflags & VRF_SPILLED)	/* Nope, so ensure 1st reg active */
	(void) vrtoreal(vr);
    vr->Vrflags |= VRF_LOCK;		/* Lock it to that phys reg, */
    (void) vrtoreal(vr2);		/* while we ensure 2nd active too! */
    vr->Vrflags &= ~VRF_LOCK;		/* OK, can unlock 1st now */
    return vr->Vrloc;
}

/* VRISPAIR - Return true if virtual register is a doubleword pair
*/
int
vrispair(VREG *reg)
{
#if	DEBUG_KCC
    printf ("vrispair (%%%o)\n", reg);
#endif

    return (reg->Vrflags & VRF_REGPAIR) != 0;
}

/* VRUFCREG - Vreg version of ufcreg().
**	If changereg() fails to change a reg (S) to the desired # (R), it
** emits a MOVE R,S.  Often the code generator later realizes the exact
** # didn't matter and so the MOVE to R can be flushed; this routine does
** exactly that for a virtual reg by updating it to reflect the new
** real reg it's associated with (S)  once the MOVE is flushed.
**
** Currently only used by switch case jump generation to avoid
** lossage that would ensue from CCCODE's calls to ufcreg.
*/

void
vrufcreg(VREG *vr)
{
#if	DEBUG_KCC
    printf ("vrufcreg (%%%o)\n", vr);
#endif

    regis[vrtoreal(vr)] = NULL;			/* Swap in, deassign it */
    regis[vr->Vrloc = ufcreg(vr->Vrloc)] = vr;	/*Maybe flush MOVE; reassign*/
}

/* RFREE - True if real register is NOT assigned to a virtual reg.
**	A function is necessary because the outside world can't see regis[]
*/
int
rfree(int rr)
{
#if	DEBUG_KCC
    printf ("rfree (%%%o)\n", rr);
#endif

    return regis[rr] == NULL;
}

#if 0
/* RHASVAL - True if real reg is still assigned and VRF_HASVAL is set
**	indicating it contains a needed value.
**
** This isn't actually used by anything yet.
*/
int
rhasval(rr)
{
    return (regis[rr] ? regis[rr]->Vrflags&VRF_HASVAL : 0);
}
#endif




/* From this point, all routines are internal auxiliaries */

/* RRFIND - Find or create an unused real register.
**	If none exist, we spill what is likely to be the
**	earliest allocated register (since our register allocation
**	will tend to act like a stack this is a win).
*/
static int
rrfind (void)
{
    VREG *vr;
    int r;


#if	DEBUG_KCC
    puts ("rrfind ()");
#endif

    if (r_preserve)
	{
	r = r_preserve;
	r_preserve = 0;
	return r;
	}

    updrfree();			/* update regfree[] to pbuf contents */

    for (r = r_minnopreserve; r <= r_maxnopreserve; r++) /* FW 2A(47) */
	if (regfree[r])
	    return r;

    for (r = r_minnopreserve; r <= r_maxnopreserve; r++) /* FW 2A(47) */
	if (regis[r] == NULL)
	    return r;

    /* All registers in use, have to decide which one to spill to stack.
    ** The heuristic for this is to use the "oldest" thing on the register
    ** list (this is the least recently created -- not necessary the least
    ** recently used -- register)
    ** This is where VRF_LOCK has its effect of DELAYING regs from being
    ** spilled. Also, for time being, don't spill 2nd reg of a pair.
    */

    for (vr = reglist.Vrprev; vr != &reglist; vr = vr->Vrprev)
	{
	if (!(vr->Vrflags & (VRF_LOCK | VRF_REG2ND)))
	    {
	    r = vr->Vrloc;		/* Remember phys reg # */
	    vrspill(vr);		/* Spill this reg to stack! */
	    return r;
	    }
	}

    /*extremely rare to fall thru here (never happens in KCC & LIBC sources)*/

    efatal("rrfind: no regs, use keyword [register] or simplify expr");
    return -1;	/* should never get here */
}

/* UPDRFIND - auxiliary for rrfind() and rrdfind().
**	Sees which registers are in use in the peephole buffer.
** We try to avoid assigning these so that common subexpression
** elimination will have the greatest opportunity to work.
**
** Since this is merely a heuristic and since it is called intensively,
** we care more about speed than accuracy.
** In particular, we don't even bother looking at the opcode or
** addressing mode of each instruction.
*/
static void
updrfree()
{
    int r;
    PCODE *p;

#if	DEBUG_KCC
    puts ("updrfree ()");
#endif

    for (r = r_minnopreserve; r <= r_maxnopreserve; r++) /* FW 2A(47) */
	regfree[r] = (regis[r] == NULL);
    for (p = previous; p != NULL && p->Pop != P_PUSHJ; p = before(p))
	regfree[p->Preg] = 0;
}




/* RRDFIND - Find (or create) a real double-register pair, returning the
**	# of the first real reg of the pair.
**	We have to be careful not to return the very last register.
*/

static int
rrdfind (void)
{
    VREG	*vr;
    VREG	*vrok[NREGS];
    int		i;
    int		nvrs;
    int		r;


#if	DEBUG_KCC
    puts ("rrdfind ()");
#endif

    if (r_preserve)
	{
	r = r_preserve;
	r_preserve = 0;
	return r;
	}

    updrfree();			/* update regfree[] to pbuf contents */

    for (r = r_minnopreserve; r < r_maxnopreserve; r++) /* FW 2A(47) */
	{
	if (regfree[r] && regfree[r + 1])
	    return r;
#if	DEBUG_KCC
	else
	    printf ("unable to use %d and %d (not free in regfree[])\n",
		    r, r + 1);
#endif
	}

    for (r = r_minnopreserve; r < r_maxnopreserve; r++) /* FW 2A(47) */
        {
	if ((regis[r] == NULL) && (regis[r + 1] == NULL))
	    return r;
#if	DEBUG_KCC
	else
	    printf ("unable to use %d and %d (not free in regis[])\n",
		    r, r + 1);
#endif
	}

    /* None free, scan the reglist in the same way as for rrfind.  But
    ** since we need a pair, we must look for the first virtual reg or
    ** combination thereof that forms a pair.
    ** Note that for time being, we avoid spilling just the 2nd reg of a pair.
    */

    nvrs = 0;

    for (vr = reglist.Vrprev; vr != &reglist; vr = vr->Vrprev)
	{
	if (vr->Vrflags & (VRF_LOCK | VRF_REG2ND))	/* If locked, */
	    {
#if	DEBUG_KCC
	    printf ("rejecting %d because flags = %%%o\n",
		    vr->Vrloc, vr->Vrflags);
#endif
	    continue;				/* don't consider it */
	    }

	if (vr->Vrflags & VRF_REGPAIR)
	    {
	    r = vr->Vrloc;		/* Remember phys reg # */
	    vrspill(vr);		/* Spill this reg to stack! */
	    return r;
	    }

	/* Not a pair, see if forms pair with anything already seen. */

	for (i = 0; i < nvrs; ++i)
	    {
	    if (((r = vrok[i]->Vrloc) == (vr->Vrloc + 1))
		    || (r == (vr->Vrloc - 1)))
		{
		if (r > vr->Vrloc)	/* get low phys reg # */
		    r = vr->Vrloc;

		vrspill(vr);		/* Spill both to stack */
		vrspill(vrok[i]);
		return r;
		}
	    }

	/* Nope, add to array and keep looking.  Should never have more
	** than NREGS active registers, so array bounds ought to be safe.
	*/

	vrok[nvrs++] = vr;
	}

    /* extremely rare to fall thru here (never in KCC & LIBC sources)*/

#if	DEBUG_KCC
    puts ("entering last-ditch code");
#endif

    for (vr = reglist.Vrprev; vr != &reglist; vr = vr->Vrprev)
	{
	if (!vr->Vrflags)
            {
	    if (((vr->Vrloc + 1) <= r_maxnopreserve) /* FW 2A(47) */
		    && (regis[vr->Vrloc + 1] == NULL))
		{
		vrspill(vr);
		return r;
		}
	    else if (((vr->Vrloc - 1) >= r_minnopreserve)
		    && (regis[vr->Vrloc - 1] == NULL))
		{
		vrspill(vr);
		return (r - 1);
		}
	    }
	}

#if DEBUG_KCC	/* Only turn on if int_error message below gets printed */
    vrdumplists ();
#endif

    efatal("rrdfind: no regs, use keyword [register] or simplify expr");
    return -1;
}

/* VRSETRR -  Set a virtual register's location(s) to be some real reg(s).
**	If the vreg is a pair, both are set.
*/
static VREG *
vrsetrr(VREG *vr, int rr)
{
#if	DEBUG_KCC
    printf ("vrsetrr (%%%o, %%%o)\n", vr, rr);
#endif

    if (vr->Vrflags & VRF_REGPAIR)
	vr1setrr(vr->Vrmate, rr+1);
    return vr1setrr(vr, rr);
}

/* VR1SETRR -  Set a virtual register's location to be some real reg
*/
static VREG *
vr1setrr(vr, rr)
VREG *vr;
{
#if	DEBUG_KCC
    printf ("vr1setrr (%%%o, %%%o)\n", vr, rr);
#endif

    return regis[vr->Vrloc = rr] = vr;
}

/* VRUNLINK - Unlink a virtual reg from whatever list it's on.
**	If the vreg is a pair, both are unlinked.
*/
static void
vrunlink(vr)
VREG *vr;
{
#if	DEBUG_KCC
    printf ("vrunlink (%%%o)\n", vr);
#endif

    if (vr->Vrflags & VRF_REGPAIR)
	vr1unlink(vr->Vrmate);
    vr1unlink(vr);
}

/* VR1UNLINK - Remove a register from whatever list it's on.
**	This is the first half of changing from one list to another
*/
static void
vr1unlink(reg)
VREG *reg;
{
#if	DEBUG_KCC
    printf ("vr1unlink (%%%o)\n", reg);
#endif

    if (reg->Vrnext == reg)
	efatal("vr1unlink: list head");

    reg->Vrnext->Vrprev = reg->Vrprev;
    reg->Vrprev->Vrnext = reg->Vrnext;

#if DEBUG_KCC
    vrdumplists ();
#endif
}


/* VRLINK - Link a register that may be the 1st of a pair; if so, link
**	the 2nd reg as well.
*/
static VREG *
vrlink(reg, list)
VREG *reg, *list;
{
#if	DEBUG_KCC
    printf ("vrlink (%%%o, %%%o)\n", reg, list);
#endif

    if (reg->Vrflags & VRF_REGPAIR)
	vr1link(reg->Vrmate, list);	/* Is pair, link 2nd first */
    return vr1link(reg, list);
}

/* VR1LINK -  Add a register to a list
**	Used when a new vreg is created and when moving between lists
*/
static VREG *
vr1link(reg, list)
VREG *reg, *list;
{
#if	DEBUG_KCC
    printf ("vr1link (%%%o, %%%o)\n", reg, list);
#endif

    reg->Vrnext = list->Vrnext;
    list->Vrnext->Vrprev = reg;
    reg->Vrprev = list;
    list->Vrnext = reg;

#if DEBUG_KCC
    vrdumplists ();
#endif

    return reg;
}




/* VRALLOC - Allocate a new virtual register structure
** VRDALLOC - Same, but returns 1st of a double register pair, linked together.
*/
static VREG *
vralloc()
{
    VREG *rp;

#if	DEBUG_KCC
    puts ("vralloc ()");
#endif

    if (empty(&freelist))
	{
	rp = (VREG *)calloc(1, sizeof (VREG));
	if (rp == NULL)
	    efatal("Out of memory for virtual registers");
	}
    else
	{
	rp = freelist.Vrnext;
	vr1unlink(rp);
	}

    rp->Vrflags = 0;
    rp->Vrloc = 0;			/* FW 2A(41) */
    rp->Vrtype = NULL;			/* FW 2A(41) */
    rp->Vroldstk = 0;			/* FW 2A(41) */
    rp->Vrnext = NULL;			/* FW 2A(41) */
    rp->Vrprev = NULL;			/* FW 2A(41) */
    rp->Vrmate = NULL;			/* FW 2A(41) */

    return rp;
}

static VREG *
vrdalloc()
{
    VREG *vr1 = vralloc();
    VREG *vr2 = vralloc();

#if	DEBUG_KCC
    puts ("vrdalloc ()");
#endif

    vr1->Vrflags |= VRF_REGPAIR;
    vr2->Vrflags |= VRF_REG2ND;
    vr1->Vrmate = vr2;
    vr2->Vrmate = vr1;
    return vr1;
}

#if	DEBUG_KCC
static void
vrdumplists (void)
    {
    VREG	*vr;
    int		r;
    

    puts ("\nreglist:");

    for (vr = reglist.Vrprev; vr != &reglist; vr = vr->Vrprev)
	{
	printf ("@%%%o, flags: %s, loc: %%%o, typespec: %%%o, mate: %%%o\n",
		vr, vrdumpflags (vr->Vrflags), vr->Vrloc,
		vr->Vrtype->Tspec, vr->Vrmate);
	}
    
    puts ("spillist:");

    for (vr = spillist.Vrprev; vr != &spillist; vr = vr->Vrprev)
	{
	printf ("@%%%o, flags: %s, loc: %%%o, typespec: %%%o, mate: %%%o\n",
		vr, vrdumpflags (vr->Vrflags), vr->Vrloc,
		vr->Vrtype->Tspec, vr->Vrmate);
	}
    
    puts ("freelist:");

    for (vr = freelist.Vrprev; vr != &freelist; vr = vr->Vrprev)
	{
	printf ("@%%%o\n", vr);		/* the rest is silence */
	}
    
    for (r = r_minnopreserve; r <= r_maxnopreserve; r++) /* FW 2A(47) */
	{
	printf ("regfree[%d] = %s\tregis[%d] = %%%o\n",
		r, (regfree[r] ? "free" : "used"), r, regis[r]);
	}
    }

static char *
vrdumpflags (int flags)
{
    static
    char	fchars[6];

    fchars[0] = flags & VRF_SPILLED ? 'S' : ' ';
    fchars[1] = flags & VRF_REGPAIR ? '1' : ' ';
    fchars[2] = flags & VRF_REG2ND  ? '2' : ' ';
    fchars[3] = flags & VRF_LOCK    ? 'L' : ' ';
    fchars[4] = flags & VRF_SPECIAL ? 'D' : ' ';
    fchars[5] = '\0';

    return fchars;
}
#endif
