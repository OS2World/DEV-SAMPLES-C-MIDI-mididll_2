# MFTOVLQ Make File

.SUFFIXES: .c

MFTOVLQ.EXE: \
  MFTOVLQ.OBJ \
  MFTOVLQ.MAK \
  {.;$(LIB)}midifile.lib \
  mftovlq.def
  link386.exe MFTOVLQ.OBJ /PACKD /NOL /PM:VIO /ALIGN:1 /BASE:0x10000 /EXEPACK,MFTOVLQ.EXE,NUL,midifile.lib,mftovlq.def;
#debug version
# link386.exe MFTOVLQ.OBJ /PACKD /NOL /CO /PM:VIO,MFTOVLQ.EXE /ALIGN:1 /BASE:0x10000 /EXEPACK,NUL,midifile.lib,mftovlq.def;

{.}.c.obj:
   icc.exe /Tdc /Q /O+ /C /Gh- /Sh- /Ti- /Ts- .\$*.c
#debug version
#  icc.exe /Tdc /Q /Ti /C .\$*.c

!include MFTOVLQ.DEP
