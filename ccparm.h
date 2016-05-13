/*	CCPARM.H - Global parameters for KCC
**
**	(c) Copyright Ken Harrenstien 1989
**		All changes after v.30, 8-Apr-1988
**	(c) Copyright Ken Harrenstien, SRI International 1985, 1986
**		Collected from various files, 15 Dec 85
*/
/*
** This file contains two conceptually distinct sets of parameters:
**	(1) Target Machine (System/CPU/ASM) definitions & parameters
**	(2) General KCC size parameters
** If the first category becomes much more complex it should be split
** off into its own file.
*/

/* Target machine definitions. */

/* Target System type.
**	The global "tgsys" is set to one of these values.
**	Note that for TGSYS_NULL, the source system can be found using c-env.h.
*/

enum systype {
	TGSYS_NULL,	/* Target system same as source system */
	TGSYS_TOPS20,	/* Target: DEC TOPS-20 system */
	TGSYS_WAITS,	/* Target: SU WAITS system */
	TGSYS_TENEX,	/* Target: BBN TENEX / TYMSHARE AUGUST system */
	TGSYS_TOPS10,	/* Target: DEC TOPS-10 system */
	TGSYS_ITS	/* Target: MIT ITS system */
};

/* Target Machine data type sizes.
**	These are universal for all PDP-10s.  KCC may never be set up to
** compile for anything else.
**
** Note that the sizes are in terms of bits, rather than "char" bytes.
*/

#define TGSIZ_WORD 36		/* Size of a PDP-10 word, in bits */
#define TGSIZ_HALFWD 18		/* Size of a PDP-10 halfword in bits */
				/* This is sometimes handled specially. */

#define TGSIZ_CHAR	(TGSIZ_WORD/4)	/* Default size of a char byte */
#define TGSIZ_SHORT	(TGSIZ_WORD/2)
#define TGSIZ_INT	TGSIZ_WORD
#define TGSIZ_LONG	TGSIZ_WORD
#define TGSIZ_FLOAT	TGSIZ_WORD
#define TGSIZ_DOUBLE	(TGSIZ_WORD*2)
#define TGSIZ_LNGDBL	(TGSIZ_WORD*2)
#define TGSIZ_PTR	TGSIZ_WORD
#define TGSIZ_ENUM	TGSIZ_WORD

/* Parameter definitions, mostly to do with sizes */
#ifndef MAXPREDEF	/* CC: # of -D and -U predefs allowed  */
 #define MAXPREDEF 20
#endif
#ifndef MAXINCDIR	/* CC: # of -I include-file search paths allowed  */
 #define MAXINCDIR 24	/* 5/91 changed 10 to 20, see PPS 4206 */
			/* KAR-8/92, changed 20 to 24 to makeup for levels */
			/* see PPS 4516                                    */
#endif
#ifndef FNAMESIZE	/* CC, CCPP: Size of a filename string */
 #if SYS_CSI
  #define FNAMESIZE (48) /*cstdio:[123456,123456]#123456789012.#123456<123> */
 #else
  #define FNAMESIZE (40*4+10)	/*	TOPS-20 has biggest possible names */
 #endif
#endif

#if !SYS_CSI	/* 5/91 Dynamic tables */
 #ifndef MAXPPTOKS	/* CCPP: # of Preprocessor tokens active */
  #define MAXPPTOKS 4000
 #endif
 #ifndef MAXPOOLSIZE	/* CCPP: Size of char pool for PP tokens */
  #define MAXPOOLSIZE 4000 /* Make dynamic later, see ccpp #if 0...#endif */
 #endif
#endif

#ifndef MAXMARG		/* CCPP: Max # of macro args */
 #define MAXMARG 40	/*	Cannot exceed 0177 */
#endif
#ifndef MAXMACNEST	/* CCPP: Max depth of macro nesting */
 #define MAXMACNEST 40
#endif
#ifndef MAXINCLNEST	/* CCPP: Max depth of include file nesting */
 #define MAXINCLNEST 10
#endif
#ifndef MAXIFLEVEL	/* CCPP: Max depth of #if nesting */
 #define MAXIFLEVEL 50
#endif
#ifndef MAXTSTACK	/* CCLEX: Size of C token stack */
 #define MAXTSTACK 16
#endif
#ifndef CPOOLSIZE	/* CCLEX: Size of string literal char pool */
/* 5/91 make dynamic later, see cclex if 0...endif */
 #if __MSDOS__
  #define CPOOLSIZE 6000
 #else
  #define CPOOLSIZE 16000
 #endif
#endif
#ifndef THASHSIZE	/* CCSYM: Size of type hash table */
 #define THASHSIZE 2557	/* primes for better hash */
#endif
#ifndef MAXTYPE	     /* CCSYM: # types possible if DEBUG_KCC, else calloced */
 #define MAXTYPE 2557	/* must be same as THASHSIZE if DEBUG_KCC */
#endif
#ifndef IDENTSIZE	/* CCSYM: Max ident length, including trailing NUL */
 #define IDENTSIZE 32
#endif
#ifndef MAXHSH		/* CCSYM: Symbol hashtable size */
 #define MAXHSH (1<<12)	/*	(4096) Must be a power of 2! */
#endif
#ifndef MAXNODE		/* CCSTMT: # of nodes in initial static table */
 #define MAXNODE 100	/* 5/91 changed 4000 to 100, see PPS 4232 */
#endif
#ifndef MAXCASE		/* CCGSWI: Max # of cases per switch (CCGSWI) */
 #define MAXCASE 513	/*	All possible values of char, plus 1 */
#endif
#ifndef MAXCODE		/* CCCODE: Size of peephole buffer */
 #define MAXCODE (1<<8)	/*	(256) Must be a power of 2! */
#endif
#ifndef ERRLSIZE        /* CCERR: Size of error context line */
 #define ERRLSIZE 256
#endif

#if SYS_CSI
 #define DYN_SIZE  128 /* 5/91 Dynamic tables (multiple of block size 128) */
 #ifndef MAXMLBUF
  #define MAXMLBUF 512 /*CC: size of mixed listing's dynamic output buffer */
 #endif
 #define TITLE_SIZE 48	/* #pragma module(title) and request_library(title) */
#endif
