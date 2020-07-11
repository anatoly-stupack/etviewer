echo off

set ROOT=%~dp0\..\

msbuild %ROOT%trunk\ETViewer.sln -p:Configuration=Release -p:Platform=Win32