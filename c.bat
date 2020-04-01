@echo off

del test.exe

set ICLCFG=icl64.cfg

set CYGWIN=nodosfilewarning
for %%a in (IDX\*.idx) do ( 
  if %%~pna.idx==%%~pnxa (
    perl IDX\idx2inc.pl %%a >nul
    move /y "%%~pna_?.inc" MOD\
  )
)

set icl=C:\IntelJ2190\bin-intel64\icl2d.bat 

call %icl% green.cpp 
rem /Od /Zi 

del *.exp *.obj
