# UC Research — Rust Unity 6 External Cheat Reference

> Compiled from UnknownCheats thread pages 1227–1244 (June 1–12, 2026)
> Build: Rust (Steam) Unity 6 Il2Cpp, GameAssembly.dll around June 10

---

## 1. BUILD IDENTIFICATION

Multiple GameAssembly.dll builds exist. Always verify which dump matches your binary.

### Matching our build (gc_handles = 0xE6A13E0):

| Source | Date | Post# | base_networkable | gc_handles | TypeInfo RVAs |
|--------|------|-------|-----------------|------------|---------------|
| **xiaoxiao1** | Jun 10 | #24803 | `0xE2D5ED8` | `0xE6A13E0` | Full dump with TypeDefIndex |
| **lemonabc** | Jun 11 | #24846 | `0xE2D5ED8` | `0xE6513A0` | BN chain + Il2cppGetHandle |

### Different builds — DO NOT USE:

| Source | Date | Post# | base_networkable | Notes |
|--------|------|-------|-----------------|-------|
| v1mper | Jun 6 | #24681 | `0xE279E00` | All BasePlayer offsets wrong for our build |
| soultaken (older) | Jun 10 | #24784 | `0xE20FB70` | Different build, different decrypts |
| Koupers | Jun 4 | #24583 | `0xE1FCD80` | Older build |
| rustslayer4123 | Jun 4 | #24553 | `0xE2EBAE0` | Older build |

### How to verify which dump matches:
```
Check GameAssembly.dll timestamp via dumpbin or Il2CppDumper
Our build: timestamp ~0x6A22B09A (v1mper dump)
xiaoxiao1: timestamp ~0x6A29190F (close match)
```

---

## 2. CONFIRMED OFFSETS — OUR BUILD

### Static Pointers (RVA from GameAssembly.dll base)

| Name | Offset | Verified By |
|------|--------|-------------|
| `basenetworkable_pointer` | `0xE2D5ED8` | xiaoxiao1, lemonabc, fefe4444, Madness1337 |
| `camera_pointer` (MainCamera) | `0xE2C22B0` | fefe4444, worked version |
| `MainCamera TypeInfo` | `0xE26E648` | v1mper, xiaoxiao1 |
| `SingletonComponent<MainCamera>` | `0xE2D5CC8` | v1mper |
| `TOD_Sky_TypeInfo` | `0xE28EB78` | Our v39 dump |
| `ConVar_Graphics` | `0xE251748` | Claq #24821, worked version |
| `Il2CppGCHandle_base` | `0xE6513A0` (or `0xE6A13E0`) | xiaoxiao1, Madness1337 |
| `il2cpp_gchandle_get_target` export | `0x819F30` | xiaoxiao1, dumpbin |
| `Object::m_CachedPtr` | `0x10` | xiaoxiao1 |
| `GameObject::TypeInfo` | `0xE2D2A58` | xiaoxiao1 |
| `Transform::TypeInfo` | `0xE2770C8` | xiaoxiao1 |
| `Camera::TypeInfo` | `0xE2C3EC8` | xiaoxiao1 |

### BasePlayer

| Field | Offset | Type | Verified By |
|-------|--------|------|-------------|
| `PlayerInput` | `0x2E8` | `PlayerInput` | Our v39 dump, worked version |
| `PlayerEyes` | `0x3E0` | `%6bc5cfba<PlayerEyes>` | Our v39 dump, xiaoxiao1, soultaken #24787 |
| `PlayerInventory` | `0x3A8` | `%6bc5cfba<PlayerInventory>` | Our v39 dump, worked version |
| `PlayerModel` | `0x470` | `PlayerModel` | Our v39 dump, worked version |
| `PlayerFlags` | `0x670` | `PlayerFlags` | Our v39 dump, all dumps agree |
| `ClActiveItem` | `0x520` | encrypted UID wrapper | Our v39 dump, all dumps agree, lemonabc #24844 |
| `BaseMovement` | `0x540` | `BaseMovement` | Our v39 dump, worked version |
| `DisplayName` | `0x330` | `string` | Our v39 dump |
| `ModelState` | `0x320` | `%3a09...` type | Our v39 dump |
| `Mounted` | `0x578` | obfuscated type | Our v39 dump |
| `CurrentTeam` | `0x4F0` | `ulong` | Our v39 dump, all dumps agree |
| `Belt` (direct) | `0x388` | `%68d9...` type | Our v39 dump |
| `HeldEntity` (direct) | `0x688` | `HeldEntity` | Our v39 dump |
| `clothingBlocksAiming` | `0x754` | `bool` | All dumps agree |

### BaseCombatEntity

| Field | Offset | Type |
|-------|--------|------|
| `lifestate` | `0x290` | `int` |
| `_health` | `0x29C` | `float` |
| `_maxHealth` | `0x2A0` | `float` |
| `skeletonProperties` | `0x218` | pointer |
| `baseProtection` | `0x220` | pointer |

### BaseEntity

| Field | Offset | Type |
|-------|--------|------|
| `model` | `0x1A8` | `Model` pointer |
| `flags` | `0x1B0` | flags |
| `bounds` | `0x17C` | Bounds |
| `positionLerp` | `0x1D8` | PositionLerp |

### Model

| Field | Offset | Type |
|-------|--------|------|
| `boneTransforms` | `0x50` | `Transform*[]` array |
| `rootBone` | `0x28` | `Transform*` |
| `headBone` | `0x30` | `Transform*` |
| `eyeBone` | `0x38` | `Transform*` |

### PlayerInput (at BasePlayer + 0x2E8)

| Field | Offset | Type |
|-------|--------|------|
| `state` | `0x28` | InputState |
| `bodyAngles` | `0x44` | **Vector3** (was Vector2 pre-Unity 6) |
| (unknown Vector3) | `0x50` | Vector3 |
| (unknown Quaternion) | `0x34` | Quaternion |

### PlayerEyes (decrypted from BasePlayer + 0x3E0)

| Field | Offset | Type |
|-------|--------|------|
| `thirdPersonSleepingOffset` | `0x28` | Vector3 |
| `defaultLazyAim` | `0x38` | LazyAimProperties |
| (encrypted Vector3) | `0x40` | Vector3 wrapper |
| **bodyRotation** | `0x50` | **Quaternion** (write for aimbot) |
| (Vector3) | `0x60` | Vector3 |

### PlayerInventory (decrypted from BasePlayer + 0x3A8)

| Field | Offset | Type |
|-------|--------|------|
| `loot` | `0x48` | loot container |
| (belt via PlayerInventory chain) | varies | ItemContainer |

### Item

| Field | Offset | Type |
|-------|--------|------|
| `uid` | `0xB8` | ulong (was possibly at 0x38 or 0x98 in older) |
| `ItemDefinition` | `0xD8` | ItemDefinition pointer |
| `heldEntity` | `0xC0` | BaseProjectile/BaseMelee pointer |
| `amount` | `0x50` | int (JJSR #24864) |
| `contents` | `0x68` | item contents |
| `parent` | `0xA0` | parent item |
| `worldEnt` | `0xA8` | world entity |

### ItemDefinition (at Item::ItemDefinition pointer)

| Field | Offset | Type |
|-------|--------|------|
| `itemid` | `0x20` | int |
| `shortname` | `0x28` | string |
| `displayName` | `0x40` | string |
| `iconSprite` | `0x50` | sprite |
| `category` | `0x58` | category |

### ItemContainer

| Field | Offset | Type |
|-------|--------|------|
| `uid` | `0x30` | ulong |
| `itemList` | `0x58` | BufferList (was 0x10 in old, 0x48 in other builds) |

### PlayerInventory (struct after decrypt)

| Field | Offset | Type |
|-------|--------|------|
| `Belt` (containerBelt) | `0x58` | ItemContainer |
| `Main` (containerMain) | `0x30` or `0x38` | ItemContainer |
| `Wear` (containerWear) | `0x60` or `0x78` | ItemContainer |

### RecoilProperties (at BaseProjectile + 0x3D0)

| Field | Offset | Type |
|-------|--------|------|
| `recoilYawMin` | `0x18` | float |
| `recoilYawMax` | `0x1C` | float |
| `recoilPitchMin` | `0x20` | float |
| `recoilPitchMax` | `0x24` | float |
| `newRecoilOverride` | `0x80` | RecoilProperties pointer (nullable) |

### BaseProjectile (at heldEntity + 0x80 / Item::heldEntity)

| Field | Offset | Type |
|-------|--------|------|
| `projectileVelocityScale` | `0x35C` | float |
| `automatic` | `0x360` | bool |
| `recoil` | `0x3D0` | RecoilProperties pointer |
| `aimCone` | `0x3E0` | float |
| `hipAimCone` | `0x3E4` | float |
| `primaryMagazine` | `0x3A8` | Magazine |
| `isBurstWeapon` | `0x407` | bool |
| `sightAimConeScale` | `0x43C` | float |
| `viewModel` | `0x2A8` | ViewModel pointer |

### BaseMelee (at heldEntity + 0x80 / Item::heldEntity)

| Field | Offset | Type |
|-------|--------|------|
| `maxDistance` | `0x370` | float |
| `attackRadius` | `0x374` | float |
| `blockSprintOnAttack` | `0x379` | bool |

### HeldEntity

| Field | Offset | Type |
|-------|--------|------|
| `viewModel` | `0x1E8` | ViewModel |
| `ownerItemUID` | `0x2B0` | ulong |

### AttackEntity

| Field | Offset | Type |
|-------|--------|------|
| `repeatDelay` | `0x2BC` | float |
| `deployDelay` | `0x2B8` | float |
| `nextAttackTime` | `0x310` | float |
| `timeSinceDeploy` | `0x328` | float |

### BaseMovement (at BasePlayer + 0x540)

| Field | Offset | Type |
|-------|--------|------|
| `adminCheat` | `0x20` | bool |

### PositionLerp (at BaseEntity + 0x1D8)

| Field | Offset | Type |
|-------|--------|------|
| `interpolator` | `0x20` | Interpolator |

---

## 3. BN CHAIN (ENTITY DISCOVERY)

```
GameAssembly + basenetworkable_pointer (0xE2D5ED8)
  → read<uint64_t> → TypeInfo pointer
    → +0xB8 (static_fields) → read<uint64_t>
      → +0x30 (wrapper_class / clientEntities) → read<uint64_t>
        → [decrypt: networkable_key] → read +0x18, decrypt, Il2cppGetHandle
          → +0x10 (parent_static_fields) → read<uint64_t>
            → [decrypt: networkable_key2] → read +0x18, decrypt, Il2cppGetHandle
              → +0x18 (entity / BufferList) → read<uint64_t>
                → BufferList+0x18 = size (uint32_t)
                → BufferList+0x10 = array pointer
                  → array+0x20 = LocalPlayer
                  → array+0x20 + i*8 = entity[i]
                    → entity+0x10 = object (GameObject)
                      → object+0x20 = objectClass
                        → objectClass+0x50 = monostr name buffer (for prefab identification)
```

### Key chain notes:
- `wrapper_class=0x30` — confirmed by clovics #24843, Madness1337 #24842, fefe4444 #24841
- `entity=0x18` — NOT 0x10 (clovics #24843: "encrypted client entities is 0x18 not 0x10")
- `ListDictionary.vals=0x18` — CRITICAL: changed from 0x20 (NafNaf123 #24847)
- BN chain fails with `wrapper_class_ptr=0` for ~30s after joining server — NORMAL

---

## 4. DECRYPT ALGORITHMS (Our Build)

### 4A. networkable_key — Client Entities (Round 0)

```cpp
// Read from: address + 0x18 (GCHandle from %6bc5cfba wrapper)
// Operations on 2x uint32_t words:
uint32_t v = *word;
v -= 0x63825A21;           // (equivalent to +0x9C7DA5DF per soultaken/Madness1337)
v = ROL(v, 29);            // (v << 29) | (v >> 3)
v -= 0x1C287C47;
v ^= 0x595DCF40;
*word = v;

// After both words decrypted:
if (value & 1) return Il2cppGetHandle(value);
return read<uint64_t>(value);
```

**Confirmed by**: lemonabc #24846, NafNaf123 #24788, Madness1337 #24842, soultaken #24787

### 4B. networkable_key2 — Entity List (Round 1)

```cpp
// Read from: address + 0x18 (GCHandle from wrapper)
// Operations on 2x uint32_t words:
uint32_t v = *word;
v = ROL(v, 6);             // (v << 6) | (v >> 26)
v += 0x718B2E6D;
v ^= 0x594204AB;
*word = v;

// After both words decrypted:
if (value & 1) return Il2cppGetHandle(value);
return read<uint64_t>(value);
```

**Confirmed by**: lemonabc #24846, NafNaf123 #24788, soultaken #24787

### 4C. cl_active_item — Held Item UID

```cpp
// Raw value from BasePlayer + 0x520 IS the encrypted UID
// NO extra read from +0x18 — decrypt the value directly
// Operations on 2x uint32_t words:
uint32_t v = *word;
v ^= 0x26C05F08;
v += 0xB3B38C5;            // 188430533
v = ROL(v, 21);            // (v << 21) | (v >> 11)
*word = v;

return a1; // 64-bit decrypted UID
```

**Confirmed by**: lemonabc #24844 (reputation 536, exact match to our code), soultaken #24787
**VERIFIED IDENTICAL TO OUR ORIGINAL CODE — DO NOT CHANGE**

### 4D. decrypt_inventory_pointer — PlayerInventory

> NOTE: Two different algorithms exist in UC posts. Our build's correct algorithm needs verification.

**Option A** — soultaken #24787 (build 23657386, merged with eyes):
```cpp
// v1mper confirmed: inventory decrypt merged with eyes decrypt
uint32_t v = *word;
v -= 0xDD388D1;
v ^= 0x715CB113;
v += 0x26879A75;
*word = v;

return Il2cppGetHandle(value);
```

**Option B** — soultaken #24784 (different build):
```cpp
uint32_t v = *word;
v += 0xC2E72004;
v = ROL(v, 16);           // (v << 16) | (v >> 16)
v += 0x6A42E1BE;
*word = v;

return Il2cppGetHandle(value);
```

**Option C** — Our worked version (ROL 28):
```cpp
uint32_t v = *word;
v = ROL(v, 28);           // (v << 28) | (v >> 4)
v -= 1953166336u;          // 0x7473EB00
v ^= 0xE75BF811u;
*word = v;

return Il2cppGetHandle(value);
```

**Current approach**: Decrypt raw value from BasePlayer+0x3A8 directly (no +0x18 read) with Option C.

### 4E. FOV Encryption

```cpp
// Read from: GameAssembly + 0xE251748 → +0xB8 → +0x560
#define oConvar_Graphics 0xE251748
#define oFOVWrite 0x560

uint32_t encrypt_fov(float fov) {
    uint32_t v = *(uint32_t*)&fov;
    uint32_t t = ((v << 27) | (v >> 5)) + 662225124;
    v = (((t << 13) | (t >> 19)) ^ 0x10051515);
    return v;
}
```

**Alternative (external, bypasses ConVar)**: Write to `camera + 0x170` directly — temopzso #24865

**Confirmed by**: Claq #24821

### 4F. Camera Matrix Chain

```
GameAssembly + camera_pointer (0xE2C22B0)
  → +0xB8 (static_fields)
    → +0x88 (wrapper_class / camera_object) — fefe4444 #24841 says 0x70 for old, 0x88 for current
      → +0x10 (parent_static_fields)
        → +0x2FC (view_matrix) — CONFIRMED by fefe4444 #24841
        → +0x444 (world_pos) — fefe4444 #24841 (for debug camera)
```

---

## 5. Il2cppGetHandle — Manual Implementation

Our implementation (bitmap-based, works):
```cpp
uint64_t Il2cppGetHandle(uint64_t handle_ptr) {
    uint64_t page_base = handle_ptr & 0xFFFFFFFFFFFFE000ULL;
    uint8_t type = read<uint8_t>(page_base + 0x20);
    if (type >= 4) return 0;
    int64_t slot = (int64_t)(handle_ptr - page_base - 0x28) >> 3;
    uint32_t size = read<uint32_t>(page_base + 0x1C);
    if ((uint32_t)slot >= size) return 0;
    uint64_t bitmap_ptr = read<uint64_t>(page_base + 0x10);
    uint32_t bitmask = read<uint32_t>(bitmap_ptr + 4 * ((uint32_t)slot >> 5));
    if (!((bitmask >> (slot & 0x1F)) & 1)) return 0;
    uint64_t entry = read<uint64_t>(page_base + 8 * ((uint32_t)slot + 5));
    if (type > 1) return entry;
    return ~entry;
}
```

Madness1337's alternative (table-based):
```cpp
uint64_t Il2CppGetHandle(int32_t ObjectHandleID) {
    uint64_t rdi_1 = (ObjectHandleID >> 3);
    uint64_t rcx_1 = ((ObjectHandleID & 7) - 1);
    uintptr_t ObjectArray = read<uintptr_t>((rcx_1 * 0x28) + (game_base + il2cpp_gchandle_base + 0x8)) + (rdi_1 << 3);
    if (read<uint8_t>((rcx_1 * 0x28) + (game_base + il2cpp_gchandle_base + 0x14)) > 1)
        return read<uintptr_t>(ObjectArray);
    else {
        uint32_t eax = read<uint32_t>(ObjectArray);
        return ~eax;
    }
}
```

Our implementation is proven working for BN chain. No change needed.

---

## 6. BONE TRANSFORM CHAIN

```
BasePlayer (IS a BaseEntity)
  → +0x1A8 (BaseEntity::model) → Model* pointer
    → +0x50 (Model::boneTransforms) → Transform*[] array
      → +0x20 + (bone_index * 8) → Transform*
        → +0x10 (Transform::transform_internal) → TransformAccess
          → [SSE transform chain] → world-space Vector3
```

**CONFIRMED by**: lemonabc #24862, Madness1337 #24842, our v39 dump

### Bone Indices (Unity 6 Humanoid Rig):
Our BoneList enum has explicit Unity 6 values. `head=53`, `neck=52`, `spine1=20`.

---

## 7. UNITY 6 GOTCHAS & MIGRATION NOTES

1. **bodyAngles**: Changed from Vector2 → Vector3 at PlayerInput+0x44. Write `Vector3(pitch, yaw, 0)` not `Vector2`.

2. **PlayerEyes.bodyRotation**: Write quaternion to `PlayerEyes+0x50` for aimbot. Survives Unity Input System overwrite.

3. **ListDictionary.vals**: Changed from 0x20 → 0x18. Breaks BN chain if wrong.

4. **FOV external bypass**: Write to `camera + 0x170` directly instead of ConVar Graphics encrypt — avoids detection. temopzso #24865.

5. **Item amount**: 0x50 (dumpers broke and got 0x4C). JJSR #24864.

6. **Belt unaligned addresses**: Some players use inline storage (aligned) vs pointer-based (unaligned). CherioLov #24544.

7. **ViewModel GameObject chain**: `baseProjectile+0x268 → +0x10 → +0x20` for GetComponentsInChildren. Changed in Unity 6.

8. **CameraUpdateHook**: Set pre-render to 0 at `0xE2E6AB8 + 0xB8 + 0x80` for FOV without decrypt.

9. **Entity wrapper offset**: 0x18 NOT 0x10. clovics #24843 confirmed.

10. **PlayerInventory decrypt may be merged with eyes** per v1mper (build 23657386). Same constants: SUB 0xDD388D1, XOR 0x715CB113, ADD 0x26879A75.

---

## 8. USEFUL UC POSTS INDEX

### Authoritative for our build:
| Post# | Page | Date | Author | Content |
|-------|------|------|--------|---------|
| #24803 | 1241 | Jun 10 | xiaoxiao1 | Full dump with TypeInfo RVAs, TypeDefIndex |
| #24844 | 1243 | Jun 11 | lemonabc | cl_active_item decrypt + HeldItem=0x80 |
| #24846 | 1243 | Jun 11 | lemonabc | BN chain decrypts + Il2cppGetHandle |
| #24847 | 1243 | Jun 11 | NafNaf123 | CRITICAL: list_dict_vals = 0x18 fix |
| #24821 | 1242 | Jun 10 | Claq | FOV encrypt, oConvar_Graphics=0xE251748 |
| #24787 | 1240 | Jun 10 | soultaken | Updated dump for build 23657386 |
| #24788 | 1240 | Jun 10 | NafNaf123 | Simplified decrypt functions |
| #24841 | 1243 | Jun 11 | fefe4444 | BN/camera chain offsets |
| #24864 | 1244 | Jun 11 | JJSR | Item amount = 0x50 |
| #24865 | 1244 | Jun 11 | temopzso | FOV at camera+0x170, bypass ConVar |

### Informational but different builds:
| Post# | Page | Date | Author | Content |
|-------|------|------|--------|---------|
| #24681 | 1235 | Jun 6 | v1mper | Mega dump — ALL BasePlayer offsets different |
| #24784 | 1240 | Jun 10 | soultaken | Earlier build, different offsets & decrypts |
| #24544 | 1228 | Jun 4 | CherioLov | Unity 6 viewModel/belt issues |
| #24553 | 1228 | Jun 4 | rustslayer4123 | Older decrypts & offsets |
| #24762 | 1239 | Jun 9 | Claq | Older FOV key (0xE1C7880) |
| #24764 | 1239 | Jun 9 | WaterTheDev | PlayerWalkMovement encrypt/decrypt pairs |
| #24863 | 1244 | Jun 11 | MrKouhx | CameraUpdateHook pre-render FOV |

---

## 9. PATTERN — HOW TO VERIFY OFFSETS AFTER UPDATE

1. Generate fresh dump with Il2CppDumper v39 (roytu fork)
2. Check `script.json` for `il2cpp_gchandle_get_target` RVA → our `offsets::Il2cppGetHandle`
3. Find `BaseNetworkable` static class → `TypeInfoRVA` = `offsets::basenetworkable_pointer`
4. Verify: `static_fields=0xB8`, `wrapper_class=0x30`, `entity=0x18`
5. Verify BasePlayer: `PlayerInput=0x2E8`, `PlayerEyes=0x3E0`, `PlayerInventory=0x3A8`, `PlayerModel=0x470`
6. Check decrypt constants against `il2cpp_dump.cs` decompiled encrypt functions
7. Cross-reference with latest UC posts from highest-rep users (xiaoxiao1, lemonabc, NafNaf123, temopzso)
8. NEVER blindly apply offsets from v1mper or other dumps without verifying against our GameAssembly.dll
