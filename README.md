# Ish's Souls to Perks - SMF Settings

**Version 1.0.0** - the build's own version, starting fresh per this project's standing rule
(`CLAUDE.md` rule 6: generated content starts at 1.0.0). This is a clean-room re-implementation,
not a fork, so there is no upstream repository/version to track.

A native SKSE plugin re-implementation of *Ish's Souls to Perks* (Nexus Skyrim Special Edition
mod 1955, by ishmaeltheforsaken - closed-source ESP+BSA, no public repository), with a real SKSE
Menu Framework settings page in place of the original's MCM page and console command.

## Where this came from

The original mod lets the player buy perk points with dragon souls at a "Dragonstone" object
placed at the Guardian Stones - activating it opens a message offering to buy 1, 5 or 10 perk
points at a configurable price (default 10 dragon souls each, changeable through its own MCM
page or the console command `set ishperkcost to X`).

This port was built from the archive itself, not just its readme (`CLAUDE.md` rule 30 - ask the
object, don't infer): the BSA's one compiled script and the ESP's own records were both parsed
directly (see `PORT-NOTES.md` for the full inspection trail and every exact value found -
`ishPerkCost` = 10.0, `ishPerkCost5`/`ishPerkCost10` = 50.0/100.0 as precomputed 5x/10x totals,
the message text, and the native calls the original script used: `ModActorValue("DragonSouls",
...)` and `AddPerkPoints(...)`). **No Papyrus code, ESP records, or BSA assets from the original
are reused** - only the general mechanic, which is a game mechanic and not copyrightable, the
same reasoning already applied to this project's other from-scratch mods (Carry Weight Per
Level, Perk Reallocation).

## What it does

**Buy perk points with dragon souls.** The settings page shows your current dragon soul count
and three buttons - Buy 1 / Buy 5 / Buy 10 - each showing its live total cost and whether you
can currently afford it. A refused purchase (not enough souls, or perk points already at the
engine's 127-point maximum) touches nothing; a successful one deducts the dragon souls and
grants the perk point(s) immediately.

**Price** (the original's one real setting, mirrored 1:1): dragon souls required per perk point,
default 10 - the same default and meaning as the original's own MCM slider and
`set ishperkcost to X` console command, now a real SMF slider that applies live.

**What changed from the original**: the in-world "Dragonstone" object at the Guardian Stones is
not reproduced - adding a new placed object at a specific world location is ESP/world-editing
work outside a script-free native plugin's scope. The purchase is exposed directly as buttons on
the settings page instead, the same design choice this project already made for **Perk
Reallocation - SMF Settings**, which replaced a potion trigger with a settings-page button. See
`PORT-NOTES.md` for the full reasoning.

**Requirement note**: because the purchase button lives only on the SMF page, SKSE Menu
Framework is a hard requirement for this mod's actual functionality, not merely for
configuring it - without SMF installed (or too old a version), there is no way to buy a perk
point at all. `PORT-NOTES.md` covers why this differs from this project's other SMF mods,
several of which keep working headlessly without SMF and only lose their settings UI.

## Settings persistence

`Restore defaults` reads only the DLL's own compiled-in value (10 dragon souls per perk point),
never a file; `Save`/`Reload from INI` both target this mod's own shipped
`IshSoulsToPerks.ini`. A saved change survives to the next game load (`CLAUDE.md` rule 16) - the
INI is rewritten with plain file I/O, not `WritePrivateProfileString`, since Mod Organizer 2's
usvfs does not reliably redirect the latter. Buying perk points never writes to the INI at all -
it only changes the actor value and perk-point count the game itself already tracks, exactly
like the original mod.

## Debugging

Send the debug log for any bug report:
`Documents\My Games\Skyrim Special Edition\SKSE\IshSoulsToPerks.log`

Ships at `uLogLevel=0` (trace), the most comprehensive level, both as the compiled default and
in the shipped INI (`CLAUDE.md` rules 14/31) - a submitted log already carries decision-point
detail (price, current souls, purchase attempted/refused/succeeded and why) without asking the
reporter to change anything first. A DevBench tool, `ishsoulstoperks.status`, is also registered
if DevBench is installed, exposing the same state live.

## Build

CMake + vcpkg, same scaffold as this project's other SMF mods (`AutoDraw-SMF`,
`CarryWeightPerLevel-SMF`): `configure.bat` then `build.bat`, preset SE/AE only
(`-UENABLE_SKYRIM_VR`, so both SE 1.5.97 and AE 1.6.x stay enabled - VR needs a different
build target this project doesn't ship). Requires `VCPKG_ROOT` pointing at a vcpkg checkout;
Visual Studio, and the CMake/Ninja that ship with it, are located automatically by
`find-msvc.bat`.

## Licence

MIT - see `LICENSE`. This is original code with no code or assets reused from the original
closed-source mod, so MIT was chosen as a permissive default consistent with this project's
other originally-authored material.

## Status

**First attempt, not yet tested in game.** Built and reviewed for correctness against the
vendored CommonLibSSE-NG headers and the real values pulled directly from the original archive
(see `PORT-NOTES.md`), but buying a perk point has not been exercised on a running save yet.
Not packaged into `7. current test builds`, not finalized, not tagged - per this task's explicit
scope, that is design call once he has reviewed it.
