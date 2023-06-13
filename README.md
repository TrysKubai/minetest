Three Cubes Minetest
=====

This library is a fork of the open-source voxel game engine [Minetest](https://minetest.net).

Forked from the [master branch](https://github.com/minetest/minetest) on 2023-06-09 (development build of version 5.8.0 [commit](https://github.com/minetest/minetest/commit/553dc02deb8c16795874469d14643f89e1cba559)) with intent on use/modification/redistribution.

In complience with LGPL2.1 requirements this repository is provided for the purpose of source code accessibility.

Three Cubes does not claim authorship of the original source code, only the modified code.

# Minetest

<!-- ![Build Status](https://github.com/minetest/minetest/workflows/build/badge.svg) -->
[![Translation status](https://hosted.weblate.org/widgets/minetest/-/svg-badge.svg)](https://hosted.weblate.org/engage/minetest/?utm_source=widget)
[![License](https://img.shields.io/badge/license-LGPLv2.1%2B-blue.svg)](https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html)

Minetest is a free open-source voxel game engine with easy modding and game creation.

Copyright (C) 2010-2022 Perttu Ahola <celeron55@gmail.com>
and contributors (see source file comments and the version control log)

In case you downloaded the source code
--------------------------------------
If you downloaded the Minetest Engine source code in which this file is
contained, you probably want to download the [Minetest Game](https://github.com/minetest/minetest_game/)
project too. See its README.txt for more information.

Table of Contents
------------------

- [Three Cubes Minetest](#three-cubes-minetest)
- [Minetest](#minetest)
  - [In case you downloaded the source code](#in-case-you-downloaded-the-source-code)
  - [Table of Contents](#table-of-contents)
  - [Further documentation](#further-documentation)
  - [Default controls](#default-controls)
  - [Paths](#paths)
  - [Configuration file](#configuration-file)
  - [Command-line options](#command-line-options)
  - [Compiling](#compiling)


Further documentation
----------------------
- Website: https://minetest.net/
- Wiki: https://wiki.minetest.net/
- Developer wiki: https://dev.minetest.net/
- Forum: https://forum.minetest.net/
- GitHub: https://github.com/minetest/minetest/
- [doc/](doc/) directory of source distribution

Default controls
----------------
All controls are re-bindable using settings.
Some can be changed in the key config dialog in the settings tab.

| Button                        | Action                                                         |
|-------------------------------|----------------------------------------------------------------|
| Move mouse                    | Look around                                                    |
| W, A, S, D                    | Move                                                           |
| Space                         | Jump/move up                                                   |
| Shift                         | Sneak/move down                                                |
| Q                             | Drop itemstack                                                 |
| Shift + Q                     | Drop single item                                               |
| Left mouse button             | Dig/punch/use                                                  |
| Right mouse button            | Place/use                                                      |
| Shift + right mouse button    | Build (without using)                                          |
| I                             | Inventory menu                                                 |
| Mouse wheel                   | Select item                                                    |
| 0-9                           | Select item                                                    |
| Z                             | Zoom (needs zoom privilege)                                    |
| T                             | Chat                                                           |
| /                             | Command                                                        |
| Esc                           | Pause menu/abort/exit (pauses only singleplayer game)          |
| +                             | Increase view range                                            |
| -                             | Decrease view range                                            |
| K                             | Enable/disable fly mode (needs fly privilege)                  |
| J                             | Enable/disable fast mode (needs fast privilege)                |
| H                             | Enable/disable noclip mode (needs noclip privilege)            |
| E                             | Aux1 (Move fast in fast mode. Games may add special features)  |
| C                             | Cycle through camera modes                                     |
| V                             | Cycle through minimap modes                                    |
| Shift + V                     | Change minimap orientation                                     |
| F1                            | Hide/show HUD                                                  |
| F2                            | Hide/show chat                                                 |
| F3                            | Disable/enable fog                                             |
| F4                            | Disable/enable camera update (Mapblocks are not updated anymore when disabled, disabled in release builds)  |
| F5                            | Cycle through debug information screens                        |
| F6                            | Cycle through profiler info screens                            |
| F10                           | Show/hide console                                              |
| F12                           | Take screenshot                                                |

Paths
-----
Locations:

* `bin`   - Compiled binaries
* `share` - Distributed read-only data
* `user`  - User-created modifiable data

Where each location is on each platform:

* Windows .zip / RUN_IN_PLACE source:
    * `bin`   = `bin`
    * `share` = `.`
    * `user`  = `.`
* Windows installed:
    * `bin`   = `C:\Program Files\Minetest\bin (Depends on the install location)`
    * `share` = `C:\Program Files\Minetest (Depends on the install location)`
    * `user`  = `%APPDATA%\Minetest` or `%MINETEST_USER_PATH%`
* Linux installed:
    * `bin`   = `/usr/bin`
    * `share` = `/usr/share/minetest`
    * `user`  = `~/.minetest` or `$MINETEST_USER_PATH`
* macOS:
    * `bin`   = `Contents/MacOS`
    * `share` = `Contents/Resources`
    * `user`  = `Contents/User` or `~/Library/Application Support/minetest` or `$MINETEST_USER_PATH`

Worlds can be found as separate folders in: `user/worlds/`

Configuration file
------------------
- Default location:
    `user/minetest.conf`
- This file is created by closing Minetest for the first time.
- A specific file can be specified on the command line:
    `--config <path-to-file>`
- A run-in-place build will look for the configuration file in
    `location_of_exe/../minetest.conf` and also `location_of_exe/../../minetest.conf`

Command-line options
--------------------
- Use `--help`

Compiling
---------

- [Compiling on GNU/Linux](doc/compiling/linux.md)
- [Compiling on Windows](doc/compiling/windows.md)
- [Compiling on MacOS](doc/compiling/macos.md)
