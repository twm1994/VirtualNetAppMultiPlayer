1. Link to windows API:
Open Properties 
-> Under the OMNeT++ section, go to Makemake 
-> Select "src:makemake deep -> <project name>" and click Option 
-> In the Option settings, switch to the Link panel -> Check "Add libraries and other linker options from enabled project features" 
-> clieck "More >>", if at all
-> Under "Additional libraries to link with: (-l optoin)", add gdi32 and WS2_32 to name the link references.