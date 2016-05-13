/*	CC.C - KCC Main program
**
**	(c) Copyright Ken Harrenstien 1989
**		All changes after v.187, 26-May-1988
**	(c) Copyright Ken Harrenstien, SRI International 1985, 1986
**		All changes after v.74, 8-Aug-1985
**
**	Original version (c) 1981 K. Chen
*/

#include "ccsite.h"
#include "cc.h"
#include "ccchar.h"
#include <string.h>
#include <stdlib.h>	/* calloc(), EXIT_SUCCESS, EXIT_FAILURE */
#include <time.h>	/* For clock() to get runtime */
#include <sys/types.h>
#include <sys/stat.h>

#if !__MSDOS__ 							// FW KCC-NT
#include <muuo.h>	/* KAR-8/92, needed for set_level(); PPS 4516 */
#endif

/*
 * KAR-8/92, value sent to STLEV$ to return current running level and to
 * return a failure.
 */
#define RET_CUR_LEV	-2
#define STLEV_FAILURE	-2

typedef struct flagent	/* Structure of an entry in flag table */
    {
    char *name;		/* Flag name */
    int *fladdr;		/* Address of runtime flag value */
    INT flval;		/* Value to set it to if user gives flag */
    }
flagent_t;

/* KAR-3/92, took out sys/kccver.h because implemented new way to spec ver */
#include "kcchst.h"	/* For revision history */

#if __MSDOS__
#ifndef	WIN32							// FW KCC-NT
    /* _stklen < 25K breaks CCDECL,CCGEN, and CCOPT (Borland: Stack overflow!)
     * _stklen > 35K breaks CCDATA,CCOUT, and KCCDBG (KCC: more nodes(x used))
     */
    extern unsigned _stklen = 30000;
    #define CLOCKS_PER_SEC 18.2
#endif
#endif

/* Exported functions */
/* none */

/* Imported functions */

extern void dbginit(void);				/* CCDBUG */
extern void outpghdr(void);				/* CCOUT */
extern void outstr (char *);				/* CCOUT */
extern void savesymtab(SYMBOL *);			/* CCSYM */
extern SYMBOL *symfidstr(char *);			/* CCSYM */
extern int asmb(char *, char *, char *);		/* CCASMB */
extern void runlink(int, int, char **, char *, char *);	/* CCASMB */
#ifdef __COMPILER_KCC__
extern char *execargs(int *, char ***);			/* CCASMB */
#endif
extern char *fnparse(char *, char *, char *, char *, char *);  /* CCASMB */
extern char *estrcpy(char *, char *);			/* CCASMB */
extern NODE *extdef(void), *tntdef(void);		/* CCDECL */
extern void syminit(void), ppinit(void), lexinit(void), initpar(void),
	outinit(void), outdone(int),
	ppdefine(int, char **, int, char **), passthru(FILE *),
	gencode(NODE *);
extern void errfopen (char *, char *);			/* CCERR */
extern int fclose (FILE *);
extern char *fnparse(char *, char *, char *, char *, char *);
extern int fnxisrel(char *);

#if DEBUG_KCC		/* 5/91 KCC size */
extern void symdump(SYMBOL *, char *), typedump(void), nodedump(NODE *);
#endif

/* Internal functions */
static void cindfiles(int *, char ***);
static void showcpu(clock_t);
static void coptimize(char *), cdebug(char *), csidebug(char *),
	ctargmach(char *), cportlev(char *), cwarnlev(char *),
	cverbose(char *);

static int cswitch(char *, int *, char ***), cfile(char *),
	files(char *), mainsymp(void), needcomp(char *, char *);
static int chkmacname(char *);
static void parcswi(char *, flagent_t *, int);
static char *cmpname(char *, char *);
static int set_level (int level); /* KAR-8/92, support leveled headers PPS 4516 */

extern char *mlbuf, *mlbptr;		/* mixed listing */
char *savofnam = NULL;			/* KAR-3/92, save -R= name */
static void getimestr(char *src_fname);

static
char mainname[FNAMESIZE]
/* = {0} */
;	/* Name of module containing "main" */

static char *savname = NULL;		/* Pointer to desired -o filename */
static int vrbarg = 0;			/* Patch 1 to show args on outmsgs */
static int sourcebytewidth = 7;		/* FW 2A(47) */

/*
 * Help Screen (displayed when KCC is invoked without parameters)
 *	- Michael Snyder, CSI, 6/3/90
 */

static const _char7 *helpscreen = "\
			CompuServe Inc. Columbus, Ohio, 1991\n\n\
Syntax is: cc [switches] file[s]		  * indicates default.\n\
-b[789]   source-file byte width\n\
-c	  compile, assemble, no link	-C	    retain space in preprocess\n\
-Dident	  #define ident			-Dident=str #define ident str\n\
-d=list   mixed listing .mac		-E	    pre-process only, stdout\n\
-g	  link with DDT			-g=debug    source-level debugger\n\
-g=fnprof function profiler		-g=sprof    statement profiler\n\
-Hppn	  ppn for #include <...>	-hppn	    ppn for #include <sys/...>\n\
-Ippn	  ppn for #include \"...\"	-lname	    use library LIBname.REL\n\
-Lppn	  ppn for libraries		-L=option   relay option to LINK\n\
-n	  do no optimizations		-Nxxx	    set stack to xxx words\n\
-o=name   generate name.EXE		-P=kcc	  * allow KCC lang. extensions\n\
-P=base   Kernighan & Ritchie C		-P=carm     Harbison & Steele C\n\
-P=ansi	  carm with some ANSI stuff	-P=stdc   * Full ANSI-Standard C\n\
-p	  link with Bliss profiler	-q	    compile only if changed\n\
-R=<fname> designate .REL, .MAC name    -s          redirect messages to stdout\n\
-S	  compile: don't asm or link	-Uident     #undef ident\n\
-v	  dump all -v messgs & cmd args	-v=fundef   dump function names\n\
-v=load	  dump link loader commands	-v=nostats  suppress lines/minute\n\
-w=note   suppress [Note] messages	-w=advise   suppress [Advisory] also\n\
-w=warn   suppress [Warning] also	-w, -w=all  suppress all warnings\n\
-x=ch7	  default char size 7 bits.";

#if DEBUG_KCC	
static const _char7 *debugscreen = "\n\
-O=parse do parser optimizations	-O=all    * do all optimizations\n\
-O=gen   do code gen optimizations	-O=object   do peephole optimizations\n\
-d=parse dump parse tree file		-d=pho      dump peephole file\n\
-d=sym   dump symbol file		-d=all      dump diagnostic files.";
#endif


/* -r	  registers OFF, only uses 1-5\n\   ;add above for KCC2B(1)
 * -i     includes files only once (at CSI)\n\
 */


int
main (int argc, char **argv)
    {
    extern int JOBERR;
    int ac;			/* temp copy of argc */
    char **av;			/* temp copy of argv */
    char *nextprog = NULL;	/* Set to program to chain through, if any */
    int toterrs = 0;		/* Total # errors for all files */
    int nfiles = 0;		/* # files to try compiling */
    int asmfiles = 0;		/* # files for which assembly was deferred */

    /* KAR-3/92, changed to use new version number specification */
    /* Set up CSI version number. See kcchst.h */
#ifdef __COMPILER_KCC__
    _version(REV);
#endif
    ver_str = REV;

    #if defined (__COMPILER_KCC__)
        /* Clear out .JBERR */
    JOBERR = EXIT_SUCCESS;
    #elif  __MSDOS__
    unsign_int = 0;	/* only turned on in gizconst() */
    #endif

    /* Initialize KCC command switch values.
    ** All are either initially 0, or given default values in CCDATA.
    */
    link = assemble = delete = 1;

    module_pragma = XF4_call_spill = r_preserve = _reg_count = 0;

#if REGISTER_VARIABLES
    /* set this to "-1" when register variables are implemented */

    use_registers = 0;		/* '-r' switch zeros this */
#endif

    r_minnopreserve = R_MIN_NOPRESERVE;	/* FW 2A(52) default */
    r_maxnopreserve = R_MAX_NOPRESERVE;	/* FW 2A(47) default */

    outmsgs = stderr;		/* '-s' switch to stdout, PPS 4298 */
    insert_all_files = mlist = 0;
    longidents = 0;			/* FW 2A(51) default */
    
    coptimize("all");			/* Turn on all optimizer flags */

    /* Get command line arguments */

    if (argc <= 1)			/* No command line? */
	{
#ifdef __COMPILER_KCC__
	nextprog = execargs(&argc, &argv);	/* Try getting from RPG/CCL */
#endif
	if (argc > 1)
	    link = 0;			/* Got stuff, so act as if -c given */
	else				/* Sigh, tell user where help lives */
	    {
	    fprintf(stderr, "KCC Version %s", ver_str);
	    fprintf(stderr, (void *) helpscreen);	/* MVS 9/8/90 */
#if DEBUG_KCC	
	    fprintf(stderr, (void *) debugscreen);
#endif
	    return EXIT_FAILURE;
	    }
	}


    /* Have initial command line; now scan for any indirect files (@file) */
    cindfiles(&argc, &argv);  
    /* Now have complete command line, report it if desired.
    ** This debugging switch needs to be patched in by hand, because at this
    ** point it cannot have been set yet from the command line!
    */
    if (vrbarg)
	{
	fprintf(outmsgs, "KCC args (%d):", argc);
	for (ac = 0; ac < argc; ++ac)
	    fprintf(outmsgs, " %s", argv[ac] ? argv[ac] : "<null>");
	fprintf(outmsgs,"\n");
	}

    /* Now process command line.  First scan for all switches */
    for (av = argv+1, ac = argc; --ac > 0; ++av)
	if (**av == '-')
	    {
	    if (cswitch(*av, &ac, &av))	/* Process a switch */
		*av = NULL;		/* OK to zap it now */
	    }
	else
	    ++nfiles;		/* Assume a filename spec */

#if !__MSDOS__  /* KAR-8/92, support for leveled header files; PPS 4516 */
		/* but not for KCCDOS.                                  */

    switch (set_level (RET_CUR_LEV))
	{
	case 5: /* level 5; fall through because we need lev 1 also */
	    if (nhfpaths < MAXINCDIR-1)
		hfpaths[nhfpaths++] = "SSL:[1,175]";
	    else
		jerr("More than %d -H paths", MAXINCDIR);

	    if (nhfpaths < MAXINCDIR-1)
		hfsypaths[nhfsypaths++] = "SSL:[1,165]";
	    else
		jerr("More than %d -h paths", MAXINCDIR);

	case 1: /* level 1 */
	    if (nhfpaths < MAXINCDIR-1)
		hfpaths[nhfpaths++] = "SSL:[1,171]";
	    else
		jerr("More than %d -H paths", MAXINCDIR);

	    if (nhfpaths < MAXINCDIR-1)
		hfsypaths[nhfsypaths++] = "SSL:[1,161]";
	    else
		jerr("More than %d -h paths", MAXINCDIR);

	    break;

	case 0:  /* level 0 system's default header file area */
	default: /* error assume level 0 */
	    break;
	}
#endif /* !__MSDOS__ */

    /* Get the string rep. of the version */
    if (mlist)
	{
	mlbptr = mlbuf = (char *) calloc (1, MAXMLBUF * sizeof(char));
	mlbuf[0] = '\0';
	if (mlbuf == NULL)
	    {
	    warn("Out of memory for mixed listing");
	    mlist = 0;
	    }
	}

    /* Now finalize after all switches scanned */

#ifndef __COMPILER_KCC__
    tgmachuse.mapdbl = -1;
#endif

    if (nfiles == 0)			/* This sometimes happens */
	jerr("No filenames specified");
	
    /* If no errors, now scan for all filenames, and process them. */
    if ((toterrs += nerrors) == 0)
	for (av = argv+1, ac = argc; --ac > 0; ++av)
	    if (*av && **av != '-')
		{
		if (cfile(*av) == 0)	/* Compile a file */
		    asmfiles++;		/* Count deferred assemblies */

		nfiles++;
		toterrs += nerrors;
		}

    /* Now see what to invoke next, if any.  Note runlink() never returns. */
    if (toterrs)			/* If any errors, */
	{
	link = 0;			/* never invoke loader. */
	nextprog = NULL;
	}

    if (asmfiles || link || nextprog)
	runlink(link,			/* Whether to invoke loader or not */
		argc-1, argv+1,		/* Loader args (.REL files) */
		(savname ? savname	/* Loader arg: output file name */
			 : mainname),
		nextprog);		/* Chained program to invoke next */

    return (toterrs ? EXIT_FAILURE : EXIT_SUCCESS);
}


/* CINDFILES() - Scan an argv array for indirect file specs and
**	expand them into a new argv array.
*/

static void
cindfiles (int *aac, char ***aav)
{
    static int dynarr = 0;		/* Set if array was calloced */
    register int i, cnt;
    register char *cp;
    FILE *f;
    char *buf;
    int bufsiz;
#define NINDARGS 500
    int locac;
    char *locav[NINDARGS];

    int ac = *aac;
    char **newav, **avp, **av = *aav;

    /* Scan current array and process any indirect files. */
    for (i = 0; i < ac; ++i)
	if (av[i] && av[i][0] == '@')
	    {
	    if ((f = fopen(&av[i][1], "r")) == NULL)
		{
		errfopen("indirect", av[i]);	/* Couldn't open, tell user */
		av[i] = NULL;
		continue;
		}
	/* Read all of file into a memory block */
	    bufsiz = cnt = 0;
	    buf = cp = NULL;
	    while (!feof(f))
		{
		if (++cnt >= bufsiz)	/* Ensure have room in buffer */
		    {
		    char *nbuf;
		    if ((nbuf = realloc(buf, bufsiz += 128)) == NULL)
			{
			jerr("Out of memory for indirect file \"%s\"", av[i]);
			--cnt;
			break;		/* Leave loop now */
			}
		    cp = (buf = nbuf) + (cnt-1);
		    }
		*cp++ = getc(f);
		}
	    fclose(f);
	    if (cp == NULL)	/* If got nothing, scan for next file */
		{
		av[i] = NULL;
		continue;
		}
	    cp[-1] = '\0';		/* Ensure buffer tied off with null */

	/* Now scan through the buffer to find arguments, dropping NULLs in
	** to split them up, and add pointers to our local array.
	** "cnt" has the # chars in the buffer, including a terminating null.
	*/
	    locac = 0;
	    cp = buf;
	    for (; --cnt > 0; ++cp)
		{
		if (!isgraph(*cp))
		    continue;		/* Ignore whitespace/cntrls */
		if (*cp == '-' && (cnt <= 0 || cp[1] == '\n'))
		    continue;		/* Ignore T20 "line continuation" */
		if (*cp == ';')
		    {
		    while (--cnt > 0 && *++cp != '\n')
			;
		    continue;		/* Ignore ;-commented lines */
		    }
		if (*cp == '!')
		    {
		    while (--cnt > 0 && *++cp != '!' && *cp != '\n')
			;
		    continue;		/* Ignore !-commented phrases/lines */
		    }

	    /* Start scanning over an argument */
		if (locac >= NINDARGS)
		    {
		    jerr("More than %d args in indirect file \"%s\"",
							    NINDARGS, av[i]);
		    break;
		    }
		locav[locac++] = cp;		/* Remember ptr to arg */
		while (--cnt >= 0 && isgraph(*++cp))
		    ;
		if (cnt >= 0)
		    *cp = '\0';			/* Terminate arg with null */
		}

	/* Now combine new args with old args.  New table size is
	** # old args (minus current arg),
	** plus # new args (plus ending null pointer).
	*/
	    if ((newav = (char **)calloc(1, (ac+locac)*sizeof(char *))) == NULL)
		{
		jerr("Out of memory for args of indirect file \"%s\"", av[i]);
		av[i] = NULL;
		continue;
		}
	    avp = newav;
	    for (cnt = 0; cnt < i; ++cnt)		/* Copy already checked args */
		*avp++ = av[cnt];
	    for (cnt = 0; cnt < locac; ++cnt)	/* Copy new args */
		*avp++ = locav[cnt];
	    for (cnt = i+1; cnt < ac; ++cnt)	/* Copy old unchecked args */
		*avp++ = av[cnt];
	    *avp = NULL;		/* Last one always null */
	    ac = (ac - 1) + locac;	/* Old args minus 1, plus new args */
	    if (!dynarr)
		++dynarr;	/* If old not dynamic, say new one is */
	    else
		free((char *)av);	/* else must free up old dynamic array */
	    av = newav;		/* Pointer to new array */
	    --i;			/* Compensate for loop increment */
	    }

    *aac = ac;		/* Return new values (normally the same) */
    *aav = av;
}

/* CSWITCH - Scan argv array for command line switches
**
** Returns 0 if should keep this switch spec around (used for -l).
** Otherwise, OK to zap switch spec, don't need to keep around.
*/

static
int
cswitch (char *s, int *aac, char ***aav)
    {
    char*	t;			/* KAR-3/91, fix bug in -L lib path */


    while (*++s)
	{
	switch (*s)
	    {
#if DEBUG_KCC		/* 9/91 KCC size */ 
	    case 'a':			/* -a<file>  Set #asm tmp file name */
		asmtfile = ++s;		/*	rest of arg is filename */
		return 1;

	    case 'A':			/* -A<file>  Set asm hdr file name */
		asmhfile = ++s;		/*	rest of arg is filename */
		return 1;

	    case 'O':			/* -O	Optimize (same as -O=all) */

		if (s[1] == '=')	/* -O=<flags>	If extended syntax, */
		    {
		    ++s;
		    coptimize(++s);	/*	go hack rest of arg string. */
		    return 1;
		    }
		else
		    coptimize("all");	/*	Else just turn everything on */

		break;
#endif  /* DEBUG_KCC */

	    case 'b':			/* FW 2A(47) */

		/*
		 * "-b[789]" specifies the desired byte width of source files.
		 */

		sourcebytewidth = atoi (++s);

		if ((sourcebytewidth < 7) || (sourcebytewidth > 9))
		    jwarn ("source-file byte width %d out of range, defaulting to 7 bits", sourcebytewidth);

		return 1;

	    case 'c':			/* -c	Compile only */
		link = 0;		/*	Don't run linking loader */
		break;

	    case 'C':			/* -C	Pass on comments during -E */
		keepcmts = 1;		/*	pass comments thru to stdout */
		break;

	    case 'd':			/* -d	Debug (same as -d=all) */

		if (s[1] == '=')	/* -d=<flags>	If extended syntax, */
		    {
		    ++s;
		    cdebug(++s);	/*	go hack rest of arg string. */
		    return 1;
		    }
		else
		    cdebug("all");	/*	Else just turn everything on */

		break;

	    case 'D':			/* -D<ident>   Define a macro */

		if (!chkmacname(s))	/* -D<ident>=<def> */
		    return 1;

		if (npredef < MAXPREDEF)
		    predefs[npredef++] = ++s;	/* rest of arg is def string */
		else
		    jerr("More than %d predefined macros", MAXPREDEF);

		return 1;			/* so don't use as switches */

	    case 'E':			/* -E	Run through preproc. only */
		prepf = 1;
		delete = assemble = link = 0;

		break;

	    case 'g':			/* -g   Debugging:   */

		if (s[1] == '=')	/* -g=[ddt,debug,fnprof,sprof,bprof] */
		    {
		    ++s;
		    csidebug(++s);	/* go parse rest of arg string */
		    ldddtf = 1;		/* do load DDT in any case */
		    return 1;
		    }
		else			/* -g all by itself */
		    ldddtf = 1;		/* added 09/15/89 by MVS: link DDT */

		break;

	    case 'H':			/* -H<path> Specify #include <> path */

		if (nhfpaths < MAXINCDIR-1)
		    hfpaths[nhfpaths++] = ++s;	/* Remember the search path */
		else
		    jerr("More than %d -H paths", MAXINCDIR);

		return 1;

	    case 'h':			/* -h<path> Specify <sys/ > path */

		if (nhfsypaths < MAXINCDIR-1)
		    hfsypaths[nhfsypaths++] = ++s; /* Remember search path */
		else
		    jerr("More than %d -h paths", MAXINCDIR);

		return 1;

	    case 'i':			/* -i #incs all files only once each */
		insert_all_files = (char) ~0; // FW KCC-NT
		return 1;

	    case 'I':			/* -I<path> Add an #include "" path */

		if (nincpaths < MAXINCDIR-1)
		    incpaths[nincpaths++] = ++s; /* Remember the search path */
		else
		    jerr("More than %d -I paths", MAXINCDIR);

		return 1;

	    case 'k':			/* FW 2A(51) */
		longidents = 1;
		break;

	    case 'l':			/* -lxxx Loader: Search library */
		return 0;		/* Just skip over this switch */

	    case 'L':			/* -L<path> Specify library path */
					/* -L=<string> Specify LINK cmds */
		if (s[1] == '=')
		    return 0;		/* Just skip over -L= for now */

		libpath = ++s;		/* Set library path */
		t = (char *) calloc(1, (strlen(libpath) + 9));

		if (t == NULL)
		    jerr("Out of memory for -L= library path\n");

		strcpy(t, libpath);
		strcat(t, "LIB+.REL");
		libpath = t;
		return 1;

	    case 'n':			/* -n	No optimize */
		coptimize("");		/*	Turn off all optimizations */
		break;			/*	just as if -O= given. */

	    case 'N':			/* -Nxx specify Runtime Stack Size */
		stksz = atoi(++s);
		return 1;

	    case 'o':			/* -o=<filename> Loader: output file */
		if (s[1] == '=')
		    s += 2;	/* -o <filename> Permit old syntax */
		else
		    {
		    **aav = NULL;	/* Flush this arg */
		    ++(*aav);		/* Point to next one */

		    if (--(*aac) <= 0 || (s = **aav) == 0)
			jerr("No filename arg for -o");
		    }

		savname = s;
		return 1;		/*  Can flush arg from switch list */

	    case 'P':			/* -P	Port level (same as -P=) */

		if (s[1] == '=')	/* -P=<flags>	If extended syntax, */
		    {
		    ++s;
		    cportlev(++s);	/*	go hack rest of arg string. */
		    return(1);
		    }
		else
		    cportlev("");	/*	Else just use basic level */

		break;

	    case 'p':			/* -p   Bliss Profiler (link locals)*/
		csidebug("bprof");	/* changed 9/8/90 MVS */
		break;

	    case 'q':			/* -q	Conditional compilation */
		condccf = 1;
		break;

	    case 'R':			/* -R=<filename> .REL file */

		if (s[1] == '=')
		    s += 2;		/* -R <filename> Permit old syntax */
		else
		    {
		    **aav = NULL;	/* Flush this arg */
		    ++(*aav);		/* Point to next one */

		    if ((--(*aac) <= 0) || ((s = **aav) == 0))
			jerr ("No filename arg for -R");
		    }

		savofnam = s;
		return 1;

	    case 'r':

#if REGISTER_VARIABLES
		use_registers = 0;
#else					/* FW 2A(47) */
		r_maxnopreserve = atoi (++s);

		if ((r_maxnopreserve < R_MAX_NOPRESERVE)
		    || (r_maxnopreserve > 12))
		    {
		    jwarn ("register count out of range, using default = %d",
			  R_MAX_NOPRESERVE);
		    r_maxnopreserve = R_MAX_NOPRESERVE;
		    }
#endif

		return 1;

	    case 's':
		outmsgs = stdout;
		break;

	    case 'S':			/* -S   Do not delete asm source */
		delete = 0;
		link = assemble = 0;	/* don't link or assemble either */
		break;

	    case 'U':			/* -U<ident>   Undefine macro */

		if (!chkmacname(s))	/*	Check out identifier syntax */
		    return 1;

		if (npreundef < MAXPREDEF)
		    preundefs[npreundef++] = ++s;
		else
		    jerr("More than %d -U macro undefinitions", MAXPREDEF);

		return 1;

	    case 'v':			/* -v  Verbosity level (same as -v=) */

		if (s[1] == '=')	/* -v=<flags>	If extended syntax, */
		    {
		    ++s;
		    cverbose(++s);	/*	go hack rest of arg string. */
		    return(1);
		    }
		else
		    cverbose("all");	/*	Else just use basic level */

		break;

	    case 'w':			/* -w   Suppress warning messages */

		if (s[1] == '=')	/* -w=<flags>	If extended syntax, */
		    {
		    ++s;
		    cwarnlev(++s);	/*	go hack rest of arg string. */
		    return(1);
		    }

		if (isdigit(s[1]))
		    wrnlev = toint(*++s);
		else
		    cwarnlev("all");	/*	Else just use basic level */

		break;

	    case 'x':			/* -x=<flags> Cross-compilation sws */

		if (s[1] == '=')	/*	If extended syntax, */
		    {
		    ++s;
		    ctargmach(++s);	/*	go hack rest of arg string. */
		    return 1;
		    }

		jerr("Syntax for -x is \"-x=flag\"");
		return 1;

	    default:
		jerr("Unknown switch: \"-%c\"", *s);
		return 1;
	    }
	}

    return 1;
    }

/*
 * Command switch auxiliary routines.
 *
 * The standard way for extending KCC switch capabilities is by using
 * parcswi() to implement the following keyword-based syntax, as exemplified by
 * the -O switch:
 *		-O=<flag>+<flag>+<flag>...
 *	The flags are handled in the order given; all are cleared
 * at the start.  Using a '-' instead of '+' as the separator will cause
 * the next flag to be turned OFF instead of ON.  Either the flag name
 * "all" or just the switch "-O" will cause all flags to be turned on.
 *	The handling of flag keywords is governed by a table of "flagent"
 * structures.
 */


/*
 * PARCSWI - Parse an extended-style command switch.
 *	A NULL name entry marks end of table.
 *	An entry with a NULL flag address is the special "all" indicator.
 *
 *	ftab points to array of flagents. 
 *	resetf is true if want all flags cleared initially.
 */

static
void
parcswi (char *s, flagent_t *ftab, int resetf)
    {
    int onoff, c, i;
    char *cp;


    /* First turn off all flags */
    if (resetf)
	for (i = 0; ftab[i].name; ++i)
	    if (ftab[i].fladdr)
		*(ftab[i].fladdr) = 0;

    while ((c = *s) != '\0')
	{
	onoff = 1;			/* First do separator */
	if (c == '-')
	    {
	    onoff = 0;
	    ++s;
	    }
	else if (c == '+')
	    ++s;

	/* Look up switch in table */
	for (i = 0; (cp = ftab[i].name) != NULL; i++)
	    {
	    cp = cmpname(s, cp);
	    if (*cp == '\0' || *cp == '+' || *cp == '-')
		break;
	    }
	if (cp)		/* Found one? */
	    {
	    s = cp;
	    if (ftab[i].fladdr)			/* Single flag */
		*(ftab[i].fladdr) = (onoff ? (int)ftab[i].flval : 0);
	    else
		for (i = 0; ftab[i].name; ++i)	/* Hack "all" */
		    if (ftab[i].fladdr)
			*(ftab[i].fladdr) = (onoff ? (int)ftab[i].flval : 0);
	    }
	else		/* Nope, error.  Find end of flag name */
	    {
	    for (cp = s; *cp && *cp != '+' && *cp != '-'; ++cp)
		;
	    c = *cp;			/* Remember last char */
	    *cp = '\0';			/* and temporarily zap it */
	    /* Give user error message */
		{
		flagent_t *ftab2 = ftab;
		char emsg[1000];	/* Lots of room for temp string */
		char *cp = emsg;

		for (; ftab2->name; ++ftab2) /* Build string of flag names */
		    *cp++ = ' ', cp = estrcpy(cp, ftab2->name);
		jerr("Unknown flag \"%s\" (choices are:%s)", s, emsg);
		}
	    s = cp;			/* Restore zapped char */
	    *s = c;			/* And carry on from that point */
	    }
	}
    }

/*
 * CMPNAME - String comparison, returns pointer to first non-matching char
 */

static
char *
cmpname (char* str, char* tst)
    {
    if (*str == *tst)
	{
	while (*++str == *++tst)
	    {
	    if (*str == 0)
		break;
	    }
	}

    return str;
    }

/*
 * CHKMACNAME - Verify syntax for -D and -U macro names.
 */

static
int
chkmacname (char* s)
    {
    char *cp = s;
    int typ = *s;	/* 'D' or 'U' */


    if (!iscsymf(*++cp))
	{
	jerr("Bad syntax for -%c macro name: \"%s\"", typ, cp);
	return 0;
	}
    while (iscsym(*++cp))
	;		/* Skip over ident name */
    if (*cp  && (typ == 'U' || (*cp != '=')))
	{
	jerr("Bad syntax for -%c macro name: \"%s\"", typ, s+1);
	return 0;
	}
    return 1;
}

/* COPTIMIZE - Set -O optimization switches
**	Flags are used to provide finer degrees of control over the
** optimization process instead of just turning everything on or
** off; this makes debugging easier.
*/
static flagent_t copttab[] = {
	"all",	NULL,	   0,	/* First element is special */
	"parse", &optpar,  1,	/* Parse tree optimization */
	"gen",	&optgen,   1,	/* Code generator optimizations */
	"object", &optobj, 1,	/* Object code (peephole) optimizations */
	NULL,	NULL,	0					// FW KCC-NT
};

static void
coptimize(s)
char *s;
{
    parcswi(s, copttab, 1);	/* Reset switches and parse */
}

/* CDEBUG - Set -d debug switches.
**	This is exactly like COPTIMIZE only the switches here are for
** controlling what sorts of debug checks or output are produced.
**	The syntax is:
**		-d=<flag>+<flag>+<flag>...
**	The flags are handled in the order given; all are cleared
** at the start.  Using a '-' instead of '+' as the separator will cause
** the next flag to be turned OFF instead of ON.  Either the flag name
** "all" or just the switch "-d" will cause all flags to be turned on.
*/
static flagent_t cdebtab[] = {
/* FW 2A(42) SPR9986 07-Dec-92 moved "all" entry outside #if conditional */
	"all",	NULL,     0,	/* First element is special */
#if DEBUG_KCC		/* 5/91 KCC size */
	"parse", &debpar, 1,	/* Parse tree output */
	"gen",	&debgen,  1,	/* Code generator output */
	"pho",	&debpho,  1,	/* Peephole optimizer output */
	"sym",	&debsym,  1,	/* Symbol table output */
#endif
	"list", &mlist,   1,    /* CSI Mixed Listing generation-KAR */
	NULL,	NULL,	0					// FW KCC-NT
};

static void
cdebug(s)
char *s;
{
    parcswi(s, cdebtab, 1);	/* Reset switches and parse */
}

/* CSIDEBUG - Set CSI -g runtime debugging switches
**	Similar to COPTIMIZE or CDEBUG; the switches here control
** runtime user code debugging (where CDEBUG is mainly for compiler
** debugging).  Although this function will handle the same syntax
** as COPTIMIZE or CDEBUG (-g=<flag>+<flag>+...), the currently
** implemented options would not make sense in combinations.  No
** "all" option is provided.  The options are:
**	-g		same as -g=ddt 
**	-g=ddt		link in the DDT object level debugger 
**	-g=debug	create code for and link in source debugger
**	-g=bprof	create code for and link in bliss profiler
**	-g=sprof	create code for and link in statement profiler
**	-g=fnprof	create code for and link in function profiler
**	-g=nullptr	add hooks for NULL poionter detection
**	-g=fndbg	add hooks for function-level debugging only
**
** -- added 9/8/90, MVS at CompuServe, for source debugging
*/
/* FW 2A(42) PPS4575 09-Dec-92 added "fndbg" keyword */

static
flagent_t csidebtab[] =
    {
    "ddt",	&ldddtf,    1,	/* link in DDT object debugger */
    "debug", 	&debcsi,    KCC_DBG_SDBG, /* use KCC Source Level Debugger */
    "bprof",  	&profbliss, 1,	/* use Benny Jones' Bliss Profiler */
    "sprof",  	&debcsi,    KCC_DBG_SPRF, /* use KCC Statement Profiler */
    "fnprof", 	&debcsi,    KCC_DBG_FPRF, /* use KCC Function Profiler */
    "nullptr",	&debcsi,    KCC_DBG_NULL, /* use null pointer detection */
    "fndbg",  	&debcsi,    KCC_DBG_FDBG, /* KCCDBG function-level only */
    NULL, 	NULL,	    0				// FW KCC-NT
    };

static void
csidebug (char *s)
    {
    parcswi (s, csidebtab, 1);	/* reset switches and parse */
    }

/* CWARNLEV - Set -w warning message suppression switches.
**	Same syntax as for -O and -d.  -w alone is same as "all".
*/
static flagent_t cwlevtab[] = {
	"all",	&wrnlev, WLEV_ALL,	/* Suppress everything */
	"note",	&wrnlev, WLEV_NOTE,	/* Suppress notes */
	"advise", &wrnlev, WLEV_ADVISE,	/* Suppress notes & advice */
	"warn",	&wrnlev, WLEV_WARN,	/* Suppress n & a & warnings */
	NULL,	NULL,	0					// FW KCC-NT
};

static void
cwarnlev(s)
char *s;
{
    parcswi(s, cwlevtab, 1);	/* Reset switches and parse */
}

/* CTARGMACH - Set -x cross-compilation switches.
**	Same syntax as for -O and -d.
**	There is no "all" and no flags are reset.  -x alone does nothing.
** Note that the value for the CPU type switches is not 1, so that
** we can distinguish between a default setting (1) and a switch setting (2).
*/
static flagent_t ctgmtab[] = {
	"ch7",	&tgcsize, 7,		/* Size of chars, in bits */
	NULL,	NULL,	0					// FW KCC-NT
};

static void
ctargmach(s)
char *s;
{
    parcswi(s, ctgmtab, 0);		/* Don't reset switches; parse */
    tgcpw = TGSIZ_WORD/tgcsize;		/* Ensure right vars set if charsize */
    tgcmask = (1<<tgcsize)-1;		/* was specified. */
#ifndef __COMPILER_KCC__
    tgmachuse.mapdbl = -1;
#endif
}

/* CPORTLEV - Set -P portability level switches.
**	Same syntax as for -O and -d.
**	There is no "all".  -P alone resets everything.
*/
static flagent_t cplevtab[] = {
	"kcc",	&clevkcc, 1,		/* Enable KCC extensions to C     */
	"base",	&clevel, CLEV_BASE,	/* Allow only very portable code  */
	"carm",	&clevel, CLEV_CARM,	/* Allow full CARM implementation */
	"ansi",	&clevel, CLEV_ANSI,	/* Parse CARM+ANSI implementation */
	"stdc",	&clevel, CLEV_STDC,	/* Parse full ANSI implementation */
	"strict", &clevel, CLEV_STRICT, /* Unforgiving ANSI ("pedantic")  */
	"nocpp", &clevnocpp, 1,         /* FW 2A(45) defeat C++ comments  */
	NULL,	NULL,	0					// FW KCC-NT
};

static void
cportlev(s)
char *s;
{
    parcswi(s, cplevtab, 1);	/* Reset switches and parse */
}

/* CVERBOSE - Set -v verboseness switches.
**	Same syntax as for -O etc.
**	-v alone is same as "all".
*/
static flagent_t cverbtab[] = {
	"all",	NULL,     0,	/* First element is special */
	"fundef", &vrbfun, 1,		/* Print function names as we go */
	"nostats", &vrbsta, 1,		/* Don't print statistics at end */
	"args", &vrbarg,   1,		/* Print KCC command line args */
	"load",	&vrbld,    1,		/* Print linking loader commands */
	NULL,	NULL,0						// FW KCC-NT
};

static void
cverbose(s)
char *s;
{
    parcswi(s, cverbtab, 1);	/* Reset switches and parse */
}

/* CFILE(filename) - Compile or otherwise process a file.
**   Return value indicates whether file was assembled:
**	-2 No assembly attempted (may or may not be error).
**	-1 Assembly failed.
**	 0 Assembly deferred, must call runasmlnk() later.
**	+1 Assembled into .REL file.
*/

static
int
cfile (char *arg)
    {
    int		mainflg;		/* Set if module contains "main" */
    int		asmdflg = -2;		/* Set to result of assembly attempt */
    clock_t	startime;
    extern
    int		nsert_file (char *f, int insert_flag);
    int		save_fline;
    int		save_tline;


    if (!vrbsta)
	startime = clock();		/* Mark cpu time */

    /* 11/91 added nwarns */

    nerrors = nwarns = err_waiting = 0;	/* KAR  added err_waiting init */

    if (!files(arg))			/* If couldn't open or file is .REL, */
	return asmdflg;			/* just return. */

    if (mlist)				/* set up output file with page hdr */
	{
	opage = 0;
	outpghdr();
	}

module_loop:

    save_fline = fline;
    save_tline = tline;

    if (!prepf)
	{
	fprintf (outmsgs, "KCC: %s\n", (module_pragma) ? title : inpfmodule);

	if (outmsgs == stdout)
	    fprintf (stderr, "KCC: %s\n",
	    	    (module_pragma) ? title : inpfmodule);
	}

    syminit ();				/* Set up symbol tables */
    ppinit ();				/* Initialize the input preprocessor */
    ppdefine (npreundef,preundefs,	/*  then can do initial -U undefs */
	     npredef, predefs);		/*   and initial -D definitions */

    if (module_pragma)
	{
	fline = save_fline;		/* ppinit() reset fline */
	tline = save_tline;
	module_pragma = 0;
	lexinit ();
	module_pragma = 1;
	}
    else
	lexinit ();			/* Initialize the input lexer */

    initpar ();				/* Initialize the input parser */

    /* KAR-6/92, moved reset of -i here to handle multiple files as well */

    nsert_file (NULL,0);		/* turn off "-i" command line switch */
    dbginit ();				/* Initialize src debugger output */

    if (prepf)				/* If only preprocessor output (-E) */
	{
	passthru (stdout);		/*   send it through specially */
	fclose (in);			/*   and then close input */

#if DEBUG_KCC				/* 5/91 KCC size */
	if (debsym)
	    {
	    symdump (minsym->Snext, "external"); /* symbol table dump */
	    typedump ();
	    fclose (fsym);
	    }
#endif
	}
    else				/* Normal compilation processing */
	{
	NODE*	    n;

	/* if (module_pragma) outinit() puts title in *.mac outpreamble()*/

	outinit ();			/* Initialize assembler code output */
	module_pragma = 0;

	if (eof)
	    {
	    if (clevel >= CLEV_STRICT)
		error ("File must contain at least one external definition");
	    else
		warn ("Null source file");
	    }

	while (!eof)			/* Process each external definition */
	    {
	    savelits = 0;		/* Reset string literal pool */
	    nodeinit ();		/* Reset parse-tree node table */
	    curfn = NULL;		/* Not in any function */
	    n = extdef ();		/* parse one external definition */
					/* 2/92 may turn on module_pragma */
#if DEBUG_KCC				/* 5/91 KCC size */
	    if (debpar)
		nodedump (n);		/* Dump parse tree if debugging */
#endif

	    gencode (n);		/* Call code generator on parse tree */
	    }

	if (!module_pragma)
	    {
	    fclose (in);		/* Done with input stream, close it. */
	    curfn = NULL;		/* Not in any function */
	    fline = 0;			/* Avoid context on errs after this */
	    }

	while ((n = tntdef ()) != NULL)	/* Output remaining tentative defs */
	    {
#if DEBUG_KCC				/* 5/91 KCC size */
	    if (debpar)
		nodedump (n);
#endif

	    gencode (n);
	    nodeinit ();		/* Reset node table after each */
	    }

	if ((mainflg = mainsymp ()) != 0) /* Is "main" defined in module? */
	    strcpy (mainname, inpfmodule); /* Yes, save module name! */

	switch (abs (debcsi))
	    {
	    case KCC_DBG_SDBG :
	    case KCC_DBG_FDBG :
		savesymtab (minsym->Snext); /* generate debug symtab */
		break;


	    default:
		break;
	    }

	outdone (mainflg);		/* Output assembler postamble stuff */

	if (module_pragma)
	    goto module_loop;

	fclose (out);			/* and close assembler output file. */

#if DEBUG_KCC				/* 5/91 KCC size */
	if (debsym)
	    {
	    symdump (minsym->Snext, "external"); /* symbol table dump */
	    typedump ();
	    fclose (fsym);
	    }

	if (debpar)
	    fclose (fdeb);		/* Close parse tree debug file */

	if (debpho)
	    fclose (fpho);		/* Close peephole debug file */
#endif

	if (!nerrors && assemble)
	    asmdflg = asmb ((savofnam ? savofnam : inpfmodule),
		(char *) NULL, outfname); /* and this. */

	if ((delete && asmdflg != 0) && (!mlist))
	    remove (outfname);
	}

    if (outmsgs == stdout)
	fprintf (outmsgs, "%% KCC - %d warning%s detected\n", nwarns,
		 nwarns == 1 ? "" : "s" );

    if (nwarns)				/* Report warnings */
	fprintf (stderr, "%% KCC - %d warning%s detected\n", nwarns,
		 nwarns == 1 ? "" : "s" );

    if (nerrors)			/* Report errors */
	jmsg ("%d error%s detected", nerrors, nerrors == 1 ? "" : "s" );
    else if (!vrbsta)
	showcpu (startime);		/* or say how much cpu we used */

    return asmdflg;			/* Return assembly result */
    }

/* Auxiliary - returns true if main() was defined in this module */

static
int
mainsymp (void)
    {
    SYMBOL*	s;


    return ((s = symfidstr ("main")) != NULL && s->Sclass == SC_EXTDEF);
    }

/*
 * FILES - parse a filename argument and set up I/O streams.
 *
 *	Note that "prefname" is set up here, but not opened.
 * This is because we may not need to use it; the call to
 * "makprefile()" will do so if necessary.
 */

static
int
files (char *fname)
    {
    int		cextf;
    char*	cp;
    char	cname[FNAMESIZE];	/* Name of .C source file */
    char	rname[FNAMESIZE];	/* Name of .REL binary file */
    char	ext[FNAMESIZE];		/* Temp to hold parsed extension */


    /*
    ** Parse filename into its various pieces, mainly to get module name.
    ** As a special case, it will work to specify a device (logical) name
    ** with no filename, e.g. you can say
    **        @CC FOO:
    ** where FOO: => <DIR>NAME.C, and the module name will be FOO.
    */

    /* in __MSDOS__  fnparse() calls fnsplit() */

    if ((cp = fnparse (fname, inpfdir, inpfmodule, ext, inpfsuf)) != NULL)
	{
	jerr ("Bad filename arg (%s): \"%s\"", cp, fname);	/* Ugh */
	return 0;			/* and don't try to compile */
	}

    if (fnxisrel(ext))	/* .obj for DOS */
	return 0;

    /* Check for .C or .H extensions.  "cextf" is set if we have it. */

    cextf = (ext[0] == '.'
	     && (toupper(ext[1]) == 'C' || toupper(ext[1]) == 'H') && !ext[2]);

    /* Now compose source filename with ".C" appended if necessary */

    if (cextf)
	strcpy (cname, fname);	/* Found .C, just copy filename */
    else
	{
#if __MSDOS__
	estrcpy (estrcpy (estrcpy (estrcpy (cname,	/* Rebuild filename */
		inpfsuf), inpfdir), inpfmodule), ".c");
#else
	estrcpy (estrcpy (estrcpy (estrcpy (cname,	/* Rebuild filename */
		inpfdir), inpfmodule), ".c"), inpfsuf);
#endif
	}

    if (mlist)
	{
	int	    i;


	for (i = 0; cname[i] != '\0'; i++)
	    dspfname[i] = toupper(cname[i]);

	dspfname[i] = '\0';
	getimestr(cname);
	}

    /*
     * If the -q conditional compile flag was
     * set, we assume that we are to check the .C and .REL extensions of
     * this file to determine whether compilation is necessary.
     */

    if (condccf)
	{
	strcpy (rname, inpfmodule);		/* Make the .REL filename */
	strcat (rname, ".rel");

	if (!needcomp (cname, rname))
	    return 0;		/* Doesn't need to be compiled! */
	}

    /*
     * Now that we've figured out what the name of the file is, we can try
     * to open it.  First we try the filename as given, and if that doesn't
     * work (most likely because no .C was specified) then we try the
     * one we constructed by adding .C.
     */

    strcpy (inpfname, fname);		/* Try filename as given */

#if __MSDOS__				/* FW 2A(47) */
    in = fopen (inpfname, "r");
#else
    switch (sourcebytewidth)		/* FW 2A(47) */
	{
	case 8 :
	    in = fopen (inpfname, "r8");
	    break;

	case 9 :
	    in = fopen (inpfname, "r9");
	    break;

	default :
	    in = fopen (inpfname, "r");
	    break;
	}
#endif
    
    if (in == NULL)
	{
	strcpy (inpfname, cname);	/* then constructed filename */

#if __MSDOS__				/* FW 2A(47) */
	in = fopen (inpfname, "r");
#else
	switch (sourcebytewidth)	/* FW 2A(47) */
	    {
	    case 8 :
		in = fopen (inpfname, "r8");
		break;

	    case 9 :
		in = fopen (inpfname, "r9");
		break;

	    default :
		in = fopen (inpfname, "r");
		break;
	    }
#endif

	if (in == NULL)
	    {
	    errfopen("input", inpfname);
	    return 0;
	    }
	}

#if DEBUG_KCC		/* 5/91 KCC size */
    /* Compose symbol table dump output filename, if desired */

    if (debsym)
	{
	strcpy (symfname, inpfmodule);
	strcat (symfname, ".cym");

	if ((fsym = fopen(symfname, "w")) == NULL)
	    {
	    errfopen("symbol table", symfname);
	    return 0;
	    }
	}
#endif

    /* If we are only doing pre-processing, then no other filenames are
    ** needed, and we can return now.
    */
    if (prepf)
	return 1;

    /*
    ** The output file is merely ".mac" etc concatenated to the stripped
    ** filename we calculated above, in the current directory.
    */

    cp = ".mac";

    if (savofnam != NULL)
	{
	char *tfnam, *ptr;

	tfnam = (char *) calloc (strlen (savofnam), sizeof(char *));
	if (tfnam == NULL)
	    jerr("Out of memory for .REL filenames\n");
	strcpy (tfnam, savofnam);
	ptr = strchr(tfnam, '.');

	if (ptr == NULL)
	    strcat (strcpy(outfname, savofnam), cp);
	else
	    {
	    *ptr = '\0';
	    strcat (strcpy(outfname, tfnam), cp);
	    }

	free (tfnam);
	}
    else
	strcat(strcpy(outfname, inpfmodule), cp); /* Compose output filename */

    if ((out = fopen(outfname, "w")) == NULL)
	{
	errfopen("output", outfname);
	return 0;
	}

    /* Now open various other debugging output files */

    if (debpar)		/* debugging output goes here */
	{
	strcpy(debfname, inpfmodule);
	strcat(debfname, ".deb");
	if ((fdeb = fopen(debfname, "w")) == NULL)
	    {
	    errfopen("parser debugging output", debfname);
	    return 0;
	    }
	}

#if DEBUG_KCC		/* 5/91 KCC size */
    if (debpho)		/* Peephole debugging output goes here */
	{
	strcpy(phofname, inpfmodule);
	strcat(phofname, ".pho");
	if ((fpho = fopen(phofname, "w")) == NULL)
	    {
	    errfopen("peephole debugging output", phofname);
	    return 0;
	    }
	}
#endif

    return 1;
}

/*
 * NEEDCOMP - Auxiliary for above.
 *	Takes source and binary filenames, returns TRUE if
 *	source needs compiling (is newer than binary).
 */

#if __MSDOS__		/* 4/92 avoid non-ANSI stat() */
#define stats stat	/* just use stat() */
#else
extern
int	    stats (char *source_fname, struct stat* statb);
#endif



static
int
needcomp (char* src, char* rel)
    {
    struct
    stat	sbuf,
		rbuf;


    if (stats(src, &sbuf) < 0)
	return 1;
			/* No source?? Try compiling anyway */
    if (stats(rel, &rbuf) < 0)
	return 1;			/* No .REL, so must compile */

    return (sbuf.st_mtime > rbuf.st_mtime);	/* Compare last mod times */
    }

/*
 * GETIMESTR - Builds time strings for mixed listing --KAR 8/90
 */

static
void
getimestr (char* src_fname)
    {
    time_t	curtime;
    time_t*	ctptr = &curtime;
    char*	tptr;
    struct
    stat	sbuf;


    stats (src_fname, &sbuf);
    curtime = time (NULL);
    tptr = ctime (ctptr);
    strcpy (comptime, tptr);

#if 0
    tptr = ctime(&sbuf.st_mtime);
    strcpy (creatime, tptr);
#endif

    strtok (comptime, "\n");
    strtok (creatime, "\n");
    }

/*
 *      show how much cpu we used
 */

static
void
showcpu (clock_t otim)
    {
    float	secs;


    secs = (float) (clock() - otim) / CLOCKS_PER_SEC;	/* Find # secs used */

    if (outmsgs == stdout)	/* 8/91 -s  PPS 4298 */
	fprintf (outmsgs, "Processed %d lines in %.2f CPU seconds "
		 "(%ld lines/min)\n", (int) tline, secs,
		 (INT)((tline*60.0)/secs));

    fprintf (stderr,"Processed %d lines in %.2f CPU seconds (%ld lines/min)\n",
	     (int) tline, secs, (INT)((tline*60.0)/secs));
    }

#if !__MSDOS__
/*
 * KAR-8/92, added to check the current level KCC is running on
 * and use the leveled header file directories. (PPS 4516)
 */

static
int
set_level (int level)
    {
    long    ac = level;
    long    ret_val;


    asm("\tSEARCH CSISYM\n");

    if (ICSIUUO_ACVAL("STLEV$", ac, ret_val))
	return (int) ret_val;
    else
	return STLEV_FAILURE;
    }
#endif
