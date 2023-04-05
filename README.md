# PoolMonX v3

Monitor Kernel pool allocations tags

## Highlights in this new release:

* Quick find (filter) (Ctrl+Q)
* If WinDbg is installed, the updated `pooltag.txt` file is used for source and description
* Nicer icons :)
* Hit SPACE to pause/resume
* Dark mode (coming soon)

## Building

I have started to move away from Nuget towards [vcpckg](https://github.com/microsoft/vcpkg). Install *Vcpkg* and then install the following packages:
* `wil:x64-windows`
* `wtl:x64-windows`
* `detours:x64-windows`

Then open the solution file with Visual Studio 2022 and build.

## Screenshot

![](https://github.com/zodiacon/PoolMonXv3/blob/master/poolmonxv3.png)
