#!/usr/bin/env python3
"""
morphine_to_cheat.py — Convert morphine-dumper2 output to cheat format

Converts the C++ header file produced by morphine-dumper2 into:
  1. rust_decrypts.dat  — Runtime decrypt config (key=value, no recompile needed)
  2. updated_offsets.txt — All offsets mapped to cheat namespace for manual offsets.hpp update

Usage:
    python morphine_to_cheat.py <morphine_output.h>
    python morphine_to_cheat.py <morphine_output.h> --output-dir <dir>

If no --output-dir, outputs to current directory.

The morphine output file is typically morphine-dumper_output.h, found on Desktop
after injecting morphine-dumper.dll into the Rust game process.
"""

import re
import sys
import os
import argparse
from pathlib import Path
from collections import OrderedDict


# ============================================================
# DECRYPT KEY MAPPING
# Maps morphine dumper function name → list of (cheat_key, op_type) tuples
# op_type: 'add', 'sub', 'xor', 'rol'
# ============================================================

DECRYPT_PREFIX = {
    'client_entities': 'nk',
    'entity_list':     'nk2',
    'clActiveItem':    'cla',
    'PlayerInventory': 'inv',
    'PlayerEyes':      'ey',
    'decrypt_fov':     'fov',
}

def generate_decrypt_keys(prefix, ops):
    """Generate key names dynamically based on operation types found.
    
    Convention:
    - If a type appears once:  <prefix>_<type>   (e.g., nk_xor, nk_sub)
    - If a type appears N times: <prefix>_<type>1..N  (e.g., nk2_rol1, nk2_rol2)
    """
    type_counts = {}
    for op_type, _ in ops:
        type_counts[op_type] = type_counts.get(op_type, 0) + 1
    
    type_seen = {}
    keys = []
    for op_type, _ in ops:
        if type_counts[op_type] > 1:
            idx = type_seen.get(op_type, 0) + 1
            type_seen[op_type] = idx
            keys.append(f"{prefix}_{op_type}{idx}")
        else:
            keys.append(f"{prefix}_{op_type}")
    return keys


# ============================================================
# OFFSET FIELD MAPPING
# Maps morphine dumper struct.field → (cheat_namespace, cheat_field, description)
# ============================================================

OFFSET_MAP = OrderedDict([
    # Static pointers (RVAs)
    ('GameAssembly.timestamp',              ('_static', 'ga_timestamp',          'GameAssembly timestamp (for PE scan)')),
    ('gc_handles.array_rva',                ('_static', 'il2cpp_get_handle',     'Il2cppGetHandle gc_handles array RVA')),
    ('klass_rvas.BaseNetworkable',          ('_static', 'basenetworkable_pointer','BaseNetworkable TypeInfo RVA')),
    ('klass_rvas.MainCamera',               ('_static', 'camera_pointer',        'MainCamera TypeInfo RVA')),
    ('klass_rvas.TOD_Sky',                  ('_static', 'tod_sky_typeinfo',      'TOD_Sky Static TypeInfo RVA')),
    ('klass_rvas.PlayerEyes',               ('_static', 'playereyes_typeinfo',   'PlayerEyes TypeInfo RVA (unused)')),
    ('klass_rvas.PlayerInventory',          ('_static', 'playerinv_typeinfo',     'PlayerInventory TypeInfo RVA (unused)')),
    ('klass_rvas.PlayerModel',              ('_static', 'playermodel_typeinfo',  'PlayerModel TypeInfo RVA (unused)')),
    ('klass_rvas.BasePlayer',               ('_static', 'baseplayer_typeinfo',   'BasePlayer TypeInfo RVA (unused)')),
    ('klass_rvas.BaseEntity',               ('_static', 'baseentity_typeinfo',   'BaseEntity TypeInfo RVA (unused)')),
    ('klass_rvas.BaseCombatEntity',         ('_static', 'basecombatent_typeinfo','BaseCombatEntity TypeInfo RVA (unused)')),
    ('klass_rvas.ModelState',              ('_static', 'modelstate_typeinfo',   'ModelState TypeInfo RVA (unused)')),
    ('klass_rvas.PlayerInput',              ('_static', 'playerinput_typeinfo',  'PlayerInput TypeInfo RVA (unused)')),
    ('klass_rvas.BaseProjectile',           ('_static', 'baseproj_typeinfo',     'BaseProjectile TypeInfo RVA (unused)')),
    ('klass_rvas.HeldEntity',               ('_static', 'heldentity_typeinfo',   'HeldEntity TypeInfo RVA (unused)')),
    ('klass_rvas.AutoTurret',               ('_static', 'autoturret_typeinfo',   'AutoTurret TypeInfo RVA (unused)')),
    ('klass_rvas.Item',                     ('_static', 'item_typeinfo',         'Item TypeInfo RVA (unused)')),
    ('klass_rvas.ItemId',                   ('_static', 'itemid_typeinfo',       'ItemId TypeInfo RVA (unused)')),

    # Singleton typeinfos
    ('singleton_typeinfos.SingletonComponent_UI_LoadingScreen',    ('_static', 'ui_loadingscreen_typeinfo', 'UI_LoadingScreen singleton RVA')),
    ('singleton_typeinfos.SingletonComponent_MixerSnapshotManager',('_static', 'mixersnapshot_typeinfo',   'MixerSnapshotManager singleton RVA')),

    # EffectNetwork
    ('EffectNetwork.static_type_info',      ('_static', 'effectnetwork_pointer', 'EffectNetwork static_type_info RVA')),
    ('EffectNetwork.type_info',             ('_static', 'effectnetwork_typeinfo','EffectNetwork type_info RVA (unused)')),

    # ConVar Graphics
    ('convar_graphics.convar_graphics',     ('_static', 'convar_graphics',        'ConVar_Graphics RVA')),

    # BaseNetworkable
    ('base_networkable.static_fields',      ('BaseNetworkable', 'static_fields',       'static_fields offset')),
    ('base_networkable.wrapper_class_ptr',  ('BaseNetworkable', 'wrapper_class',       'wrapper_class offset')),
    ('base_networkable.parent_static_fields',('BaseNetworkable', 'parent_static_fields','parent_static_fields offset')),
    ('base_networkable.entities',           ('BaseNetworkable', 'entity',              'entities offset in parent_class')),
    ('base_networkable.children',           ('BaseNetworkable', 'children',            'children list offset')),
    ('base_networkable.prefabID',           ('BaseNetworkable2','prefab_id',           'prefabID offset')),
    ('base_networkable.hv_offset',          ('BaseNetworkable', 'encrypted_handle',    'encrypted handle offset (unused)')),

    # Camera
    ('camera.camera_static',               ('BaseCamera', 'static_fields',     'static_fields offset')),
    ('camera.camera_object',               ('BaseCamera', 'wrapper_class',      'camera_object/instance offset')),
    ('camera.entity',                      ('BaseCamera', 'entity',             'native Camera deref offset')),
    ('camera.viewMatrix',                  ('BaseCamera', 'viewMatrix',         'view matrix offset')),
    ('camera.projectionMatrix',             ('BaseCamera', 'projectionMatrix',   'projection matrix offset')),
    ('camera.position',                    ('BaseCamera', 'world_position',     'world position offset')),
    ('camera.fieldOfView',                 ('BaseCamera', 'field_of_view',      'field of view offset')),
    ('camera.cullingMask',                 ('BaseCamera', 'culling_mask',       'culling mask (unused — RemoveLayers removed)')),

    # BasePlayer
    ('BasePlayer.clActiveItem',             ('BasePlayer', 'ClActiveItem',      'ClActiveItem offset')),
    ('BasePlayer.PlayerEyes',              ('BasePlayer', 'PlayerEyes',         'PlayerEyes offset')),
    ('BasePlayer.PlayerInventory',         ('BasePlayer', 'PlayerInventory',    'PlayerInventory offset')),
    ('BasePlayer.player_input',            ('BasePlayer', 'PlayerInput',        'PlayerInput offset')),
    ('BasePlayer.player_model',            ('BasePlayer', 'PlayerModel',        'PlayerModel offset')),
    ('BasePlayer.player_flags',            ('BasePlayer', 'PlayerFlags',         'PlayerFlags offset')),
    ('BasePlayer.base_movement',           ('BasePlayer', 'BaseMovement',       'BaseMovement offset')),
    ('BasePlayer.display_name',            ('BasePlayer', 'DisplayName',        'DisplayName offset')),
    ('BasePlayer.current_team',            ('BasePlayer', 'CurrentTeam',        'CurrentTeam offset')),
    ('BasePlayer.mounted',                 ('BasePlayer', 'Mounted',            'Mounted offset')),
    ('BasePlayer.model_state',             ('BasePlayer', 'ModelState',         'ModelState offset')),
    ('BasePlayer.ModelTransform',          ('BasePlayer', 'ModelTransform',     'ModelTransform offset')),

    # BaseCombatEntity
    ('BaseCombatEntity.lifestate',         ('BaseCombatEntity', 'Lifestate',   'Lifestate offset')),
    ('BaseCombatEntity._health',           ('BaseCombatEntity', 'Health',       'Health offset')),
    ('BaseCombatEntity._maxHealth',        ('BaseCombatEntity', 'MaxHealth',    'MaxHealth offset')),
    ('BaseCombatEntity.model',             ('BaseCombatEntity', 'model',        'model offset')),
    ('BaseCombatEntity.skeletonProperties', ('BaseCombatEntity', 'skeletonProperties', 'skeletonProperties (unused)')),
    ('BaseCombatEntity.baseProtection',    ('BaseCombatEntity', 'baseProtection','baseProtection (unused)')),

    # BaseEntity
    ('BaseEntity.positionLerp',            ('BaseEntity', 'positionLerp',      'positionLerp offset')),
    ('BaseEntity.bounds',                  ('BaseEntity', 'bounds',             'bounds offset')),
    ('BaseEntity.flags',                   ('BaseEntity', 'flags',              'flags offset')),
    ('BaseEntity.model',                   ('BaseEntity', 'model',              'model offset')),

    # PlayerInventory
    ('PlayerInventory.containerBelt',      ('PlayerInventory', 'Belt',          'Belt container offset')),
    ('PlayerInventory.containerWear',      ('PlayerInventory', 'Wear',          'Wear container offset')),
    ('PlayerInventory.containerMain',      ('PlayerInventory', 'Main',          'Main container offset')),
    ('PlayerInventory.loot',               ('PlayerInventory', 'loot',           'loot container offset')),

    # ItemContainer
    ('ItemContainer.ItemList',             ('ItemContainer', 'ItemList',       'ItemList offset')),
    ('ItemContainer.flags',                ('ItemContainer', 'flags',            'flags (unused)')),

    # Item
    ('item.info',                          ('Item', 'ItemDefinition',            'ItemDefinition offset')),
    ('item.uid',                           ('Item', 'ItemId',                   'ItemId offset')),
    ('item.amount',                        ('Item', 'Amount',                    'Amount offset')),
    ('item.HeldEntity',                    ('Item', 'HeldEntity_1',              'HeldEntity offset')),

    # ItemDefinition
    ('ItemDefinition.itemid',              ('ItemDefinition', 'itemid',          'itemid offset')),
    ('ItemDefinition.shortname',           ('ItemDefinition', 'shortname',       'shortname offset')),
    ('ItemDefinition.displayName',        ('ItemDefinition', 'displayName',     'displayName offset')),
    ('ItemDefinition.category',           ('ItemDefinition', 'category',         'category offset')),
    ('ItemDefinition.stackable',           ('ItemDefinition', 'stackable',       'stackable offset')),

    # PlayerModel
    ('PlayerModel._multiMesh',            ('PlayerModel', 'SkinnedMultiMesh',   'SkinnedMultiMesh offset')),
    ('PlayerModel.isNpc',                  ('PlayerModel', 'is_npc',             'isNpc offset')),
    ('PlayerModel.position',               ('PlayerModel', 'position',           'position offset')),
    ('PlayerModel.newVelocity',            ('PlayerModel', 'velocity',           'velocity offset')),
    ('PlayerModel.visible',                ('PlayerModel', 'visible',            'visible offset')),
    ('PlayerModel.boneTransforms',         ('PlayerModel', 'boneTransforms',      'boneTransforms offset')),

    # Model
    ('model.boneTransforms',              ('Model', 'boneTransforms',            'boneTransforms offset')),
    ('model.rootBone',                     ('Model', 'rootBone',                 'rootBone offset')),
    ('model.headBone',                     ('Model', 'headBone',                 'headBone offset')),
    ('model.eyeBone',                      ('Model', 'eyeBone',                  'eyeBone offset')),

    # HeldEntity
    ('HeldEntity.ownerItemUID',            ('HeldEntity', 'ownerItemUID',        'ownerItemUID offset')),
    ('HeldEntity.viewModel',               ('HeldEntity', 'viewModel',           'viewModel offset')),

    # RecoilProperties
    ('RecoilProperties.recoilYawMin',      ('RecoilProperties', 'RecoilYawMin',  'RecoilYawMin offset')),
    ('RecoilProperties.recoilYawMax',      ('RecoilProperties', 'RecoilYawMax',  'RecoilYawMax offset')),
    ('RecoilProperties.recoilPitchMin',    ('RecoilProperties', 'RecoilPitchMin','RecoilPitchMin offset')),
    ('RecoilProperties.recoilPitchMax',    ('RecoilProperties', 'RecoilPitchMax','RecoilPitchMax offset')),
    ('RecoilProperties.aimconeCurveScale', ('RecoilProperties', 'AimconeCurveScale','AimconeCurveScale offset')),
    ('RecoilProperties.newRecoilOverride', ('RecoilProperties', 'NewRecoilOverride','NewRecoilOverride offset')),

    # BaseProjectile
    ('BaseProjectile.recoil',              ('BaseProjectile', 'recoil',           'recoil offset')),
    ('BaseProjectile.primaryMagazine',     ('BaseProjectile', 'primaryMagazine',  'primaryMagazine offset')),
    ('BaseProjectile.aimCone',             ('BaseProjectile', 'AimCone',          'AimCone offset')),
    ('BaseProjectile.hipAimCone',          ('BaseProjectile', 'HipAimCone',       'HipAimCone offset')),
    ('BaseProjectile.hasADS',              ('BaseProjectile', 'HasADS',           'HasADS offset')),
    ('BaseProjectile.automatic',           ('BaseProjectile', 'automatic',       'automatic offset')),
    ('BaseProjectile.isReloading',          ('BaseProjectile', 'is_reloading',     'is_reloading offset')),
    ('BaseProjectile.reloadTime',          ('BaseProjectile', 'reload_time',      'reload_time offset')),
    ('BaseProjectile.projectileVelocityScale',('BaseProjectile','velocity_scale', 'velocity_scale offset')),
    ('BaseProjectile.aimSway',             ('BaseProjectile', 'aimSway',         'aimSway offset')),
    ('BaseProjectile.aimSwaySpeed',        ('BaseProjectile', 'aimSwaySpeed',     'aimSwaySpeed offset')),
    ('BaseProjectile.stancePenalty',       ('BaseProjectile', 'StancePenalty',   'StancePenalty offset')),
    ('BaseProjectile.sightAimConeScale',    ('BaseProjectile', 'SightAimConeScale','SightAimConeScale offset')),
    ('BaseProjectile.aimconePenalty',      ('BaseProjectile', 'AimconePenalty',  'AimconePenalty offset')),
    ('BaseProjectile.isBurstWeapon',       ('BaseProjectile', 'isBurstWeapon',   'isBurstWeapon offset')),
    ('BaseProjectile.canChangeFireModes',  ('BaseProjectile', 'canChangeFireModes','canChangeFireModes offset')),
    ('BaseProjectile.internalBurstFireRateScale',('BaseProjectile','internalBurstFireRateScale','internalBurstFireRateScale (unused)')),

    # BaseMovement
    ('base_movement.adminCheat',           ('BaseMovement2', 'admin_cheat',       'admin_cheat offset')),
    ('PlayerWalkMovement.TargetMovement',  ('BaseMovement2', 'target_movement',  'target_movement offset')),

    # TOD_Sky
    ('TOD_Sky_Static.instance',            ('TOD_Sky_Static', 'instances',        'instances offset in static_fields')),
    ('TOD_Sky_Static.tod_sky_c',           ('_static', 'tod_sky_typeinfo',         'TOD_Sky Static TypeInfo RVA (duplicate)')),
    ('TOD_Sky.Cycle',                      ('TOD_Sky', 'CycleParameters',         'CycleParameters offset')),
    ('TOD_Sky.Night',                      ('TOD_Sky', 'NightParameters',         'NightParameters offset')),
    ('TOD_Sky.Ambient',                    ('TOD_Sky', 'AmbientParameters',       'AmbientParameters offset')),
    ('TOD_Sky.timeSinceAmbientUpdate',     ('TOD_Sky', 'timeSinceAmbientUpdate',  'timeSinceAmbientUpdate offset')),

    # TOD parameters
    ('tod_night_parameters.AmbientMultiplier',('TOD_NightParameters', 'AmbientMultiplier','AmbientMultiplier offset')),
    ('tod_ambient_parameters.Saturation',  ('TOD_AmbientParameters', 'Saturation','Saturation offset')),
    ('tod_ambient_parameters.UpdateInterval',('TOD_AmbientParameters', 'UpdateInterval','UpdateInterval offset')),

    # PlayerInput
    ('PlayerInput.bodyAngles',             ('PlayerInput', 'bodyAngles',          'bodyAngles offset')),
    ('PlayerInput.yaw',                    ('PlayerInput', 'yaw',                 'yaw offset')),

    # PlayerEyes
    ('PlayerEyes.viewOffset',              ('PlayerEyes', 'viewOffset',           'viewOffset offset')),
    ('PlayerEyes.bodyRotation',            ('PlayerEyes', 'bodyRotation',         'bodyRotation offset')),
    ('PlayerEyes.worldPosition',           ('PlayerEyes', 'headPosition',         'headPosition offset')),

    # FOV
    ('convar_graphics.fov',                ('FOV', 'fovWrite',                   'FOV write offset (convar_graphics::fov)')),

    # EffectNetwork fields
    ('EffectNetwork.static_fields',        ('EffectNetwork', 'static_fields',     'static_fields offset')),
    ('EffectNetwork.instance',             ('EffectNetwork', 'instance',          'instance offset')),
    ('EffectNetwork.hitPosition',          ('EffectNetwork', 'hitPosition',       'hitPosition offset')),

    # ModelState flags (bit positions, not offsets)
    ('ModelState.Flying',                  ('ModelState', 'Flying',               'Flying bit position')),
    ('ModelState.OnGround',                ('ModelState', 'OnGround',             'OnGround bit position')),
    ('ModelState.Sleeping',                ('ModelState', 'Sleeping',             'Sleeping bit position (unused)')),
    ('ModelState.Mounted',                 ('ModelState', 'Mounted',              'Mounted bit position (unused)')),
    ('ModelState.Ducked',                  ('ModelState', 'Ducked',               'Ducked bit position')),

    # Transform data (Unity internals)
    ('transform_data.pos_base_off',        ('BaseEntity', 'posChain0',            'transform pos_base_off')),
    ('transform_data.root_pos_off',        ('BaseEntity', 'posFinal',             'transform root_pos_off')),
    ('unity_transform_native.access_struct_off',('BaseEntity', 'posChain2',      'transform access_struct_off')),
])


# ============================================================
# PARSING FUNCTIONS
# ============================================================

def extract_decrypt_function(content, func_name):
    """Extract a decryption function's body from the C++ header."""
    # Match: uintptr_t decryption::FUNC_NAME(...) { ... }
    # The function may use different variable names (ecx/edx/val/rax etc)
    pattern = rf'(?:uintptr_t|uint32_t|inline\s+uint32_t)\s+decryption::{re.escape(func_name)}\s*\([^)]*\)\s*\{{'
    match = re.search(pattern, content)
    if not match:
        return None

    start = match.end()
    # Find matching closing brace
    depth = 1
    pos = start
    while pos < len(content) and depth > 0:
        if content[pos] == '{':
            depth += 1
        elif content[pos] == '}':
            depth -= 1
        pos += 1
    return content[start:pos-1]


def parse_decrypt_ops(func_body):
    """Parse crypto operations from a decryption function body.
    
    Returns list of (op_type, value) tuples.
    op_type: 'add', 'sub', 'xor', 'rol'
    """
    ops = []
    
    # Remove comments
    body = re.sub(r'//[^\n]*', '', func_body)
    body = re.sub(r'/\*.*?\*/', '', body, flags=re.DOTALL)
    
    # Pattern 1: ROL via shift+shift+or (two-line or three-line form)
    # ecx = ecx << N; eax = eax >> M; ecx = ecx | eax;
    # OR: ecx = (ecx << N) | (ecx >> M)  (single line)
    # OR: edx = (edx << N) | (edx >> M)
    # The ROL amount is N (left shift), M should be 32-N
    
    # Pattern for multi-line ROL:
    # var = var << N
    # other = other >> M
    # var = var | other
    rol_pattern_multi = re.compile(
        r'(\w+)\s*=\s*\1\s*<<\s*(0x[0-9A-Fa-f]+|\d+)\s*;\s*'
        r'(?:\w+\s*=\s*\w+\s*>>\s*(0x[0-9A-Fa-f]+|\d+)\s*;\s*)?'
        r'\1\s*=\s*\1\s*\|\s*\w+\s*;'
    )
    
    # Pattern for single-line ROL:
    # var = (var << N) | (var >> M)
    rol_pattern_single = re.compile(
        r'(\w+)\s*=\s*\(\s*\1\s*<<\s*(0x[0-9A-Fa-f]+|\d+)\s*\)\s*\|\s*\(\s*\1\s*>>\s*(0x[0-9A-Fa-f]+|\d+)\s*\)'
    )
    
    # Pattern for ADD: var = var + CONST  OR  var += CONST
    add_pattern = re.compile(r'(\w+)\s*(?:\+=|\s*=\s*\1\s*\+)\s*(0x[0-9A-Fa-f]+|\d+)\s*;')
    
    # Pattern for SUB: var = var - CONST  OR  var -= CONST
    sub_pattern = re.compile(r'(\w+)\s*(?:-=|\s*=\s*\1\s*-)\s*(0x[0-9A-Fa-f]+|\d+)\s*;')
    
    # Pattern for XOR: var = var ^ CONST
    xor_pattern = re.compile(r'(\w+)\s*(?:\^=|\s*=\s*\1\s*\^)\s*(0x[0-9A-Fa-f]+|\d+)\s*;')
    
    # We need to process the body line by line, tracking state for multi-line ROL
    lines = body.split('\n')
    i = 0
    while i < len(lines):
        line = lines[i].strip()
        
        # Try single-line ROL
        m = rol_pattern_single.search(line)
        if m:
            shift_val = m.group(2)
            ops.append(('rol', int(shift_val, 0)))
            i += 1
            continue
        
        # Try multi-line ROL: look for "var = var << N" followed by "other = other >> M" and "var = var | other"
        m = re.match(r'(\w+)\s*=\s*\1\s*<<\s*(0x[0-9A-Fa-f]+|\d+)\s*;', line)
        if m:
            var_name = m.group(1)
            shift_val = int(m.group(2), 0)
            # Check if next lines complete the ROL
            if i + 2 < len(lines):
                next1 = lines[i+1].strip()
                next2 = lines[i+2].strip()
                # Pattern: eax = ecx; ecx = ecx << N; eax = eax >> M; ecx = ecx | eax;
                # Or: ecx = ecx << N; ... eax = eax >> M; ecx = ecx | eax;
                if (re.search(r'\w+\s*=\s*\w+\s*>>\s*(?:0x[0-9A-Fa-f]+|\d+)', next1) and
                    re.search(rf'{var_name}\s*=\s*{var_name}\s*\|', next2)):
                    ops.append(('rol', shift_val))
                    i += 3
                    continue
            # Also check: "eax = ecx; ecx = ecx << N; eax = eax >> M; ecx = ecx | eax;"
            # where the "eax = ecx" line was before this one
            if i + 1 < len(lines):
                next1 = lines[i+1].strip()
                if re.search(r'\w+\s*=\s*\w+\s*>>\s*(?:0x[0-9A-Fa-f]+|\d+)', next1):
                    # Look ahead for OR
                    if i + 2 < len(lines):
                        next2 = lines[i+2].strip()
                        if re.search(rf'{var_name}\s*=\s*{var_name}\s*\|', next2):
                            ops.append(('rol', shift_val))
                            i += 3
                            continue
            # Fallback: just record the ROL from the shift
            ops.append(('rol', shift_val))
            i += 1
            continue
        
        # Try ADD
        m = add_pattern.search(line)
        if m:
            val = int(m.group(2), 0)
            # Skip if it's a small value used for pointer arithmetic (like + 0x4 for rdx increment)
            if val > 0xFF or (val >= 0x10 and val <= 0x100):
                pass  # Could be a real operation
            if val >= 0x10000:  # Only large constants are crypto
                ops.append(('add', val))
                i += 1
                continue
        
        # Try SUB
        m = sub_pattern.search(line)
        if m:
            val = int(m.group(2), 0)
            if val >= 0x10000:
                ops.append(('sub', val))
                i += 1
                continue
        
        # Try XOR
        m = xor_pattern.search(line)
        if m:
            val = int(m.group(2), 0)
            if val >= 0x10000:
                ops.append(('xor', val))
                i += 1
                continue
        
        i += 1
    
    return ops


def parse_struct_fields(content):
    """Parse all struct/namespace fields from the C++ header.
    
    Returns dict: {struct_name.field_name: value}
    """
    fields = {}
    
    # Match struct/namespace blocks:
    # struct NAME { ... } or namespace NAME { ... }
    # Fields: inline static constexpr uintptr_t FIELD = VALUE;
    # or: constexpr std::uintptr_t FIELD = VALUE;
    
    struct_pattern = re.compile(
        r'(?:struct|namespace)\s+(\w+)\s*\{',
        re.MULTILINE
    )
    
    for struct_match in struct_pattern.finditer(content):
        struct_name = struct_match.group(1)
        start = struct_match.end()
        
        # Find matching closing brace
        depth = 1
        pos = start
        while pos < len(content) and depth > 0:
            if content[pos] == '{':
                depth += 1
            elif content[pos] == '}':
                depth -= 1
            pos += 1
        
        body = content[start:pos-1]
        
        # Parse fields
        # Pattern: (inline static constexpr|constexpr|inline) (uintptr_t|std::uintptr_t|uint32_t|std::uint32_t|int|float) FIELD = VALUE;
        field_pattern = re.compile(
            r'(?:inline\s+)?(?:static\s+)?(?:constexpr\s+)?(?:std::)?(?:uintptr_t|uint32_t|int|float)\s+(\w+)\s*=\s*(0x[0-9A-Fa-f]+|\d+\.?\d*[fF]?)\s*;'
        )
        
        for field_match in field_pattern.finditer(body):
            field_name = field_match.group(1)
            field_value = field_match.group(2)
            key = f"{struct_name}.{field_name}"
            fields[key] = field_value
    
    return fields


def parse_build_id(content):
    """Extract build ID from the header."""
    m = re.search(r'Build\s*=\s*"(\d+)"', content)
    if m:
        return m.group(1)
    return "unknown"


def parse_game_timestamp(content):
    """Extract GameAssembly timestamp."""
    m = re.search(r'timestamp\s*=\s*(0x[0-9A-Fa-f]+)', content)
    if m:
        return m.group(1)
    return None


# ============================================================
# OUTPUT GENERATION
# ============================================================

def generate_decrypts_dat(decrypt_results, output_path):
    """Generate rust_decrypts.dat file."""
    lines = [
        "# rust_decrypts.dat — Auto-generated from morphine-dumper2 output",
        "# Format: key=value (one per line, hex values with 0x prefix)",
        r"# Place this file in the DLL directory or C:\ to override embedded decrypt defaults",
        "",
    ]
    
    for func_name, ops in decrypt_results.items():
        if func_name not in DECRYPT_PREFIX:
            continue
        
        prefix = DECRYPT_PREFIX[func_name]
        keys = generate_decrypt_keys(prefix, ops)
        
        for i, key in enumerate(keys):
            actual_type, value = ops[i]
            lines.append(f"{key}=0x{value:X}")
        
        lines.append("")
    
    with open(output_path, 'w', newline='\n') as f:
        f.write('\n'.join(lines))
    
    return len([l for l in lines if '=' in l and not l.startswith('#')])


def generate_offsets_txt(fields, decrypt_results, build_id, timestamp, output_path):
    """Generate updated_offsets.txt with all offsets mapped to cheat names."""
    lines = [
        "=" * 70,
        "Updated Offsets from morphine-dumper2",
        "=" * 70,
        f"Build: {build_id}",
        f"GameAssembly timestamp: {timestamp or 'not found'}",
        "",
        "This file contains all offsets extracted from the morphine-dumper2 output,",
        "mapped to the cheat's namespace::field naming convention.",
        "Use this to manually update offsets.hpp, or use rust_decrypts.dat for",
        "runtime decrypt override (no recompile needed).",
        "",
        "=" * 70,
        "DECRYPT CONSTANTS (rust_decrypts.dat)",
        "=" * 70,
        "",
    ]
    
    for func_name, ops in decrypt_results.items():
        if func_name not in DECRYPT_PREFIX:
            continue
        prefix = DECRYPT_PREFIX[func_name]
        keys = generate_decrypt_keys(prefix, ops)
        lines.append(f"  {func_name}:")
        for i, key in enumerate(keys):
            if i < len(ops):
                actual_type, value = ops[i]
                lines.append(f"    {key} = 0x{value:X}  ({actual_type})")
            else:
                lines.append(f"    {key} = MISSING")
        lines.append("")
    
    lines.extend([
        "=" * 70,
        "STATIC POINTERS (update in offsets.hpp)",
        "=" * 70,
        "",
    ])
    
    for dumper_key, (ns, field, desc) in OFFSET_MAP.items():
        if ns != '_static':
            continue
        if dumper_key in fields:
            lines.append(f"  {field} = {fields[dumper_key]}  // {desc}")
        else:
            lines.append(f"  # {field} — NOT FOUND in dumper output  // {desc}")
    lines.append("")
    
    lines.extend([
        "=" * 70,
        "FIELD OFFSETS (update in offsets.hpp)",
        "=" * 70,
        "",
    ])
    
    current_ns = None
    for dumper_key, (ns, field, desc) in OFFSET_MAP.items():
        if ns == '_static':
            continue
        if ns != current_ns:
            current_ns = ns
            lines.append(f"  namespace {ns} {{")
        
        if dumper_key in fields:
            lines.append(f"    {field} = {fields[dumper_key]};  // {desc}")
        else:
            lines.append(f"    // {field} — NOT FOUND  // {desc}")
    
    lines.append("  }")
    lines.append("")
    
    lines.extend([
        "=" * 70,
        "OFFSETS NOT IN DUMPER (hardcoded — Unity internals, rarely change)",
        "=" * 70,
        "",
        "  BaseEntity::isVisible        = 0x150   // Unity occlusion flag",
        "  BaseEntity::isAnimatorVisible= 0x151   // Unity animator flag",
        "  BaseEntity::isShadowVisible  = 0x152   // Unity shadow flag",
        "  BaseEntity::objRef           = 0x10    // Unity Object.m_CachedPtr",
        "  BaseEntity::posChain0        = 0x20    // Unity transform chain",
        "  BaseEntity::posChain1        = 0x20",
        "  BaseEntity::posChain2        = 0x8",
        "  BaseEntity::posChain3        = 0x28",
        "  BaseEntity::posFinal         = 0x90",
        "  ItemMagazine::contents       = 0x1C",
        "  PlayerInventory::BeltFallback1 = 0x78  // = Wear (fallback)",
        "  PlayerInventory::BeltFallback2 = 0x30  // = Main (fallback)",
        "  ItemContainer::ItemListFallback = 0x10",
        "  Item::ItemIdFallback1        = 0x38",
        "  Item::ItemIdFallback2        = 0x70",
        "",
        "=" * 70,
        "INSTRUCTIONS",
        "=" * 70,
        "",
        "1. DECRYPTS: Copy rust_decrypts.dat to DLL directory or C:\\",
        "   The cheat loads this at startup (OffsetManager::LoadDecryptConfig).",
        "   No recompile needed — decrypts are runtime-overridable.",
        "",
        "2. STATIC POINTERS: Update offsets.hpp with the new RVA values.",
        "   These MUST be recompiled into the DLL (not runtime-overridable).",
        "",
        "3. FIELD OFFSETS: Update offsets.hpp with new offset values.",
        "   These MUST be recompiled into the DLL.",
        "",
        "4. HARDcoded offsets (Unity internals) do NOT change with game updates,",
        "   only with Unity engine version upgrades. Safe to leave as-is.",
        "",
    ])
    
    with open(output_path, 'w', newline='\n') as f:
        f.write('\n'.join(lines))


# ============================================================
# AUTO-APPLY: Patch offsets.hpp + Driver.hpp
# ============================================================

# Mapping from dumper keys to offsets.hpp top-level variable names
STATIC_POINTER_MAP = {
    'klass_rvas.BaseNetworkable':           'basenetworkable_pointer',
    'klass_rvas.MainCamera':                'camera_pointer',
    'gc_handles.array_rva':                 'Il2cppGetHandle',
    'klass_rvas.TOD_Sky':                   ['TOD_Sky_TypeInfo', 'Class_TOD_Sky_Static'],
    'EffectNetwork.static_type_info':       'EffectNetwork_Pointer',
    'singleton_typeinfos.SingletonComponent_UI_LoadingScreen':    'Class_SingletonComponent_UI_LoadingScreen',
    'singleton_typeinfos.SingletonComponent_MixerSnapshotManager': 'Class_SingletonComponent_MixerSnapshotManager__c',
}

# Mapping from dumper keys to (offsets.hpp namespace, field_name)
# for offsets that live inside namespace blocks
FIELD_OFFSET_MAP = {
    'convar_graphics.convar_graphics':      ('FOV', 'ConVar_Graphics'),
    'convar_graphics.fov':                  ('FOV', 'fovWrite'),
    'base_networkable.static_fields':      ('BaseNetworkable', 'static_fields'),
    'base_networkable.wrapper_class_ptr':   ('BaseNetworkable', 'wrapper_class'),
    'base_networkable.parent_static_fields':('BaseNetworkable', 'parent_static_fields'),
    'base_networkable.entities':           ('BaseNetworkable', 'entity'),
    'base_networkable.children':           ('BaseNetworkable', 'children'),
    'base_networkable.prefabID':           ('BaseNetworkable2', 'prefab_id'),
    'camera.camera_static':                ('BaseCamera', 'static_fields'),
    'camera.camera_object':               ('BaseCamera', 'wrapper_class'),
    'camera.entity':                      ('BaseCamera', 'entity'),
    'camera.viewMatrix':                   ('BaseCamera', 'viewMatrix'),
    'camera.projectionMatrix':             ('BaseCamera', 'projectionMatrix'),
    'camera.position':                    ('BaseCamera', 'world_position'),
    'camera.fieldOfView':                 ('BaseCamera', 'field_of_view'),
    'BasePlayer.clActiveItem':             ('BasePlayer', 'ClActiveItem'),
    'BasePlayer.PlayerEyes':              ('BasePlayer', 'PlayerEyes'),
    'BasePlayer.PlayerInventory':         ('BasePlayer', 'PlayerInventory'),
    'BasePlayer.player_input':            ('BasePlayer', 'PlayerInput'),
    'BasePlayer.player_model':            ('BasePlayer', 'PlayerModel'),
    'BasePlayer.player_flags':            ('BasePlayer', 'PlayerFlags'),
    'BasePlayer.base_movement':           ('BasePlayer', 'BaseMovement'),
    'BasePlayer.display_name':            ('BasePlayer', 'DisplayName'),
    'BasePlayer.current_team':            ('BasePlayer', 'CurrentTeam'),
    'BasePlayer.mounted':                 ('BasePlayer', 'Mounted'),
    'BasePlayer.model_state':             ('BasePlayer', 'ModelState'),
    'BaseCombatEntity.lifestate':         ('BaseCombatEntity', 'Lifestate'),
    'BaseCombatEntity._health':           ('BaseCombatEntity', 'Health'),
    'BaseCombatEntity._maxHealth':        ('BaseCombatEntity', 'MaxHealth'),
    'BaseCombatEntity.model':             ('BaseCombatEntity', 'model'),
    'BaseEntity.positionLerp':            ('BaseEntity', 'positionLerp'),
    'BaseEntity.bounds':                  ('BaseEntity', 'bounds'),
    'BaseEntity.flags':                   ('BaseEntity', 'flags'),
    'BaseEntity.model':                   ('BaseEntity', 'model'),
    'PlayerInventory.containerBelt':      ('PlayerInventory', 'Belt'),
    'PlayerInventory.containerWear':      ('PlayerInventory', 'Wear'),
    'PlayerInventory.containerMain':      ('PlayerInventory', 'Main'),
    'PlayerInventory.loot':               ('PlayerInventory', 'loot'),
    'ItemContainer.ItemList':             ('ItemContainer', 'ItemList'),
    'item.info':                          ('Item', 'ItemDefinition'),
    'item.uid':                           ('Item', 'ItemId'),
    'item.amount':                        ('Item', 'Amount'),
    'item.HeldEntity':                    ('Item', 'HeldEntity_1'),
    'ItemDefinition.itemid':              ('ItemDefinition', 'itemid'),
    'ItemDefinition.shortname':           ('ItemDefinition', 'shortname'),
    'ItemDefinition.displayName':        ('ItemDefinition', 'displayName'),
    'ItemDefinition.category':           ('ItemDefinition', 'category'),
    'ItemDefinition.stackable':           ('ItemDefinition', 'stackable'),
    'PlayerModel._multiMesh':            ('PlayerModel', 'SkinnedMultiMesh'),
    'PlayerModel.isNpc':                  ('PlayerModel', 'is_npc'),
    'PlayerModel.position':               ('PlayerModel', 'position'),
    'PlayerModel.newVelocity':            ('PlayerModel', 'velocity'),
    'PlayerModel.visible':                ('PlayerModel', 'visible'),
    'PlayerModel.boneTransforms':         ('PlayerModel', 'boneTransforms'),
    'model.boneTransforms':               ('Model', 'boneTransforms'),
    'model.rootBone':                     ('Model', 'rootBone'),
    'model.headBone':                     ('Model', 'headBone'),
    'model.eyeBone':                      ('Model', 'eyeBone'),
    'HeldEntity.ownerItemUID':            ('HeldEntity', 'ownerItemUID'),
    'HeldEntity.viewModel':               ('HeldEntity', 'viewModel'),
    'RecoilProperties.recoilYawMin':      ('RecoilProperties', 'RecoilYawMin'),
    'RecoilProperties.recoilYawMax':      ('RecoilProperties', 'RecoilYawMax'),
    'RecoilProperties.recoilPitchMin':    ('RecoilProperties', 'RecoilPitchMin'),
    'RecoilProperties.recoilPitchMax':    ('RecoilProperties', 'RecoilPitchMax'),
    'RecoilProperties.aimconeCurveScale': ('RecoilProperties', 'AimconeCurveScale'),
    'RecoilProperties.newRecoilOverride': ('RecoilProperties', 'NewRecoilOverride'),
    'BaseProjectile.recoil':              ('BaseProjectile', 'recoil'),
    'BaseProjectile.primaryMagazine':     ('BaseProjectile', 'primaryMagazine'),
    'BaseProjectile.aimCone':            ('BaseProjectile', 'AimCone'),
    'BaseProjectile.hipAimCone':          ('BaseProjectile', 'HipAimCone'),
    'BaseProjectile.hasADS':              ('BaseProjectile', 'HasADS'),
    'BaseProjectile.automatic':           ('BaseProjectile', 'automatic'),
    'BaseProjectile.isReloading':          ('BaseProjectile', 'is_reloading'),
    'BaseProjectile.reloadTime':          ('BaseProjectile', 'reload_time'),
    'BaseProjectile.projectileVelocityScale': ('BaseProjectile', 'velocity_scale'),
    'BaseProjectile.aimSway':             ('BaseProjectile', 'aimSway'),
    'BaseProjectile.aimSwaySpeed':        ('BaseProjectile', 'aimSwaySpeed'),
    'BaseProjectile.stancePenalty':       ('BaseProjectile', 'StancePenalty'),
    'BaseProjectile.sightAimConeScale':    ('BaseProjectile', 'SightAimConeScale'),
    'BaseProjectile.aimconePenalty':       ('BaseProjectile', 'AimconePenalty'),
    'BaseProjectile.isBurstWeapon':       ('BaseProjectile', 'isBurstWeapon'),
    'BaseProjectile.canChangeFireModes':  ('BaseProjectile', 'canChangeFireModes'),
    'base_movement.adminCheat':           ('BaseMovement2', 'admin_cheat'),
    'PlayerWalkMovement.TargetMovement':  ('BaseMovement2', 'target_movement'),
    'TOD_Sky_Static.instance':            ('TOD_Sky_Static', 'instances'),
    'TOD_Sky.Cycle':                      ('TOD_Sky', 'CycleParameters'),
    'TOD_Sky.Night':                      ('TOD_Sky', 'NightParameters'),
    'TOD_Sky.Ambient':                    ('TOD_Sky', 'AmbientParameters'),
    'TOD_Sky.timeSinceAmbientUpdate':     ('TOD_Sky', 'timeSinceAmbientUpdate'),
    'tod_night_parameters.AmbientMultiplier': ('TOD_NightParameters', 'AmbientMultiplier'),
    'tod_ambient_parameters.Saturation':  ('TOD_AmbientParameters', 'Saturation'),
    'tod_ambient_parameters.UpdateInterval': ('TOD_AmbientParameters', 'UpdateInterval'),
    'PlayerInput.bodyAngles':             ('PlayerInput', 'bodyAngles'),
    'PlayerInput.yaw':                    ('PlayerInput', 'yaw'),
    'PlayerEyes.viewOffset':              ('PlayerEyes', 'viewOffset'),
    'PlayerEyes.bodyRotation':            ('PlayerEyes', 'bodyRotation'),
    'PlayerEyes.worldPosition':           ('PlayerEyes', 'headPosition'),
    'EffectNetwork.static_fields':        ('EffectNetwork', 'static_fields'),
    'EffectNetwork.instance':             ('EffectNetwork', 'instance'),
    'EffectNetwork.hitPosition':          ('EffectNetwork', 'hitPosition'),
}


def apply_offsets(fields, timestamp, offsets_hpp_path, driver_hpp_path):
    """Auto-patch offsets.hpp and Driver.hpp with new values from dumper output.
    
    Returns a list of (description, old_value, new_value, status) tuples.
    status: 'changed', 'unchanged', 'not_found', 'skipped'
    """
    changes = []
    
    # --- Backup files ---
    import shutil
    offsets_bak = str(offsets_hpp_path) + '.bak'
    driver_bak = str(driver_hpp_path) + '.bak'
    shutil.copy2(offsets_hpp_path, offsets_bak)
    shutil.copy2(driver_hpp_path, driver_bak)
    
    # --- Patch offsets.hpp ---
    offsets_content = offsets_hpp_path.read_text(encoding='utf-8', errors='replace')
    offsets_lines = offsets_content.split('\n')
    
    # Track current namespace
    current_ns = None
    ns_depth = 0
    patched_count = 0
    unchanged_count = 0
    
    # Build a lookup: (namespace, field_name) → new_value
    field_updates = {}
    for dumper_key, (ns, field_name) in FIELD_OFFSET_MAP.items():
        if dumper_key in fields:
            field_updates[(ns, field_name)] = fields[dumper_key]
    
    for i, line in enumerate(offsets_lines):
        stripped = line.strip()
        
        # Track namespace — handles both "namespace X {" and "namespace X\n{"
        ns_match = re.match(r'\s*namespace\s+(\w+)', line)
        if ns_match and '{' not in line:
            # Namespace name on this line, brace might be on next line
            current_ns = ns_match.group(1)
            ns_depth = -1  # Will be set to 1 when we see the {
            continue
        elif ns_match and '{' in line:
            current_ns = ns_match.group(1)
            ns_depth = 1
            continue
        
        if ns_depth == -1 and '{' in stripped:
            ns_depth = 1
        
        if ns_depth > 0:
            if '{' in stripped:
                ns_depth += stripped.count('{')
            if '}' in stripped:
                ns_depth -= stripped.count('}')
                if ns_depth <= 0:
                    current_ns = None
                    ns_depth = 0
        
        # Try to match a static pointer (always — they may be inside namespace offsets)
        for dumper_key, var_name in STATIC_POINTER_MAP.items():
            if dumper_key not in fields:
                continue
            new_val = fields[dumper_key]
            if isinstance(var_name, list):
                names = var_name
            else:
                names = [var_name]
            for name in names:
                pattern = rf'(\b{name}\s*=\s*)0x[0-9A-Fa-f]+'
                m = re.search(pattern, line)
                if m:
                    old_val_match = re.search(r'0x[0-9A-Fa-f]+', line[m.start():m.end()])
                    old_val = old_val_match.group(0) if old_val_match else '?'
                    new_val_fmt = f'0x{int(new_val, 0):X}'
                    old_norm = f'0x{int(old_val, 16):X}'
                    new_norm = f'0x{int(new_val, 0):X}'
                    if old_norm == new_norm:
                        changes.append((name, old_val, new_val_fmt, 'unchanged'))
                        unchanged_count += 1
                    else:
                        offsets_lines[i] = re.sub(r'0x[0-9A-Fa-f]+', new_val_fmt, line, count=1, flags=re.IGNORECASE)
                        changes.append((name, old_val, new_val_fmt, 'changed'))
                        patched_count += 1
        
        # Try to match a field offset within the current namespace
        if current_ns is not None:
            matched_keys = []
            for (ns, field_name), new_val in field_updates.items():
                if ns != current_ns:
                    continue
                pattern = rf'(\b{re.escape(field_name)}\s*=\s*)0x[0-9A-Fa-f]+'
                m = re.search(pattern, line, re.IGNORECASE)
                if m:
                    old_val_match = re.search(r'0x[0-9A-Fa-f]+', line[m.start():m.end()])
                    old_val = old_val_match.group(0) if old_val_match else '?'
                    new_val_fmt = f'0x{int(new_val, 0):X}'
                    old_norm = f'0x{int(old_val, 16):X}'
                    new_norm = f'0x{int(new_val, 0):X}'
                    if old_norm == new_norm:
                        changes.append((f'{ns}::{field_name}', old_val, new_val_fmt, 'unchanged'))
                        unchanged_count += 1
                    else:
                        offsets_lines[i] = re.sub(r'0x[0-9A-Fa-f]+', new_val_fmt, line, count=1, flags=re.IGNORECASE)
                        changes.append((f'{ns}::{field_name}', old_val, new_val_fmt, 'changed'))
                        patched_count += 1
                    matched_keys.append((ns, field_name))
            # Remove matched items after iteration
            for key in matched_keys:
                del field_updates[key]
    
    # Write patched offsets.hpp
    offsets_hpp_path.write_text('\n'.join(offsets_lines), encoding='utf-8')
    
    # Report fields that weren't found
    for (ns, field_name), new_val in field_updates.items():
        changes.append((f'{ns}::{field_name}', '?', f'0x{int(new_val, 0):X}', 'not_found'))
    
    # --- Patch Driver.hpp (GA timestamp) ---
    driver_content = driver_hpp_path.read_text(encoding='utf-8', errors='replace')
    if timestamp:
        old_ts_match = re.search(r'GA_TIMESTAMP\s*=\s*(0x[0-9A-Fa-f]+)', driver_content)
        if old_ts_match:
            old_ts = old_ts_match.group(1)
            if old_ts.lower() == timestamp.lower():
                changes.append(('GA_TIMESTAMP', old_ts, timestamp, 'unchanged'))
                unchanged_count += 1
            else:
                driver_content = re.sub(
                    r'(GA_TIMESTAMP\s*=\s*)0x[0-9A-Fa-f]+',
                    rf'\g<1>{timestamp}',
                    driver_content
                )
                driver_hpp_path.write_text(driver_content, encoding='utf-8')
                changes.append(('GA_TIMESTAMP', old_ts, timestamp, 'changed'))
                patched_count += 1
    
    print(f"\n--- Auto-Apply Results ---")
    print(f"  Backed up to: {offsets_bak}")
    print(f"  Backed up to: {driver_bak}")
    print(f"  Changed: {patched_count}")
    print(f"  Unchanged: {unchanged_count}")
    not_found = [c for c in changes if c[3] == 'not_found']
    if not_found:
        print(f"  Not found in offsets.hpp: {len(not_found)}")
        for desc, _, new, _ in not_found:
            print(f"    {desc} -> {new} (manual update needed)")
    
    changed = [c for c in changes if c[3] == 'changed']
    if changed:
        print(f"\n  Changes applied:")
        for desc, old, new, _ in changed:
            print(f"    {desc}: {old} -> {new}")
    
    return changes


# ============================================================
# MAIN
# ============================================================

def main():
    parser = argparse.ArgumentParser(
        description='Convert morphine-dumper2 output to cheat format (rust_decrypts.dat + updated_offsets.txt)',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
    python morphine_to_cheat.py morphine-dumper_output.h
    python morphine_to_cheat.py "C:\\Users\\...\\Desktop\\morphine-dumper_output.h" --output-dir .
    python morphine_to_cheat.py morphine-dumper_output.h --apply --cheat-dir ..
        """)
    parser.add_argument('input', help='Path to morphine-dumper2 output header file')
    parser.add_argument('--output-dir', '-o', default='.', help='Output directory (default: current)')
    parser.add_argument('--apply', action='store_true', help='Auto-patch offsets.hpp and Driver.hpp with new values')
    parser.add_argument('--cheat-dir', default='..', help='Path to cheat root directory (default: parent of script)')
    
    args = parser.parse_args()
    
    input_path = Path(args.input)
    if not input_path.exists():
        print(f"ERROR: Input file not found: {input_path}")
        sys.exit(1)
    
    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    
    cheat_dir = Path(args.cheat_dir)
    
    print(f"Reading: {input_path}")
    content = input_path.read_text(encoding='utf-8', errors='replace')
    
    build_id = parse_build_id(content)
    timestamp = parse_game_timestamp(content)
    print(f"Build: {build_id}")
    print(f"GameAssembly timestamp: {timestamp or 'not found'}")
    
    # Parse decrypt functions
    print("\n--- Parsing decrypt functions ---")
    decrypt_results = {}
    for func_name in DECRYPT_PREFIX.keys():
        body = extract_decrypt_function(content, func_name)
        if body:
            ops = parse_decrypt_ops(body)
            decrypt_results[func_name] = ops
            print(f"  {func_name}: {len(ops)} ops found")
            for op_type, val in ops:
                print(f"    {op_type}(0x{val:X})")
        else:
            print(f"  {func_name}: NOT FOUND in dumper output")
            decrypt_results[func_name] = []
    
    # Parse struct fields
    print("\n--- Parsing struct fields ---")
    fields = parse_struct_fields(content)
    print(f"  Found {len(fields)} fields across all structs")
    
    mapped_count = 0
    for dumper_key in OFFSET_MAP:
        if dumper_key in fields:
            mapped_count += 1
    print(f"  Mapped to cheat names: {mapped_count}/{len(OFFSET_MAP)}")
    
    # Generate outputs
    print("\n--- Generating output files ---")
    
    decrypts_path = output_dir / 'rust_decrypts.dat'
    count = generate_decrypts_dat(decrypt_results, decrypts_path)
    print(f"  {decrypts_path}: {count} decrypt constants")
    
    offsets_path = output_dir / 'updated_offsets.txt'
    generate_offsets_txt(fields, decrypt_results, build_id, timestamp, offsets_path)
    print(f"  {offsets_path}: full offset summary")
    
    # Auto-apply if requested
    if args.apply:
        offsets_hpp = cheat_dir / 'offsets.hpp'
        driver_hpp = cheat_dir / 'Driver.hpp'
        
        if not offsets_hpp.exists():
            print(f"\nERROR: offsets.hpp not found at {offsets_hpp}")
            print("  Use --cheat-dir to specify the cheat root directory")
            sys.exit(1)
        if not driver_hpp.exists():
            print(f"\nERROR: Driver.hpp not found at {driver_hpp}")
            sys.exit(1)
        
        apply_offsets(fields, timestamp, offsets_hpp, driver_hpp)
    
    print("\n--- Done ---")
    print(f"  rust_decrypts.dat — runtime decrypt override (no recompile)")
    if args.apply:
        print(f"  offsets.hpp + Driver.hpp — auto-patched (backup .bak created)")
        print(f"  Rebuild DLLs to apply pointer/offset changes")
    else:
        print(f"  Use --apply to auto-patch offsets.hpp + Driver.hpp")
        print(f"  Use updated_offsets.txt to manually update offsets.hpp")
    
    # Check for missing items
    missing_decryps = [f for f, ops in decrypt_results.items() if not ops]
    if missing_decryps:
        print(f"\n  WARNING: {len(missing_decryps)} decrypt functions not found: {missing_decryps}")
    
    missing_offsets = [k for k in OFFSET_MAP if k not in fields]
    if missing_offsets:
        print(f"  WARNING: {len(missing_offsets)} offsets not found in dumper output:")
        for k in missing_offsets:
            print(f"    {k}")


if __name__ == '__main__':
    main()
