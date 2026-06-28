# NiceChunk Guardian Registry v0

`nicechunk_guardian` is the devnet Guardian registry for NiceChunk region service nodes.

This version only covers registration, fixed 100 x 100 region ownership, treasury-paid NCK stake, hourly proofs, and accounting slashes. It does not implement stake withdrawal, service fee distribution, challenges, governance, or multi-region operators.

## Devnet Program

```txt
nicechunk_guardian = 6frJyJSirfEwsztsxijcJLe29LSaceET1wanXSFwPQyE
nicechunk_core     = 9EhMCRYMJej1F21KzaA5Zao3khGGc5aJbDGbnxaogQHu
devnet NCK mint    = HSnWF5kjkWVrceW2SaSskScuLveUZE4gpthZ2ZXRPQPo
registry PDA       = 2CM93cdoLNzTD1MMse4AmVKyHmKcBJeZQ7tkjQZ61Q9j
treasury authority = FzwpJVdhWnejAvaem33jZRQQc5GW4gYfgPAKTyA992Eh
treasury NCK ATA   = EKF93kNu1ygAtqpJMwAgRde5ZyL1sTLkhKJrzRvyHK3R
```

Use Solana devnet for this program. Do not use mainnet RPC or mainnet NCK while testing.

## Region Model

Guardian regions are fixed grid cells:

```txt
region_x = floor(chunk_x / 100)
region_y = floor(chunk_y / 100)
```

Each active guardian owns exactly one 100 x 100 chunk region:

```txt
min_chunk_x = region_x * 100
max_chunk_x = min_chunk_x + 99
min_chunk_y = region_y * 100
max_chunk_y = min_chunk_y + 99
```

This avoids arbitrary rectangle overlap checks. Overlap is prevented because each region PDA can only be active once.

## PDA Seeds

Registry:

```txt
seeds = ["guardian-registry", global_config]
```

Treasury authority:

```txt
seeds = ["guardian-treasury", global_config]
```

Region:

```txt
seeds = ["guardian-region", global_config, i32_le(region_x), i32_le(region_y)]
```

## Treasury Stake

Registration transfers:

```txt
100,000 NCK = 100_000_000_000 base units
```

The transfer goes directly to the treasury NCK token account. There is intentionally no withdraw instruction in v0. Future guardian compensation must be implemented through a separate reward mechanism.

## Adjacency Rule

The first genesis guardian must be registered by the configured development wallet.

Every non-genesis guardian must pass all four neighbor PDA addresses and at least one must be active:

```txt
(region_x + 1, region_y)
(region_x - 1, region_y)
(region_x, region_y + 1)
(region_x, region_y - 1)
```

This prevents isolated guardian islands.

## Instructions

### InitializeRegistry

```txt
instruction_data = [0]
```

Accounts:

```txt
0. payer              writable signer
1. registry           writable PDA
2. global_config      readonly
3. treasury_authority readonly PDA
4. treasury_nck_token readonly token account
5. nck_mint           readonly
6. system_program     readonly
```

### RegisterGenesisGuardian

```txt
instruction_data = [1, i32_le(region_x), i32_le(region_y), u16_le(port), u8(use_tls), u8(host_len), host_bytes, operator_pubkey]
```

Accounts:

```txt
0. payer              writable signer
1. owner              signer, must be development wallet
2. owner_nck_token    writable
3. registry           writable PDA
4. guardian_region    writable PDA
5. global_config      readonly
6. treasury_authority readonly PDA
7. treasury_nck_token writable
8. nck_mint           readonly
9. token_program      readonly
10. system_program    readonly
```

### RegisterGuardian

```txt
instruction_data = [2, i32_le(region_x), i32_le(region_y), u16_le(port), u8(use_tls), u8(host_len), host_bytes, operator_pubkey]
```

Accounts are the same as genesis, plus:

```txt
11. east_neighbor  readonly
12. west_neighbor  readonly
13. north_neighbor readonly
14. south_neighbor readonly
```

### SubmitGuardianProof

```txt
instruction_data = [3, i32_le(region_x), i32_le(region_y)]
```

Accounts:

```txt
0. operator        signer
1. registry        writable PDA
2. guardian_region writable PDA
3. global_config   readonly
```

### SettleGuardian

```txt
instruction_data = [4, i32_le(region_x), i32_le(region_y)]
```

Accounts:

```txt
0. registry        writable PDA
1. guardian_region writable PDA
2. global_config   readonly
```

Anyone can settle a stale guardian. Solana programs do not run timers automatically.

## Proof And Slashing

Guardian operators should submit one proof per hour.

If proof is late, settlement records missed hours:

```txt
slash_per_missed_hour = 10,000 NCK
```

When the region's accounting stake reaches zero:

```txt
status = Removed
```

The region can then be registered by another guardian.

## Scripts

Initialize registry:

```bash
PAYER_KEYPAIR=/path/to/devnet-payer.json \
NCK_MINT=HSnWF5kjkWVrceW2SaSskScuLveUZE4gpthZ2ZXRPQPo \
npm run guardian:init-registry
```

Register genesis guardian:

```bash
PAYER_KEYPAIR=<development-wallet-keypair.json> \
GUARDIAN_GENESIS=1 \
CHUNK_X=0 \
CHUNK_Y=0 \
GUARDIAN_HOST=guardian.example.com \
GUARDIAN_PORT=8899 \
GUARDIAN_USE_TLS=1 \
npm run guardian:register
```

Register a normal adjacent guardian:

```bash
PAYER_KEYPAIR=<guardian-owner-keypair.json> \
CHUNK_X=100 \
CHUNK_Y=0 \
GUARDIAN_HOST=guardian-east.example.com \
GUARDIAN_PORT=8899 \
GUARDIAN_USE_TLS=1 \
npm run guardian:register
```

Submit proof:

```bash
PAYER_KEYPAIR=<operator-keypair.json> \
REGION_X=0 \
REGION_Y=0 \
npm run guardian:proof
```

List guardians:

```bash
npm run guardian:list
```

## Website

The website page is:

```txt
https://nicechunk.com/guardian/
```

It can:

- connect a wallet,
- list active guardians,
- compute region coordinates from chunk coordinates,
- register genesis or adjacent guardians,
- send the 100,000 NCK registration transaction to the treasury.
