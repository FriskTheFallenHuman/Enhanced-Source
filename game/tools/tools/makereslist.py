"""Little gizmo for compiling all alien swarm reslists"""

import os, sys
from filesystem import P4File

def LoadMaplist( filename ):
    """really simple parser for maplist.txt: ignore comments, strip quotes.
    returns a list."""

    data = file(filename,"r").readlines()
    # filter out blank lines, quote marks and comments
    data = [ x for
             x in ( y.strip().strip("'\"") for y in data )
               if ( len(x) > 0 and not x.startswith("//") ) ]
    # filter out quotes
    return data
#end def LoadMaplist()

# you should perform a % substitution with (gamename, mapname)
g_commandline = """..\\swarm.exe +sv_lan 1 -allowdebug -window -dev +sv_consistency 0 -tempcontent -makereslists makereslists.txt +map \"%s\""""
    

## the commandlines for compiling reslists are patently redundant
## and bizarre.

os.chdir(os.path.join( os.getenv("VGAME","."), os.getenv( "VMOD", "." )))
for mapname in LoadMaplist("maplist.txt") :
    # open in perforce
    for f in ( ( "reslists/%s.snd" % mapname ) ,
               ( "reslists/%s.lst" % mapname ) ) :
        P4File(f).editoradd()
        
    print g_commandline % ( mapname )
    os.system(g_commandline % ( mapname ))
