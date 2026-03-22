# Project Tracking – Zelda OOT GBA

# Next Action

Validate modular refactor and confirm all gameplay systems still work

## Overview

This document tracks the progress of the project.

The goal is to build a 2D Zelda-like game on Game Boy Advance, inspired by Ocarina of Time, using an incremental development approach.

---

# Current Phase

---

# Roadmap

## Phase 1 – Environment Setup

Status: ✅ Completed

Tasks:
- [x] Install devkitPro
- [x] Install libgba
- [x] Create minimal project
- [x] Compile ROM (.gba)
- [x] Run ROM in emulator

Validation:
- [x] ROM builds successfully
- [x] ROM launches in mGBA

---

## Phase 2 – Rendering Basics

Status: ✅ Completed

Tasks:
- [x] Initialize display mode
- [x] Clear screen
- [x] Display simple background
- [x] Render test rectangle
- [x] Render multiple colored rectangles

Validation:
- [x] Screen displays correctly without artifacts
- [x] Multiple visible markers displayed correctly

---

## Phase 3 – Player Movement

Status: ✅ Completed

Tasks:
- [x] Add player sprite
- [x] Handle input
- [x] 4-direction movement
- [x] Keep rectangle inside screen bounds
- [x] Stabilize rendering without flicker

Validation:
- [x] Player moves smoothly on screen without flicker
- [x] Player movement implemented with D-pad
- [x] Rendering stabilized with proper VBlank synchronization
  
---

## Phase 4 – Collision System

Status: ✅ Completed

Tasks:
- [x] Add fixed obstacle
- [x] Draw obstacle
- [x] Implement AABB collision
- [x] Block player movement on collision
- [x] Separate X and Y axis movement
- [x] Allow sliding along walls

Validation:
- [x] Player cannot pass through obstacles
- [x] Player can slide along walls
- [x] Player can navigate a multi-obstacle room

---

## Phase 5 – Interaction

Status: ✅ Completed

Tasks:
- [x] Add a special interactive object
- [x] Detect player proximity to the object
- [x] Detect A button press
- [x] Toggle object state on interaction
- [x] Provide visible feedback through color change
- [x] Prevent obstacle activation if overlapping player
- [x] Support multiple interactive objects
- [x] Support multiple independent obstacles
- [x] Refactor interaction logic using reusable object structures
- [x] Link interaction to world changes
- [x] Create a simple puzzle using interactions
- [x] Add a win condition based on world state + player position
- [x] Improved puzzle readability
- [x] Inverted interaction logic (objects remove obstacles)
- [x] Ensured obstacles fully block progression
- [x] Validated intuitive puzzle behavior

Validation:
- [x] Interaction works only near the object
- [x] Interaction requires button press
- [x] Object state changes visibly
- [x] No invalid overlap between player and obstacle
- [x] Extended interaction system to support multiple objects
- [x] Validated independent object-obstacle relationships
- [x] Improved scalability of interaction logic
- [x] Refactor preserved behavior
  

---

## Phase 6 – Combat

Status: ⏳ In progress

Tasks:
- [x] Add one static enemy
- [x] Add player attack input
- [x] Add directional attack hitbox
- [x] Detect hit between attack and enemy
- [x] Remove enemy on hit
- [x] Fix attack rendering trail
- [x] Fix rendering flicker
- [x] Restore background under dynamic elements correctly
- [x] Add simple enemy movement
- [x] Make enemy respect room obstacle collisions
- [x] Fix rendering issues during combat
- [x] Add player damage on enemy contact
- [x] Add player health system
- [x] Add player damage feedback
- [ ] Fix true player dead-state behavior

- [ ] Sword attack
- [ ] Enemy entity
- [ ] Damage system
- [ ] Health system

Validation:
- Combat loop works

---

## Phase 7 – First Playable Slice

Status: ⬜ Not started

Goal:
Create a small playable area inspired by Kokiri Forest.

Content:
- Overworld area
- NPCs
- Basic interactions
- Simple combat

Validation:
- Player can explore and complete a small objective

---

# Completed

- Minimal GBA project created
- Makefile fixed and working
- ROM successfully compiled
- ROM successfully launched in mGBA

- Rendering in Mode 3 validated
- Multiple colored rectangles displayed successfully

- Refactored player state into a Player struct
- Separated player update and draw logic into dedicated functions
- Added player direction tracking
- Player movement implemented with D-pad
- Rendering stabilized with proper VBlank synchronization

- Basic collision system implemented (player vs obstacle)
- Collision handling improved with separated X/Y axis movement
- Smooth wall sliding validated

- Built first navigable room using collision system
- Fixed player spawn positioning
- Validated movement in constrained space

- Implemented first gameplay interaction (object → world change)
- Added object state toggles with visible feedback
- Linked interactive objects to world changes
- Created basic puzzle mechanic (toggle obstacle)
- Fixed interaction edge-case (prevent invalid overlap)
- Extended interaction system to support multiple objects
- Validated independent object-obstacle relationships
- Improved scalability of interaction logic
- Refactored interaction logic using reusable object structures
- Built first complete gameplay loop (move → interact → solve → reach goal)
- Validated puzzle logic based on world state and player position
- Designed first intuitive puzzle (unlock path via interaction)
- Validated player understanding through level design

- Implemented first combat interaction (attack → hit → enemy removal)
- Fixed rendering artifacts in combat system
- Stabilized incremental rendering approach
- Added basic enemy health system
- Validated multi-hit enemy defeat
- Refactored codebase from single-file prototype to modular C structure
- Preserved gameplay behavior during architectural cleanup
  
# Project Architecture

## Root

- README.md → project overview  
- AI_RULES.md → AI behavior and coding constraints  
- PROJECT_TRACKING.md → project progress  

---

## src/

Main game source code.

- main.c → entry point, game loop  
- game.c → global game state  
- player.c → player logic  
- map.c → tilemap and world  
- entity.c → enemies and NPCs  

---

## include/

Header files corresponding to source files.

---

## assets/

Game assets.

- sprites  
- tilesets  
- maps  

---

## build/

Compiled output files.

---

## tools/

Optional tools for asset processing.

---

# Development Rules

- Work incrementally
- Validate each step before moving on
- Keep code simple
- Avoid premature optimization

---

# Notes

Always provide context to AI tools:

- README.md  
- AI_RULES.md  

Avoid large prompts.  
Prefer small, focused tasks.