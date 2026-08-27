# Plugin notes - `IshSoulsToPerks.esl`

The placement carrier for **Ish's Souls to Perks - SMF Settings**. It holds the Dragonstone
activator and its placed reference at the Guardian Stones, and **nothing else**. All behaviour
stays in the DLL; the plugin hosts no script.

This file records the extraction so a future session can rebuild the plugin **without redoing
the archaeology**. Everything below was read out of real files, not inferred.

---

## 1. Where the data came from

| Source | Path | What it gave |
|---|---|---|
| Original mod ESP | `C:\Modlists\Apostasy\mods\[NoDelete] 0132 Ish's Souls to Perks\Ish's Souls to Perks.esp` | The ACTI record and the exact placement |
| Vanilla master | `C:\Modlists\Apostasy\Stock Game\Data\Update.esm` | Authoritative WRLD / CELL parent data |
| Vanilla master | `C:\Modlists\Apostasy\Stock Game\Data\Skyrim.esm` | Cross-check of the same two parents |

The ESP was decoded with a purpose-written TES5 record walker (group tree + subrecord walk +
zlib decompression), not with xEdit. **xEdit's `-script:` mode is not automatable** - it opens a
modal dialog and never exits - so it was not used at any point.

---

## 2. The original's ACTI record

`ACTI 0x020022F2` — EditorID `ishsoultoperktriggerstone`

| Subrecord | Value |
|---|---|
| `FULL` | `Dragonstone` |
| `MODL` | `Clutter\Ruins\DragonStone\RuinsDragonStone01.nif` |
| `MODT` | `020000000000000000000000` (version 2, zero texture hashes - an empty placeholder) |
| `OBND` | `-66, -57, 0` .. `66, 57, 155` |
| `PNAM` | `cc4c3300` (activator marker colour) |
| `FNAM` | `0000` (no flags) |
| `VMAD` | script `ishsoultoperktrigger` — **deliberately NOT reproduced** |

The model path was confirmed to be a real vanilla asset: `RuinsDragonStone01.nif` is present in
`Skyrim - Meshes0.bsa`, so the plugin needs no meshes of its own.

The original's second activator, `ishSoulsToPerksTriggerNOTUSED` (`0x020012C8`), is unused by its
own EditorID and was ignored.

---

## 3. The exact placement (the part that matters)

`REFR 0x020022F3` — places `0x020022F2`

| Property | Value |
|---|---|
| Worldspace | `Tamriel` (`WRLD 0x0000003C`) |
| Cell | `GuardianStones` (`CELL 0x00009B91`) — **exterior** |
| Exterior grid | **X = 0, Y = -15** |
| Exterior block / sub-block | block `(Y=-1, X=0)`, sub-block `(Y=-2, X=0)` |
| Position | **X = 2428.6306, Y = -59305.6328, Z = 1432.5502** |
| Rotation (radians) | **X = 0, Y = 0, Z = 1.3832** |
| Scale | **0.7** (`XSCL`) |
| Persistence | **TEMPORARY** — record flag `0x400` clear, and it lives in **cell children group type 9** |

Raw bytes, for byte-exact rebuilds:

```
DATA  17ca1745 a2a967c7 9b11b344 00000000 00000000 b00cb13f
XSCL  3333333f
```

**Persistence changes where the REFR must live in the group tree.** A persistent reference goes
in group **type 8**; a temporary one goes in group **type 9**. The original is temporary, so ours
is too. (For contrast, the vanilla Guardian Stones *map marker* `0x0001BABD` in the same
worldspace IS persistent and sits in a type 8 group under the Tamriel persistent cell `0x00000D74`
— a useful sanity anchor, but not what we are copying.)

Getting this wrong is not cosmetic: a temporary ref written into a type 8 group, or vice versa,
is a malformed plugin.

---

## 4. What our plugin actually contains

```
TES4                       flags 0x00000201 (ESM | Light), form version 44, master: Skyrim.esm
GRUP ACTI
  ACTI 0x01000800          ISTP_DragonstoneActivator
GRUP WRLD
  WRLD 0x0000003C          Tamriel        - parent override (structurally mandatory)
  GRUP type 1  (0x3C)
    GRUP type 4  (Y=-1, X=0)
      GRUP type 5  (Y=-2, X=0)
        CELL 0x00009B91    GuardianStones - parent override (structurally mandatory)
        GRUP type 6  (0x9B91)
          GRUP type 9  (0x9B91)           - TEMPORARY children
            REFR 0x01000801  ISTP_DragonstoneRef
```

4 records + 7 groups = **11**, which is what `HEDR`'s count field holds (it counts records *and*
groups, excluding TES4 itself — verified against the original ESP, whose 12 records + 12 groups
match its header count of 24). `nextObjectID` = `0x802`.

### The FormID contract

| On disk | At runtime | Type | EditorID |
|---|---|---|---|
| `0x01000800` | `0xFE0xx800` | `ACTI` | `ISTP_DragonstoneActivator` |
| `0x01000801` | `0xFE0xx801` | `REFR` | `ISTP_DragonstoneRef` |

The DLL resolves these as `LookupForm(0x800, "IshSoulsToPerks.esl")`. **These must never drift.**
Light-plugin local FormIDs must sit in `0x800`–`0xFFF`, which is why those two were chosen.

On disk the high byte is the *master index*, and index 1 means "this file" (there is 1 master).
That is why the records read `0x01000800`, not `0xFE000800`. Adding a second master would shift
them to `0x02000800` — harmless to the DLL, which resolves by plugin name, but it is why the
master list is worth keeping stable.

### Why the CELL and WRLD parents are in the file at all

A placed reference cannot exist without its parent cell, and the cell cannot exist without its
worldspace. Both are **overrides of vanilla records**, not new content. They are structurally
mandatory and must not be "cleaned" out of the plugin.

Because the engine replaces a record **wholesale** when it overrides one — it does not merge
subrecords — these parents have to carry the real vanilla field values. A stripped-down CELL
override would silently wipe the cell's lighting template, water height, regions and location.

---

## 5. Two deliberate deviations, and how to reverse them

### 5a. The WRLD parent is the 312-byte form (no `RNAM` / `MHDT` / `OFST`)

`Skyrim.esm`'s Tamriel record is 1.4 MB (8,455 `RNAM` large-reference entries plus `OFST`);
`Update.esm`'s is 52 KB. Ours carries the 16 standard fields only (`EDID FULL CNAM NAM2 NAM3
NAM4 DNAM MNAM ONAM NAMA DATA NAM0 NAM9 ZNAM TNAM UNAM`) = 312 bytes.

This is not a guess. A survey of the Apostasy load order found that **every** plugin overriding
Tamriel ships one of exactly two shapes: the 312-byte form (xEdit's output — e.g. `DynDOLOD.esp`,
`APO_SynBugFixes.esp`) or that same set plus `MHDT` at 45,926 bytes (the Creation Kit's output).
**Not one carries `RNAM`.** The original Ish ESP shipped the 312-byte form too, and its field
values are byte-identical to what `Update.esm` holds — two independent sources agreeing.

`FULL` is written as the literal string `Skyrim`, **not** copied from `Update.esm`, because
`Update.esm` is a localised plugin where `FULL` is a 4-byte string-table ID. Copying that ID into
our non-localised plugin would render as garbage.

### 5b. The CELL parent drops `XCWT`

`Update.esm`'s version of `CELL 0x00009B91` carries `XCWT = 0x01001232` — a **water type record
owned by `Update.esm`**. Carrying it would force `Update.esm` in as a second master, so it is
dropped. Everything else in the cell (`EDID DATA XCLC TVDT MHDT LTMP XCLW XCLR XLCN`) is copied
from `Update.esm` verbatim.

**The consequence:** if this plugin is the last thing to touch that cell, the Guardian Stones cell
falls back to the worldspace default water instead of Update's assigned water type. In practice
this is very unlikely to be visible — a `.esl` carries the ESM flag and therefore loads in the
**master block, ahead of every `.esp`**, so USSEP and the landscape mods in the load order
override this cell afterwards anyway. The original mod dropped `XCWT` too, despite mastering
`Update.esm`.

**To eliminate the delta entirely** (if it ever proves to matter): add `MAST Update.esm` +
`DATA` to the TES4 header, stop skipping `XCWT` in the builder, and note that the new records
shift to `0x02000800` / `0x02000801` on disk.

---

## 6. Rebuild and re-verify

```
python "D:\Claude output\.MD\scripts\Build-IshSoulsToPerksESL.py"  <out.esl>  [Update.esm]
python "D:\Claude output\.MD\scripts\Verify-IshSoulsToPerksESL.py" <out.esl>  [original.esp]
```

The builder re-reads the parent records from the live `Update.esm` each run rather than embedding
blobs, so the overrides always carry authoritative vanilla data. The verifier deliberately shares
no code with the builder: it re-walks the finished file from scratch and diffs the placement
against the original ESP, so a builder bug cannot hide itself.

### Verification performed on the shipped file (all passed)

- ESL flag `0x200` **and** ESM flag `0x1` set; not flagged localised
- Master list is exactly `[Skyrim.esm]`
- `HEDR` count (11) matches the actual 4 records + 7 groups; `nextObjectID` (`0x802`) is highest
  local ID + 1
- Both new records present, correct type, correct EditorID, local IDs inside `0x800`–`0xFFF`,
  written at this file's own master index
- Exactly two new records; no FormID anywhere points past the master list
- Every record carries SSE form version 44
- `Get-PluginRecordTypes.ps1` reports exactly `ACTI, CELL, REFR, WRLD` — nothing unexpected
- REFR is in group type 9 (temporary) with the persistent flag clear, under the correctly
  labelled block/sub-block/cell groups; cell grid decodes to X=0 Y=-15
- **The position derives back to the cell it is filed under**: exterior cells are 4096 units
  square, and `floor(2428.63 / 4096), floor(-59305.63 / 4096)` = `(0, -15)`, matching the cell's
  own `XCLC` grid. A reference filed under the wrong cell or block is silent breakage, so this is
  derived rather than assumed.
- The placement is **388.9 units from the vanilla Guardian Stones map marker** (`0x0001BABD`, at
  `2806.98, -59215.8, 1436.49`) at essentially the same height — independent confirmation that
  these coordinates really are at the Guardian Stones
- CELL override retains location, water height, regions and lighting template
- **Round-trip**: `DATA` (position + rotation) and `XSCL` are byte-identical to the original ESP,
  as are the ACTI's `FULL`, `MODL`, `OBND`, `MODT`, `PNAM`, `FNAM`
- ACTI carries no `VMAD`; no `QUST` / `MESG` / `GLOB` / script records were copied

### Independent validation with Mutagen (Spriggit)

The checks above all use decoders written for this job, so the plugin was additionally parsed by
**Mutagen**, a completely independent third-party implementation, via the Spriggit CLI — run with
`Throwing on unknown records: True`, so any structure Mutagen did not recognise would have been a
hard error rather than a silent skip. It parsed cleanly and round-tripped every value:

```
Spriggit.CLI.exe serialize -i <plugin.esl> -o <folder> -g SkyrimSE
                           -p Spriggit.Yaml.Skyrim -v 0.41.0 -c
```

`-c` / `--Check` is worth always passing: it deserializes the YAML back into a plugin and compares,
so it verifies a full **binary round-trip** rather than merely that the file could be read. It
returned **exit 0** on this plugin.

`--PackageVersion` (`-v`) is mandatory — omitting it errors with *"PackageVersion needs to be set
if GameRelease or PackageName are."* The CLI itself is at
`D:\Claude output\2. Mod Types\Framework or Utility\_tools\Spriggit\Spriggit.CLI.exe`.

**A correction worth recording, because the error message lies.** A first attempt at this
validation failed with *"Settings file 'DotnetToolSettings.xml' was not found in the package"*,
and was initially written up here as "translation package 0.41.0 is broken, use 0.40.1".
**That was the wrong root cause.** The file *is* present in the nupkg, at `tools/net10.0/any/`;
the real problem was that this machine had only .NET SDK 8 and 9, so `dotnet tool install` could
not resolve a supported target framework. Installing the .NET 10 SDK
(`winget install --id Microsoft.DotNet.SDK.10 -e`) fixed it, and 0.41.0 then worked. 0.40.1
"worked" only because it targets an older TFM — pinning to it would have quietly held this project
a version behind indefinitely. If this error reappears on another machine, **install the newer SDK
rather than downgrading the package.**

What Mutagen independently reported:

- Mod header flags **`Master` + `Small`** — i.e. it agrees the file is ESM-flagged and light
- `MasterReferences:` a single entry, `Skyrim.esm`
- Activator `000800:IshSoulsToPerks.esl`, EditorID `ISTP_DragonstoneActivator`, Name
  `Dragonstone`, model `Clutter\Ruins\DragonStone\RuinsDragonStone01.nif`
- Cell resolved as **`009B91:Skyrim.esm`** — correctly understood as an override of a master's
  record rather than new content — with `Grid Point: 0, -15`, `WaterHeight: 600`, and its Regions
  and Location resolving into `Skyrim.esm`
- Worldspace resolved as `Tamriel - 00003C_Skyrim.esm`, filed under block `0, -1` / sub-block `0, -2`
- The reference listed under the cell's **`Temporary:`** collection (not `Persistent:`), as
  `PlacedObject 000801:IshSoulsToPerks.esl`, EditorID `ISTP_DragonstoneRef`,
  `Base: 000800:IshSoulsToPerks.esl`, `Scale: 0.7`,
  `Position: 2428.6306, -59305.633, 1432.5502`, `Rotation: 0, 0, 1.3831997`

### Not verified

The file was **not** opened in the SSEEdit GUI. `D:\Modlists\SME\tools\SSEEdit\` has SSEEdit
4.1.5.0 available if a human wants that extra pass; it needs a person to click through the module
selection dialog, which an agent session cannot drive — and xEdit's `-script:` mode is not an
escape hatch, since it too shows a modal dialog and never exits. The Mutagen parse above is the
substitute: an independent implementation, not the one that wrote the file.

Nothing here confirms in-game behaviour. The plugin has not been loaded by Skyrim, and the
Dragonstone has not been seen standing at the Guardian Stones. That is a test-run observation,
not something static verification can supply.

---

## 7. Consequences to keep honouring

- **The plugin is a hard requirement** and ships with the mod. A category-6 world reference is
  never separated from its plugin.
- **The reference bakes into saves.** Once placed, its FormID persists in any save made with the
  plugin active — so the plugin must never be casually renamed or removed, and the two FormIDs
  above must not drift.
- **The SMF page buttons stay.** The Dragonstone is an additional trigger, not a replacement;
  losing SMF must not lose the mechanic.
