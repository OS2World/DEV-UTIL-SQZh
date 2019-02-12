
.c.obj:
  cl -c /AS /Zi /W4 /G2rs /Ozax $*.c

sqzH.exe : $$(@B).obj $$(@B).def token.obj makefile
  link /NOD/ST:8192 $(*B) token,,nul,os2 slibce,$*.def;
#  cvpack -p $*.exe
  bind $*.exe api.lib os2.lib

sqzH.obj : $$(@B).c token.h

token.obj : $$(@B).c $$(@B).h
