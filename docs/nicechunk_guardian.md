# NiceChunk Guardian Registry v2

`nicechunk_guardian` is the devnet Guardian registry for NiceChunk region service nodes.

Guardian v2 uses a new Program ID and does not read or mutate accounts owned by the retired Guardian program. It covers NCK-staked registration, fixed 100 x 100 region operation, operator-only endpoint updates, governance-controlled operator recovery, and a chain-committed regional blueprint hash published through the Building Program.

The retired program remains deployed only as immutable historical state. All clients, scripts, Building CPI calls, and new Guardian PDAs use the v2 Program ID. No migration instruction accepts a retired-program account.

## Devnet Program

```txt
nicechunk_guardian = RQQZKA1fGELBxtxCQ6q7P26GJH4whWmPjH9XqmihVRK
nicechunk_core     = 9EhMCRYMJej1F21KzaA5Zao3khGGc5aJbDGbnxaogQHu
devnet NCK mint    = HSnWF5kjkWVrceW2SaSskScuLveUZE4gpthZ2ZXRPQPo
registry PDA       = 6F8TWaDFTP2RWyPtdS4aeTcGpfECXQiiA61pBe5Wyyg9
treasury authority = Bhvm8YPXT11CrJUmcaW4oLA2RoD1T6Kvp7mJRJYV3q2V
treasury NCK ATA   = fC35KXTXwDJVsWaExV9rc4gAeAhFYHXsiTvofd3MW91
governance wallet  = 9XuoVVwqP2jipt3jpJVXCSS2N2jr9vDuV3d6K73FKVud
building program    = 39UMTUWXQkuomkFNbDPF5NGZnJmG6pDkJHVSkZyqVwWx
```

Use Solana devnet for this program. Do not use mainnet RPC or mainnet NCK while testing.

## Region Model

Guardian regions are fixed grid cells:

```txt
region_x = floor(chunk_x / 100)
region_y = floor(chunk_y / 100)
```

Each active guardian operates exactly one 100 x 100 chunk region. `GuardianRegion.owner` always stores the treasury wallet, while `GuardianRegion.operator` stores the Guardian wallet:

```txt
min_chunk_x = region_x * 100
max_chunk_x = min_chunk_x + 99
min_chunk_y = region_y * 100
max_chunk_y = min_chunk_y + 99
```

This avoids arbitrary rectangle overlap checks. Overlap is prevented because each region PDA can only be active once.

The program enforces the treasury value both when the account is created and whenever an active Region is accepted. There is no generic owner setter, so a Guardian operator cannot acquire governance by rewriting account metadata.

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

The first genesis guardian requires signatures from both the staking Guardian operator and the configured governance wallet. The wallets may be different.

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
0. treasury           writable signer
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
1. operator           signer, must match operator_pubkey
2. operator_nck_token writable
3. registry           writable PDA
4. guardian_region    writable PDA
5. global_config      readonly
6. treasury_authority readonly PDA
7. treasury_nck_token writable
8. nck_mint           readonly
9. token_program      readonly
10. system_program    readonly
11. governance        signer
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

Registration always writes the configured treasury wallet to `GuardianRegion.owner`. The registering and staking wallet is written to `GuardianRegion.operator`; the payload cannot assign a different operator.

### UpdateGuardianEndpoint

```txt
instruction_data = [5, i32_le(region_x), i32_le(region_y), u16_le(port), u8(use_tls), u8(host_len), host_bytes]
```

Accounts:

```txt
0. authority       signer
1. registry        readonly PDA
2. guardian_region writable PDA
3. global_config   readonly
```

Only the stored Guardian operator can update its endpoint. Governance cannot bypass this rule; recovery is performed by rotating the operator first. No other Guardian metadata is writable through this instruction.

### UpdateGuardianBlueprint

This instruction commits the Guardian's current regional building manifest metadata.

```txt
instruction_data = [6, i32_le(region_x), i32_le(region_y), blueprint_hash_16, u64_le(revision), u32_le(record_count)]
```

Accounts:

```txt
0. Building Program `guardian-blueprint` PDA signer
1. guardian_region writable PDA
2. global_config   readonly
```

Direct wallet calls are rejected, including calls signed by governance or the Guardian operator. The Building Program derives `seeds = ["guardian-blueprint", global_config]`, signs the CPI, and is the only accepted writer. Its outer publish instruction accepts only governance or the dedicated index publisher, verifies the global config and Guardian Program ID, then performs the signed CPI. Bytes `256..272` store the manifest hash, `272..280` the revision, and `280..284` the record count.

### UpdateGuardianOperator

```txt
instruction_data = [8, i32_le(region_x), i32_le(region_y), new_operator_pubkey]
```

Accounts:

```txt
0. governance      signer
1. guardian_region writable PDA
2. global_config   readonly
```

Only governance can rotate a Guardian operator. This provides key-loss and compromise recovery without granting the Guardian wallet control over any additional settings.

## Operator Permission Boundary

After registration, the Guardian operator wallet can invoke only `UpdateGuardianEndpoint`. Instruction tags `3` and `4` from the retired proof/settlement design are rejected. Their state transition code has also been removed. The existing proof-related account bytes remain reserved so deployed 288-byte Region PDAs stay compatible; no current instruction mutates them.

Governance can rotate a compromised operator through `UpdateGuardianOperator`. Blueprint changes never use the Guardian operator and never accept a direct wallet signature in the Guardian Program.

Future Guardian settings must be introduced as explicit, separately authorized instructions. They must not be added to `UpdateGuardianEndpoint`, and the operator receives no implicit write access to reserved bytes.

## Scripts

Initialize registry:

```bash
PAYER_KEYPAIR=/path/to/treasury-keypair.json \
NCK_MINT=HSnWF5kjkWVrceW2SaSskScuLveUZE4gpthZ2ZXRPQPo \
npm run guardian:init-registry
```

Register genesis guardian:

```bash
PAYER_KEYPAIR=<treasury-keypair.json> \
GUARDIAN_OPERATOR_KEYPAIR=<guardian-operator-keypair.json> \
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
PAYER_KEYPAIR=<guardian-operator-keypair.json> \
CHUNK_X=100 \
CHUNK_Y=0 \
GUARDIAN_HOST=guardian-east.example.com \
GUARDIAN_PORT=8899 \
GUARDIAN_USE_TLS=1 \
npm run guardian:register
```

Continuously commit the Guardian's manifest hash whenever its list changes:

```bash
PAYER_KEYPAIR=<building-index-publisher-keypair.json> \
GUARDIAN_BUILDINGS_URL=http://127.0.0.1:8080/buildings \
GUARDIAN_WATCH=1 \
npm run guardian:sync-blueprint
```

The watcher verifies the local binary manifest against Building and BuildSite PDAs, then invokes the Building Program. The Building Program signs the Guardian CPI with its PDA; the watcher sends no transaction while the on-chain blueprint hash is unchanged.

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
