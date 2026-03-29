# Project Tracking – Zelda OOT GBA

# Next Action

Redesign the latest room so the puzzle is enforced by level geometry

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

Status: ✅ Completed

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
- [x] Fix true player dead-state behavior
- [x] Player knockback on hit
- [x] Stable combat rendering

Validation:
- [x] Player can attack in the current facing direction
- [x] Enemy takes damage and dies after multiple hits
- [x] Enemy contact damages the player
- [x] Player invulnerability prevents repeated instant damage
- [x] Knockback makes enemy contact readable
- [x] Combat rendering remains stable without flicker or trails

---

## Phase 7 – First Playable Slice

Status: ✅ Completed

Tasks:
- [x] Rework room layout into a coherent playable slice
- [x] Include movement, collision, interaction, combat, and goal
- [x] Improve gameplay readability (color differentiation)
- [x] Improve gameplay readability (shape differentiation)
- [x] Refine visual balance of obstacles
- [x] Validate clarity of progression and challenge

Validation:
- [x] Player can understand the goal without explanation
- [x] Interactive objects are identifiable
- [x] Obstacles and their effects are readable
- [x] Enemy is clearly identifiable as a threat
- [x] Gameplay loop is understandable and playable

---

## Phase 8 – Rendering Architecture Upgrade

Status: ✅ Completed

Tasks:
- [x] Add room transition system
- [x] Add a second room
- [x] Fix HUD rendering instability
- [x] Validate room-to-room gameplay flow

Validation:
- [x] Player can reach the next room
- [x] HUD remains stable during gameplay and transitions
- [x] Room transitions feel reliable

---

## Phase 9 – Room Expansion

Status: ✅ Completed

Tasks: 
- [x] Add a third room
- [x] Redesign the third room so the puzzle is spatially necessary
- [x] Add a fourth room with meaningful gameplay variation
- [x] Fix global blink on interaction, goal validation, and room transitions
- [x] Validate multi-room gameplay flow
- [x] Validate final visual stability across all rooms
- [x] Clarify transition types between rooms
- [x] Use spatial passage only where it improves exploration feel
- [x] Keep teleporter transitions explicit and intentional
- [x] Remove Room 2 teleporter and make Room 2 -> Room 3 a true spatial passage
- [x] Make Room 2 <-> Room 3 bidirectional through matching openings
- [x] Clarify transition identity of each room

Validation:
- [x] Room 3 & 4 provides a meaningful gameplay variation
- [x] World state changes no longer cause visible blinking
- [x] Multi-room flow feels visually stable
- [x] Each room has a clear transition identity
- [x] Room-to-room flow feels understandable

---

## Phase 10 – Combat Refinement

Status: ⏳ In progress

Tasks:
- [x] Add a second enemy behavior variation
- [ ] Integrate enemy variations into different rooms
- [ ] Balance combat difficulty across rooms
- [ ] Improve gameplay progression using combat + puzzle combinations
- [ ] Improve hit feedback (enemy reaction)
- [ ] Improve player damage feedback
- [ ] Improve interaction feedback
- [ ] Improve overall game feel

Validation:
- [ ] Different rooms create different combat situations
- [ ] Enemy variations impact player behavior
- [ ] Difficulty increases progressively across rooms

---

## Phase 11 – Core Polish Preparation


Status: ⬜ Not started

Tasks: 

Validation:

---

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

- Completed first playable combat loop
- Added enemy damage, player damage, invulnerability, and knockback
- Reached Zelda-like combat feel prototype

- Added room transition system
- Added a second playable room
- Extended the prototype beyond the initial roadmap

- Stabilized HUD rendering in Mode 3
- Resolved rendering artifacts affecting player and UI
- Validated multi-room gameplay loop

- Added a fourth room with stronger gameplay variation
- Removed global screen blink during world state changes
- Validated stable multi-room gameplay progression
- Fixed invalid initial room placements
- Added room layout validation safeguards
- Stabilized multi-room progression and room flow
  
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

## Known Issues

- Room 3 still has a localized rendering artifact when the player stands on a trigger:
  a rectangle may appear over the enemy.
- Minor rendering artifacts can appear when the player attack overlaps a wall.
- These issues are currently deferred because they do not block gameplay progression.