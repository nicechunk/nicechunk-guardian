#!/usr/bin/env python3
import argparse
import asyncio
import random
import statistics
import struct
import time
import websockets

MSG_HELLO = 0x01
MSG_HELLO_ACK = 0x02
MSG_MOVE = 0x10


def test_wallet(nonce: int) -> bytes:
    return bytes(((nonce + i) & 0xFF) or 1 for i in range(32))


def hello(chunk_x: int, chunk_z: int, nonce: int) -> bytes:
    return struct.pack("<BBH32siiQ", MSG_HELLO, 1, 0, test_wallet(nonce), chunk_x, chunk_z, nonce)


def move(local_x: int, local_z: int, tick: int) -> bytes:
    return struct.pack("<BBBHHHBBH", MSG_MOVE, local_x, local_z, random.randint(0, 2047), 64, random.randint(0, 2047), 64, 0, tick)


async def client_task(i, args, results):
    latencies = []
    sent = 0
    disconnected = False
    if args.hotspot:
        chunk_x = args.center_x
        chunk_z = args.center_z
    else:
        chunk_x = random.randint(args.center_x - args.radius, args.center_x + args.radius)
        chunk_z = random.randint(args.center_z - args.radius, args.center_z + args.radius)
    local_x = chunk_x - (args.center_x - args.radius)
    local_z = chunk_z - (args.center_z - args.radius)
    try:
        async with websockets.connect(args.url, max_size=args.max_size, compression=None, ping_interval=None) as ws:
            await ws.send(hello(chunk_x, chunk_z, i + 1))
            ack = await ws.recv()
            if not isinstance(ack, bytes) or ack[0] != MSG_HELLO_ACK:
                raise RuntimeError("bad hello ack")
            interval = 1.0 / args.move_hz
            end_at = time.perf_counter() + args.seconds
            tick = 0
            while time.perf_counter() < end_at:
                t0 = time.perf_counter()
                await ws.send(move(local_x, local_z, tick & 0xFFFF))
                sent += 1
                tick += 1
                latencies.append((time.perf_counter() - t0) * 1000)
                await asyncio.sleep(interval)
    except Exception:
        disconnected = True
    results.append((sent, disconnected, latencies))


async def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--url", default="ws://127.0.0.1:8080/ws")
    parser.add_argument("-n", "--connections", type=int, default=1000)
    parser.add_argument("--move-hz", type=float, default=10)
    parser.add_argument("--seconds", type=float, default=30)
    parser.add_argument("--center-x", type=int, default=0)
    parser.add_argument("--center-z", type=int, default=0)
    parser.add_argument("--radius", type=int, default=100)
    parser.add_argument("--hotspot", action="store_true")
    parser.add_argument("--max-size", type=int, default=4096)
    args = parser.parse_args()

    results = []
    started = time.perf_counter()
    await asyncio.gather(*(client_task(i, args, results) for i in range(args.connections)))
    elapsed = time.perf_counter() - started

    sent = sum(item[0] for item in results)
    disconnected = sum(1 for item in results if item[1])
    latencies = [lat for _, _, values in results for lat in values]
    latencies.sort()
    avg = statistics.mean(latencies) if latencies else 0
    p95 = latencies[int(len(latencies) * 0.95)] if latencies else 0
    p99 = latencies[int(len(latencies) * 0.99)] if latencies else 0

    print(f"connections_requested {args.connections}")
    print(f"connections_completed {len(results)}")
    print(f"disconnects {disconnected}")
    print(f"moves_sent {sent}")
    print(f"throughput_moves_per_sec {sent / elapsed:.2f}")
    print(f"send_latency_ms_avg {avg:.3f}")
    print(f"send_latency_ms_p95 {p95:.3f}")
    print(f"send_latency_ms_p99 {p99:.3f}")


if __name__ == "__main__":
    asyncio.run(main())
