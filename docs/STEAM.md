# Steam Release Checklist - Reinforcement Forge

## Build

1. Package a Shipping build:
   ```
   powershell -ExecutionPolicy Bypass -File Tools\PackageGame.ps1
   ```
   Output lands in `Packaged\Windows\`.

2. The game already enables **OnlineSubsystemSteam** (`Config/DefaultEngine.ini`)
   with the SpaceWar placeholder AppId **480**. After Steamworks approval,
   replace `SteamDevAppId=480` with your real AppId and add a
   `steam_appid.txt` next to the shipped exe for local testing only
   (do NOT ship it in the depot).

## Steamworks setup

- App type: Game. OS: Windows 64-bit.
- Depot: upload the entire `Packaged\Windows\` folder.
- Launch option: `BrainForge.exe` (the packaged root stub).
- Set "DirectX 12" and 4 GB RAM as minimum requirements; the game is
  UI-driven and runs comfortably on integrated GPUs.

## Store assets to prepare

- Capsule images: use the `images/` sheet logo plaque as the base
  (`Content/RawAssets/Sprites/logo_plaque.png` is a pre-keyed version).
- 5+ screenshots: main menu, whole-brain view, subnetwork editor,
  generation choice, research archive. `Tools/Screenshot.ps1` helps.
- Short trailer: screen-record a 2x-speed session; the generative score is
  royalty-free (synthesized in-engine).

## Feature flags for the store page

- Single-player
- Steam Achievements (hook points exist: victory, first discovery,
  generation 10 - wire them through OnlineSubsystem when the AppId is live)
- Partial Controller Support (mouse recommended)

## Content ratings notes

No violence, no text chat, no user-generated content shared online.
Abstract simulation of biological evolution. Suitable for all ages.

## Pricing / positioning

Comparable to Mini Motorways / Mini Metro ($9.99-$14.99). The calm,
generative-audio, blind-discovery angle is the differentiator.
