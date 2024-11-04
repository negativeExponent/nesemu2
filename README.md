nesemu2
=======
  * Cycle accurate cross platform Nintendo NES emulator for Win32/Linux/OSX
  * Copyright 2013 James Holodnak
  * Visit the official website at http://www.nesemu2.com/

Building with make
------------------
A few compile time options are available, they are listed below.

 * CPU_UNDOC        - Enable the undocumented opcodes.
 * QUICK_SPRITES    - Use the fast sprite code.

All of these are currently enabled by default.

The build target is automatically determined by the makefile.  Linux/OSX must use
the SDL library.  For Win32 just specify USESDL=0 on the command line to build the
Win32 API version.  Here are some example command lines for building:

To build using the default options and autodetect your OS.

    make

To build with SDL2 (experimental and still needs SDL1 libraries for input and sounds, linux only)

    make USESDL=2
      
For building the Win32 API version.

    make USESDL=0

Enabling the undocumented opcodes.

    make CPU_UNDOC=1

Forcing to build on a specific OS.

    make OSTARGET=OSX

Building on forced Win32, undocument opcodes and fast sprites enabled.

    make OSTARGET=WIN32 CPU_UNDOC=1 QUICK_SPRITES=1

Basic Usage
-----------
Just run it like this:

    ./nesemu2 <ines/ines 2.0/unif/fds file>
   
The Win32 API target operates like a standard Win32 program.  Use File -> Open.

Configuration Files
-------------------
The program will search for configuration files to use.  It looks for it like this:

  1.  Check the current working directory for nesemu2.cfg
  2.  If the HOME environment variable is set, it tries ~/.nesemu/nesemu2.cfg
  3.  (Win32 only) Check the directory the .exe is stored in.
  
If it is not found, it creates it in:

  * Win32:  The same directory as the executable.
  * Linux/OSX:  The users home directory (~/.nesemu/nesemu2.cfg)

If you do not like any of these locations, you can use --config <filename> as a
command line parameter.

Subtrees in the Code
--------------------
To update the subtrees use the following commands:

    git fetch slre master
    git subtree pull --prefix source/misc/slre slre master --squash

These will update slre to the latest version.

Credits
-------
Thanks goes out to the following groups/individuals:

  * the #nesdev irc channel.
  * the nesdev wiki and message boards.
  * Martin Freij for Nestopia's great mapper reference.
  * Quietust for Nintendulator, sound code was based upon his.
  * Quietust for Nintendulator, zapper code based upon his.
  * SDLMAME project, sdl sound code is ripped directly from it.
  * SLRE at https://code.google.com/p/slre
  * r0ni for giving me access to an OSX machine.
