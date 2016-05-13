/*	CC.H - Declarations for KCC data structures
**
**	(c) Copyright Ken Harrenstien 1989
**		All changes after v.199, 16-Apr-1988
**	(c) Copyright Ken Harrenstien, SRI International 1985, 1986
**		All changes after v.84, 8-Aug-1985
**
** Original version merged with cc.s / David Eppstein / 13 Mar 85
*/

/* Note: EXT must be used to declare anything which is not explicitly declared
 * in CCDATA.  All others are "extern".
 */

#ifndef EXT
#define EXT extern
#endif

#ifdef __COMPILER_KCC__
 #define __MSDOS__  0		/* Turbo C and C++ define __MSDOS__ */
 #define _char7	   _KCCtype_char7
 #define INT	   int
#else
 #define _char7     char
 #define INT	   long
 #define _ERRNO_LASTSYSERR (-1)
#endif

#if __MSDOS__ && !defined (SYS_CSI)
 #define SYS_CSI	1
#endif

#ifndef DEBUG_KCC		/* 5/91 KCC size */
 #define DEBUG_KCC	0	/* always zero for production KCC */
#endif

#ifndef	REGISTER_VARIABLES
#define	REGISTER_VARIABLES	0
#endif

#include <stdio.h>
#include "ccparm.h"	/* First come the various KCC parameters */
#include "ccsym.h"	/* SYMBOL and TYPE definitions */
#include "ccnode.h"	/* NODE definitions */
#include "ccerr.h"	/* Error reporting routine declarations */

/* KAR-4/92, for collision between sixbit() in KCC and typdef in comdef */
#ifndef sixbit
#define sixbit _sixbit
#endif

typedef char filename[FNAMESIZE];

extern int cvercode, cverlib;	/* $$CVER version numbers (no switches) */
extern int cverdist, cverkcc;	/* Info-only version numbers (no switches) */

/* KCC switch flags.  These are set when KCC is first invoked and remain
**	constant over all files processed.
*/

/* Simple switches and flags */
EXT int
    assemble,			/*      1 Assemble after compile */
				/*	  (set/cleared by other switches) */
    delete,			/*      1 Delete assembler file when done */
				/* -S = 0 Don't (don't run anything either) */
    link,			/*      1 run link after assembling */
				/* -c = 0 Compile only, don't run link */
    condccf,			/* -q = 1 Compile files conditionally */
    prepf,			/* -E = 1 Run through preprocessor only */
    keepcmts;			/* -C = 1 For -E, leave comments in output */

/*
 * Register-allocation macros.  These set up the default boundary
 * between preserved and non-preserved registers.
 *
 * Default = nopreserve 1-5, preserve 6-13 for BLISS36C-like linkage
 */

#define R_MIN_NOPRESERVE	1
#define R_MAX_NOPRESERVE	5
#define	R_MAXREG		015	/* highest expr scratch register */
#define R_PRESERVE_COUNT	(R_MAXREG - r_maxnopreserve) /* FW 2A(52) */

EXT
SYMBOL*	    Reg_Id[R_MAXREG - R_MAX_NOPRESERVE]; /* Can't possibly need more */

EXT
FILE*	    outmsgs;			/* -s redirect errors to stdout */

EXT
char	    _ch_cpy,	/* 12/90 set in ccpp.c and used in CCSTMT */
	    title[TITLE_SIZE], /* title #pragma module(title) */
	    module_pragma,	/* flags if #pragma module(title) */
	    insert_all_files,	/* -i which #includes files only once each */

#if __MSDOS__
	    unsign_int,		/* Flags 31st bit on for unsigned int const */
#endif

	    _reg_count,		/* count of regs in a function */

#if REGISTER_VARIABLES    
	    use_registers,
#endif

	    r_minnopreserve,	/* FW 2A(52) replaces R_MIN_NOPRESERVE macro */
	    r_maxnopreserve,	/* FW 2A(47) replaces R_MAX_NOPRESERVE macro */
	    r_preserve,		/* flags vrget() with preserved reg#, else 0 */
	    XF4_call_spill,	/* spill preserved regs if XF4 */
	    fn_main,		/* set in ccdecl, used in ccgen and ccgen1 */
	    longidents,		/* FW 2A(51) */
	    isr,		/* FW 2A(52) compiling an ISR */
	    err_waiting;	/* signals if error is waiting to be printed*/

EXT int profbliss,		/* -p = 1 Include BLISS profiling stuff */
    mlist,			/* -d=list Generate mixed listing KAR 8/90 */
    stksz;			/* -Nxxxx User selectable run time stack
				   	  size (default 4K or 8 pages)  */

EXT char creatime[30], comptime[30];   /* Time variables for mixed listing */
EXT filename dspfname;	/* Mixed listing input file name -- KAR 8/90	   */
EXT char *errbuf;		/* contains buffered err msgs (mlist)
					-KAR 8/90 */
EXT char *ver_str;		/* ptr for version string--KAR 11/90 */

#define FN_ENTRY 0
#define FN_EXIT  1
#define STMT	 2
#define MAX_OLINE 56
#define HDR_LINES 3

/* String switches */
extern char *asmhfile;		/* -Afile = name of assembler preamble file */
extern char *asmtfile;		/* -afile = name of #asm temporary file */
extern int
	npredef,		/* -Dmac=d # of -D macro predefinitions */
	npreundef,		/* -Umac   # of -U macro pre-undefinitions */
	nincpaths,		/* -Ipath  # of "" include-file directories */
	nhfpaths,		/* -Hpath  # of <> include-file dirs */
	nhfsypaths,		/* -hpath  # of <sys/> include-file dirs */
	nihfpaths,		/* # of default -H and -h paths */
	nihfsypaths;
extern char *predefs[];		/*	Pointers to -D args */
extern char *preundefs[];	/*	Pointers to -U args */
extern char *incpaths[];	/*	Pointers to -I args */
extern char *hfpaths[];		/*	Pointers to -H args */
extern char *hfsypaths[];	/*	Pointers to -h args */
extern char *ihfpaths[];	/*	Pointers to default -H paths */
extern char *ihfsypaths[];	/*	Pointers to default -h paths */
extern char *libpath;		/* -Lpath  for -l library files */

/* General "extended syntax" switches */
EXT int
				/* -O or -O=all	All optimizer flags */
    optpar,			/* -O=parse	Parse tree optimizations */
    optgen,			/* -O=gen	Code generator " */
    optobj,			/* -O=object	Obj code (peephole) " */
				/* -n = 0 Don't optimize anything */
				/* -d or -d=all All debug flags */
    debpar,			/* -d=parse	Parse tree debug output */
    debgen,			/* -d=gen	Code generator "   "    */
    debpho,			/* -d=pho	Peephole optim "   "    */
    debsym,			/* -d=sym	Symbol table   "   "    */
    debcsi,			/* -g=*		CSI Source Lang. Debug  */
				/* -v or -v=all	  All verboseness flags */
    vrbfun,			/* -v=fundef	Print function names */
    vrbsta,			/* -v=stats	Print stats at end */
    vrbld;			/* -v=load	Print linking loader cmds */
EXT int
    ldddtf;
#ifdef	MULTI_SECTION /* FW 2A(51) */
EXT int
    ldextf,			/* -i or -i=extend */
    ldddtf,			/* -i=ddt */
    ldpsectf;			/* -i=psect */
extern struct psect {		/*	Structure for psect specifications */
    INT ps_beg, ps_len, ps_lim;	/*	Psect start, max length, max addr */
}	ldpsdata,		/* -i=data:<beg>:<len>	Data segment */
	ldpscode,		/* -i=code:<beg>:<len>	Code segment */
	ldpsstack;		/* -i=stack:<beg>:<len>	Stack segment */
#endif
EXT int wrnlev;			/* -w=	Specifies warning level, one of: */
#define WLEV_ALL 0		/* -w=all or -w	Show all warnings */
#define WLEV_NOTE 1
#define WLEV_ADVISE 2
#define WLEV_WARN 3

/*
 * Debugging/profiling mnemonics  FW 2A(42)
 *
 * The following are the permissible values of "debcsi".
 */

#define KCC_DBG_SDBG	(1)		    /* statement debugging */
#define	KCC_DBG_SPRF	(2)		    /* statement profiling */
#define KCC_DBG_FPRF	(3)		    /* function profiling */
#define KCC_DBG_NULL	(4)		    /* NULL pointer detection */
#define KCC_DBG_FDBG	(5)		    /* function debugging */

extern int clevkcc;		/* -P=KCC Asks for KCC extensions */
extern int clevnocpp;           /* -P=nocpp asks to forbid "//" comments */
extern int clevel;		/* -P=    Specifies C implem level, one of: */
#define CLEV_BASE 0		/* Base (default) should always be 0 */
#define CLEV_CARM 1
#define CLEV_ANSI 2
#define CLEV_STDC 3
#define CLEV_STRICT 4		/* "pedantic" interpretation of STDC */
				/* (some warnings become errors) */

/* -x= Cross-compiling switch variables (target environment settings) */
extern int tgsys;		/* Target System type */
				/*      0 (default) same as source system */
				/*	n different, some TGSYS_ value */
extern int tgcsize;		/* Target Char Size, in bits */
extern int tgcpw;		/* Target # Chars Per Word */
extern int tgcmask;		/* Target Char Mask */

EXT struct {			/* Target CPU/SYS use-feature flags */
	int mapdbl;		/*	Mach: Must map double format */
} tgmachuse;

/* Constant variables and tables - not changed at any time */

extern TOKEN tok[];		/* Token/Node-Op attributes */
extern char *nopname[];		/* Token/Node-Op names */
extern char *tokstr[];		/* Token literal strings */

/* Pointers to basic data types supported */
#define deftype	inttype		/* Default type - set to (int) */
#define voidtype	typeptr[TS_VOID]	/* (void)	*/
#define flttype		typeptr[TS_FLOAT]	/* (float)	*/
#define dbltype		typeptr[TS_DOUBLE]	/* (double)	*/
#define lngdbltype	typeptr[TS_LNGDBL]	/* (long double)	*/
#define schartype	typeptr[TS_CHAR]	/* (signed char)	*/
#define shrttype	typeptr[TS_SHORT]	/* (short)	*/
#define inttype		typeptr[TS_INT]		/* (int)	*/
#define longtype	typeptr[TS_LONG]	/* (long)	*/
#define uinttype	typeptr[TS_UINT]	/* (unsigned int) */
#define ulongtype	typeptr[TS_ULONG]	/* (unsigned long) */
#define ushrttype	typeptr[TS_USHORT]	/* (unsigned short) */
#define uchartype	typeptr[TS_UCHAR]	/* (unsigned char) */
EXT TYPE *chartype;		/* (char) - set to schartype or uchartype */
EXT TYPE *strcontype;		/* (char *) - type for string constants */
EXT TYPE *voidptrtype;		/* (void *) - to help check for NULL */
EXT TYPE *siztype;		/* Type for "sizeof" operator */
EXT TYPE *ptrdifftype;		/* Type for ptrdiff_t (ptr - ptr) */

extern TYPE *typeptr[];		/* Type pointers */
/* 9/91  typsiztab[], typbsiztab[] were int */
extern char typsiztab[];	/* Type sizes in words */
extern char typbsiztab[];	/* Type sizes in bits */
extern char *tsnames[];		/* Type names */


/* Per-file variables.  These are reset for each file being processed. */
EXT filename
	inpfname,	/* Current input file arg (may have .C inserted) */
	inpfdir,	/* Directory part of inpfname (before module)	*/
	inpfmodule,	/* Filename part of inpfname (no dir or ext)	*/
	inpfsuf,	/* Suffix part of inpfname (after module & ext) */
	outfname,	/* Assembler main output file:	module.FAI/MAC/MID */
	debfname,	/* Debugging output file (parse tree):	module.DEB */
	phofname,	/* Debugging output file (peephole):	module.PHO */
	symfname;	/* Debugging output file (symtab):	module.SYM */

EXT FILE *in, *out, *fdeb, *fpho, *fsym;	/* STDIO file I/O pointers */

/* Variables used during input file processing */
EXT int page,		/* position in input file */
    opage,		/* page in the output file for mixed-list  KAR 8/90 */
    line,		/* ditto - line # on current page */
    oline,		/* lines on current page of output file    KAR 8/90 */
    fline,		/* ditto - line # in current file */
    tline,		/* total # of lines (including includes) */
    eof,		/* end of file flag */
    token;		/* current input token from lexer */
EXT int nerrors,	/* # errors in current file being compiled */
    nwarns;		/* # warnings in current file being compiled */

/* Per-declaration variables.  These are reset for each top-level
** declaration or function.
*/
extern NODE nodes[];	/* Parse tree node table (CCSTMT, CCDATA) */
EXT int savelits;	/* 0 when OK to reset string literal pool */
EXT SYMBOL *curfn;	/* Name of current function */
EXT int curfnloc, curfnnew;	/* where in file it started (CCERR, CCDECL) */
EXT INT maxauto;	/* Current fn: size of auto vars (CCGEN, CCDECL) */
EXT int stackrefs;	/* Current fn: Whether it addresses locals */
EXT int stkgoto;	/* Current fn: Whether it contains a non-local goto */
