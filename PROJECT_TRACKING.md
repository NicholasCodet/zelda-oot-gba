# Project Tracking – Zelda OOT GBA

# Next Action

Create a small test room with multiple obstacles

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
- [x] Player cannot pass through obstacle
- [x] Player can slide along walls smoothly

---

## Phase 5 – Interaction

Status: ⬜ Not started

Tasks:
- [ ] Dialogue box
- [ ] Interaction trigger
- [ ] Basic transitions

Validation:
- Player can interact with world elements

---

## Phase 6 – Combat

Status: ⬜ Not started

Tasks:
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