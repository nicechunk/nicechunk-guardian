# Guardian Load Audit

This document defines the deterministic Guardian core load audit that can run in a local CMake build without connecting to production infrastructure.

## Command

Build Guardian and run the CTest suite:

```bash
cmake -S Guardian -B Guardian/build -DCMAKE_BUILD_TYPE=Release
cmake --build Guardian/build -j
npm run validate:guardian
```

For environments that only need the deterministic core audit, or where the network server dependencies are not installed, build the CTest targets without the uWebSockets server:

```bash
cmake -S Guardian -B Guardian/build -DCMAKE_BUILD_TYPE=Release -DGUARDIAN_BUILD_SERVER=OFF
cmake --build Guardian/build -j
npm run validate:guardian
```

`npm run validate:guardian` runs the Guardian CTest suite, including:

- `range_test` for service-region range checks.
- `aoi_test` for the default 15 x 15 AOI fanout shape.
- `protocol_test` for binary protocol encoding and decoding.
- `load_test` for deterministic Guardian core load coverage.

## Load Workload

`Guardian/tests/load_test.cpp` exercises the hot core behaviors without using live sockets:

- 4,096 virtual players placed inside a `201 x 201` service region.
- 240 movement ticks.
- 225 AOI offsets per player, matching the default radius-7 Guardian fanout.
- MOVE_BATCH encoding in batches of up to 255 movement items.
- Local chunk index roundtrips across the configured service range.
- Out-of-range chunk rejection.
- Client movement rate-limit accept/reject/reset behavior.
- A 10 second elapsed-time budget for the deterministic local workload.

The test prints counters for virtual players, ticks, AOI fanout checks, service range checks, local chunk roundtrips, encoded batches, encoded bytes, and elapsed time.

## What This Proves

This audit gives reviewers repeatable evidence that Guardian core protocol helpers, AOI iteration, range checks, local chunk conversion, movement batching, and rate limiting remain stable under a deterministic high-loop workload.

It is intentionally a core logic audit, not a production capacity claim.

## Remaining Manual Scope

The following Guardian release claims still need targeted evidence before they can be treated as complete:

- Networked Guardian soak tests with many real WebSocket clients.
- Slow-client and backpressure behavior under live socket pressure.
- Production host CPU, memory, file descriptor, kernel socket, TLS, and reverse-proxy capacity review.
- Multi-node region routing and failure-mode testing.

Use `Guardian/scripts/bench_ws.py` for manual networked Guardian soak tests when a local or staging Guardian service is available.
