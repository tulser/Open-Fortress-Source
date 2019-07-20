@echo off

devtools\bin\vpc.exe /ofd +gamedlls /mksln openfortress.sln /2013

for /D %%x in (*) do (
 cd %%x
 for %%y in (*.vcxproj) do (
  echo %%y
  "C:\Program Files (x86)\MSBuild\12.0\Bin\MSBuild.exe" %%y
 )
 cd ..
)

exit