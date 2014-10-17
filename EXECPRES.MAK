# Created by IBM WorkFrame/2 MakeMake at 14:52:18 on 09/03/94
#
# This makefile should be run in the following directory:
#   e:\bryan\execsrce
#
# The actions included in this makefile are:
#   COMPILE::IBM C/C++ Tools Version 2.0
#   LINK::IBM C/C++ Tools Version 2.0

.all: \
  .\EXECPRES.EXE

.SUFFIXES:

.SUFFIXES: .c

.c.obj:
      @echo WF::COMPILE::IBM C/C++ Tools Version 2.0
      ICC.EXE /Tl- /Tdc /Q /Wpro /Gm /C %s

.\EXECPRES.EXE: \
    .\executil.obj \
    .\runprog.obj \
    .\execpres.obj \
    {$(LIB)}EXECPRES.DEF \
    EXECPRES.MAK
      @echo WF::LINK::IBM C/C++ Tools Version 2.0
      ICC.EXE @<<
 /B" /pmtype:pm /nologo"
 /FeEXECPRES.EXE 
 EXECPRES.DEF
 .\executil.obj
 .\runprog.obj
 .\execpres.obj
<<


!include EXECPRES.Dep