## Description

After dealing for some time with TraceView, the only avaible tool for ETW / WPP tracing, I decided to create a really easy to use tool to view my kernel mode and user mode traces. 

ETViewer is that tool, it is still under development but is fully funcional. In some way is a TraceView/DebugView combination. 

ETViewer latest version is 1.1. 
## System Requirements
Windows XP: Whenever possible, the lastest dbghelp.dll available from "Debugging Tools for Windows" package.

## New in Version 1.1
* Converted project to VS2015 with v140_XP and v140 toolsets
* Fixed bug with loading source files for viewing
* Made it so you can still drop .pdb files when running as admin when UAC is enabled
* Shortened the source file name field so you can see the filename
* Made it so special formatters (like !HRESULT!) will show their value
* Added trace example application that generates trace messages
* Moved around some of the folders and made it so everything builds to a bin folder that starts at the solution root

## New in Version 1.0
* Fixed issue with double %% in trace formatting.
* Added support for having multiple PDB files with same control GUID
* Now compiling with VS 2013 and have second target for XP compatibility.

## New in Version 0.9
* Fixed 64 bits PDB issues.
* File association support (optional).
* Command line arguments to open ETL, source and PDB files.
* Reload of PDB and source files (Auto / Ask / Disable).
* Configurable trace font.
* Configurable source file search paths.
* Settings dialog.
 
![](Home_HighlightSample.jpg)
Trace highlight snapshot

## Features

Supported session types

* Full Real time tracing.
* Full log file support (.etl).

Trace provider management

* PDB based (no .tmf/.tmc support).
* Complete trace provider management (Activation, Flags and Levels).
* Multiple concurrent providers in the same session.
* Multiple providers in the same PDB.

Trace management

* Search (to find, delete or mark traces).
* Mark (Visual Studio style).
* Highlight (coloring traces, DebugView style).
* Instant Exclusion/Inclusion filters for Real-Time tracing.
* Column sorting.

Other features

* Source code visualization with simple sintax highlight.
* Drag&Drop support.
* Clipboard support.
* Export to text file.
* File association support (optional).

Tested platforms

* Windows XP 32 Bits
* Windows 2003 32 Bits
* Windows Vista 32 Bits
* Windows 7 64 Bits
* W2K not yet supported.

![](Home_SourceSample.jpg)

Source file viewer snapshot