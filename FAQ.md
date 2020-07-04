## What does "Wrong version of DBGHelp.dll found (does not have the expected exports)." mean ?
ETViewer uses a version of dbghelp.dll newer than the version found in your system, this is common in Windows XP but should not happen in Windows Vista/7.
You have to download a newer version and copy it to the same folder as ETViewer, your can get it by downloading "Debugging Tools for Windows".
Although ETViewer can show traces of both 32 and 64 bits,  is a 32-bit application, so you must download the 32-bit version of  "Debugging Tools for Windows".

## What does "Unknown Trace, Missing provider? Please load the correct PDB file. " mean ?
These messages can be shown when reading .etl files. They are usually caused because no PDB file has been loaded, or because the loaded PDB files do not match the providers in the .etl file.
