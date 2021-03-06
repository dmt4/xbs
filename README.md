
xbs ball-and-sticks plotting program
====================================
###### M. Methfessel Nov. 1995, Aug 2015,
###### contributions from A. Mottura, J. Labanowski,
###### packaging and small modifications from D. Pashov 2015


This program uses X-window graphics to produce ball-and-sticks plots.
It reads coordinates and other data from an input file (ie. ch4.bs)
and possibly different "frames" with shifted coordinates from an
auxillary move file (ie. ch4.mv). The default files are in.bs and in.mv.


INSTALLATION
------------

Look in the Makefile and adjust as necessary (mostly likely no change is needed).
Run make.


INPUT
-----

In a simple case, a file ch4.bs could look like this:

    atom      C       0.000      0.000      0.000
    atom      H       1.155      1.155      1.155
    atom      H      -1.155     -1.155      1.155
    atom      H       1.155     -1.155     -1.155
    atom      H      -1.155      1.155     -1.155

    spec      C      1.000   0.7
    spec      H      0.700   1.00

    bonds     C     C    0.000    4.000    0.109   1.00
    bonds     C     H    0.000    3.400    0.109   1.00
    bonds     H     H    0.000    2.800    0.109   1.00

This sets the coordinates in the format
    atom  species  x  y  z

and how to draw each atomic species, in the format
    spec name radius color

and how to draw bonds, in the format
    bonds name1 name2 min-length max-length radius color

A move file contains additional frames like this:

    frame This is frame number two
       0.000      0.000      0.000    1.155      1.155      1.155
      -1.155     -1.155      1.155    1.155     -1.155     -1.155
      -1.155      1.155     -1.155

After the keyword `frame` comes an indentifying string,
then come the coordinates for all the atoms. Line breaks can
appear anywhere between the coordinates.

Other input lines are also possible, namely the lines which set
parameters (see below). Example:  `inc  5`  sets the increment
for the rotation to 5 degrees.

In both files, lines starting with * and blank lines are comments.


USAGE
-----

After starting the program with `xbs ch4` the plot can be controlled
directly by selected keys. For example, to rotate the molecule
use the cursor keys and the keys "," and "." The last two were chosen
because they are below "<" and ">" whch look like arrows.
To step through the frames to show a "movie", use keys `[` and `]`.
A number of other keys are defined to function directly.
More complicated functions are done by pressing `i` to get an
input line and then typing a command.
Use `xbs -hh` to get information on keys and commands.

Sizing: Keys `+` and `-` make the plot bigger or smaller.

Perspective: Key p switches the perspective. Default is off.
For pseudoperspective, the sphere sizes depend on the distance
to the viewer but the positions on the page are unchanged.
For true perspective, the sizes and the positions both change.
The strength of the perspective effect depends on the distance to
the viewer, which is shown in the status line. It is changed with
keys `d` and `D` or can be set directly with command `dist`.
(Note: if you come too close and put the viewpoint inside an atom,
the program will try to color the whole universe, which takes very long).

Lighting: command `gramp slope middle` greys out the atoms in the
back by an exponential ramp. Command `light x y z` shades
the atoms somewhat as if light shines along vector (x,y,z).
These commands only work in black/white mode. To switch back
to normal coloring, enter `gramp` or `light` without arguments.

Positioning: Keypad keys 8,6,4,2 move the plot about by an increment
dpos (which can be set by the command `dpos`). Keypad key `*` saves
the current position as `home` (or use command `pos` to set the
home position directly). Keypad key 7 moves the plot home.
Positions are relative to the center of the window.

Saving: command `save` writes the data to a file (default Save.bs).
If there are multiple frames, a move file is also written.
The `save` command has some options, see below.

Printing: the command `print` writes output to a postscript file,
by default the file 'Bs.ps'. A print file stays open until it
is explicitly closed. In this way several plots can be superimposed
on the same page, using the keypad cursor keys to shift the subplots
on the page. The `print` command has some options, see below.


GETTING HELP
------------

Typing `xbs -h` and `xbs -hh` gives short respectively long help.

After starting xbs, key `h` toggles an overview. This is the same
information as is obtained with `xbs -hh`.

On the input line, use `help print` or `print ?` or even `print -h`
to get help on a specific command such as (in this case) print.
This help also shows any possible options for the command.

The space bar toggles extra information about the plot written
into the window.


SOME MORE DETAILS AND RECENT CHANGES
------------------------------------

1. Color:

  - A color is specified either as a number between 0 and 1 (gray value),
    three numbers between 0 and 1 (red, green, blue values or RGB),
    or as a color name from the file /usr/lib/X11/rgb.txt (or similar).

  - In the input file, the color of each species or bond is set
    in these three ways.

  - The following xbs options are related to color:
     -color   enable colors (default)
     -bw      uses greys only
     -st      uses grays only, stippled from a few values
     -rv      reverse all colors
     -auto    choose own colors.
   You can change these interactively with the update command.
   For consistency, black is now 0 and white is 1. The -rv option
   switches this. The only reason for using -bw or -st is that
   the commands `light` and `gramp` do not work with colors.

 - The `-auto` option is used to color the atoms in some standard way.
   These colors are set in routine `set_auto_colors` in file subs.h.
   Starting from the species label (ie. Pd3 or Mg-a) the leading
   alpha part is extracted and capitalized (ie. PD or MG) and
   this string is used to select a color.
   The idea is to put one's favorite colors into the routine
   and then recompile xbs.

 - The command `color` changes the colors of atoms interactively,
   ie. `color C* green` changes the color of all matching species.
   To show the color of a species, use the command without
   specifying a color (ie. `color C1` or `color C-a`).

 - When data is saved with the `save` command, the current colors
   (as set with `color`) are saved as a string.
   With `save -rgb` the RGB values are written instead of color names.
   If the -rv option is used, the reversed colors are saved as RGB values.

2. Some commands have options, eg. print, save, update. For example:
   ```
    print -T                   print and add some info as title
    print -t 'text .... '      print and put text as title
    update -rv                 update but switch on reversing
    update +rv                 update but switch off reversing
    update -bw                 update but switch to black/white
    save -rgb                  save with colors as rgb values
    save -step n               save with only every n'th frame
                               (good to compress a big .mv file).
   ```
   Use `help update` etc on the input line to see the options.

3. Pattern matching for atom labels: * matches any string, + any char.
   This can be used in the `color` command and in the `bonds` lines
   in the input file (which determine what bonds are drawn).
   For example, `bonds C* H*  ...` selects all bonds between
   atoms like C1 and H34 etc. and `bonds * * ...` selects all pairs.

4. Other miscellaneous changes:

  - Key `a` displays the axis directions.

  - Key `n` shows atom names or numbers. Key `c` shows the coordinates.

  - To close a postscript print file, use command `close`.

  - Command `dup x y z` duplicates all atoms shifted by (x,y,z).

  - Command `cut x y z a b` cuts to those atoms between a and b
    along the vector (x,y,z).

  - The input line has a history list to retrieve old commands.
    Use the up and down arrows when the input line is active.

  - On some systems, the backspace key does not work to edit the
    input line. Therefore the left-arrow key was defined to have
    the backspace function also.
