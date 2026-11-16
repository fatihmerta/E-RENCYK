# E-RENCYK // Portable Early-2000s Hi-Fi Audio Deck (PSP Homebrew)

**E-RENCYK** is a custom C / `sceGu` hardware-accelerated early-2000s Sony MiniDisc & Walkman Hi-Fi inspired MP3 player built for Sony PSP (PSP-1000 / 2000 / 3000 / PSP Go / PS Vita Adrenaline & PPSSPP).

### [**⬇ Download Ready-to-Play PSP Build (`E-RENCYK-v1.0-PSP.zip`) from Releases**](https://github.com/fatihmerta/E-RENCYK/releases/latest)
> *Note: Use the **Releases** link above to download the compiled `EBOOT.PBP` package ready for your PSP. (GitHub's green "Code -> Download ZIP" button only downloads raw C source code.)*

---

## Key Features

- **Studio-Clean 32-bit Float MP3 Engine (`libmpg123` + `sceAudioSRC`)**:
  - Decodes MP3 files in **32-bit IEEE Floating-Point** at their **native sample rate** (`44.1 kHz`, `48 kHz`, `32 kHz`, etc.) directly into PSP's dedicated hardware Sample Rate Converter (`sceAudioSRCChReserve`) with explicit CPU D-cache writeback and a transparent soft-knee analog limiter — zero sample-dropping aliasing, zero DMA crackles, and zero bass clipping.
  - Defaults to pure **`FLAT STUDIO`** EQ out of the box, plus 5 headroom-compensated DSP presets: `MEGA BASS 1`, `MEGA BASS 2`, `ROCK STAGE`, `WARM JAZZ`, `Y2K CLUB`.
- **Full-Device Hardware Theme Studio (`SELECT` Button)**:
  - Transforms the **entire device** (brushed metal chassis, bezels, optical disc chamber, VU meter dial & needle colors, VFD spectrum colors, bottom control strip, and top hardware emblem icon):
    1. **Walkman Silver 2001** *(MiniDisc emblem)*
    2. **Champagne Gold ES** *(Hi-Fi Speaker emblem)*
    3. **Midnight Cyber Black** *(Studio Headphones emblem)*
    4. **Cobalt Blue Y2K** *(Optical Laser Disc emblem)*
    5. **Crimson Carbon GT** *(Graphic EQ Sliders emblem)*
    6. **Frost Mint Edition** *(Stereo Music Note emblem)*
  - Also includes an independent **LCD Backlight Tint** selector (`Theme Default`, `Ice Cyan EL`, `Phosphor Green`, `Warm Amber`, `Ruby Orange`, `Pure Frost EL`) and automatically saves your preferences to `erencyk.cfg`.
- **3-Tab Music Library, Up-Next Queue & `.m3u` Playlists (`Circle` Button)**:
  - **FILES Tab**: Browse `ms0:/MUSIC`, `ms0:/PSP/MUSIC`, `ef0:/MUSIC`, `./music`, or any folder on your Memory Stick. Add single songs or entire album folders directly to your **Queue (`□`)** or **Saved Playlists (`△`)**.
  - **QUEUE Tab**: Manage your active *Up Next* queue — jump to any queued song, remove songs, clear the queue, or save the entire queue as a persistent `.m3u` playlist.
  - **PLAYLISTS Tab**: Create, browse, play, shuffle, and edit persistent `.m3u` playlists stored in `PSP/GAME/ERENCYK/playlists/`.
- **Album Cover Art (ID3v2 APIC + Folder Cover + Procedural MiniDisc)**:
  - Streams embedded **ID3v2 `APIC`** (`JPEG` / `PNG`) cover art safely in tiny scanline buffers, falls back to folder `cover.jpg/png` / `folder.jpg` / `album.jpg`, or generates a custom Y2K iridescent MiniDisc prism graphic.
- **Clear Two-Row Contextual Control Bar**:
  - Drawn geometric PlayStation button icons (`✕`, `○`, `□`, `△`, `L/R`, `D-PAD`, `START`, `SELECT`) with plain, full English descriptions that dynamically adapt to every screen.

---

## Installation

1. Download **`E-RENCYK-v1.0-PSP.zip`** from [**Releases**](https://github.com/fatihmerta/E-RENCYK/releases/latest).
2. Extract the archive directly to the root of your PSP Memory Stick (or copy the `ERENCYK` folder into `PSP/GAME/`):
   - **PSP-1000 / 2000 / 3000 / Vita Adrenaline:** `ms0:/PSP/GAME/ERENCYK/EBOOT.PBP`
   - **PSP Go:** `ef0:/PSP/GAME/ERENCYK/EBOOT.PBP`
   - **PPSSPP Emulator:** Open `PSP/GAME/ERENCYK/EBOOT.PBP` (or the `.zip` file itself) directly in PPSSPP.
3. Place your MP3 songs/folders inside `ms0:/MUSIC` or `ms0:/PSP/MUSIC`.

---

## Building from Source

Requires [PSPDEV](https://pspdev.github.io/) toolchain:

```bash
./build.sh
```
