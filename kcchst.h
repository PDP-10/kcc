#define _version(ver)	asm(".text ~/version:" ver "~\n")
#define REV			   "2A(52)"
/*                                  ^^ ^
 * Parameters:                      || |
 *  - Major => major version  ------ | |
 *                                   | |
 *  - Minor => minor version  -------  |
 *	(use 0 to omit from version)   |
 *                                     |
 *  - Edit  => edit number    ---------
 *
 *  - Modified-by => programmer can be appended to the end of
 *	the string if desired (e.g. 2A(34)-4) and should be a
 *	number (use 0 to omit from version)
 *
 * NOTE: For less confusion use octal values for the numeric
 *	 fields and uppercase letters for the minor version.
 */

/************************************************************************
 *                                                                      *
 *  Copyright (c) 1990                                                  *
 *  by CompuServe Incorporated, Columbus, Ohio                          *
 *                                                                      *
 *  The information in this software  is  subject  to  change  without  *
 *  notice  and  should not be construed as a commitment by CompuServe  *
 *  Incorporated.                                                       *
 *                                                                      *
 ************************************************************************
 *
 * Facility:  KCC C Compiler
 *
 * Abstract:
 *	This file contains the revision history of the KCC compiler,
 *	starting with version 2A(1).
 *
 * Environment:  CompuServe specific.
 *
 * Author:  Kevin A. Russo, December 3, 1990
 *
 * Revision History:
 *
 * 2A(1)  KAR, October 1990
 *	  - First version number implementation (PPS 3980, SPR 9226)
 *	  and first mixed-listing implementation (PPS 3974).
 *	  Modules: CCPP CCOUT CCGEN CC.H CC KCCVER.H CCERR
 *
 * 2A(2)  KAR, TEA, November 1990
 *	  - Fixed bugs in mixed-listing when encountered a storage
 *	  declaration within a header file and had to expand the
 *	  internal source code buffer.  Also added functionality for
 *	  zeroing .JBERR upon command line startup.
 *	  Modules: CC.H CCPP CC
 *
 * 2A(3)  KAR, January and February 1991
 *	  - Changed all "#ifdef SYS_CSI"'s to #if's and initial 
 *	  implementaion of in-line monitor calls.  Also changed
 *	  the default for -v=stats to print the stats and the 
 *	  switch is now -v=nostats to turn it off and added the
 *	  count of words for each function to the output of 
 *	  -v=fundef.
 *	  Modules: (ifdef's to if's) CCPP CCERR CCASMB CCGEN
 *	  	   CC CCOUT
 *		   (in-line muuo's) CCSTMT CCCODE CCGEN2 CCOUT
 *		   CCTOKS.H CCCODE.H MUUO.H
 *		   (statistics) CC CCASMB CCGEN CCOUT CCSTMT
 *
 * 2A(4) TEA, March 1991
 *	 - Unified the Host and PC versions of KCC. Removed several
 *	 PC warnings including 3 uninitialized variables. Expanded 
 *	 the cases where BLT is used (PPS 4130). Includes two debugger
 *	 fixes see (SPR 9410, SPR 4121).
 *	 Modules: All *, 11 of 18 *.H, namely CC.H, CCCHAR.H CCCODE.H
 *	 CCGEN.H, CCLEX.H, CCNODE.H, CCPARM.H, CCREG.H, CCSITE.H, CCSYM.H
 *	 CCTOKS.H
 *
 * 2A(5) TEA, KAR March 1991
 *	 - Kevin put in a fix for the -L switch.  Path name was not 
 *	 correct in MACRO code (SPR 9463).
 *	 - Tim fixed a bug having to do with emitting arguments
 *	 when a Bliss function is called and replaced numerous spaces
 *	 with tabs in various code generation routines.
 *	 Modules: CC, CCOUT, CCDECL, CCASMB
 *
 * 2A(6) TEA, KAR May 1991
 *	 - KAR Fixed page break bug in mixed listings.
 *	 - TEA Replaced large static tables with dynamic tables, removing
 *	 the upper size limits and no longer creating maximum sized tables
 *	 (PPS 4206, PPS 4232), reducing initial low-seg 33% (from 80 to 52).
 *	 Sped up each KCC getc by eliminating tgmapch() and sped up 
 *	 typechecking by rewriting sideffp() and edisc().
 *	 Jacketed 17 unreachable functions with #if 0...#endif and 15
 *	 LDG-specific info dumping functions with #DEBUG_KCC...#endif,
 *	 reducing high-seg 4% (from 154 to 148).
 *	 1) CC, CCCODE, CCDATA, CCNODE, CCSYM, and CCASMB require 
 *	 being compiled with -DDEBUG_KCC=1 (for LDG, not production).
 *	 2) KCC2A(6) compiles itself on the PC. Each of the 42 source files
 *	 was too large for KCC2A(5).
 *	 Modules: CC, CCCODE, CCCREG, CCDECL, CCERR, CCLEX, 
 *	 CCGEN, CCNODE, CCPP, CCSYM, KCCDBG, LEXYY, CC.H, 
 *	 CCERR.H, CCLEX.H, CCPARM.H, CCPROT.H, CCREG.H, CCSYM.H, FSKELD.H
 *
 * 2A(7) TEA May 1991
 *	 Fixed slcpool bug that did not allow recompiling large macros in
 *	 LIBC.REL and fixed -DMACRO -UMACRO bug.
 *	 Modules: CCPP, CCPARM.H
 *
 * 2A(10) KAR June 1991
 *	  Fixed mixed listing bug having to do with the character combo
 *	  " /" where whatever followed the '/' would be echoed twice in
 *	  the listing.  Also added the last two formats of monitor calls
 *	  to the in-line monitor calls facility.  Also needed to recompile
 *	  to get the fix to the clock() function so the compiler will 
 *	  report the correct number of CPU seconds used.
 *	  Modules: CCPP, CCGEN.H, CCOUT, CCCODE, CCGEN2
 *
 * 2A(11) TEA July 1991
 *	  Fixed "[internal error] rrdfind: no regs" bug.
 *	  Generate better code for switch statements.
 *	  Increased type table size from 1021 to 1279.
 *	  Modules: CCREG, CCGSWI, CCPARM.H
 *
 * 2A(12) TEA, KAR August 1991
 *	  Closed SPRs 9562, 9568, 9578, and 9579 regarding invalid address
 *	  generated by optimizer, spurios [internal error] message, sizeof
 *	  short array of 1 element being 2 (was 4), and infinite loop of
 *	  error messages. Uses new KCCVER.H so only 1 copy of version number.
 *	  Rebuilt compiler so that it would use the new LIBC and took out
 *	  the -i switches from the banner.
 *	  KAR changed help banner 1990 to 1991 and replaced malloc's with 
 *	  calloc's in CCSYM.
 *	  Modules: CC, CCDECL, CCOUT, CCOPT, CCSTMT, CCSYM 
 *
 * 2A(13) TEA  August 1991
 *	  Changed chars to default to unsigned chars.
 *	  Added "#pragma eof" switch to CCPP (PPS 4297).
 *	  Added "-s" switch to redirect KCC messages to stdout (PPS 4298).
 *	  Linked to new LIBC with _cleanup fix (SPR 9593).
 *	  Now more than 3 file names are allowed on comand line (SPR 9590).
 *	  Fixed int a[2][2], **ptr=&a[1] (SPR 9503).
 *	  Fixed int i1 = struc.sub_struc.i2 (for more than one '.' or
 *	  "->" on a global initializer) (SPR 9604).
 *	  Fixed negative values of unsigned ints with 1 in high bit(SPR 9603).
 *	  Fixed seven unsafe macros that had 10 calls to them.
 *	  Added new-style prototypes for all KCC functions.
 *	  Removed need to pull in ULTOA.
 *	  Added verbose switches "-v=load" and "-v=fundef" and expanded
 *	  "-v" switch. Removed several switches from non-debug (#if DEBUG_KCC)
 *	  version, i.e. what "cc" displays is all that is available.
 *	  Minimized __COMPILER_KCC__ jacketed code to 50 lines.
 *	  Jacketed unreachable code with #if 0..#endif  shrinking KCC.
 *	  Several cleanups recommended by PC-Lint including 7 new defaults
 *	  in switch statements (new [internal errors] that should never be
 *	  reached and dozens of variables and functions became static to one
 *	  file).
 *	  Modules: ALL but CCDATA, CTYPE.H, and C-ENV.H
 *
 * 2A(14) TEA September 1991
 *	  Fixed #pragma eof bug that popped off #if's from all files.
 *	  Changed one line in ifpopchk().
 *	  Module: CCPP
 *
 * 2A(15) TEA September 1991
 *	  Fixed -DD [internal error] bug in CCPP.
 *	  Fixed giznull [internal error bug in CCSYM.
 *	  Shrank KCC by making PPTOK table in CCPP dynamic.
 *	  Replaced all malloc's with calloc's.
 *	  Modules: CC, CCASMB, CCCODE, CCERR, CCNODE, CCOUT, CCPP, 
 *	  	   CCREG, CCSYM
 *
 * 2A(16) TEA September 1991
 *	  Fixed -g=debug, -g=sprof bug in ridlsym() in CCSYM (caused by
 *	  indexing through a calloc'ed NULL ptr that had been a malloc'ed
 *	  random pointer before 2A(15)).
 *	  Moved error message from CCSYM to giznull() in CCGEN (SPR 9617).
 *	  Killed #pragma eof in CCPP (PPS 4309 replaced PPS 4297).
 *	  Modules: CCGEN, CCSYM, CCPP
 *
 * 2A(17) TEA, KAR September 1991
 *	  Shrank KCC low seg from 50 to 35 pages, by making emsgbuf[]
 *	  and types[] dynamic (but types[] non-dynamic with -DDEBUG_KCC=1)
 *	  and the remaining by shuffling struct members of type short
 *	  and char to be adjacent (PPTOKS, SYMBOL, TYPE, NODE, VREG, 
 *	  and PCODE structs) and making int arrays into char arrays
 *	  (convtab, popflg, popprc, typsiztab, and typbsiztab).
 *	  Kevin reordered -I search path to imitate -H (SPR   ).
 *	  Modules: CC.H, CCGEN.H, CCLEX.H, CCNODE.H, CCREG.H, CCSYM.H,
 *	  CCDATA, CCERR, CCPP, and CCTYPE
 *
 * 2A(20) TEA October 1991
 *	  Closed SPRs 9631, 9633 regarding bug that ignores keywords "bliss"
 *	  and "fortran" (CCSYM.H). Now KCCDOS recognizes these words (CCDECL,
 *	  and CCGEN2).
 *	  Modules: CCSYM.H, CCDECL, CCGEN2
 *
 * 2A(21) KAR,TEA,BEN October 1991
 *	  Nailed out a few problems with the mixed source-assembly language
 *	  listing facility including SPR 9640.
 *	  Benny fixed -p switch in outprolog() in CCOUT, SPR 9649.
 *	  Modules: CC.H, CC, CCPP, CCOUT
 *
 * 2A(22) TEA October 1991
 *	  Closed SPR 9650 concerning [internal error] generated by gotos.
 *	  Built with LIBC with stat() fix, SPR 9651.
 *	  Modules: CCNODE.H
 *
 * 2A(23) KAR November 1991
 *	  Implemented a project that generates a warning when an
 *	  identifier is being used before its initialization
 *	  (PPS #4159).
 *	  Modules: CCSYM.H, CCDECL, CCPP, CCLEX, CCSTMT
 *
 * 2A(24) TEA, KAR November 1991
 *	  Printed count of warnings to stderr (and stdout if -s), PPS 4340.
 *        Implemented #pragma include_once and #pragma message in CCPP,
 *	  PPS 4309 and 4???. Added -i switch (causes #pragma include_once for
 *	  all include files).
 *	  Shrank FNAMESIZE from 170 to 48 chars (1K low seg) in CCPARM.H
 *	  Shrank boolean flags in CC.H from int to char.
 *	  Made bin_muuo() in CCSTMT and gmuuo() in CCGEN2 compilable by BC++.
 *	  KAR, avoids false uninit var errors when &var is used in CCLEX.
 *	  Modules: CC, CCERR, CCGEN2, CCLEX, CCPP, CCSTMT, CC.H, CCPARM.H
 *
 * 2A(25) TEA, November 1991
 *        Fixed insert_file() to work for more than 16 *.H files.
 *	  Modules: CCPP
 *
 * 2A(26) KAR, November 1991
 *	  Only output the external request for the CRT module if a 
 *	  main() is present in the .C file.  This was done to ensure that
 *	  C modules meant as standalone functions for a FORTRAN, Bliss
 *	  or MACRO program would not bring in all of the C runtime code
 *	  (SPR #9595).
 *	  Also added a fix for the usage before initialization facility
 *	  regarding function parameters using the '&'.
 *	  NOTE: This compiler and the associated library must propagate
 *	  together.
 *	  Modules: CCOUT, CCLEX, CCSTMT
 *
 * 2A(27) KAR,TEA, January 1992
 *	  Closed SPRs 9718, 9719 about faulty optimization in foldadjbp().
 *	  Added KCC_DEBUG info to helpscreen for departmental debugging.
 *	  Implemented PPS asking KCC to signal the user of a possible
 *	  assignment operator instead of the equality operator seen
 *	  as the root operator of a control expression in an, if-stmt,
 *	  while-stmt, for-stmt, do-while-stmt, etc. (PPS #4293)
 *	  Modules: CC, CCOPT, CCSTMT
 *
 * 2A(30) KAR,TEA, January 1992
 *	  Changed compiler to place a /RUN switch in the LINK command
 *	  file which will run MAKSHR on the generated executable so
 *	  the .EXE will not have any initialized low-seg memory unless
 *	  any of the debugging (-g) switches have been specified.
 *	  Also added a facility that will generate code to check if
 *	  a dereferenced pointer is NULL.  If so, a message will be
 *	  sent to the users screen informing them of the NULL pointer.
 *	  This will be available by specifiying -g=nullptr on the KCC
 *	  command line,
 *	  Tim implemented the #pragma module(ID) facility (PPS 4329).
 *	  Modules: CC, CCASMB, CCGEN2, CCCODE, CCSTMT, CCOUT, CCNODE.H,
 *	  CCCODE.H, CCPP, CC.H
 *
 * 2A(31) KAR, TEA, January 1992
 *	  Eliminated *.PRE files.
 *	  Kevin added /o switch to makshr command.
 *	  Modules: CC, CCOUT, CCASMB, CC.H
 *
 * 2A(32) KAR, January, February 1992
 *	  In-line monitor calls using channel numbers can now use
 *	  channel numbers greater than 5 (SPR 9738).
 *	  Also changed makprefile() to output the ENTRY point list into
 *	  the .MAC file just before the END or PRGEND statements so 
 *	  indexed libraries can become a reality once again.  This was
 *	  the chosen solution over reactivating the .PRE files because
 *	  bringing back the .PRE files would not work for #pragma module
 *	  usage.
 *	  Modules: CCOUT
 *
 * 2A(33) KAR,TEA February 1992
 *	  TEA fixed KCCDBG (CC.H, CCSYM.H, CCDATA, CCSYM) and removed
 *	  BC++ error in CCASMB.
 *	  KAR added -K switch to signal the user wishes to make use of the
 *	  new strictly ANSI library, LIBCA.
 *	  Modules: CC, CCASMB, CCDATA, CCOUT, CCSYM, CC.H, CCSYM.H
 *
 * 2A(34) KAR,TEA  May 7 1992
 *	  KAR fixed the order in which the header files are searched
 *	  when the -K switch is used.  Also implemented PPS 4296, which
 *	  calls for a method to specify the name of the .REL file
 *	  produced, which implies the name of the .MAC file.  This was
 *	  done by providing the -R=<filename>.REL switch.  Also changed
 *	  the switch on the MAKSHR command line to /d(elete) from 
 *	  /o(verwrite) to delete the .MKS file rather than just
 *	  overwrite it.  Also removed dependency on LIBC and its non-ANSI
 *	  routines by using ANSI I/O and an internal macro for getting the
 *	  job number.  Lastly, I changed the version number facility used 
 * 	  by KCC so that it wouldn't need Get_Version() from LIBC.
 *
 *	  TEA fixed DEBUG_KCC=1 in CCDATA and CCSYM. Closed SPR 9774
 *	  in CCCSE by avoiding an invalid optimization that folds identical
 *	  mod and div expressions, overwriting the mod register. Closed SPR
 *	  9807 by fixing 18 heap calls in 7 files that did not test for NULL.
 *	  Rewrote stat() and fstat() so KCC can be built with just LIBCA.
 *	  Rewrote -q to avoid recompiling regardless of ".C" suffix.
 *	  Closed SPR 9813 about sizeof(s) where s is array of short/ushort.
 *	  Modules: CC, CCASMB, CCCODE, CCDATA, CCERR, CCLEX, CCPP,
 * 	  CCSTMT, CCSYM, KCCHST.H
 *
 * 2A(35) KAR,TEA May 27, 1992
 *	  TEA changed the help screen to include the new -R switch.
 *	  KAR fixed the problems with the replacement for stat() in the
 *	  compiler.  I had to fix a problem it had with finding the
 *	  symbol files.  It involved assigning an undefined value to the
 *	  PPN field in the arg-block for the FILOP.  Then I realized 
 *	  why the -q switch wouldn't work on occasion.  There was no
 *	  facility that parsed a PPN when given as part of a filename
 *	  to the new stats() function.  I added a function that would
 *	  parse a PPN if one was given and fixed fnparse() to handle 
 *	  device names of 6 characters thus fixing the -q switch.
 *	  Modules: CC, CCASMB
 *
 * 2A(36) KAR June 1992
 *	  Closed SPR 9833 that caused the #pragma include_once to span
 *	  across source files if more than one was placed on the command
 *	  line.
 *	  Closed SPR 9862 which generated bad pointer values for constant
 *	  pointers to typedef'ed structs.  The fix was put into elembsize(),
 *	  it checks if it is looking at a struct and passes back word size
 *	  instead of 0 which translates to a void * and causes the pointer
 *	  to be generated as a 9-bit byte pointer.
 *	  Modules: CC, CCSYM, CCSYM.H
 *
 * 2A(37) KAR, FEW, July 1992
 *	  KAR closed SPR 9879 that caused any error in any of the files in
 *	  a list of files given to KCC to not run MACRO even on the files
 *	  that had no errors.  Now it will run MACRO, but not LINK if
 *	  there are any files that did not have errors.
 *	  Modules: CC
 *	  FEW closed SPR 9577 (source filespec prefix and suffix, if any,
 *	  were being forced onto "" #include filespecs).
 *	  Module: CCPP
 *	  FEW closed SPR 9877.  Function ccopt:foldboth () transformed
 *	  code sequence <MOV R,S><OP R,x> into <OP S,x><MOV R,S>.  This
 *	  transformation is invalid in the case where "x" refers to "R".
 *	  Module: CCOPT
 *	  FEW closed SPR 9574.  Function ccjskp:optlab () failed to detect
 *	  the invalidation of one optimization by a JUMP or JRST out of the
 *	  peephole buffer.
 *	  Module: CCJSKP
 *	  FEW added a LINK command to /SEARCH KCCDBG.REL.
 *	  Module: CCASMB
 *
 * 2A(40) FEW, July 1992
 *	  KAR, August 1992
 *	  FEW closed SPR 9835.  The VREG struct was trying to keep the stack
 *	  offset of a spilled object in a char.  This meant that any
 *	  function with more than 512 words of locals would get bad code
 *	  when any spilled object was reloaded.
 *	  Module: CCREG.H
 *	  FEW closed SPR 9747.  The code that checked for arithmetic overflow
 *	  always as only checked for signed overflow, which caused it to
 *	  falsely report overflow on some unsigned operations.  Code was
 *	  added to handle unsigned addition and subtraction by checking
 *	  the processor CARRY0 flag.
 *	  Module: CCEVAL
 *	  FEW closed SPR 9734.  Type qualifiers, applied to declared instances
 *	  of typedef'ed arrays, were being wrongly applied to the type
 *	  itself.
 *	  Module: CCDECL
 *	  KAR closed PPS 4551.  Added built-in macro in KCC for DOS,
 *	  __KCCDOS__ so that host specifics in header files can be jacketed
 *	  like in the new <csisym.h> for syntax checking on DOS
 *	  And closed PPS 4543.  Removed the note class message of unknown
 *	  #pragma.
 *	  Modules: CCPP, CSISYM.H, UUOSYM.H
 *	  KAR had to add an #undef and #define of sixbit around the #include
 *	  of csimon.h because of the STUPID typedef of char6 to sixbit in
 *	  comdef.h!  Something needs to be done about this.
 *	  Modules: CCASMB
 *
 * 2A(41) KAR August 1992
 *	  Implemented PPS 4516 which called for support of leveled header
 *	  files.
 *	  Modules: CC CCPARM.H
 *	  Also implemented PPS 4544 which asks for a warning (advisory)
 *	  when an auto variable is given a value but not used.
 *    NB. This was not completed in this release.
 *	  Modules: CCLEX CCSYM CCSYM.H
 *    FEW resolved SPR 9925.  Code in CCREG.C to manipulate double-word
 *	  virtual registers was not prepared to handle cases where the
 *	  given register was the second half of a double.
 *	  Module: CCREG
 *    FEW resolved SPR 9919.  Code that cast word pointers to
 *	  byte pointers formerly used TLO, which assumed left half
 *	  empty; substituted HRLI for TLO.
 *	  Modules: CCOUT CCCODE.H
 *    FEW changed the interface to KCCDBG so that the address of
 *	  its LUUO handler is set in the .MAC file.  This makes KCCDBG
 *	  independent of SIX36 () and thus useable in the absence of a
 *	  C main () function.
 *        Module: CCOUT
 *    FEW resolved SPR 9977.  In the absence of any global symbols,
 *	  the label indicating the end of the local symbol table (in a
 *        module compiled with -g=debug) was not being emitted.
 *        Module: CCSYM
 *
 * 2A(42) FEW December 1992
 *	- in CCPARM.H, increased THASHSIZE and MAXTYPE from 1279 to 2557
 *	    to accomodate Tools Development Group (no SPR).
 *	- in CCASMB.C, changed the forced LINK switches for KCCDBG.REL
 *	    versions 2(2) and later (overlooked in 2A(41), no SPR).
 *	- resolved SPR 9986.  The "-d" option, given alone, is now
 *	    tantamount to "-d=all".  Module CC.C.
 * 	- implemented PPS 4575, function-level-only debugging.  This
 * 	    involved a new keyword for the -g switch.  Modules:
 * 	    CC.H, CC.C, CCDBUG.C, CCSTMT.C, CCOUT.C, CCASMB.C, CCSYM.C,
 * 	    CCPP.C, CCDECL.C, CCGEN2.C
 *
 * 2A(43) KAR February 1993
 *	- Implemented PPS 4574 calling for C++ comments in KCC.
 *	Modules: CCPP.C
 *
 * 	FEW, March 1993
 * 	- resolved SPR 9943 ("scope mismatch" error after function-
 * 	    pointer declaration).  Module CCDECL.C.
 * 	- resolved SPR 0041 (faulty optimization that ignored a transfer
 *	    of control occurring between two instructions being folded).
 * 	    Module CCCODE.C.
 *
 * 2A(44) FW March 1993
 *      - Experimental version identical to 2A(43) except using more
 *        registers internally.  Not merged with production sources.
 *
 * 2A(45) FW July 1993
 *      - added "-P=nocpp" option and global variable "clevnocpp"
 *        to defeat C++ "//" comments.  Default is to permit them.
 *        Files affected: CC.C, CCPP.C, CCDATA.C, CC.H
 *
 * 2A(46) FW August 1993
 *	- "-P=nocpp" wasn't right the first time; patches in lots of
 *	  different places were needed.
 *	  Files affected: CCPP.C
 *
 * 2A(47) FW September 1993
 *	- fixed EOL detection in ccpp:flushtoeol ().
 *	- implemented new command-line option "-b[89]" for
 *		wide-character source files (PPS 4761).
 *	- implemented new command-line option "-r=n" to specify
 *		the number of non-preserved registers.
 *	- added function jwarn () to deal with situations that
 *		involve bad arguments to command-line options
 *		where defaults can be supplied so that compilation
 *		need not be aborted.
 *	  Files affected: CC.C, CC.H, CCPP.C, CCREG.C, CCREG.H,
 *		CCDECL.C, CCGEN.C, CCOUT.C, CCERR.C, CCERR.H
 *
 * 2A(50) FW September 1993
 *	- more corrections in CCPP for "//" comments.  Usually
 *		(but not always), we need to call pushch ()
 *		after a scancomm () to discard a "//" comment,
 *		to make sure that the EOL is detected.  There
 *		was also a wrong condition test in pass_line ().
 *		However, a test program for "//" comments has
 *		composed by Tim Abels and added to the Plum-
 *		Hall validation suite; these comments should now
 *		be right.
 *	  File affected: CCPP.C
 *
 * 2A(51) FW January 1994
 *	- removed ALL remaining support for: processors other than
 *		single-section KL (and workalikes); operating
 *		systems other than TOPS-10 (and workalikes);
 *		assemblers other than MACRO.  The processor
 *		restriction has the extra benefit that KCC now
 *		generates *no* MACRO constructs that result in
 *		Polish fixups.
 *	- added "-k" option for long identifiers.  MACRO is
 *		invoked with "/k", and identifiers are emitted
 *		to the MACRO code full length, full strength.
 *
 * 2A(52) FW February 1994
 *	- implemented "interrupt" function type qualifier.
 *		Modules: cc, ccgen, ccgen1, ccgen2, ccdecl,
 *		cc.h, ccsym.h, cctoks.h
 */
