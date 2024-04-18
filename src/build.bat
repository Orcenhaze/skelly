@echo off

REM The app name that will be used for the executable. 
set APPNAME=app


REM Find latest Visual Studio installation via vswhere.exe to use MSVC toolset from command line.
setlocal enabledelayedexpansion
where /Q cl.exe || (
  set __VSCMD_ARG_NO_LOGO=1
  for /f "tokens=*" %%i in ('"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" -latest -requires Microsoft.VisualStudio.Workload.NativeDesktop -property installationPath') do set "VS=%%i"
  if "!VS!" equ "" (
    echo ERROR: Visual Studio installation not found
    exit /b 1
  )  
  call "!VS!\VC\Auxiliary\Build\vcvarsall.bat" amd64 || exit /b 1
)
if "%VSCMD_ARG_TGT_ARCH%" neq "x64" (
  echo ERROR: please run this from MSVC x64 native tools command prompt, 32-bit target is not supported!
  exit /b 1
)


REM USED COMPILER OPTIONS:
REM /Od      : Disables optimization.
REM /MTd     : Links to the static version of the CRT (debug version).
REM /fp:fast : Specifies how the compiler treats floating-point expressions.
REM /GM-     : Enables minimal rebuild.
REM /GR-     : Disables RTTI (C++ feature).
REM /EHa-    : Disables C++ stack unwinding for catch(...).
REM /Oi      : Generates intrinsic functions.
REM /WX      : Treats all warnings as errors.
REM /W4      : Sets warning level 4 to output.
REM /wd####  : Disables the specified warning.
REM /FC      : Displays full path of source code files passed to cl.exe in diagnostic text.
REM /Z7      : Generates C 7.0-compatible debugging information.
REM /Zo      : Generates enhanced debugging information for optimized code (release build).
REM /Fm      : Creates a map file.
REM /opt:ref : Eliminates functions and data that are never referenced.
REM /LD      : Creates a .dll.
REM /I       : Additional include directories.


REM CF: common compiler flags; LF: common linker flags.
REM
set CF=/nologo /fp:fast /Gm- /GR- /EHa- /Oi /WX /W4 /wd4201 /wd4100 /wd4189 /wd4505 /wd4456 /FC /Z7 /Zo /I..\src\vendor
REM set CF=/fsanitize=address %CF%
set LF=/incremental:no /opt:ref user32.lib gdi32.lib winmm.lib shell32.lib 


REM pushd takes you to a directory you specify and popd takes you back to where you were.
REM "%~dp0" is the drive letter and path combined of the batch file being executed.
REM
pushd %~dp0
if not exist ..\build mkdir ..\build
pushd ..\build

REM Debug build
cl /Od /MTd /DDEVELOPER=1 /Fe%APPNAME%_dev.exe %CF% ..\src\win32_main.cpp /link /MANIFEST:EMBED /MANIFESTINPUT:../src/%APPNAME%.manifest /entry:WinMainCRTStartup /subsystem:windows %LF% DXGI.lib

REM Release build
cl /O2 /MT  /DDEVELOPER=0 /DNDEBUG /Fe%APPNAME%.exe %CF% ..\src\win32_main.cpp /link /MANIFEST:EMBED /MANIFESTINPUT:../src/%APPNAME%.manifest /entry:WinMainCRTStartup /subsystem:windows %LF%

REM Delete intermediates
del *.obj *.res >NUL

REM Copy data folder to build folder
rd /S /Q .\data
xcopy ..\data .\data /E /I /Y >NUL

popd
popd
