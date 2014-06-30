# MFVLQ Make File

.SUFFIXES: .c

MFVLQ.EXE: \
  MFVLQ.OBJ \
  MFVLQ.MAK \
  {.;$(LIB)}midifile.lib \
  {.;$(LIB)}mfvlq.def
   link386.exe MFVLQ.OBJ /PACKD /NOL /PM:VIO /ALIGN:1 /BASE:0x10000 /EXEPACK,MFVLQ.EXE,NUL,midifile.lib,mfvlq.def;
#debug version
#  link386.exe MFVLQ.OBJ /PACKD /NOL /CO /PM:VIO /ALIGN:1 /BASE:0x10000 /EXEPACK,MFVLQ.EXE,NUL,midifile.lib,mfvlq.def;

{.}.c.obj:
   icc.exe /Tdc /Q /O+ /C /Gh- /Sh- /Ti- /Ts- .\$*.c
#debug version
#  icc.exe /Tdc /Q /Ti /C .\$*.c

!include MFVLQ.DEP
