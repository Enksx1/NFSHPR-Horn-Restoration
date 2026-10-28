# Building HPR Horn Restoration

This version is a **native Visual Studio 2022 C++ project**. It uses the Microsoft C++ compiler and the Microsoft Macro Assembler (`ml64.exe`) for its x64 hook stubs; **LLVM/Clang and the older `kernel32.def` workflow are not required**.

## Requirements

- Visual Studio 2022 (Community edition is sufficient)
- **Desktop development with C++** workload
- MSVC **v143 x64/x86 build tools**
- Windows 10/11 SDK
- x64 MASM build support (provided with Visual Studio C++ build tools)

## Build

1. Open `hpr_horn.sln` in Visual Studio.
2. In the solution configuration selector, choose **Release** and **x64**.
3. Choose **Build > Build Solution** (`Ctrl+Shift+B`).
4. The output is created in `bin\Release\`:

   - `hpr_horn.asi` — the compiled x64 plugin
   - `hpr_horn.ini` — the copied configuration file
   - `hpr_horn.pdb` — debug symbols, if generated (not required by players)

For debugging, select `Debug | x64`; output will be under `bin\Debug\`.

The `.asi` extension is configured through `<TargetExt>.asi</TargetExt>` in the `.vcxproj` and is built as a Windows DLL. The project explicitly uses `DllMain` as its entry point and omits default runtime libraries, relying on the source's own runtime helpers.

## Project organization

- `.cpp` and `.h` source files are under `src/`.
- `src/Stubs.asm` contains the architecture-specific hooks assembled by MASM.
- `hpr_horn.vcxproj.filters` organizes files in Solution Explorer.
- `hpr_horn.ini` is copied into the build output directory.
- `.vs/`, `obj/`, `bin/`, and `*.vcxproj.user` are local/generated files and are ignored by Git.

## Before publishing a release

Use **Release | x64**. Package `bin\Release\hpr_horn.asi` together with `bin\Release\hpr_horn.ini`. Avoid distributing `.pdb` files unless you want to provide debug symbols.

**Custom sound notice:** The current INI specifies `Custom Horn Sound = True` and `Custom Horn File = 70_1C_1D_00.dat`. That audio asset is included in the uploaded project for testing purposes. To disable the experimental feature, set the INI configuration to `Custom Horn Sound = Off`.

## Validation status

The supplied project archive contains a compiled Release `.asi` and its Visual Studio Release build log showing successful link output. This package has been checked for project layout and referenced files, but **has not been compiled on this environment or tested in the game**.
