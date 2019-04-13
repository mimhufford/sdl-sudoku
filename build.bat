@echo off

SET SDLPath=lib\SDL2-2.0.9

REM clean up old crap so we never run an out of date version
IF EXIST main.exe DEL main.exe
IF EXIST main.obj DEL main.obj

REM check we can actually link to SDL
IF NOT EXIST %SDLPath% (
  ECHO Could not find %SDLPath%
  EXIT /B
)

REM copy the dll so the exe can pick it up
IF NOT EXIST SDL2.dll XCOPY /q %SDLPath%\lib\x64\SDL2.dll .

REM nologo - get rid of cl header crap
REM wd4530 - I don't want exception handling, ignore warnings

cl main.cpp ^
   /nologo /wd4530 ^
   /I %SDLPath%\include ^
   /link /LIBPATH:%SDLPath%\lib\x64 SDL2.lib SDL2main.lib /SUBSYSTEM:CONSOLE