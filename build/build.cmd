echo off

set ROOT=%~dp0\..\

if exist nuget.exe goto restore

powershell.exe -Command "Invoke-WebRequest -OutFile nuget.exe https://dist.nuget.org/win-x86-commandline/latest/nuget.exe"

:restore
nuget restore %ROOT%src\ETViewer.sln
IF NOT %ERRORLEVEL% == 0 goto end

:build
msbuild %ROOT%src\ETViewer.sln -p:Configuration=Release -p:Platform=Win32
IF NOT %ERRORLEVEL% == 0 goto end

:end