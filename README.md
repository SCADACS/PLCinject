# PLCinject


## Compiling

As easy as

    $ make

Usually this is enough.
Then you can find the binary in the current dir.


### Troubleshooting

This tool depends on the Snap7 library available at
http://sourceforge.net/projects/snap7/.

#### Compilation error

We include the libraries of the latest release (1.4.0) and use the Linux
x86_64 version for compiling.
If you need a different version, you'll have to pick it from the
snap7/release/ dir and copy it to lib/.
If that won't work, you'll have to get the full release of Snap7, compile it
yourself and copy the library to lib/.

#### cannot open shared object file

PLCinject is dynamically linked with the Snap7's shared library in lib/.
If you (re)move the binary or the library, it won't work.


## Usage

The directory given by parameter -f must only contain the blocks to download to the plc.
The filename convention is &lt;blocktype&gt;_&lt;blocknumber&gt;.mc7 (e.g. "FC_1000.mc7")

    usage: plcinject -c ip [-r rack=0] [-s slot=2] [-b block] [-p block] [-f dir] [-d]
    
    -d      Display available blocks on PLC
    -p      Block that has to be injected/patched with a call instruction:  OBx, FBx or FCx on PLC, e.g. OB1
    -b      Block to call
    -f      Path to your block(s) you want to download to the plc


Example:

    plcinject -c 10.0.0.1 -p OB1 -b FC1000 -f /home/user/PATH 

This example will upload OB1 from the PLC at 10.0.0.1, inject this OB1 with a call instruction
to the function block FC1000 which is located at /home/user/PATH and then
downloads all blocks in /home/user/PATH and the modified OB1 back to the PLC.
