# SV Event Raid Injector

Inject Tera Raid event data into Pokemon Scarlet & Violet save files directly on Nintendo Switch.

## Disclaimer

This software is provided "as-is" without any warranty.\
While the app has been tested, it may contain bugs that could corrupt or damage your save files.

**Use at your own risk.**

The author is not responsible for any data loss or damage to your save data.

This is why the automatic backup system exists — always verify your backups before making changes.

If you need to restore a backup, use a save manager such as [Checkpoint](https://github.com/FlagBrew/Checkpoint) or [JKSV](https://github.com/J-D-K/JKSV) to import the backup files back onto your Switch.

## Compatibility

- Scarlet / Violet version **4.0.0 only** !

## Features

- **Profile & Game Selection** — Pick any Switch user profile and choose between Scarlet or Violet
- **Event Injection** — Inject Tera Raid event data from prepared event folders into your save file
- **Event Clearing** — Remove all active raid event data (inject null)
- **Automatic Backups** — Save data is backed up to SD card before any modification
- **Round-Trip Verification** — Encryption integrity is verified after decryption to ensure data safety
- **Theme Support** — Switch between Default (dark) and HOME (light pastel) themes
- **Validity Cache** — Event folder validation results are cached to disk for instant browsing
- **Joystick & D-Pad Navigation** — Full analog stick support with repeat on all screens

## Requirements

- Nintendo Switch with CFW
- **Title override mode required** — Launch via a game title, not the album (applet mode is not supported)
- Pokemon Scarlet and/or Violet save data on the console
- SD card with sufficient free space for backups

## Installation

1. Copy `EventRaidInjector.nro` to `sdmc:/switch/EventRaidInjector/` on your SD card
2. Place event folders in `sdmc:/switch/EventRaidInjector/events/`
3. Launch via title override (hold R while opening a game)

## Event Folder Structure

Each event folder placed in `events/` must contain:

```
MyEvent/
  Identifier.txt              # Event name/description (first line is displayed)
  Files/
    event_raid_identifier      # 4 bytes
    raid_enemy_array           # 29,936 bytes
    fixed_reward_item_array    # 27,456 bytes
    lottery_reward_item_array  # 53,464 bytes
    raid_priority_array        # 88 bytes
```

Binary files support version suffixes (checked in order): `_3_0_0`, `_2_0_0`, `_1_3_0`, or no suffix.

Event data compatible with [Tera-Finder](https://github.com/Manu098vm/Tera-Finder) format.

Event data is available here : [ProjectPokemon](https://github.com/projectpokemon/EventsGallery/tree/master/Released/Gen%209/SV/Raid%20Events)

## Usage

1. **Select Profile** — Choose the Switch user profile with save data
2. **Select Game** — Pick Scarlet or Violet (a backup is created automatically)
3. **Browse Event Folder** — Select an event from the list (validity is shown for each folder)
4. **Inject Event** — Write the event data into the save file
5. **Save & Exit** — Commit changes and close the app

To remove active events, use **Clear Event (Inject Null)** instead.

```
**Note:** After injecting a raid event, you may need to advance the system date by 1 day in-game to refresh the active raids.
```

Use **Clear Cache & Revalidate Events** if you've added or modified event folders and need to refresh validation results.

## Controls

| Screen | Action | Button |
|---|---|---|
| **Profile Selector** | Navigate | D-Pad Left/Right, Stick |
| | Select | A |
| | Theme | Y |
| | About | - |
| | Quit | + |
| **Game Selector** | Navigate | D-Pad Left/Right, Stick |
| | Select | A |
| | Back | B |
| | Theme | Y |
| | About | - |
| | Quit | + |
| **Main Menu** | Navigate | D-Pad Up/Down, Stick |
| | Select | A |
| | Back | B |
| | Theme | Y |
| | About | - |
| | Quit | + |
| **Folder Browser** | Navigate | D-Pad Up/Down, Stick |
| | Select | A |
| | Cancel | B |
| | Page Up/Down | L / R |
| | About | - |
| **Theme Selector** | Navigate | D-Pad Up/Down, Stick |
| | Confirm | A |
| | Cancel | B |

## Backups

Backups are created automatically in:

```
sdmc:/switch/EventRaidInjector/backups/{ProfileName}/{Game}/{ProfileName}_{Timestamp}/
```

If there isn't enough free space (2x save size), you'll be prompted to continue without a backup.

## Themes

- **Default** — Dark theme with gold and cyan accents
- **HOME** — Light pastel theme inspired by Pokemon HOME

Theme preference is saved to `theme.cfg` and persists across sessions.

## Credits

- **[Tera-Finder](https://github.com/Manu098vm/Tera-Finder)** by Manu098vm — Injection logic & block definitions
- **[ProjectPokemon Events Gallery](https://github.com/projectpokemon/EventsGallery)** by ProjectPokemon — Event data source
- **[PKHeX](https://github.com/kwsch/PKHeX)** by kwsch — SCBlock format & encryption
- **[JKSV](https://github.com/J-D-K/JKSV)** by J-D-K — Save backup and write logic reference
- **[pkHouse](https://github.com/Insektaure/pkHouse)** by Insektaure — UI framework & Themes
- Built with [libnx](https://github.com/switchbrew/libnx) and [SDL2](https://www.libsdl.org/)
