:restart

@echo off
cls

set game=
set features=
set vsversion=
set vs2019hack=

echo Games
echo --------
echo.

set /p HL2MP=Compile Half-Life 2 DM? (y): 
If not "%HL2MP%"=="n" (
    set game=%game% /hl2mp
)

cls

echo.
echo Optional Features
echo --------
echo.

set /p Episodic=Use Episodic Code? (y): 
If not "%Episodic%"=="n" (
    set features=%features% /define:USE_EPISODIC
)

set /p SSE=Use Source Shader Editor? (y): 
If not "%SSE%"=="n" (
    set features=%features% /define:USE_SSE
)

set /p Deffered=Use Deffered Lighting? (y): 
If not "%Deffered%"=="n" (
    set features=%features% /define:USE_DEFFERED
)

cls

echo.
echo Visual Studio Version
echo --------
echo.

set /p VS2010=Use VS2010 Toolset? (y): 
If not "%VS2010%"=="n" (
    set vsversion=%vsversion% /2010
)

set /p VS2012=Use VS2012 Toolset? (y): 
If not "%VS2012%"=="n" (
    set vsversion=%vsversion% /2012
)

set /p VS2013=Use VS2013 Toolset? (y): 
If not "%VS2013%"=="n" (
    set vsversion=%vsversion% /2013
)

set /p VS2019=Apply VS2019 Support? (y): 
If not "%VS2019%"=="n" (
    set vs2019hack=%vs2019hack% /define:VS2019
)

cls

title Generating %game% %features% %vsversion% %vs2019hack% Solution File

devtools\bin\vpc.exe +game %game% %features% /mksln Games.sln %vsversion% %vs2019hack%

if errorlevel 1 (
    pause
	goto restart
)

pause