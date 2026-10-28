# NFS Hot Pursuit Remastered — Horn Restoration

A mod for **Need for Speed: Hot Pursuit Remastered** that restores player-controlled horn functionality using the game's existing traffic horn audio logic.

The mod supports hold-to-honk input, configurable keyboard, controller bindings, and synchronization of horn input with other modded multiplayer clients.

## Features

- Hold-to-honk control for the player's vehicle
- Keyboard, Xbox (XInput), and DS4/DS5 controller bindings
- Configurable player and competitor horn volume
- Multiplayer horn-state synchronization (each participant must have the mod to hear horns)
- Optional private audio-resource copy to help avoid interrupted playback when vehicles change
- **Experimental:** custom SPS horn audio files (generic NFSMW 2012 horn file was included for testing)

## Installation

Drop dinput8.dll and the scripts folder in your main directory (\Need For Speed Hot Pursuit Remastered), and configure any custom keybinds/experimental settings if needed in the `hpr_horn.ini`config file.


> **Compatibility:** The source uses hard-coded executable offsets and checks the expected instruction bytes before installing hooks. It does NOT work with other versions or the original 2010 game.

## Configuration

Edit `hpr_horn.ini` next to the `.asi` file before starting the game. The included configuration uses:

| Setting | Included value | Purpose |
| --- | --- | --- |
| `Keyboard Horn Hotkey` | `VK_H` | Keyboard binding |
| `Xbox Horn Bind` | `Y` | Xbox controller binding |
| `DS4/DS5 Horn Bind` | `Circle` | PlayStation controller binding |
| `Player Horn Gain Level` | `30.0` | Local horn volume multiplier |
| `Competitor Horn Gain Level` | `1.5` | Remote horn volume multiplier |
| `Private Horn Copy` | `On` | Keep a private copy of the game's horn sound resource |
| `Custom Horn Sound` | `True` | Enable experimental custom SPS playback |
| `Custom Horn File` | `70_1C_1D_00.dat` | Custom SPS file path, relative to the `.asi` folder |

**Important:** The referenced `70_1C_1D_00.dat` file is **included** in this repository for testing purposes, taken from Most Wanted 2012. To use the game's original horn audio without providing that file, set `Custom Horn Sound = Off`. When a custom sound cannot be loaded, the source falls back to the available game horn resource.

Bindings may be set to `None` to disable them. The configuration contains available key names / button names / and raw codes. For multiplayer custom audio, each player needs the same custom file and settings for matching playback.

## Building from source

Open `hpr_horn.sln` in **Visual Studio 2022** with the Desktop development with C++ workload, x64 MSVC v143 toolset, Windows SDK, and MASM build support.

- Set the configuration to `Release | x64`.
- Run **Build > Build Solution**.
- Find the output at `bin/Release/hpr_horn.asi`. The build copies `hpr_horn.ini` alongside it.

See [BUILDING.md](BUILDING.md) for the complete steps and project notes.

## Source layout

```text
hpr_horn.sln                Visual Studio 2022 solution
hpr_horn.vcxproj            Native MSVC x64 DLL/ASI project
hpr_horn.vcxproj.filters    Visual Studio Solution Explorer filters
hpr_horn.ini               Configuration template / example
src/
  Main.cpp                 ASI entry point and initialization
  Config.*                 INI parsing and settings
  Input.*                  Keyboard and controller input
  Game.*                   Game-specific addresses and sound interfaces
  Hooks.*                  Executable-hook installation
  HornPlayer.*             Local and remote horn playback
  HornSample.*             Horn sample/resource handling
  Str.*                    Internal string parsing utilities
  Runtime.cpp              Internal runtime helpers
  Stubs.asm                x64 MASM hook stubs
  Types.h, Win32.h, Shared.h
```

## Troubleshooting

- **Your custom horn doesn't play:** Check the path in `Custom Horn File`, or disable custom audio to fall back to the built-in horn.
- **Horn doesn't work on a specific car:** Heavily modified _EN vehicle files may stall the hook and cause slight frame rate issues, as the vehicle horn may be allocated into a different memory region due to changes/additions to a vehicles EN file. This will be patched in future updates. 
- **Other players cannot hear your horn:** Every player who should hear horns must install the mod. Custom horn playback additionally needs matching audio files/settings on those clients.
- **Build errors:** Confirm you selected x64 and have the Visual Studio C++/MSVC and MASM components installed. See [BUILDING.md](BUILDING.md).

## Misc

This mod was made to be compatible with other ASI patches, such as AkaSokuro's 'NFSHPR-Patches-Pack'. Both mods patch the same memory region, which caused conflicting issues prior. To resolve this, Horn Restoration includes a compatibility fallback that detects the existing patch and skips its own overlapping modification, allowing both mods to function alongside each other without interference.

The mod originally was written in Cheat Engines' assembly for proof of concept/functionality. Reverse engineering, memory tracing, ASM hooks, and the horn restoration method was done manually by me. 

**AI Usage Disclosure:** Claude AI was used to assist in writing the C++ portion of this mod along with providing a few lua engine debugging scripts to trace horn network synchronization across clients.
