/*	CCNODE.C - Parse-tree node routines
**
**	(c) Copyright Ken Harrenstien 1989
**		Plus all CCDUMP changes after v.142, 8-Apr-1988
**	(c) Copyright Ken Harrenstien, SRI International 1985, 1986
**		All CCDUMP changes after v.58, 8-Aug-1985
**
** Original CCDUMP (C) 1981  K. Chen
*/

#include "cc.h"
#include <string.h>	/* memcpy */
#include <stdlib.h>	/* calloc, free */

/* Exported functions */
NODE *ndef(int op, TYPE *t, int f, NODE *l, NODE *r); /* NODE defn routine */
NODE *ndefop(int op);				/* Auxiliary variations */
NODE *ndeft(int op, TYPE *t);
NODE *ndeftf(int op, TYPE *t, int f);
NODE *ndeftl(int op, TYPE *t, NODE *l);
NODE *ndeftr(int op, TYPE *t, NODE *r);
NODE *ndefl(int op, NODE *l);	
NODE *ndefr(int op, NODE *r);	
NODE *ndeflr(int op, NODE *l, NODE *r);
NODE *ndeficonst(INT val);
NODE *ndefident(SYMBOL *s);
int nodeidx(NODE *);		/* CCERR */
#if DEBUG_KCC		/* 5/91 KCC size */
  void nodedump(NODE *n);	/* CC */
#endif

/* Internal functions */
static NODE *nget(void);
#if DEBUG_KCC		/* 5/91 KCC size */
  static int dmpnsum(NODE *);
  static void dumpnode(NODE *, char *, int);
  static void dmpntype(TYPE *);
  static void dmpcast(INT);
#endif

#if 0
static NODE *nget();
  #if DEBUG_KCC		/* 5/91 KCC size */
    static int dmpnsum();
    static void dumpnode(), dmpntype(), dmpcast();
  #endif
#endif

/* Internal data & defs */
#if 0	/* __MSDOS__ */
 #define NODEBLKSIZ 20		/* Dynamically alloc in increments of this */
#else
 #define NODEBLKSIZ 100
#endif

struct xnode {
	NODE xn;	/* only used indireclty, DO NOT REMOVE */
	int xn_idx;
};

struct nodeblk {		/* Dynamically allocated node blocks */
	struct nodeblk *nb_next;
	char nb_check[4];	/* To hold "NODE" for possible debug check */
	struct xnode nb_nodes[NODEBLKSIZ];
};

static int maxnode = 0;			/* Total # nodes in use */
static struct nodeblk *nodehd = NULL;	/* Start of any allocated blks */
static int nodebcnt = 0;		/* # of nodes used in current blk */

/* NODEINIT - Initialize the static node table and free any dynamic nodes.
*/
void
nodeinit(void)
{
    struct nodeblk *this;

    maxnode = 0;		/* Init static table */
    while ((this = nodehd) != NULL) {
	nodehd = this->nb_next;	/* Save ptr to next */
	free((char *)this);	/* Free up this one */
    }
    nodebcnt = 0;		/* Nothing in any blk */
}

/* NGET - Auxiliary to obtain a free node, without filling it in.
*/
static NODE *
nget(void)
{
    NODE *nd;

    if (++maxnode < MAXNODE)		/* Normal case, use static array */
	nd = &nodes[maxnode-1];
    else {
	/* Static table full, try to get dynamic allocation */
	struct nodeblk *nb;

	if (!nodehd || nodebcnt >= NODEBLKSIZ-1) {
	    if ((nb = (struct nodeblk *) calloc
		    (1, sizeof(struct nodeblk)))==NULL)
		efatal("Out of memory, cannot allocate more nodes (%d used)", 
			maxnode);
	    memcpy(nb->nb_check, "NODE", 4);	/* Install check value */
	    nb->nb_next = nodehd;		/* Link into block list */
	    nodehd = nb;
	    nodebcnt = 0;			/* Initialize count */
	}
	nd = (NODE *) &(nodehd->nb_nodes[nodebcnt++]);
	((struct xnode *)nd)->xn_idx = maxnode;		/* Put in index # */
    }
    return nd;
}

/* NODEIDX - Return index for a node pointer.
**	Returns -1 if pointer seems bad.
*/
int
nodeidx(NODE *n)
{
    INT i;

    i = n - &nodes[0];	/* First try to derive index into static array */
    if (i < 0 || i >= MAXNODE) {	/* If bad result, maybe dynamic */
	if (nodehd == NULL)		/* see if have any dynamic stuff */
	    return -1;			/* Nope, error */
	i = ((struct xnode *)n)->xn_idx;	/* Else assume dynamic */
    }
    return (int) i;
}

/* NDEF - Node DEFinition routines
*/

#define NSET(op,t,f,l,r) NODE *n = nget(); \
	n->Nop = op, n->Ntype = t, n->Nflag = f, n->Nleft = l, n->Nright = r;\
	n->Nendlab = 0, n->Nxfint = 0; n->Nreg = 0; \
	return n;
NODE *
ndef(int op, TYPE *t, int f, NODE *l, NODE *r)
/* op Node token value (a N_ or Q_ token) */
/* t Node's C type */
/* f Flags */
/* l, r, Left and Right links */
{
    NSET(op,t,f,l,r)
}

/* Handy variations for defining a node.  The op is always required.
**	NDEFOP	- Op
**	NDEFT	- Op, Type.
**	NDEFTF	- Op, Type, Flags.
**	NDEFTL	- Op, Type, Left.
**	NDEFTR	- Op, Type, Right.
**	NDEFL	- Op, Left.
**	NDEFR	- Op, Right.
**	NDEFLR	- Op, Left, Right.
*/
NODE *ndefop(int op)			{ NSET(op,0,0,0,0) }
NODE *ndeft(int op, TYPE *t)  		{ NSET(op,t,0,0,0) }
NODE *ndeftf(int op, TYPE *t, int f)	{ NSET(op,t,f,0,0) }
NODE *ndeftl(int op, TYPE *t, NODE *l)	{ NSET(op,t,0,l,0) }
NODE *ndeftr(int op, TYPE *t, NODE *r)	{ NSET(op,t,0,0,r) }
NODE *ndefl(int op, NODE *l)		{ NSET(op,0,0,l,0) }
NODE *ndefr(int op, NODE *r)		{ NSET(op,0,0,0,r) }
NODE *ndeflr(int op, NODE *l, NODE *r)	{ NSET(op,0,0,l,r) }

/* Auxiliaries for common kinds of node definitions */

/* NDEFICONST - Makes a N_ICONST node of type (int) and sets its value.
*/
NODE *
ndeficonst(INT val)
{
    NODE *n = ndeft(N_ICONST, inttype);
    n->Niconst = val;
    return n;
}


/* NDEFIDENT - Makes a Q_IDENT node and sets its symbol.
*/
NODE *
ndefident(SYMBOL *s)
{
    NODE *n = ndeft(Q_IDENT, s->Stype);
    n->Nid = s;
    return n;
}

#if DEBUG_KCC		/* 5/91 KCC size */
/* ---------------------------- */
/*	dump code 		*/
/* ---------------------------- */

static char dmphlp[] = "\
Each line represents one parse-tree node, in the format:\n\
 # <L/R>: <Nname> (N#), nflag: <#>, ntype: <desc>, <extra stuff>\n\
where\n\
   #	- Node index.  This is sometimes shown in internal error messages.\n\
  <L/R> - Left or Right.  Left nodes are considered inferior (child) nodes\n\
	and are indented.  Right nodes are considered successors and keep\n\
	the existing indentation.  L/R == Lisp CAR/CDR.\n\
  <Nname> - Node op name, as used in KCC, followed by actual decimal index.\n\
  <#> - octal value of nflag member, if non-zero.\n\
  <desc> - description of ntype member, if non-zero: #n -> ttype\n\
	where n is its index in the types table and ttype the type's type.\n\
  <extra> - node specific information, if any.\n\
";

void
nodedump(NODE *n)
{
    char *s;
    static int helpdone = 0;
    int ind = 0;

    if (n == NULL) return;
    if(!helpdone) {
	fputs(dmphlp,fdeb);
	helpdone++;
    }
    switch (n->Nop) {
    case N_DATA:
	s = "Data";
	break;
    case N_FUNCTION:
	s = "Function";
	break;
    default:
	s = "ILLEGAL toplevel node";
	ind++;		/* Don't process further */
	break;
    }
    fprintf(fdeb, "---- %s ----\nTop: ", s);
    dmpnsum(n);
    fprintf(fdeb, "\n");

    if(ind) return;		/* Hack to avoid unknown nodes */

    if (n->Nleft)
	dumpnode(n->Nleft, "L", 8);
    if (n->Nright)
	dumpnode(n->Nright, "R", 4);
}

static void
dumpnode(n, branch, ind)
NODE *n;
char *branch;
int ind;
{
    int size;

    /* First print node index as an identifier (helps relate to err msgs) */
    fprintf(fdeb,"%4d ", nodeidx(n));

    /* Do indentation.  Avoid using fancy printf features for now. */
    size = (ind - 1) - strlen(branch);
    while(--size >= 0)
	putc(' ', fdeb);

    /* Output same summary for all nodes */
    fprintf(fdeb, "%s: ", branch);	/* Do indentation */
    if(!dmpnsum(n)) return;

    /* Now do stuff specific to each node op.  In particular, every
     * op which does NOT have left+right pointers should be trapped.
     */
    switch (n->Nop) {

	/* Default is to assume that node has a link structure, since this
	 * is true for almost all node ops.  The exceptions should be
	 * handled as specific cases below.
	 */
    default: break;

    case Q_GOTO:
    case N_LABEL:
	fprintf(fdeb, " = label sym \"%s\"", n->Nxfsym->Sname);
	break;

    case Q_SWITCH:
	fprintf(fdeb, ", caselist %o", nodeidx(n->Nxswlist));
	break;		/* Now do normal linkages */

    case Q_DEFAULT:
	fprintf(fdeb, ", caseptr %o\n", nodeidx(n->Nright));
	if(n->Nleft)
	    dumpnode(n->Nleft, "L", ind+4);
	return;

    case Q_CASE:
	fprintf(fdeb, " = case value %d, caseptr %o\n",
		(int) n->Nxfint, nodeidx(n->Nright));
	if(n->Nleft)
	    dumpnode(n->Nleft, "L", ind+4);
	return;

    case Q_DOT:
    case Q_MEMBER:
	fprintf(fdeb, ", offset %ld\n", (INT) n->Nxoff);
	if(n->Nleft)
	    dumpnode(n->Nleft, "L", ind+4);
	return;

    case N_CAST:
	fprintf(fdeb, ", ");
	dmpcast(n->Ncast);
	putc('\n', fdeb);
	if(n->Nleft)
	    dumpnode(n->Nleft, "L", ind+4);
	return;

    /* Now handle special cases which never have links */
    case N_FCONST:
	fprintf(fdeb, ", val = %.20g\n", n->Nfconst);
	return;
    case N_ICONST:
	fprintf(fdeb, ", val = %ld\n", (INT) n->Niconst);
	return;
    case N_SCONST:
	fprintf(fdeb, ", val = \"%s\" (%d)\n", n->Nsconst, n->Nsclen);
	return;
    case Q_IDENT:
	fprintf(fdeb, ", name \"%s\"\n", n->Nid->Sname);
	return;

    }

    if (tok[n->Nop].tktype == TKTY_ASOP && (n->Nascast != CAST_NONE)) {
	fprintf(fdeb, ", ascast=");
	dmpcast(n->Nascast);
    }

    /* Do default */
    putc('\n', fdeb);
    if (n->Nleft)
	dumpnode(n->Nleft, "L", ind+4);
    if (n->Nright)
	dumpnode(n->Nright, "R", ind);
}

/* dmpnsum - dump a standard summary of the node's generic contents.
 *	Returns 0 if node address is suspected to be bad.
 */
static int
dmpnsum(n)
NODE *n;
{
    int i;

    i = nodeidx(n);		/* Attempt to derive index of node */
    if(i < 0) {
	fprintf(fdeb, "ERROR: bad node address %lo (= index %d)\n", 
		(INT) n, i);
	return(0);
    }
    fprintf(fdeb, "%s (%d)", ((n->Nop >= NTOKDEFS) ? "??" : nopname[n->Nop]),
		n->Nop);
    if(n->Nflag) fprintf(fdeb, ", nflag: 0%o", n->Nflag);
    if(n->Ntype) {
	fprintf(fdeb, ", ntype: ");
	dmpntype(n->Ntype);
    }
    return(1);
}

/* dmpntype - dump node's ntype value in readable form */
static void
dmpntype(typ)
TYPE *typ;
{   char *s;

    if(!typ) {
	fprintf(fdeb, "0");
	return;
    }
    fprintf(fdeb,"#%d ->", typ - types);
    while(typ) {
	if (0 <= typ->Tspec && typ->Tspec < TS_MAX) {
	    s = tsnames[typ->Tspec];
	    if (typ->Tspec == TS_STRUCT || typ->Tspec == TS_UNION)
		typ = 0;
	} else {
	    fprintf(fdeb, "ILLEGAL! %d=", typ->Tspec);
	    s = "?";
	    typ = 0;
	}
	fprintf(fdeb, " %s", s);
	if(typ) typ = typ->Tsubt;	/* Get next subtype */
    }
}

static char *castnames[] = {
#define castspec(op,str) str,
	allcastmacro		/* Expand */
#undef castspec
};

static void
dmpcast(castop)
INT castop;
{
    if (0 <= castop && castop < CAST_MAX)
	fprintf(fdeb, "%s", castnames[(int)castop]);
    else fprintf(fdeb, "ERROR: unknown cast-type = %d", castop);
}
#endif
