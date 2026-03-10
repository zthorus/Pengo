# Pengo
Pengo game for Ubuntu

A simple version of the classic Pengo arcade game for Unix/Linux computers. Requires to have X11 installed.
The executable for Ubuntu 24.04 (Intel CPU) is provided. It can be generated from the provided source code
by typing from the shell:

g++ pengo.c -o pengo -lm -lX11

Use the arrow keys to move Pengo, to push/crash the cubes and to activate the electric border (to shock the Snobees).
Each level is completed when the three diamond cubes are grouped and in line.
Use ESC to quit the game.
This game has no sound features (sorry).
