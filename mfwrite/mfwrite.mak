# MFWRITE Make File
.SUFFIXES: .c

MFWRITE.EXE: \
  MFWRITE.OBJ \
  MFWRITE.MAK \
  {.;$(LIB)}midifile.lib \
  {.;$(LIB)}mfwrite.def
   link386.exe MFWRITE.OBJ  /PACKD /NOL /PM:VIO /ALIGN:1 /BASE:0x10000 /EXEPACK,MFWRITE.EXE,NUL,midifile.lib,mfwrite.def;
#debug version
#  link386.exe MFWRITE.OBJ  /PACKD /NOL /CO /PM:VIO /ALIGN:1 /BASE:0x10000 /EXEPACK,MFWRITE.EXE,NUL,midifile.lib,mfwrite.def;

{.}.c.obj:
   icc.exe /Tdc /Q /O+ /C /Gh- /Sh- /Ti- /Ts- .\$*.c
#debug version
#  icc.exe /Tdc /Q /Ti /C .\$*.c

!include MFWRITE.DEP
