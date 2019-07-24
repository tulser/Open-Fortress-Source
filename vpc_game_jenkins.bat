@echo off

devtools\bin\vpc.exe /ofd +game /mksln openfortress_all.sln /2013
type sln_fix.txt >> openfortress.sln