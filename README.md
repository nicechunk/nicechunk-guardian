# NiceChunk Guardian

![NiceChunk Guardian overview](docs/screenshots/overview.png)

Realtime Guardian server for multiplayer area ownership and world events.

## Project Overview

This repository contains the C++ Guardian service. Guardian is the realtime server component that handles local player presence, movement messages, dig events, chat, AOI behavior, and service-region boundaries.

The service is intentionally separate from the browser client and the chain registry program. The chain program records ownership and registration; this server handles low-latency realtime traffic for the region it operates.

The C++ codebase is small enough for direct systems-level review while still supporting practical deployment concerns such as configuration files, protocol tests, and local client scripts.

## Realtime Loop

![Realtime Guardian loop](docs/diagrams/realtime-guardian-loop.svg)

Guardian is the low-latency half of the world protocol. A client starts with a fixed-size `HELLO`, receives a local player ID and region bounds, then sends compact movement, dig, and chat messages. The service assigns players to chunk rooms and publishes updates only to the area of interest around each room.

The protocol is intentionally small: fixed message tags, explicit payload sizes, bounded chat bytes, dig sequence checks, and move batching. That shape keeps realtime traffic cheap to process and easy to test. Persistent ownership, staking, and proof submission remain outside this server and belong to the Solana program layer.

## System Principles

- Realtime traffic stays off-chain: movement and chat use a compact binary WebSocket protocol, while persistent ownership and proofs belong to Solana programs.
- Region boundaries are explicit: Guardian instances operate around configured chunk centers and service radii.
- Protocol compatibility is testable: message structures and edge cases are covered by focused tests.
- Operational visibility matters: the TUI and stats paths are part of making a Guardian node observable during development.

## How It Works

- Build the service with CMake, run protocol tests, and launch with a Guardian configuration file.
- Use local scripts to simulate clients and benchmark WebSocket traffic.
- Pair service configuration with on-chain Guardian registry entries so clients can discover the correct region endpoint.
- Keep binary protocol changes synchronized with the browser Guardian client.

## Why This Project Matters

NiceChunk needs a realtime layer that can move faster than chain confirmation while remaining anchored to public ownership rules. Guardian provides that layer.

A standalone service repository makes it possible for operators to fork, inspect, and run Guardian nodes without pulling frontend or contract documentation assets.

## Repository Layout

- `Guardian/src/`
- `Guardian/tests/`
- `Guardian/config/`
- `Guardian/scripts/`

## Development Workflow

1. Clone the repository and inspect the focused source tree before changing shared contracts or generated artifacts.
2. Keep changes scoped to the domain of this repository. Cross-domain changes should be coordinated through the matching split repositories.
3. Run the smallest meaningful validation for the touched surface: build checks for programs, browser checks for pages, or fixture checks for deterministic libraries.
4. Update screenshots and documentation when behavior, visible UI, public constants, or developer-facing workflows change.

## Future Development Direction

- Add production-grade metrics export and structured logs.
- Introduce stricter protocol version negotiation and compatibility tests.
- Integrate automated proof submission against the Guardian registry program.
- Document deployment topologies without committing environment-specific deployment scripts.

## Maintenance Notes

This repository is a focused split from the main NiceChunk working tree. Keep the public surface explicit: avoid committing private keys, wallet files, deployment-only scripts, machine-specific configuration, or generated build artifacts. Runtime user-facing copy should stay behind the i18n layer where the project has an i18n surface.
