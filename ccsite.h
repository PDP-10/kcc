/*	CCSITE.H - Site-dependent declarations for KCC
**
**	(c) Copyright Ken Harrenstien 1989
**		All changes after v.14, 18-Feb-1987
**	(c) Copyright Ken Harrenstien, SRI International 1985, 1986
*/

/* BEG of site-specific defines */
	/* Any site-dependent definitions which are different from
	** the standard defaults provided in this file should be
	** inserted here.
	*/
/* END of site-specific defines */

#ifndef __MSDOS__
#define __MSDOS__ 0		/* defined by Turbo C and Turbo C++ */
#endif

#include "c-env.h"		/* Get OS defs locally, not from <c-env.h> */

#if (SYS_T20|SYS_10X|SYS_T10|SYS_CSI|SYS_WTS|SYS_ITS)==0
#error KCC cannot run on this system!
#endif

/* Ensure all definitions are set to some default value if not already
** specifically defined.
*/

/* Separator char for prefix/suffix filename components in KCC switches */
#ifndef FILE_FIX_SEP_CHAR
#define FILE_FIX_SEP_CHAR '+'
#endif

/* Define KCC standard Header File location (prefix and suffix strings)
** This is used to search for all <> include files, as well
** as the Assembler Header file.
*/
#ifndef SWI_HFPATH		/* Define path for standard header dir */
#if SYS_T20
#define SWI_HFPATH "C:"
#elif __MSDOS__
#define SWI_HFPATH "\\kcc\\include\\", "\\tc\\include\\"
#elif SYS_10X
#define SWI_HFPATH "<C>"
#elif SYS_ITS
#define SWI_HFPATH "KC;"
#elif SYS_WTS
#define SWI_HFPATH "+[INC,KCC]"
#elif SYS_T10
#define SWI_HFPATH "C:"
#elif SYS_CSI
#define SWI_HFPATH "ALL:[1,17]"
#else
#define SWI_HFPATH NULL
#endif   /* what system */
#endif   /*   defined   */

#ifndef SWI_HFSYPATH		/* Default path for <sys/ > files */
#if SYS_10X
#define SWI_HFSYPATH "<CSYS>"
#elif __MSDOS__
#define SWI_HFSYPATH "\\kcc\\include\\sys\\", "\\tc\\include\\sys\\"
#elif SYS_WTS
#define SWI_HFSYPATH "+[INS,KCC]"
#elif SYS_T10
#define SWI_HFSYPATH "CSYS:"
#elif SYS_CSI
#define SWI_HFSYPATH "ALL:[1,16]"
#else
#define SWI_HFSYPATH NULL
#endif   /* what system */
#endif   /*   defined   */

/* KCC Library file path definitions
**	Same principle as for the header file location.
*/
#ifndef SWI_LIBPATH		/* Define library file prefix */
#if SYS_T20
#define SWI_LIBPATH "C:LIB+.REL"
#elif SYS_10X
#define SWI_LIBPATH "<C>LIB+.REL"
#elif SYS_ITS
#define SWI_LIBPATH "KC;LIB+ REL"
#elif SYS_WTS
#define SWI_LIBPATH "LIB+.REL[INC,KCC]"
#elif SYS_T10
#define SWI_LIBPATH "C:LIB+.REL"
#elif SYS_CSI
#define SWI_LIBPATH "SYS:LIB+.REL"  /* Moved library to SYS: -KAR */
#else
#error Library file location must be specified for system.
#endif   /* what system */
#endif   /*   defined   */

#ifndef SWI_CLEV		/* Specify default C implem level */
#define SWI_CLEV CLEV_STDC	/* Default: STDC!!!  Yay! */
#endif

#ifndef SWI_TGSYS		/* Specify default target system */
#if SYS_T20
#define SWI_TGSYS TGSYS_TOPS20
#elif SYS_T10+SYS_CSI
#define SWI_TGSYS TGSYS_TOPS10
#elif SYS_10X
#define SWI_TGSYS TGSYS_TENEX
#elif SYS_WTS
#define SWI_TGSYS TGSYS_WAITS
#elif SYS_ITS
#define SWI_TGSYS TGSYS_ITS
#else
#define SWI_TGSYS TGSYS_NULL
#endif  /* what system */
#endif  /*   defined   */

#ifndef SWI_ASMHFILE		/* ASM header file location, normally this */
#define SWI_ASMHFILE NULL	/* should be 0 to have KCC generate header. */
#endif

/* The following should eventually be flushed when library tmpfile() works. */
#ifndef SWI_ASMTFILE		/* Specify #asm temporary file name */
#if SYS_T20+SYS_10X
#define SWI_ASMTFILE "ASMTMP.TMP;T"
#else
#define SWI_ASMTFILE "ASMTMP.TMP"
#endif
#endif

#ifdef	MULTI_SECTION /* FW 2A(51) */

/* Loader psect default specifications */
#ifndef PSDATA_BEG	/* Start of data area */
#define PSDATA_BEG 01000
#endif

#ifndef PSCODE_BEG	/* Start of code (text) area */
#define PSCODE_BEG 0400000L
#endif

#ifndef PSCODE_END	/* Upper limit of code area (+1) */
#if SYS_T10+SYS_CSI+SYS_WTS
#define PSCODE_END 0700000L	/* VMDDT starts at page 700 */
#elif SYS_T20+SYS_10X
#define	PSCODE_END 0765000L	/* UDDT vars start at page 770-2 */
#elif SYS_ITS
#define	PSCODE_END 01000000L	/* ITS has HACTRN, needs no crufty DDT */
#endif
#endif

#endif
