# AI Coding Rules

This file defines the rules AI assistants must follow when contributing to this repository.

AI assistants must read this file before generating or modifying code.

---

# Project Context

This project is a **Game Boy Advance game written in C**.

The game is a **2D adaptation inspired by The Legend of Zelda: Ocarina of Time**, designed specifically for the constraints of the Game Boy Advance hardware.

The project aims to create a **Zelda-like engine** and a small playable game built incrementally.

The game is not a direct port of Ocarina of Time.

Instead, it reinterprets its locations, themes and progression in a **top-down 2D gameplay format** similar to classic Zelda games.

---

# Initial Setup Rule

During the initial setup phase:

- Only create the minimal project required to compile a GBA ROM
- Do not generate gameplay systems
- Do not create unnecessary files
- Do not introduce architecture beyond main.c and Makefile
  
---

# Target Platform

Game Boy Advance (GBA)

Important hardware characteristics:

- tile-based graphics
- sprite-based entities
- limited VRAM
- limited CPU resources
- limited memory

Code should remain **simple and efficient**.

---

# Programming Language

Language used in this project:

C

Compatible with:

devkitPro  
libgba

Do not generate code using:

- C++
- Rust
- scripting languages
- complex frameworks

---

# Core Development Principles

## Simplicity

Prefer simple and readable code.

Avoid complex abstractions or over-engineering.

## Small Systems

Implement systems in small independent modules.

Examples:

player.c  
map.c  
entity.c  

Each module should have a single responsibility.

## Incremental Development

Implement **one feature at a time**.

Example tasks:

- player movement
- tile collision
- simple enemy
- dialogue box

Do not generate multiple large systems at once.

---

# Code Architecture Guidelines

Prefer the following structure:

src/
main.c
game.c
player.c
map.c
entity.c

include/
header files

assets/
sprites
tilesets
maps

Avoid introducing unnecessary directories.

---

# Memory and Performance

Avoid dynamic memory allocation unless strictly necessary.

Prefer static or stack-based memory.

Keep systems lightweight.

Avoid heavy data structures.

---

# Gameplay Systems

Typical gameplay systems include:

Player
- movement
- attack
- collision

World
- tilemap rendering
- collision tiles
- triggers

Entities
- enemies
- NPCs
- items

UI
- health display
- dialogue box

Game State
- exploration
- combat
- dialogue

---

# Code Generation Rules

When generating code:

1. Modify as few files as possible.
2. Do not rewrite entire systems unnecessarily.
3. Prefer extending existing files rather than creating new frameworks.
4. Keep functions small and readable.

---

## Code Comments

All generated code must include clear and concise comments explaining:

- what the function does
- important logic
- GBA-specific behavior when relevant

---

# Avoid These Patterns

Do not generate:

- full game engines
- complex object-oriented systems
- unnecessary abstraction layers
- overly generic frameworks

This project favors **practical and simple implementations**.

---

# Expected Behavior for AI Assistants

Before writing code:

1. Read README.md
2. Read AI_RULES.md
3. Understand the current code structure

When implementing features:

- implement only what is requested
- keep code small
- prefer clarity over cleverness

If multiple solutions exist, choose the **simplest working solution**.

