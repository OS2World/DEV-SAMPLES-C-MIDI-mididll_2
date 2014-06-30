# MFREAD Make File
.SUFFIXES: .c

MFREAD.EXE: \
  MFREAD.OBJ \
  MFREAD.MAK \
  {.;$(LIB)}midifile.lib \
   mfread.def
   link386.exe MFREAD.OBJ /PACKD /NOL /PM:VIO /ALIGN:1 /BASE:0x10000 /EXEPACK,MFREAD.EXE,NUL,midifile.lib,mfread.def;
#debug version
#  link386.exe MFREAD.OBJ /PACKD /NOL /CO /PM:VIO /ALIGN:1 /BASE:0x10000 /EXEPACK,MFREAD.EXE,NUL,midifile.lib,mfread.def;

{.}.c.obj:
   icc.exe /Tdc /Q /O+ /C /Gh- /Sh- /Ti- /Ts- .\$*.c
#debug version
#  icc.exe /Tdc /Q /Ti /C .\$*.c

!include MFREAD.DEP
