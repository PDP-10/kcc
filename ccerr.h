/*	CCERR.H - KCC Error declarations
**
**	(c) Copyright Ken Harrenstien 1989
*/

#if 0

#ifndef __STDC__
#define __STDC__ 0
#endif

#if __STDC__
#define FMTARG char *, ...	/* Prototype args if using ANSI */
#else
#define FMTARG			/* No prototype */
#endif

#endif

extern void
	jmsg(char *fmt, ...),		/* Job message,	"? KCC - " */
	jwarn (char *fmt, ...),		/* Job warning, "% KCC - " FW 2A(47) */
	jerr(char *fmt, ...),		/* Job err msg,	"? KCC - " */
	note(char *fmt, ...),		/* "[Note] " */	
	advise(char *fmt, ...),		/* "[Advisory] " */	
	warn(char *fmt, ...),		/* "[Warning] " */	
	int_warn(char *fmt, ...),	/* "[Warning] Internal error - " */ 
	error(char *fmt, ...),		/* "" */	
	int_error(char *fmt, ...),	/* "Internal error - " */
	efatal(char *fmt, ...),		/* "[Fatal] " with context */
	fatal(char *fmt, ...);		/* "[Fatal] " without context */
void errfopen (char *desc, char *fnam);
extern int expect(int t);
