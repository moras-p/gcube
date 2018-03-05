GCUBE
=====

Open source Nintendo Gamecube emulator
-----------------------------

![Alt text](/screenshots/ks.jpg?raw=true "Optional Title")

gcube is a gamecube emulator for linux, windows and mac os x.
Compatibility is still very low so don't expect much. It is not even a beta-quality product.
It hasn't been thoroughly tested and may contain bugs. Patches and suggestions are always welcome.

Software requirements
----------------------
* [SDL library 1.2.7 or newer](www.libsdl.org) (won't work with older versions)
* [zlib](www.zlib.org)
* [NVIDIA Cg toolkit](www.developer.nvidia.com/cg-toolkit)
* GNU C compiler
* Visual Studio project files are included (compiles with mingw on Windows)


Hardware requirements
----------------------
* Something good. I'm developing it on Athlon XP 2.0 with 0.5 GB ram and
  GeForce 2 GTS, and that is not enough for a full-speed game of pong;)
* Video card must support the following extensions:
  - GL_EXT_texture_rectangle or GL_NV_texture_rectangle
  - GL_IBM_texture_mirrored_repeat
	- GL_ARB_imaging
	- GL_EXT_texture_lod_bias
	- GL_EXT_texture_filter_anisotropic


Building gcube
---------------
* Before compiling check Makefile for options.
* To compile just type 'make' or 'make release' (optimized) (on mac use make -f Makefile.mac).
  - on Windows:
    -  install mingw with msys and libz-dev
    -  add mingw32/bin and mingw32/msys/1.0/bin to path
    -  download and manually install SDL-devel-1.2.15-mingw32.tar.gz and nvidia-cg-toolkit
    -  from command line type make -f Makefile.win, or just use Visual Studio project files
* To install, copy gcube to /usr/local/bin.


Usage tips
-----------
* If You have binfmt_misc support compiled into kernel,
  copy gcube to /usr/local/bin and put this in some
  startup script (eg rc.local):
    echo ":DOL:E::dol::/usr/local/bin/gcube:" >/proc/sys/fs/binfmt_misc/register
    echo ":GCM:E::gcm::/usr/local/bin/gcube:" >/proc/sys/fs/binfmt_misc/register
    echo ":IMP:M::\x7fIMP::/usr/local/bin/gcube:" >/proc/sys/fs/binfmt_misc/register
  You will then be able to run gc-binary files / mini-dvd images
  just like any other executables.
* The easiest way to use it on windows is to make a shortcut to gcube
  on the desktop and then just drop Your gc-binary files on it. You can
  also associate gcube with dol / gcm files.


A few notes
----------------------------------------
* In order to work properly, savestates must be placed in the same directory
  as the game they were created with.
* Some games will need specific settings to run. Run gcube --help to
  see the list of parameters. If the game doesn't work, try starting it
  with hle enabled (-ls) or bigger refresh delay (-r value, eg. 900000).
* GCM's can be compressed with the supplied utility isopack. Compression
  uses a very simple method but it is quite effective with some dvd images.
  For example, size of 'The Legend Of Zelda - Four Swords' decreases from
  1.4G to 255M. Other files might not compress that well (if at all).
* In order to run most of the SDK demos, a directory containing files needed
  by the demo will have to be mounted as the root directory of dvd.
  If a directory named 'dvdroot' is present in the same path as the 'elf' file
  it will be mounted automagically. Otherwise, use the --mount option to
  specify the needed dvdroot.
* Only interpreter is implemented at the moment. That means it is slow
  and You might have to wait a long time before something happens
  (eg action replay and xrick).
* Debugger is integrated with emulator, so it will always pop up
  whenever a fatal error occurs. Pressing F11 (or using 'x' command)
  will run the program ignoring the error (if further execution is
  at all possible). F12 will quit emulator.
* Configuration file is kept in users home directory
  ("/home/username/.gcube-0.4" on Linux and
	 "/documents and settings/username/.gcube-0.4" on windows).
* Default keys are
  - arrows                      - digital pad
  - keypad 8/5/4/6              - first analog
  - home/end/delete/page down   - second analog
  - q/w/a/s/z/x/c               - A/B/L/R/X/Y/Z
  - enter                       - start

  - F1 will disable/enable texture caching (slow workaround until correct texture
     caching will be implemented).
  - F2 will switch wireframe mode on / off.
  - F3 switches between old and new rendering engine. The new one is at an early
     stage. Some games will look better with it, and others will look worse.
  - F4 forces linear filtering on all textures.
  - F5 makes all textures transparent. use if one texture occludes the view.
  - F7 fixes dark textures in custom robo.
  - F8 enables/disables opengl generated mipmaps. By default, gcube will use
     mipmaps supplied by emulated program.
  - F9 will create a screenshot.
  - F11 creates a savestate. Savestates must reside in the same directory
     as the game they were created with.
  - F12 loads a savestate.


Credits...
----------
Big thanks goes to:
* Dolwin authors, org and hotquick. If they wouldn't release
  sources of their emulator, gcube wouldn't exist.
* gropeaz / hitmen for Yet Another Gamecube Documentation,
  also to everyone who contributed to it in any way.
* Frank Willie (phx) for PowerPC disassembler.
* Every GC homebrew developer releasing sources.
* Metalmurphy for gcube homepage and EXEmu.net staff for hosting it.
* Adam Green for helping me out with the Mac OS X port.
* Dolphin authors, F|RES, ector and schibo, for the Dolphin sources. It helped
  me improve the emu, as well as kill some ugly bugs.
* GCEmu authors, Duddie and Tratax, for releasing sources of their emu, what
  helped in improving compatibility. Also big thanks to Duddie for his work
	on Gamecube's DSP processor.
* Shinji Chiba for Gamecube's ADPCM Decoder.

* Great icons by rodimus:
  www.rodimusconvoy.com

Authors
-------

* **Monk** - *Initial work* - [dev.monk](https://gitlab.com/dev.monk/gcube)
