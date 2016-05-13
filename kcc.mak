# Microsoft Visual C++ Generated NMAKE File, Format Version 2.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

!IF "$(CFG)" == ""
CFG=Win32 Debug
!MESSAGE No configuration specified.  Defaulting to Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "Win32 Release" && "$(CFG)" != "Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "kcc.mak" CFG="Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

################################################################################
# Begin Project
# PROP Target_Last_Scanned "Win32 Debug"
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WinRel"
# PROP BASE Intermediate_Dir "WinRel"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "WinRel"
# PROP Intermediate_Dir "WinRel"
OUTDIR=.\WinRel
INTDIR=.\WinRel

ALL : $(OUTDIR)/kcc.exe $(OUTDIR)/kcc.bsc

$(OUTDIR) : 
    if not exist $(OUTDIR)/nul mkdir $(OUTDIR)

# ADD BASE CPP /nologo /W3 /GX /YX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /FR /c
# ADD CPP /nologo /W3 /GX /YX /O2 /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "SYS_CSI" /D __MSDOS__=1 /D "_CHAR_UNSIGNED" /FR /J /c
CPP_PROJ=/nologo /W3 /GX /YX /O2 /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D\
 "SYS_CSI" /D __MSDOS__=1 /D "_CHAR_UNSIGNED" /FR$(INTDIR)/\
 /Fp$(OUTDIR)/"kcc.pch" /Fo$(INTDIR)/ /J /c 
CPP_OBJS=.\WinRel/
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o$(OUTDIR)/"kcc.bsc" 
BSC32_SBRS= \
	$(INTDIR)/CCGEN.SBR \
	$(INTDIR)/CCOUT.SBR \
	$(INTDIR)/CCDATA.SBR \
	$(INTDIR)/CCGSWI.SBR \
	$(INTDIR)/CC.SBR \
	$(INTDIR)/CCLEX.SBR \
	$(INTDIR)/CCERR.SBR \
	$(INTDIR)/CCDECL.SBR \
	$(INTDIR)/CCREG.SBR \
	$(INTDIR)/CCCREG.SBR \
	$(INTDIR)/CCTYPE.SBR \
	$(INTDIR)/CCSYM.SBR \
	$(INTDIR)/CCCODE.SBR \
	$(INTDIR)/CCGEN1.SBR \
	$(INTDIR)/CCPP.SBR \
	$(INTDIR)/CCOPT.SBR \
	$(INTDIR)/CCNODE.SBR \
	$(INTDIR)/CCSTMT.SBR \
	$(INTDIR)/CCGEN2.SBR \
	$(INTDIR)/CCASMB.SBR \
	$(INTDIR)/CCEVAL.SBR \
	$(INTDIR)/CCCSE.SBR \
	$(INTDIR)/CCJSKP.SBR \
	$(INTDIR)/CCDBUG.SBR

$(OUTDIR)/kcc.bsc : $(OUTDIR)  $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /NOLOGO /SUBSYSTEM:console /MACHINE:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /NOLOGO /SUBSYSTEM:console /MACHINE:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /NOLOGO /SUBSYSTEM:console /INCREMENTAL:no\
 /PDB:$(OUTDIR)/"kcc.pdb" /MACHINE:I386 /OUT:$(OUTDIR)/"kcc.exe" 
DEF_FILE=
LINK32_OBJS= \
	$(INTDIR)/CCGEN.OBJ \
	$(INTDIR)/CCOUT.OBJ \
	$(INTDIR)/CCDATA.OBJ \
	$(INTDIR)/CCGSWI.OBJ \
	$(INTDIR)/CC.OBJ \
	$(INTDIR)/CCLEX.OBJ \
	$(INTDIR)/CCERR.OBJ \
	$(INTDIR)/CCDECL.OBJ \
	$(INTDIR)/CCREG.OBJ \
	$(INTDIR)/CCCREG.OBJ \
	$(INTDIR)/CCTYPE.OBJ \
	$(INTDIR)/CCSYM.OBJ \
	$(INTDIR)/CCCODE.OBJ \
	$(INTDIR)/CCGEN1.OBJ \
	$(INTDIR)/CCPP.OBJ \
	$(INTDIR)/CCOPT.OBJ \
	$(INTDIR)/CCNODE.OBJ \
	$(INTDIR)/CCSTMT.OBJ \
	$(INTDIR)/CCGEN2.OBJ \
	$(INTDIR)/CCASMB.OBJ \
	$(INTDIR)/CCEVAL.OBJ \
	$(INTDIR)/CCCSE.OBJ \
	$(INTDIR)/CCJSKP.OBJ \
	$(INTDIR)/CCDBUG.OBJ

$(OUTDIR)/kcc.exe : $(OUTDIR)  $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WinDebug"
# PROP BASE Intermediate_Dir "WinDebug"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "WinDebug"
# PROP Intermediate_Dir "WinDebug"
OUTDIR=.\WinDebug
INTDIR=.\WinDebug

ALL : $(OUTDIR)/kcc.exe $(OUTDIR)/kcc.bsc

$(OUTDIR) : 
    if not exist $(OUTDIR)/nul mkdir $(OUTDIR)

# ADD BASE CPP /nologo /W3 /GX /Zi /YX /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /FR /c
# ADD CPP /nologo /W3 /GX /Zi /YX /Od /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "SYS_CSI" /D __MSDOS__=1 /FR /J /c
CPP_PROJ=/nologo /W3 /GX /Zi /YX /Od /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D\
 "SYS_CSI" /D __MSDOS__=1 /FR$(INTDIR)/ /Fp$(OUTDIR)/"kcc.pch" /Fo$(INTDIR)/\
 /Fd$(OUTDIR)/"kcc.pdb" /J /c 
CPP_OBJS=.\WinDebug/
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o$(OUTDIR)/"kcc.bsc" 
BSC32_SBRS= \
	$(INTDIR)/CCGEN.SBR \
	$(INTDIR)/CCOUT.SBR \
	$(INTDIR)/CCDATA.SBR \
	$(INTDIR)/CCGSWI.SBR \
	$(INTDIR)/CC.SBR \
	$(INTDIR)/CCLEX.SBR \
	$(INTDIR)/CCERR.SBR \
	$(INTDIR)/CCDECL.SBR \
	$(INTDIR)/CCREG.SBR \
	$(INTDIR)/CCCREG.SBR \
	$(INTDIR)/CCTYPE.SBR \
	$(INTDIR)/CCSYM.SBR \
	$(INTDIR)/CCCODE.SBR \
	$(INTDIR)/CCGEN1.SBR \
	$(INTDIR)/CCPP.SBR \
	$(INTDIR)/CCOPT.SBR \
	$(INTDIR)/CCNODE.SBR \
	$(INTDIR)/CCSTMT.SBR \
	$(INTDIR)/CCGEN2.SBR \
	$(INTDIR)/CCASMB.SBR \
	$(INTDIR)/CCEVAL.SBR \
	$(INTDIR)/CCCSE.SBR \
	$(INTDIR)/CCJSKP.SBR \
	$(INTDIR)/CCDBUG.SBR

$(OUTDIR)/kcc.bsc : $(OUTDIR)  $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /NOLOGO /SUBSYSTEM:console /DEBUG /MACHINE:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /NOLOGO /SUBSYSTEM:console /DEBUG /MACHINE:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /NOLOGO /SUBSYSTEM:console /INCREMENTAL:yes\
 /PDB:$(OUTDIR)/"kcc.pdb" /DEBUG /MACHINE:I386 /OUT:$(OUTDIR)/"kcc.exe" 
DEF_FILE=
LINK32_OBJS= \
	$(INTDIR)/CCGEN.OBJ \
	$(INTDIR)/CCOUT.OBJ \
	$(INTDIR)/CCDATA.OBJ \
	$(INTDIR)/CCGSWI.OBJ \
	$(INTDIR)/CC.OBJ \
	$(INTDIR)/CCLEX.OBJ \
	$(INTDIR)/CCERR.OBJ \
	$(INTDIR)/CCDECL.OBJ \
	$(INTDIR)/CCREG.OBJ \
	$(INTDIR)/CCCREG.OBJ \
	$(INTDIR)/CCTYPE.OBJ \
	$(INTDIR)/CCSYM.OBJ \
	$(INTDIR)/CCCODE.OBJ \
	$(INTDIR)/CCGEN1.OBJ \
	$(INTDIR)/CCPP.OBJ \
	$(INTDIR)/CCOPT.OBJ \
	$(INTDIR)/CCNODE.OBJ \
	$(INTDIR)/CCSTMT.OBJ \
	$(INTDIR)/CCGEN2.OBJ \
	$(INTDIR)/CCASMB.OBJ \
	$(INTDIR)/CCEVAL.OBJ \
	$(INTDIR)/CCCSE.OBJ \
	$(INTDIR)/CCJSKP.OBJ \
	$(INTDIR)/CCDBUG.OBJ

$(OUTDIR)/kcc.exe : $(OUTDIR)  $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

################################################################################
# Begin Group "Source Files"

################################################################################
# Begin Source File

SOURCE=.\CCGEN.C
DEP_CCGEN=\
	.\CC.H\
	.\CCGEN.H\
	.\CCCHAR.H\
	.\CCPARM.H\
	.\CCSYM.H\
	.\CCNODE.H\
	.\CCERR.H\
	.\CCREG.H\
	.\CCCODE.H\
	.\CCTOKS.H

$(INTDIR)/CCGEN.OBJ :  $(SOURCE)  $(DEP_CCGEN) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CCOUT.C
DEP_CCOUT=\
	.\CC.H\
	.\CCGEN.H\
	.\CCCHAR.H\
	.\CCPARM.H\
	.\CCSYM.H\
	.\CCNODE.H\
	.\CCERR.H\
	.\CCREG.H\
	.\CCCODE.H\
	.\CCTOKS.H

$(INTDIR)/CCOUT.OBJ :  $(SOURCE)  $(DEP_CCOUT) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CCDATA.C
DEP_CCDAT=\
	.\CCSITE.H\
	.\CC.H\
	.\CCLEX.H\
	.\CCCHAR.H\
	.\CCGEN.H\
	.\CCCODE.H\
	.\CCTOKS.H\
	.\CCPARM.H\
	.\CCSYM.H\
	.\CCNODE.H\
	.\CCERR.H\
	.\CCREG.H

$(INTDIR)/CCDATA.OBJ :  $(SOURCE)  $(DEP_CCDAT) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CCGSWI.C
DEP_CCGSW=\
	.\CC.H\
	.\CCGEN.H\
	.\CCPARM.H\
	.\CCSYM.H\
	.\CCNODE.H\
	.\CCERR.H\
	.\CCREG.H\
	.\CCCODE.H\
	.\CCTOKS.H

$(INTDIR)/CCGSWI.OBJ :  $(SOURCE)  $(DEP_CCGSW) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CC.C
DEP_CC_C4=\
	.\CCSITE.H\
	.\CC.H\
	.\CCCHAR.H\
	.\KCCHST.H\
	.\CCPARM.H\
	.\CCSYM.H\
	.\CCNODE.H\
	.\CCERR.H\
	.\CCTOKS.H

$(INTDIR)/CC.OBJ :  $(SOURCE)  $(DEP_CC_C4) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CCLEX.C
DEP_CCLEX=\
	.\CC.H\
	.\CCCHAR.H\
	.\CCLEX.H\
	.\CCPARM.H\
	.\CCSYM.H\
	.\CCNODE.H\
	.\CCERR.H\
	.\CCTOKS.H

$(INTDIR)/CCLEX.OBJ :  $(SOURCE)  $(DEP_CCLEX) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CCERR.C
DEP_CCERR=\
	.\CC.H\
	.\CCLEX.H\
	.\CCCHAR.H\
	.\CCPARM.H\
	.\CCSYM.H\
	.\CCNODE.H\
	.\CCERR.H\
	.\CCTOKS.H

$(INTDIR)/CCERR.OBJ :  $(SOURCE)  $(DEP_CCERR) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CCDECL.C
DEP_CCDEC=\
	.\CC.H\
	.\CCPARM.H\
	.\CCSYM.H\
	.\CCNODE.H\
	.\CCERR.H\
	.\CCTOKS.H

$(INTDIR)/CCDECL.OBJ :  $(SOURCE)  $(DEP_CCDEC) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CCREG.C
DEP_CCREG=\
	.\CC.H\
	.\CCGEN.H\
	.\CCPARM.H\
	.\CCSYM.H\
	.\CCNODE.H\
	.\CCERR.H\
	.\CCREG.H\
	.\CCCODE.H\
	.\CCTOKS.H

$(INTDIR)/CCREG.OBJ :  $(SOURCE)  $(DEP_CCREG) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CCCREG.C
DEP_CCCRE=\
	.\CC.H\
	.\CCGEN.H\
	.\CCPARM.H\
	.\CCSYM.H\
	.\CCNODE.H\
	.\CCERR.H\
	.\CCREG.H\
	.\CCCODE.H\
	.\CCTOKS.H

$(INTDIR)/CCCREG.OBJ :  $(SOURCE)  $(DEP_CCCRE) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CCTYPE.C
DEP_CCTYP=\
	.\CC.H\
	.\CCPARM.H\
	.\CCSYM.H\
	.\CCNODE.H\
	.\CCERR.H\
	.\CCTOKS.H

$(INTDIR)/CCTYPE.OBJ :  $(SOURCE)  $(DEP_CCTYP) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CCSYM.C
DEP_CCSYM=\
	.\CC.H\
	.\CCCHAR.H\
	.\CCPARM.H\
	.\CCSYM.H\
	.\CCNODE.H\
	.\CCERR.H\
	.\CCTOKS.H

$(INTDIR)/CCSYM.OBJ :  $(SOURCE)  $(DEP_CCSYM) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CCCODE.C
DEP_CCCOD=\
	.\CC.H\
	.\CCGEN.H\
	.\CCPARM.H\
	.\CCSYM.H\
	.\CCNODE.H\
	.\CCERR.H\
	.\CCREG.H\
	.\CCCODE.H\
	.\CCTOKS.H

$(INTDIR)/CCCODE.OBJ :  $(SOURCE)  $(DEP_CCCOD) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CCGEN1.C
DEP_CCGEN1=\
	.\CC.H\
	.\CCGEN.H\
	.\CCPARM.H\
	.\CCSYM.H\
	.\CCNODE.H\
	.\CCERR.H\
	.\CCREG.H\
	.\CCCODE.H\
	.\CCTOKS.H

$(INTDIR)/CCGEN1.OBJ :  $(SOURCE)  $(DEP_CCGEN1) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CCPP.C
DEP_CCPP_=\
	.\CC.H\
	.\CCCHAR.H\
	.\CCLEX.H\
	.\CCPARM.H\
	.\CCSYM.H\
	.\CCNODE.H\
	.\CCERR.H\
	.\CCTOKS.H

$(INTDIR)/CCPP.OBJ :  $(SOURCE)  $(DEP_CCPP_) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CCOPT.C
DEP_CCOPT=\
	.\CC.H\
	.\CCGEN.H\
	.\CCPARM.H\
	.\CCSYM.H\
	.\CCNODE.H\
	.\CCERR.H\
	.\CCREG.H\
	.\CCCODE.H\
	.\CCTOKS.H

$(INTDIR)/CCOPT.OBJ :  $(SOURCE)  $(DEP_CCOPT) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CCNODE.C
DEP_CCNOD=\
	.\CC.H\
	.\CCPARM.H\
	.\CCSYM.H\
	.\CCNODE.H\
	.\CCERR.H\
	.\CCTOKS.H

$(INTDIR)/CCNODE.OBJ :  $(SOURCE)  $(DEP_CCNOD) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CCSTMT.C
DEP_CCSTM=\
	.\CC.H\
	.\CCLEX.H\
	.\CCPARM.H\
	.\CCSYM.H\
	.\CCNODE.H\
	.\CCERR.H\
	.\CCTOKS.H

$(INTDIR)/CCSTMT.OBJ :  $(SOURCE)  $(DEP_CCSTM) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CCGEN2.C
DEP_CCGEN2=\
	.\CC.H\
	.\CCGEN.H\
	.\CCPARM.H\
	.\CCSYM.H\
	.\CCNODE.H\
	.\CCERR.H\
	.\CCREG.H\
	.\CCCODE.H\
	.\CCTOKS.H

$(INTDIR)/CCGEN2.OBJ :  $(SOURCE)  $(DEP_CCGEN2) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CCASMB.C
DEP_CCASM=\
	.\CCSITE.H\
	.\CCCHAR.H\
	.\CC.H\
	.\CCPARM.H\
	.\CCSYM.H\
	.\CCNODE.H\
	.\CCERR.H\
	.\CCTOKS.H

$(INTDIR)/CCASMB.OBJ :  $(SOURCE)  $(DEP_CCASM) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CCEVAL.C
DEP_CCEVA=\
	.\CC.H\
	.\CCPARM.H\
	.\CCSYM.H\
	.\CCNODE.H\
	.\CCERR.H\
	.\CCTOKS.H

$(INTDIR)/CCEVAL.OBJ :  $(SOURCE)  $(DEP_CCEVA) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CCCSE.C
DEP_CCCSE=\
	.\CC.H\
	.\CCGEN.H\
	.\CCPARM.H\
	.\CCSYM.H\
	.\CCNODE.H\
	.\CCERR.H\
	.\CCREG.H\
	.\CCCODE.H\
	.\CCTOKS.H

$(INTDIR)/CCCSE.OBJ :  $(SOURCE)  $(DEP_CCCSE) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CCJSKP.C
DEP_CCJSK=\
	.\CC.H\
	.\CCGEN.H\
	.\CCPARM.H\
	.\CCSYM.H\
	.\CCNODE.H\
	.\CCERR.H\
	.\CCREG.H\
	.\CCCODE.H\
	.\CCTOKS.H

$(INTDIR)/CCJSKP.OBJ :  $(SOURCE)  $(DEP_CCJSK) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CCDBUG.C
DEP_CCDBU=\
	.\CC.H\
	.\CCPARM.H\
	.\CCSYM.H\
	.\CCNODE.H\
	.\CCERR.H\
	.\CCTOKS.H

$(INTDIR)/CCDBUG.OBJ :  $(SOURCE)  $(DEP_CCDBU) $(INTDIR)

# End Source File
# End Group
# End Project
################################################################################
