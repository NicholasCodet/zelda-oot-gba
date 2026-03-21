# Project Tracking – Zelda OOT GBA

# Next Action

Add input handling and move a simple on-screen rectangle

## Overview

This document tracks the progress of the project.

The goal is to build a 2D Zelda-like game on Game Boy Advance, inspired by Ocarina of Time, using an incremental development approach.

---

# Current Phase

Phase 1 – Environment Setup

Status: In progress

Next objective:
- Build a minimal GBA project
- Compile a working ROM
- Run it in mGBA

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

Status: ⬜ Not started

Tasks:
- [ ] Add player sprite
- [ ] Handle input
- [ ] 4-direction movement

Validation:
- Player moves smoothly on screen

---

## Phase 4 – Collision System

Status: ⬜ Not started

Tasks:
- [ ] Tile-based collision
- [ ] Solid tiles

Validation:
- Player cannot walk through walls

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