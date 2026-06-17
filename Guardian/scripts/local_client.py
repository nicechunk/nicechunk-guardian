#!/usr/bin/env python3
import argparse
import asyncio
import os
import struct
import websockets

MSG_HELLO = 0x01
MSG_HELLO_ACK = 0x02
MSG_MOVE = 0x10
MSG_DIG = 0x20


def test_wallet(nonce: int) -> bytes:
    return bytes(((nonce + i) & 0xFF) or 1 for i in range(32))


def hello(chunk_x: int, chunk_z: int) -> bytes:
    return struct.pack("<BBH32siiQ", MSG_HELLO, 1, 0, test_wallet(1), chunk_x, chunk_z, 1)


def move(local_x: int, local_z: int, tick: int) -> bytes:
    return struct.pack("<BBBHHHBBH", MSG_MOVE, local_x, local_z, 100, 64, 100, 20, 0, tick)


def dig(local_x: int, local_z: int, seq: int) -> bytes:
    return struct.pack("<BHBBBHB BB".replace(" ", ""), MSG_DIG, seq, local_x, local_z, 1, 40, 1, 1, 0)


async def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--url", default=os.environ.get("GUARDIAN_URL", "ws://127.0.0.1:8080/ws"))
    parser.add_argument("--chunk-x", type=int, default=0)
    parser.add_argument("--chunk-z", type=int, default=0)
    args = parser.parse_args()

    async with websockets.connect(args.url, max_size=4096, compression=None) as ws:
        await ws.send(hello(args.chunk_x, args.chunk_z))
        msg = await ws.recv()
        if not isinstance(msg, bytes) or msg[0] != MSG_HELLO_ACK:
            raise RuntimeError(f"unexpected ack: {msg!r}")
        local_player_id = struct.unpack_from("<H", msg, 2)[0]
        print(f"HELLO_ACK local_player_id={local_player_id}")

        # default center=0 radius=100, so chunk 0,0 maps to local 100,100
        await ws.send(move(100, 100, 1))
        await ws.send(dig(100, 100, 1))
        print("sent MOVE and DIG")


if __name__ == "__main__":
    asyncio.run(main())
