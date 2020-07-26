echo off

set ROOT=%~dp0\..\

nuget restore %ROOT%trunk\ETViewer.sln
IF NOT %ERRORLEVEL% == 0 goto end

msbuild %ROOT%trunk\ETViewer.sln -p:Configuration=Release -p:Platform=Win32
IF NOT %ERRORLEVEL% == 0 goto end

:end