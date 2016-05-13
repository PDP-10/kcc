/*	CCASMB.C - Assembler and Linker invocation
**
**	(c) Copyright Ken Harrenstien 1989
**		All changes after v.97, 12-Aug-1988
**	(c) Copyright Ken Harrenstien, SRI International 1985, 1986
**		All changes after v.23, 8-Aug-1985
**
**	Original version (c) 1981 K. Chen
*/

#include "ccsite.h"
#include "ccchar.h"
#include "cc.h"
#include <stdlib.h>			/* calloc, realloc, free */
#include <sys/types.h>			/* For stat(), for symval stuff */
#include <sys/stat.h>

#if !__MSDOS__ 							// FW KCC-NT
 #include <sys/file.h>			/* For open() */
#endif

#include <errno.h>			/* For strerror */
#include <string.h>			/* For strchr etc */

#if !__MSDOS__							// FW KCC-NT
 #include <frkxec.h>			/* New stuff */
#endif

#if __MSDOS__				/* 4/92 avoid non-ANSI stat() */
 #define stats stat			/* just use stat() function */
#else

 #ifdef mod /* KAR-4/92, remove collision with mod in comdef (boom!) */
  #undef mod
 #endif

 #include <sys/usysio.h>

 #undef sixbit	/* KAR-8/92, accomodate comdef.h def of sixbit */
 #include <csimon.h>	/* KAR-5/92, for MON_ calls for times */
 #define sixbit _sixbit /* KAR-8/92, see comment on #undef */

int	    stats (char *source_fname, struct stat *statb);
static
int	    long to_sixbit (char *str);
static
char	    stats_parse = 0;
#endif

#include <stdio.h>

#if !__MSDOS__							// FW KCC-NT
 #define _getpid(_pid)  ((MUUO_VAL("PJOB", &_pid)), _pid)

 #include <muuo.h>	/* For TMPCOR etc */
#endif
#define PRGBLEN 0

#ifndef RH
 #define RH 0777777L	/* Mask for RH of a word */
#endif

#ifndef LH
 #define LH (-1<<18)	/* Mask for LH */
#endif

#ifndef XWD		/* Put halves together */
 #define XWD(a,b) (((unsigned)(a)<<18) | ((unsigned)(b)&RH))
#endif

/* Exported routines */

int	    asmb (char *, char *, char *);
char*	    fnparse (char *, char *, char *, char *, char *);
int	    fnxisrel (char *);
void	    runlink (int, int, char **, char *, char *);
int	    symval (char *fnam, char *sym, int valf);
char*	    estrcpy (char *, char *);
char*	    fstrcpy (char *, char *, char *);
INT	    sixbit (char *);

/* Imported routines */

#if !SYS_CSI /* KAR-3/92, removed LIBC dependency, use ANSI calls */

extern
int	    open (char *path, int flags),		/* , int mode)?? */
	    getpid (void),
	    stat (char *, struct stat *),
	    fstat (int handle, struct stat *),

#ifdef __COMPILER_KCC		/* BC++ error */
	    unlink (char *),		/* Syscalls */
#endif

	    close (int fd),
	    read (int fd, char *buf, int nbytes);
#else
extern
int	    remove (const char *);
#endif

#ifdef __COMPILER_KCC__
extern
int	    forkexec (struct frkxec *f);
#endif /* KCC */

/* Internal routines */

static
char*	    gtmpfile (char *);
static
int*	    stmpfile (char *, char *, char *);
static
int*	    maktflink (int, char **, char *, char *),
	    crsfunv (char *, long),
	    hackfork (char *, int *, int, int, int),
	    ldsymfile (char *);

#if DEBUG_KCC	/* 8/91 KCC size */
static
int	    tdebug = 0;	/* Set non-zero to print out tmpcor args */
#endif

static
char*	    asmtfptr = NULL;	/* Assembler tmpcor file contents */
static
int	    asmtflen = 0;	/* Length not including trailing NUL */

/* BP7 - macro to convert a char ptr into a 7-bit byte pointer */

#if __MSDOS__
#include <process.h>	/* getpid() */
#define bp7(cp) ((char *)(cp))
#else
#define bp7(cp) ((char *)(int)(_char7 *)(cp))
#endif

/*
Description of COMPIL (or RPG) argument passing mechanism.

On TOPS-10 and CSI there is a convention for passing arguments
amongst programs which compile and load programs.  This is known as
RPG ("Rapid Program Generation"), and is only invoked by the so-called
COMPIL-class commands:
    COMPILE, LOAD, EXECUTE, DEBUG.

When a program is invoked by the TOPS-20 EXEC (or the TOPS-10
monitor) at its starting address plus 1, it is expected to look for
command input from a temporary file rather than from the user's TTY.
This consists of first looking for a TMPCOR file of a specific name
associated with that program (MAC for MACRO, LNK for LINK, KCC for
KCC, etc.).

This is done with the TMPCOR UUO.  The file is deleted when read.
If there is no such temporary file, then the program is expected to look
for an actual disk file of the name:
DSK:nnnNAM.TMP where nnn is the job number in decimal (right-justified
with leading zeros).  This file should likewise be deleted after reading.

The TMPCOR file furnished to a compiler/assembler contains a single
command line for each file to be compiled, of the
format:
NAME,=NAME.EXT<crlf>

The monitor has arcane special knowledge of just how each
program, especially LINK, likes its commands formatted.  The above
appears to be most common, however.  Finally, the way that the monitor sets
up a chain of programs is by adding a final command line to the file
of the form:
<program-file>!<crlf>
which instructs the compiler/assembler to load and run the specified
program, starting it with an offset of 1.  This is a full filename
specification and no searching should be needed.  For KCC this will normally
be SYS:LINK.EXE if LINK is to be invoked next.
*/

/*
 * ASMB - Set up assembler arguments.  We must defer assembly to the end
 *		of compilation, so remember all the assembler arguments
 *		in an allocated buffer.
 *
 *		Returns -1 if error, 0 if arguments successfully stored.
 */

int
asmb (char *m, char *f1, char *f2)
    {
    char	str[FNAMESIZE*4];	/* Big enough for 3 names plus punct */
    char*	nptr;
    int		n;


    sprintf (str, "%s%s=%s%s%s%s%s\n",
	     m,				/* Specify output file name */
	     (longidents ? "/k" : ""),	/* FW 2A(51) Using long identifiers? */
	     (f1 ? f1 : ""),		/* Specify 1st input file if one */
	     ((f1 && delete) ? "/d" : ""), /* delete asm source  */
	     (f1 && f2) ? "," : "",	/* Use separator if 2 inputs */
	     (f2 ? f2 : ""),		/* Specify 2nd input file if exists */
	     ((f2 && delete && (!mlist)) ? "/d" : ""));
					/* Specify delete asm source */

    /* Add new command to assembler's saved TMPCOR file block */

    n = strlen (str);			/* Find # chars needed */

    if ((nptr = realloc (asmtfptr, asmtflen + n + 1)) == NULL)
	jerr ("Out of memory for assembler tmpcor file (%d chars)", asmtflen);
    else
	{
	asmtfptr = nptr;
	strcpy (asmtfptr + asmtflen, str); /* Add new cmd to end */
	asmtflen += n;
	}

    return 0;				/* Say assembly was deferred */
    }

/*
 * EXECARGS - Get args, UNIX style, from exec or monitor TMPCOR file
 *	The code is given a char pointer to a TMPCOR-format file
 * in memory, and assumes that this memory will stay constant throughout
 * this invocation of KCC, so it is OK to traffic in pointers into this
 * area.
 */

char *
execargs (int *cntp, char ***vecp)
    {
#define MAXARGS 200
    static char *vecs[MAXARGS];		/* Easier than calloc */
    int c, vcnt;
    char *cp, *err = NULL;
    char *nextprog = NULL,
	 *p = gtmpfile("KCC"); /* Get temp file of this name */
#if DEBUG_KCC
    char *begp = p;
    int flen = strlen(begp); /* Remember length now before chopping it up */
#endif


    if (p == NULL)
	{
	*cntp = 0;
	*vecp = NULL;
	return NULL;
	}

    vecs[0] = "exec-args";	/* Always furnish 1st arg */
    vcnt = 1;
    while (1)			/* For each command line in file */
	/* Skip to start of input filename */
	{
	cp = p;			/* Remember start of stuff */
	while (1)
	    {
	    switch (c = *p)
		{
		case '=':		/* Found filename to compile? */
		    break;		/* just get out of loop. */

		case '\0':		/* Ran out of input */
		    if (cp != p)	/* If not at beg of line, */
			err = "command line doesn't end with CR or LF";
		    break;

		case '!':		/* Chain through this program */
		    if (cp == p)
			err = "null program name for chain";
		    *p = '\0';		/* Terminate program name */
		    nextprog = cp;	/* Save pointer to it */
		    if (*++p != '\r' && *p != '\n')	/* Must end in EOL */
			err = "no EOL after chain program name";
		    else if (*++p	/* If there is another char */
			&& ((*p != '\r' && *p != '\n')	/* it shd be EOL */
			   || *++p))	/* followed by a null. */
			err = "unexpected stuff after chain program name";
		    break;

		case '\r':
		case '\n':
		    if (cp == p)	/* Allow null lines */
			{
			cp = ++p;	/* Just reset ptr */
			continue;
			}
		    /* Line has something but we didn't understand it */
		    err = "command line not in recognized format";
		    break;

		default:
		    ++p;
		    continue;
		}
	    break;
	    }
	if (!c || err)		/* If hit end of input, or had error, */
	    break;		/* stop now. */

	c = *++p;		/* Hit '=', move on to next char */

    /*
    ** Allow /LANGUAGE-SWITCHES:" -x" to work
    ** This is done by scanning the argument string for spaces,
    ** and processing strings after the spaces as separate arguments.
    **
    ** We can't just do the usual thing of breaking on slashes
    ** because we treat those as parts of the filenames, for UNIX
    ** pseudo-compatibility.  This may change someday.
    */
	while(1)
	    {
	    while(c == ' ')
		c = *++p;	/* Skip over initial spaces */
	    vecs[vcnt] = p;		/* Remember this pointer */

	    while ((c != '\0') && (c != '\n') && (c != '\r')
			&& (c != ' '))
		{
		c = *++p;
		}
	    if (vecs[vcnt] != p)	/* Did we get anything? */
		{
		*p = '\0';		/* Yes, ensure null-terminated */
		if (vcnt >= MAXARGS-2)
		    {
		    err = "too many arguments!  (internal error)";
		    break;
		    }
		vcnt++;			/* Welcome it to the ranks */
		}
	    if (c != ' ')
		break;	/* Unless space, done */
	    } /* end inner loop (process line) */
	if (!c || err)
	    break;		/* If done or error, stop now */
	++p;				/* Move on to next char */
	}
    vecs[vcnt] = NULL;		/* Make list end with null */

#if DEBUG_KCC	/* 8/91 KCC size */
    if (tdebug || err)
	{
	if (err)
	    jerr("Bad exec/monitor args - %s", err);
	fprintf(outmsgs,"Contents of PRARG%%/TMPCOR file:\n");
	fwrite(begp, sizeof(char), flen, outmsgs);
	fprintf(outmsgs, "\nKCC args:");
	for (c = 0; c < vcnt; ++c)
	    fprintf(outmsgs, " %s", vecs[c]);
	fprintf(outmsgs, "\n");
	if (err)
	    {
	    *cntp = 0;
	    *vecp = NULL;
	    return NULL;
	    }
	}
#endif
    *cntp = vcnt;		/* Set return value = # of args */
    *vecp = vecs;
    return nextprog;
}

/* GTMPFILE - Get "temporary file" which contains arguments to program.
**	See description of RPG argument passing.
*/
static char *
gtmpfile(char *name)
{
    int nchars, i, c;
    char *cp, *rp;
    FILE *tf;
    char tmpfile[20];		/* For DSK:nnnNAM.TMP */
    int pid = 0;

    /* See if TMPCOR UUO has anything for us */
	{
	INT argblk[2];
	int junk, ret = 20;		/* MUUO call sets ret */
	char *tfbuf;

	argblk[0] = sixbit(name) & ~RH;		/* Set up TMPCOR arg blk */
	argblk[1] = XWD(0, &junk-1);
#ifdef __COMPILER_KCC__
	if (MUUO_ACVAL("TMPCOR", XWD(uuosym(".TCRRF"),argblk), &ret) > 0)
#endif
	    {
	/* If we succeed, file exists and # wds is now in ret */
	    if ((tfbuf = calloc(1,ret*sizeof(int)+1)) == NULL)
		{
		jerr("Unable to alloc %d wds for TMP file %s", ret,tmpfile);
		return NULL;
		}
	    argblk[1] = XWD(-ret, ((int *)tfbuf)-1);
	    tfbuf[ret*sizeof(int)] = '\0';	/* Ensure null-terminated */
#ifdef __COMPILER_KCC__	
	    if (MUUO_AC("TMPCOR", XWD(uuosym(".TCRDF"),argblk)) <= 0)
		{
		jerr("TMPCOR of %s failed on re-read", tmpfile);
		return NULL;
		}
	    return bp7(tfbuf);		/* Return 7-bit byte ptr */
#endif
	    }
	}

    /* Try opening a .TMP file */
    sprintf(tmpfile,
#if !__MSDOS__
	/* KAR-3/92, use new _getpid() macro instead of routine in LIBC */
	"DSK:%03.3d%.3s.TMP", ((pid == 0) ? _getpid(pid) : pid)
#else
	"%03.3d%.3s.TMP", getpid()
#endif
		    , name);
    if ((tf = fopen(tmpfile, "r")) == NULL)
	return NULL;
#define TMPBSIZ (0400*sizeof(int))		/* # chars in incr blk */
    nchars = TMPBSIZ;
    rp = calloc(1,nchars);
    cp = rp-1;
    for (;;)
	{
	if (rp == NULL)
	    {
	    jerr("Unable to alloc %d chars for TMP file %s", nchars, tmpfile);
	    return NULL;
	    }
	for (i = TMPBSIZ; --i >= 0;)
	    {
	    if ((c = getc(tf)) == EOF)
		{
		*++cp = 0;		/* Fix up last char */
		fclose(tf);		/* Close the stream */

		remove(tmpfile);
		return rp;		/* Won! */
		}
	    *++cp = c;
	    }
	rp = realloc(rp, (nchars + TMPBSIZ));
	if (rp)
	    cp = rp + nchars - 1;	/* Point to last char deposited */
	nchars += TMPBSIZ;
	}
}



/* STMPFILE - Set "temporary file" furnishing args to another program.
**	See description of RPG argument passing.
*/
#if 0				/* 8/91  KCC size */
static int usetmpcor = 0;	/* For time being, don't use it! */
#endif

static int *
stmpfile(char *name, char *str, char *nextprog)
{
    FILE *tf;
    char *cp = NULL;
    char tmpfile[20];		/* For DSK:nnnNAM.TMP */
    int nchs = strlen(str) + 1;		/* total chs we'll need */
    int pid = 0;

#if 0		/* 8/91  KCC size */
    int nwds = (nchs+4)/5;
#endif

    if (nextprog && *nextprog)
	{
	nchs += strlen(nextprog)+3;	/* nextprog can add ! and CRLF */
	if ((cp = calloc(1,nchs)) == NULL)
	    {
	    jerr("Cannot get memory for %s program args", name);
	    return NULL;
	    }
	estrcpy(estrcpy(estrcpy(cp, str), nextprog), "!\r\n");
	str = cp;
	}
    if (vrbld)
	fprintf(outmsgs, "%s program args: \"%s\"\n", name, str);

#if 0		/* 8/91  KCC size */
    /* Try invoking TMPCOR UUO */
    if (usetmpcor)
	{
	INT argblk[2];

	argblk[0] = sixbit(name) & ~RH;		/* Set up TMPCOR arg blk */
	argblk[1] = XWD(-nwds, ((int *)str)-1);
	strcpy(bp7(str), str); /* Make 7-bit buff */
	if (MUUO_AC("TMPCOR", XWD(uuosym(".TCRWF"),argblk)) > 0)
	    {
	    if (cp)
		free(cp);
	    return NULL;			/* Won, that's all! */
	    }
	/* No tmpcor, must use file.  Fix up buff ptr to reflect 7-bitness */
	str = bp7(str);
	}

#endif

    /* Can't use PRARG% or TMPCOR, make .TMP file */
    sprintf(tmpfile,
#if __MSDOS__
	"%03.3d%.3s.TMP",   getpid() & 0777,
#else
	"DSK:%03.3d%.3s.TMP", ((pid == 0) ? _getpid(pid) : pid),
#endif
		    name);
    if ((tf = fopen(tmpfile, "w")) == NULL)
	{
	errfopen("output TMP", tmpfile);
	if (cp)
	    free(cp);
	return NULL;
	}
    fputs(str, tf);		/* Write out the string */
    fclose(tf);
    if (cp)
	free(cp);
    return NULL;
}

/* MAKTFLINK - Make argument "file" for LINK.
**	This may be either a PRARG% block or TMPCOR-type file.
**	Note that ALL of the argument array is examined, including
**	the first one!  This may be set by the mainline to something special.
*/
/* ofilename is the filename to save executable image to */
/* nextprog is the next program to chain thru, if any */

static
int *
maktflink (int argct, char **argvt, char *ofilename, char *nextprog)
{
    extern char *libpath;	/* Specified in CCDATA */
    char *s, *t, *cp;
    char module[FNAMESIZE], ext[FNAMESIZE];
    char tmpfile[4000];			/* Very large buffer (sigh) */
    char *libstart = NULL;
    extern char *savofnam;	/* Defined in CC */


    /* Put together the command string for LINK */

    t = tmpfile;

    sprintf (t, "/define:$stksz:%d\n", stksz);	/* define stack size */
    t += strlen (t);

    if (profbliss)

        /*
	 * The BLISS profiler wants symbols high.
	 */

	t = estrcpy (t, "/symseg:high\n");
    else

    	/*
	 * The C profilers need symbols low.
	 */

	switch (abs (debcsi))
	    {
	    case KCC_DBG_SPRF :
		t = estrcpy (t, "SYS:KCCDBG/INCL:SPROF\n");
		break;


	    case KCC_DBG_FPRF :
		t = estrcpy (t, "SYS:KCCDBG/INCL:FNPROF\n");
		break;


	    default:
		break;
	    }

    if (ldddtf)					/* shall I link in DDT?  */
	t = estrcpy (t, "/test:DDT\n");		/* added 09/15/89 by MVS */

    /* Do preliminary stuff */

#ifdef	MULTI_SECTION /* FW 2A(51) */
    if (ldpsectf)		/* Hacking PSECTs? */
	{
	sprintf (t,"/SET:DATA:%o/LIMIT:DATA:%o/SET:CODE:%o/LIMIT:CODE:%o\n",
		ldpsdata.ps_beg, ldpsdata.ps_lim,
		ldpscode.ps_beg, ldpscode.ps_lim );
	t += strlen (t);
	t = estrcpy (t, "/REDIRECT:DATA:CODE/SYMSEG:PSECT:DATA\n");
	}

    if (ldextf)		/* Hacking extended addressing? */
	{
	t = fstrcpy (t, libpath, "ckx"); /* -i Preload x-addr module */
	*t++ = ',';
	}
#endif

    while (--argct >= 0)		/* while we have arguments left */
	{
	s = *argvt++;

	if (s == NULL)
	    continue;	/* Skip over ex-switches */

	if (*s == '-')
	    {
	    switch (*++s)	/* Is this a still-alive switch? */
		{
		case 'l':			/* -l Library search request */
		    if (libstart == NULL)
			libstart = t;
		    t = estrcpy (fstrcpy (t, libpath, s+1), "/SEARCH");
		    break;


		case 'L':			/* -L= Pass string to loader */
		    if (*++s == '=')
			{
			if (libstart == NULL)
			    libstart = t;
			t = estrcpy (t, s + 1);	/* Just copy it directly! */
			break;
			}

		/* If -L not followed by =, drop thru and complain */

		default:
		    jerr ("Internal error: bad LINK sw \"%s\"", argvt[-1]);
		}
	    }
	else		/* Assume it's a filename */
	    {
	    if ( (cp = fnparse (s, NULL, module, ext, NULL)) != NULL)
		jerr ("Bad filename arg for LINK (%s): \"%s\"", cp, s);
	    else if (fnxisrel (ext))	/* If it's a .REL file, copy */
		t = estrcpy (t, s);	/* entire name */
	    else if (libstart == NULL)
		t = estrcpy (t, (savofnam ? savofnam : module));
	    else
		{
		char *ptr;
		int length = strlen (module) + 1;

		/* ensure module(s) before libpath(s) */

		for (ptr = t - 2; ptr >= libstart; ptr--)
		    ptr[length] = ptr[0];	/* shift over libpath (s) */

		libstart = estrcpy (libstart, module);	/* insert module */
		*libstart++ = ',';
		t = t + length - 1;
		}
	    }

	if (profbliss)				/* for BLISS profiler */
	    t = estrcpy (t, "/locals");		/* added 09/15/89 by MVS */

	*t++ = ',';
	}

    /* Add final commands to SAVE and GO */

    t = estrcpy (estrcpy (estrcpy (t, "\n"), ofilename),
	    (!(ldddtf | debcsi | profbliss) ? "/SSA/RUN:MAKSHR" : "/SSA"));
    estrcpy (t, "/GO\n");

    if (vrbld)
	fprintf (outmsgs, "Loader argument: \"%s\"\n", tmpfile);

    return stmpfile ("LNK", tmpfile, nextprog);
}
    
/*
 * RUNLINK - invoked when all compilation done.
 *	May invoke assembler, linker, or a "next program".
 *	Always chains through, never returns.
 */

void
runlink (int linkf, int argc, char** argv, char* ofilename, char* nextprog)
    {
    int*	loc = NULL;
    char*	pname = NULL;
    char	mks_cmd_str[25];


    /* Need to invoke assembler? */

    if (asmtfptr)
	{
	pname = "MACRO";		/* Default */
	stmpfile (pname, asmtfptr,	/* Build TMPCOR or .TMP file */
		 (link ? "LINK" : nextprog));
	}

    /* Need to invoke loader? */

    if (linkf)
	{
	loc = maktflink (argc, argv, ofilename, nextprog);

	if (!(ldddtf | debcsi | profbliss))
	    {
	    sprintf (mks_cmd_str, "%s/o\n", ofilename);
	    stmpfile ("MKS", mks_cmd_str, NULL);
	    }

	if (!pname)
	    pname = "LINK";
	}

    /*
     * Check for case of chaining through random program.  If so,
     * we don't even bother setting the PRARG% block or TMPCOR file or
     * anything, since all should already have been set up by the EXEC, and
     * as long as we're chaining, the new program will have access to the same
     * PRARG% block that we originally read.  And of course if a disk .TMP
     * file was used, that should already exist.
     */

    if (!pname)				/* If neither asm or link, */
	{
	if ((pname = nextprog) == NULL)	/* must be chaining random prog. */
	    fatal ("Internal error: Chaining to no program!");
	}

    /* Invoke program!! */
    /* Note that loc will be set only if a PRARG% block is being used. */

    if (!hackfork (pname, loc, PRGBLEN, 1, 1))
	fatal ("Could not chain to %s", pname);	/* never to return */
    }

static
int
hackfork (char* pgmname, int* argblk, int blklen, int stoffset, int chainf)
    {
#if !__MSDOS__							// FW KCC-NT
    struct
    frkxec	fx;


    fflush (stdout);	/* Make sure no output lost if chaining */

    fx.fx_flags = FX_PGMSRCH | FX_WAIT
		| (stoffset ? FX_STARTOFF : 0)
		| (chainf ? FX_NOFORK : 0);
    fx.fx_name = pgmname;
    fx.fx_argv = fx.fx_envp = NULL;
    fx.fx_startoff = stoffset;
    fx.fx_blkadr = (char *) argblk;
    fx.fx_blklen = blklen;

#ifdef __COMPILER_KCC__
    return (forkexec (&fx) < 0 ? 0 : 1);
#else
    return 1;	/* assume OK */
#endif

#else
	return 1;
#endif
}


/*
 * FNPARSE - do simple parsing of filename; tries to find the module
 *	name component, and everything else is derived from that.
 */

char *
fnparse (char* source, char* dir, char* name, char* ext, char* suf)
    {
#if __MSDOS__
#ifndef	WIN32							// FW KCC-NT
    int		fnsplit (const char *source_path, char *suf_drive, char *dir,
			 char *name, char *ext);


    fnsplit (source, suf, dir, name, ext);
    return NULL;	/* assume OK */
#else
	char		buf[256];
	char*		cp;


	strcpy (buf, source);				// Make a copy we can play with.
	*suf = '\0';						// Not going to return this anyhow.

	cp = strrchr (buf, '.');			// Is there an extension?

	if (cp)
		{
		strcpy (ext, cp);				// Yes: return it.
		*cp = '\0';						// Delimit the basename.
		}
	else
		{
		*ext = '\0';					// No: tell the caller.
		cp = buf + strlen (buf);		// Set up to parse the basename.
		}

	do
		{
		--cp;
		}
	while ((cp >= buf) && (*cp != '\\') && (*cp != '/') && (*cp != ':'));

	strcpy (name, ++cp);
	*cp = '\0';
	strcpy (dir, buf);

	return NULL;
#endif
#else
    register
    char*	cp = source;
    char*	start = cp;
    int		devf = 0;
    int		i;


    *name = *ext = 0;	/* Init all returned strings */

    if (dir)
	*dir = 0;	/* optional args */

    if (suf)
	*suf = 0;

    for (;;)
	{
	switch (*cp)
	    {
	/* Check possible name terminators */
	    case '\0':
	    case '.':
		break;

	/* Check for directory start chars (must skip insides) */

#define DIRSTOP ']'
	    case '[':
		if (start != cp)		/* If collected a name, */
		    break;			/* we've won, OK to stop! */

		if ( (cp = strchr (cp, DIRSTOP)) == NULL)
		    return "malformed directory name";
	    /* Now drop through to handle as if randomly terminated */

	/* Check for other random word terminators */
	    case DIRSTOP:		/* Directory */
	    case '/':			/* Un*x-simulation directory */
		start = ++cp;
		continue;

	/* Do special check to permit "DEV:" alone to succeed */
	    case ':':			/* Device */
		if (start == source && !cp[1])	/* 1st & only word? */
		    {
		    ++devf;		/* Yep, set flag and stop */
		    break;
		    }
		else if (start == source && stats_parse)
		    {
		    short pos = cp - source;
		    strncpy (dir,source,pos);
		    dir[pos]='\0';
		    }

		start = ++cp;		/* Nope, start new name search */
		continue;

	/* Check for string-type quote chars */
	    case '\"':
		i = *cp++;			/* Remember the char */
		if ( (cp = strchr (cp, i)) == NULL)
		    return "unbalanced quoted name";
		++cp;			/* Include ending quote */
		break;			/* and assume this was filename */

	/* Check for single-char quote chars */
	    case '\\':			/* Backslash */
		if (!cp[1])			/* Don't permit quoting NUL */
		    return "NUL char quoted";
		++cp;			/* Else OK, include quote char. */

	/* Handle normal filename char */
	    default:
		++cp;
		continue;
	    }
	break;			/* If break from switch, stop loop. */
	}

    /* Found name if any, rest is easy */
    i = cp - start;
    if (i == 0)	/* Find length of name */
	return "no module name";	/* Ugh */

    strncat (name, start, i);	/* Return module name */
    if (devf)			/* Handle "FOO:" specially */
	{
	if (dir)
	    strcpy (dir, source);
	return NULL;
	}
    if (start != source && dir && !stats_parse)	/* Copy stuff preceding name */
	strncat (dir, source, start-source);

    /* Now check for extension.  Slightly tricky. */
    if (*cp == '.')
	{
	start = cp;
	while (isalnum (*++cp))
	    ;		/* Scan over valid chars */
	strncat (ext, start, cp-start);	/* Copy what we got of extension */
	}

    if (*cp && suf)			/* If anything left after ext, */
	strcpy (suf, cp);		/* just copy it all. */

    return NULL;			/* Say we won, no error msg! */
#endif	/* not MSDOS */
    }

int
fnxisrel (char* cp)
    {
    return (*cp == '.'
#if __MSDOS__
	    && toupper (*++cp) == 'O'
	    && toupper (*++cp) == 'B'
	    && toupper (*++cp) == 'J'
#else
	    && toupper (*++cp) == 'R'
	    && toupper (*++cp) == 'E'
	    && toupper (*++cp) == 'L'
#endif
	    && ! (*++cp));
    }



/* Misc auxiliaries */
/* ESTRCPY - like STRCPY but returns pointer to END of s1, not beginning.
**	Copies string s2 to s1, including the terminating null.
**	The returned pointer points to this null character which ends s1.
*/

char *
estrcpy (register char *s1, register char *s2)
{
    if ( (*s1 = *s2) != '\0')
	while ( (*++s1 = *++s2) != '\0')
	    ;
    return s1;
}

/* FSTRCPY - Filename string copy.  Builds a filename string out of
**	a name and prefix+suffix string, and copies it.
**	Like estrcpy (), the returned pointer points to the END of the
**	copied string.
*/
char *
fstrcpy (register char *d, register char *ps, register char *fname)
{
    if (!ps || !*ps)			/* If no prefix+suffix, */
	return estrcpy (d, fname);	/* just copy basic filename */
    if ( (*d = *ps) != FILE_FIX_SEP_CHAR)	/* If have a prefix, */
	while ( (*++d = *++ps) != '\0' && *d != FILE_FIX_SEP_CHAR)
	    ;	/* copy it */
    d = estrcpy (d, fname);		/* Done, now copy filename */
    return *ps ? estrcpy (d, ps+1) : d;	/* then suffix, if any */
}

INT
sixbit (str)
register char *str;
{
    register int i = 6*6, c;
    register unsigned INT val = 0;

    --str;
    while (i > 0 && (c = *++str) != '\0')
	val |= tosixbit ((char) c) << (i -= 6);	// FW KCC-NT

    return val;
}
INT
rad50 (str)
register char *str;
{
    register char i = 6, c;
    register unsigned INT val = 0;

    --str;
    while (i-- > 0 && (c = *++str) != '\0')
	val = val * 050 + torad50 (c);

    return val;
}

/* SYMVAL - Implements _KCCsymval ("filename", "symbol").
**	Called by CCPP to evaluate as a macro expansion.
**	Returns the value of the symbol as looked up in "filename".
**	May encounter an error, in which case a value of 0 is returned.
** If "valf" is zero, we are just seeing whether it is defined or not,
** and only problems with loading the file cause errors.
*/

#define WORD INT
#define WDSIZ (sizeof (INT))
#define RNDUP(a) ((((a)+WDSIZ-1)/WDSIZ)*WDSIZ)	/* Round up # bytes */

#define SF_UNVTYP 0			/* Only .UNV format understood */

struct symfile
    {
    struct symfile
    *sf_next;	/* Ptr to next sym file in chain */
    int sf_type;			/* Type of file (UNV, etc) */
    int sf_ents;			/* # entries in table */
    struct sfent			/*	Format of a table entry */
	{
	INT se_sym;		/*		SIXBIT symbol name */
	INT se_val;		/*		symbol value */
	}
    *sf_entp;			/* Location of table array */
    char sf_fname[1];		/* Name of this file */
    };	/* Filename string is stored immediately following struct */

static struct symfile
*sfhead = NULL;	/* Initially no files loaded */


int
symval (char *fnam, char *sym, int valf)	/* valf=true if want symbol's value */
{
    register struct symfile
    *sf;
    register struct sfent
    *ep;
    register int cnt;
    INT sym6;			/* Symbol name in sixbit */

    /* First search existing loaded files to find right one */
    for (sf = sfhead; sf; sf = sf->sf_next)
	if (strcmp (fnam, sf->sf_fname)==0)	/* Must be exact match */
	    break;

    /* If not already loaded, try to load it. */
    if (!sf)
	{
	if (!ldsymfile (fnam))	/* Load up a symbol file */
	    return 0;		/* If failed, just return now */
	sf = sfhead;		/* else 1st symfile block is right one! */
	}

    /* Now have sf pointing to right symfile block. */
    if (sf->sf_type < 0 || (cnt = sf->sf_ents) <= 0)
	{
	if (valf)
	    error ("No symbols for \"%s\"", fnam);
	return 0;
	}

    sym6 = sixbit (sym);		/* Get sixbit for symbol */
    ep = sf->sf_entp;
    for (; --cnt >= 0; ++ep)	/* Scan thru table looking for sym match */
	{
	if (ep->se_sym == sym6)
	    return (int) (valf ? ep->se_val : 1);	/* Found it! */
	}

    if (valf)
	error ("Symbol lookup failed for \"%s\" in \"%s\"", sym, fnam);
    return 0;			/* Failed, just return zero. */
}

/* LDSYMFILE - Auxiliary for SYMVAL, loads a symbol def file.
**	Returns 0 if error, but new symfile struct is always added to
**	list.
*/
static int
ldsymfile (fnam)
char *fnam;
{
    register struct symfile
    *sf;
    char *tabp;
    struct stat statb;
    int typ, nsyms, sts;
    long flen;
    FILE *fp;


    /* Allocate a symfile struct, plus room for filename after it */
    sf = (struct symfile
    *)calloc (1,sizeof (struct symfile
    ) + strlen (fnam)+1);
    if (sf == NULL)
	{
	error ("Out of memory for symfile \"%s\"", fnam);
	return 0;
	}
    strcpy (sf->sf_fname, fnam);		/* Copy filename into block */
    sf->sf_type = -1;			/* Say no contents yet */
    sf->sf_next = sfhead;		/* Link onto start of list */
    sfhead = sf;


    if ( (sts = stats (fnam, &statb) != 0)
      || (fp = fopen (fnam, "rb")) == NULL)
	{
#if 0 /* kar */
	fprintf (stderr, "sts: %d fp: %o, %s\n", sts, fp, fnam);
#endif
	error ("Could not open symbol file \"%s\": %s",
		    fnam, strerror (_ERRNO_LASTSYSERR));

	if (fp == NULL)
	    fclose (fp);
	return 0;
	}
    flen = statb.st_size;		/* Get file length in bytes */

    /* Find initial size to allocate for contents, and read it in. */
    
    if ((tabp = (char *) calloc (1, (size_t) (RNDUP (flen)))) == NULL)
	error ("Out of memory for symbol file \"%s\"", fnam);

    else if ((long) (fread (tabp, 1, flen, fp)) != flen)
	{
	error ("Could not read symbol file \"%s\": %s",
			    fnam, strerror (_ERRNO_LASTSYSERR));
	free (tabp);
	tabp = NULL;
	}

    fclose (fp);

    if (!tabp)
	return 0;

    /* Now grovel over the contents, turning it into a simple array
    ** of "sfent" symbol entries.
    */
    switch (typ = SF_UNVTYP)		/* May someday have other types */
	{
	case SF_UNVTYP:
	    nsyms = crsfunv (tabp, flen);
	    if (nsyms < 0)
		{
		error ("Symbol file \"%s\" not in UNV format", fnam);
		nsyms = 0;
		}
	    break;
	default:
	    nsyms = 0;
	    break;
	}

    /* Done crunching file into table, now reallocate storage so as to
    ** free up the space we don't need.
    */
    if (nsyms == 0)
	{
	free (tabp);
	tabp = NULL;
	}
    else if ( (tabp = realloc (tabp, nsyms*sizeof (struct sfent
    ))) == NULL)
	int_warn ("Couldn't trim mem for symbol file \"%s\"", fnam);

    sf->sf_entp = (struct sfent
    *)tabp;
    sf->sf_ents = nsyms;
    sf->sf_type = typ;		/* Store type to say now OK */
    return 1;
}


#if !__MSDOS__	/* 4/92 avoid non-ANSI stat () (use LIBCA) */

static void ppnprs (struct _filespec
*f, char *beg, char *end);

int
stats (char *source_fname, struct stat
*statb)
{
    int ret;
    struct _filespec fs = {0};
    struct _filehack ff, *f = &ff;
    MON_MJD	mtime;
    char nname[64];		/* KAR-5/92, added to hold name w/o ppn */
    char *beg, *end;		/* and added these to marc beg & end of PPN */
    char dev[7], fname[7], ext[5], suffix[5];

    if ( (beg = strchr (source_fname, '[')) != NULL)
	{
	if ( (end = strchr (beg, ']')) == NULL)
	    return -1;					/* invalid PPN */
	else
	    ppnprs (&fs, beg, end);

	strncpy (nname, source_fname, (beg - source_fname));
	strcat (nname, (end + 1));
	f->lerppn = fs.fs_path.p_path.ppn;
	}
    else
	{
	strcpy (nname, source_fname);
	f->lerppn = 0;
	}

#if 0
    fprintf (stderr, "nname=%s\n", nname);
#endif

    stats_parse = 1;
    fnparse (nname, dev, fname, ext, suffix);
    stats_parse = 0;
    fs.fs_dev = to_sixbit (dev);
    fs.fs_nam = to_sixbit (fname);
    fs.fs_ext = to_sixbit (&ext[1]);	/* want C not .C */
#if 0 /* kar */
    fprintf (stderr, "%s=0%o %s=0%o %s=0%o, ppn=0%o\n", dev, fs.fs_dev, fname,
	fs.fs_nam, ext, fs.fs_ext, f->lerppn);
#endif
    ff.fs = fs;	/* Copy filespec into big hack struct */

    if (!fs.fs_dev)
	fs.fs_dev = _SIXWRD ("DSK");/*supply default device*/

#if 0 /* KAR-5/92, removed DEVCHR call because it's result wasn't used */
    MUUO_ACVAL ("DEVCHR", fs.fs_dev, &ret);
    if (!ret)
	return -1;
#endif

    f->filop.fo_ios = 0;		/* Simply zap all flags & mode */
    f->filop.fo_brh = 0;		/* Use no buffering */
    f->filop.fo_dev = fs.fs_dev;
    f->xrb.s.rb_cnt = uuosym (".RBTIM");        /* # wds following */
    
    /* KAR-5/92, changed assignment from f->lerppn to 0, lerppn was undef */
    f->xrb.s.rb_ppn = f->lerppn;
    f->xrb.s.rb_nam = fs.fs_nam;
    f->xrb.s.rb_ext.wd = fs.fs_ext;
    f->xrb.s.rb_prv.wd = 0;
    f->filop.fo_chn = uuosym ("FO.ASC") >> 18;
    f->filop.fo_fnc = uuosym (".FORED");
    f->filop.fo_nbf = 0;
    f->filop.fo_leb = XWD (0, (int)&f->xrb);
    f->error = -1;
    if (!MUUO_ACVAL ("FILOP.", XWD (6, (int)&f->filop), &ret))
	f->error = ret;
    if (f->error != -1)
	return -1;

    /* KAR-5/92, added MON_ calls to get correct times */
    if (MON_Get_File_Times (f->filop.fo_chn, &mtime, 0))
	return -1;

    statb->st_mtime = MON_MJD_To_Time (mtime);

    ret = XWD (f->filop.fo_chn, uuosym (".FOREL"));
    MUUO_AC ("FILOP.", XWD (1, (int)&ret));
    statb->st_size = 4 * f->xrb.s.rb_siz;

#if 0 /* KAR-5/92, replaced with MON_ calls, this needs another call; see stat.c */
    statb->st_mtime = (XWD (
	 (f->xrb.s.rb_ext.rb.crx << 12) | f->xrb.s.rb_prv.rb.crd,
	f->xrb.s.rb_prv.rb.crt * 60));
#endif
    return 0;
}

static long
to_sixbit (char *str)
{
    long    tmp, sbit_val = 0;
    short    cnt = 36;

    while (*str != NULL)
	{
	tmp = toupper (*str) - 040;
	cnt -= 06;
	sbit_val |= tmp << cnt;
	str++;
	} /* while */

    return sbit_val;
}

#define _LHALF 0777777000000	/* Left half mask */
#define _RHALF 0777777		/* Right half mask */

/* FLDGET (wd,mask)     - Get right-justified value from field in word */
#define FLDGET(wd,mask) ( ( (unsigned) (wd)& (mask))/ ( (mask)& (- (mask))))

/* KAR-5/92, added to allow stats () to parse PPN numbers */
/* beg pts to '[' and end ptrs to ']' */
static void
ppnprs (struct _filespec* f, char *beg, char *end)
    {
    int lh = 0, rh = 0, defppn = 0;
    char tmp[16];

    strncpy (tmp, beg, (end - beg) - 1); /* get P,PN into tmp */

    sscanf (tmp, "%o,%o", &lh, &rh);	 /* get P into lh and PN into rh */

    if (!lh || !rh)			 /* get default PPN if not specifed */
	MUUO_VAL ("GETPPN", &defppn);

    if (!lh)
	lh = FLDGET (defppn, _LHALF);

    if (!rh)
	rh = FLDGET (defppn, _RHALF);

    f->fs_path.p_path.ppn = XWD (lh,rh);
    f->fs_nfds = 1;
    f->fs_path.p_arg = 0;
    }
#endif

/* UNV format symbol file cruncher.
**	Argument is char ptr to file in memory, plus length.
**	Returns # of symbol def entries (sfent) that were made;
**	array starts in same location as file.
*/

/* From MACRO source:

The first word of a .UNV file must be:
777,,<UNV version stuff>	= <UNVF_xxx flags>

If the UNVF_MACV flag is set, the next word will be:
<.JBVER of the MACRO that produced this UNV>

The next word is:
<# symbols>

Followed by the symbol table definitions, which are all at least
2 words long:
<SIXBIT symbol name>
<flags or value>

*/

#define UNVF_MACV 020	/* Has MACRO .JBVER in 2nd word */
#define UNVF_SYN 010	/* "New SYN handling in universal" */
#define UNVF_BAS 004	/* Must always have this bit on for compatibility */
#define UNVF_POL 002	/* Polish fixups included */
#define UNVF_MAD 001	/* "macro arg default value bug fixed" */

/* Symbol table flags */
#define USF_SYMF (0400000L <<18) /* Symbol */
#define	USF_TAGF (0200000L <<18) /* Tag */
#define USF_NOUT (0100000<<18)	/* No DDT output */
#define USF_SYNF  (040000<<18)	/* Synonym */
#define	USF_MACF  (020000<<18)	/* Macro */
#define USF_OPDF  (010000<<18)	/* Opdef */
#define	USF_PNTF   (04000<<18)	/* Symtab "val" points to real 36-bit val */
#define	USF_UNDF   (02000<<18)	/* Undefined */
#define	USF_EXTF   (01000<<18)	/* External */
#define	USF_INTF    (0400<<18)	/* Internal */
#define	USF_ENTF    (0200<<18)	/* Entry */
#define USF_VARF    (0100<<18)	/* Variable */
#define USF_NCRF     (040<<18)	/* Don't cref this sym */
#define	USF_MDFF     (020<<18)	/* multiply defined */
#define	USF_SPTR     (010<<18)	/* special external pointer */
#define USF_SUPR      (04<<18)	/* Suppress output to .REL and .LST */
#define	USF_LELF      (02<<18)	/* LH relocatable */
#define	USF_RELF      (01<<18)	/* RH relocatable */

#define	SYM6_SYMTAB	0166371556441L	/* .SYMTAB in sixbit */
#define	SYM6_UNVEND	0373737373737L

static int
crsfunv (tabp, flen)
char *tabp;			/* Location of file in memory */
long flen;			/* # bytes in file */
{
    register INT *rp;
    register INT cnt;
    register struct sfent
    *ep;
    INT wd, flags;

    cnt = flen/WDSIZ;		/* Get # words in file (round down) */
    rp = (INT *)tabp;		/* Set up word pointers to start */
    ep = (struct sfent
    *)rp;

#define nextwd() (--cnt >= 0 ? *++rp : 0)
#define skipwd(n) ( (void) (cnt -= n, rp += n))

    if ( (*rp&LH) != (0777<<18))	/* Is it really a UNV file? */
	{
	return -1;		/* No, say bad format. */
	}
    if (*rp&UNVF_MACV)		/* If has MACRO version word, */
	skipwd (1);		/* skip it. */
    skipwd (1);			/* Skip # of symbols for now */
    skipwd (2);			/* Also skip 1st symbol def (why??) */
    while (cnt > 0)
	{
	if (nextwd () == 0)	/* Get next word, */
	    continue;		/* ignoring zero words. */
	if (*rp == SYM6_UNVEND)	/* End of UNV file contents? */
	    break;
	flags = nextwd ();	/* Next word is flags or value */

	/* Quick check for any bad bits. */
	if ( (flags & (USF_MACF|USF_TAGF|USF_UNDF|USF_EXTF
		|USF_ENTF|USF_MDFF|USF_SPTR|USF_LELF|USF_RELF))==0)
	    {
	    ep->se_sym = rp[-1];	/* Hurray, set symbol name in entry */
	    ep->se_val = (flags & USF_PNTF)	/* 36-bit value? */
			? nextwd () : (*rp&RH);	/* If not, just use RH */

	    if (ep->se_sym != SYM6_SYMTAB)/* Unless sym we don't want, */
		++ep;			/* make it part of table. */
	    continue;			/* Won, back to start of loop */
	    }

	/* Not a constant symbol def, so we have to flush it. */
	if (flags & USF_MACF)		/* Macro? */
	    {
	    do
		{
		wd = nextwd ();
		skipwd (3);
		}
	    while (wd & LH)
		;
	    wd = nextwd ();
	    if (wd & 0770000000000L)	/* possible sixbit sym? */
		{
		++cnt, --rp;		/* Yes, assume no macro args */
		continue;		/* Back up one and continue */
		}
	    wd = (wd >> 18) & RH;
	    while (wd-- >= 0)		/* Flush macro args */
		skipwd (5);
	    continue;
	    }

	/* Special external pointer?  Relocatable or polish fixup */
	if (flags & USF_SPTR)
	    {
	    skipwd (1);		/* Flush value */
	    if ( (wd = nextwd ()) >= 0)	/* Get relocation word */
		{
		if (wd&RH)
		    skipwd (2);	/* Flush any right-half relocation */
		if (wd&LH)
		    skipwd (2);	/* Ditto for left-half reloc */
		continue;
		}
	    /* Ugh, polish.  Val plus <polish expression> */
	    while (cnt > 0)		/* Ensure don't run off */
		{
		while (nextwd ())
		    ;	/* Flush links */
		wd = nextwd ();		/* Hit zero word, get next */
		if (wd < 0 || wd >= 14)
		    break;
		else
		    skipwd (6);		/* Flush 6 wds */
		}
	    }
	else if (flags & USF_EXTF)	/* simple external? */
	    {
	    skipwd (2);			/* 2 wds, <value> + <symbol> */
	    }
	else if (flags & USF_PNTF)	/* Simple full-word value? */
	    {
	    skipwd (1);			/* 1 wd, <value> */
	    }
	}	/* Loop til all symbols scanned */

    return ep - ( (struct sfent
    *)tabp); /* Return # entries we got */
}
