/*	CCDBUG.C - Support for Source Language Debugging / Profiling
**
**	(c) Copyright CompuServe Incorporated, Columbus Ohio, 1990
**
**	Facility: KCC Compiler
**
**	Abstract: This module of the compiler provides features added
**		  at CompuServe: it is called by the parsing and code
**		  generating phases, to generate "hooks" into various
**		  debugging and profiling programs from the user code.
**
**	Environment: Host C Language
**
**	Author: Michael V. Snyder
**
**	Revision History:
**	1.0	May 18, 1990	-Initial Revision
*/

/*
**	External Interfaces:
*/

#include "cc.h"

extern void  outstr(char *);			/* CCOUT */
extern void  flushcode(void);			/* CCCODE */
extern void  codgolab(SYMBOL *);		/* CCCODE */
extern SYMBOL *creatsym(char *);		/* CCSYM */

/*
** Global Data
*/

#define BUFFSIZE	6 /* Labels have 6 chars, 1<<18 exec.stmts overflows*/
static char buff[BUFFSIZE];
static int stmt_index;

/*
 *	Table of Contents:
 */

NODE *debug_node(NODE *s, int lineno, int stmt_number, int type);
void  code_debugcall(NODE *n);
void  dbginit(void);
static char *otoa(int i);

/*
** How it works:
** 
** If "debcsi" is greater than zero, the parser will call debug_node()
** when it recognizes appropriate points in the source code [function
** entry points, function exits, and statement breaks].  debug_node()
** will add a node to the parse tree representing a goto label with 
** an automatically generated name: the '%' character followed by 
** the source line number.
**
** Later, when the code generation phase recognizes one of these nodes,
** it will call code_debugcall() to output both the label and a pseudo-
** op representing a call to the appropriate debugger or profiler 
** function at runtime.  Each call generated to the debugger or profiler
** will have a parameter "stmt_index", which will have a unique value.
** At runtime, this unique value can be used to quickly identify the
** source of the call.
**
** At compile time, the calling of these functions is controlled by a 
** variable "debcsi".  Command line options to the compiler will assign
** debcsi a positive value associated with either source debugging, 
** statement profiling or function profiling.  However, a #pragma
** "nodebug" can make the value of debcsi negative, in which case the
** debugger hooks will no longer be generated.  A second #pragma "debug"
** can later make debcsi positive.  Neither #pragma should change debcsi
** if its value is zero (default).
*/

NODE *
debug_node(NODE *n, int this_line, int stmt_number, int calltype)

/*
 * Functional Description:
 *	 
 *	Generates an extra node to be inserted into the parse tree.
 *	This node will contain information about the line number of
 *	the statement being generated.  The code generation phase 
 *	will recognize this node, and generate an appropriate hook
 *	for a statement debugger or profiler.
 *
 * Formal Parameters:
 *
 * n:         the parent node of the statement being parsed.
 * this_line: current line number from lexer.
 * calltype:  one of FN_ENTRY, FN_EXIT, or STMT
 *
 * Implicit Parameters:	none.
 *
 * Return Value: pointer to a new node.
 *
 * Side Effects: updates several static variables.
 */
{
    static int last_line = 0;
    static char label_str[40];

    switch (debcsi)
	{

	/*
	 * FW 2A(42) PPS4575: added case KCC_DBG_FBDG
	 */

	case KCC_DBG_FDBG:		    /* function debugging */
	case KCC_DBG_FPRF:		    /* function profiling */

	    /*
	     * If this is only a statement break, skip it.
	     */

	    if (calltype == STMT)
		break;			    /* else fall thru */


	case KCC_DBG_SDBG:		    /* statement debugging */
	case KCC_DBG_SPRF:		    /* statement profiling */
	    
	    if (last_line != this_line)
		{

		/*
		 * Make a node if we have moved onto a new source line.
		 *
		 * FUTURE:
		 * 
		 * Break the source-line connection and hook per statement.
		 */

		last_line = this_line;
		sprintf (label_str, "%%%d", this_line);
		n = ndefl (N_LABEL, n);
		n->Nxfsym = creatsym (label_str);
		n->Nxfsym->Sflags |= SF_LABEL;
		n->Nxfsym->Sclass = SC_LABEL;
		n->Nxfsym->Skey = calltype;
		sprintf (n->Nxfsym->Sname + 7, "%d", stmt_number);
		}
	    break;


	default:
	    break;			    /* turned off (0 or negative) */
	}	/* end switch */
    return n;
}

void
code_debugcall(NODE *n)

/*
 * Functional Description: 
 *
 *	Generates an instruction into the output file, which will be 
 *	translated into a transfer of control to a controlling program
 *	such as a debugger.
 *
 * Formal Parameters: 
 *	n: node containing line number information
 *
 * Implicit Parameters:	stmt_index.
 *
 * Return Value:	none.
 *
 * Side Effects:	increments stmt_index.
 */
{
    char *str;

    flushcode();
    codgolab(n->Nxfsym);		/* send goto label */
    switch (n->Nleft->Nop)			/* decoration for .MAC file */
	{
	case Q_CASE:
	    str = "\t; case stmt\n";
	    break;
	case Q_DEFAULT:
	    str = "\t; default stmt\n";
	    break;
	case N_LABEL:
	    str = "\t; label stmt\n";
	    break;
	case Q_BREAK:
	    str = "\t; break\n";
	    break;
	case Q_GOTO:
	    str = "\t; goto\n";
	    break;
	case Q_CONTINUE:
	    str = "\t; continue\n";
	    break;
	case Q_SWITCH:
	    str = "\t; switch\n";
	    break;
	case N_EXPRLIST:
	    str = "\t; expr list\n";
	    break;
	case Q_IF:
	    str = "\t; if stmt\n";
	    break;
	case Q_WHILE:
	    str = "\t; while loop\n";
	    break;
	case Q_DO:
	    str = "\t; do loop\n";
	    break;
	case Q_FOR:
	    str = "\t; for loop\n";
	    break;
	case Q_RETURN:
	    str = "\t; return\n";
	    break;
	case N_STATEMENT:
	    str= "\t; fn entry\n";
	    break;
	default:
	    str = "\t; expr stmt\n";
	    break;
	}
    switch (n->Nxfsym->Skey)
	{
	case FN_ENTRY:
	    outstr("\tDEBUGP	0, ");
	    break;
	case FN_EXIT:
	    outstr("\tDEBUGE	1, ");
	    break;
	case STMT:
	    outstr("\tDEBUGS	2, ");
	    break;
	default:
	    int_error("code_debugcall: invalid debug call %d",
		    n->Nxfsym->Skey);
	    break;
	}

    outstr(otoa(++stmt_index));
    outstr(str);
}

void dbginit(void)

/*
 * Functional Description: Initialize debug output and label buffer for new 
 *	source module.
 *
 * Formal Parameters:	none.
 *
 * Implicit Parameters:	none.
 *
 * Return Value:	none.
 *
 * Side Effects:	Zeroes a static variable.
 */
{
    stmt_index = 0;
    buff[BUFFSIZE-1] = '\0';
}

/* 6/91, change octal values to strings. Example: 9 becomes "11" */
static char *otoa(int i)
{
    int j = BUFFSIZE - 2;

    while (i > 0)
	{
	buff[j--] = i % 8 + '0';
	i /= 8;
	}
    return &buff[j+1];
}
