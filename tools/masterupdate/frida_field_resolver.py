#!/usr/bin/env python3
"""
frida_field_resolver.py - Test if Frida can resolve Rust's obfuscated field names.

Tries multiple naming conventions for each field to see which one Frida accepts.
If this works, we can get 100% of field offsets without Morphine.
"""
import json
import sys
import time
from pathlib import Path

OUTPUT_DIR = Path(__file__).parent / "output"
BRIDGE_PATH = Path(r"C:\frida-il2cpp\dist\index.js")
OUTPUT_FILE = OUTPUT_DIR / "frida_resolved_fields.json"

# Fields to resolve: (class_name, field_variants_to_try)
FIELDS_TO_RESOLVE = {
    "BasePlayer": {
        "PlayerEyes": ["player_eyes", "PlayerEyes", "eyes", "_playerEyes", "%"],
        "PlayerInput": ["player_input", "PlayerInput", "input", "_playerInput"],
        "PlayerModel": ["player_model", "PlayerModel", "model", "_playerModel"],
        "PlayerInventory": ["player_inventory", "PlayerInventory", "inventory"],
        "BaseMovement": ["base_movement", "BaseMovement", "movement"],
        "ClActiveItem": ["cl_active_item", "ClActiveItem", "activeItem", "clActiveItem"],
        "DisplayName": ["display_name", "DisplayName", "displayName", "_displayName"],
        "CurrentTeam": ["current_team", "CurrentTeam", "currentTeam"],
        "PlayerFlags": ["player_flags", "PlayerFlags", "playerFlags"],
        "Mounted": ["mounted", "Mounted"],
        "PlayerRigidbody": ["player_rigidbody", "PlayerRigidbody"],
        "CurrentGesture": ["current_gesture", "CurrentGesture"],
    },
    "BaseCombatEntity": {
        "Health": ["_health", "Health", "health"],
        "MaxHealth": ["_maxHealth", "MaxHealth", "maxHealth"],
        "Lifestate": ["lifestate", "Lifestate", "_lifestate"],
    },
    "Item": {
        "ItemDefinition": ["info", "itemdefinition", "ItemDefinition", "_info"],
        "ItemId": ["uid", "ItemId", "itemId", "_uid"],
        "Amount": ["amount", "Amount", "_amount"],
        "HeldEntity_1": ["held_entity", "HeldEntity", "heldEntity"],
    },
    "PlayerInventory": {
        "Belt": ["containerBelt", "Belt", "belt", "_containerBelt"],
        "Main": ["containerMain", "Main", "main", "_containerMain"],
        "Wear": ["containerWear", "Wear", "wear", "_containerWear"],
        "loot": ["loot", "Loot", "_loot"],
    },
    "PlayerModel": {
        "skinnedMultiMesh": ["skinnedMultiMesh", "SkinnedMultiMesh", "_skinnedMultiMesh"],
        "position": ["position", "Position", "_position"],
        "velocity": ["velocity", "Velocity", "_velocity"],
        "visible": ["visible", "Visible", "_visible"],
        "boneTransforms": ["boneTransforms", "BoneTransforms"],
        "headBone": ["headBone", "HeadBone"],
    },
    "ItemContainer": {
        "ItemList": ["itemList", "ItemList", "item_list", "_itemList"],
    },
}

FRIDA_AGENT = r"""
try {
    setTimeout(() => {
        Il2Cpp.perform(() => {
            send({action: "init"});
            
            const targets = %TARGETS%;
            const results = {};
            
            for (const className of Object.keys(targets)) {
                // Find the class
                let klass = null;
                Il2Cpp.domain.assemblies.forEach(asm => {
                    if (klass) return;
                    asm.image.classes.forEach(c => {
                        if (klass) return;
                        if (c.name === className) klass = c;
                    });
                });
                
                if (!klass) {
                    results[className] = {error: "class not found"};
                    return;
                }
                
                results[className] = {};
                
                // Get ALL fields for reference
                const allFields = {};
                klass.fields.forEach(f => {
                    allFields[f.name] = {offset: f.offset, type: f.type.name};
                });
                results[className]["_all_fields"] = allFields;
                
                // Try each field variant
                for (const [fieldKey, variants] of Object.entries(targets[className])) {
                    results[className][fieldKey] = {found: false};
                    
                    for (const variant of variants) {
                        try {
                            const field = klass.field(variant);
                            if (field) {
                                results[className][fieldKey] = {
                                    found: true,
                                    name: variant,
                                    offset: field.offset,
                                    type: field.type.name
                                };
                                break;
                            }
                        } catch(e) {
                            // Field not found with this name, try next
                        }
                    }
                }
            }
            
            send({action: "results", data: results});
        }).catch(e => send({action: "error", error: e.message}));
    });
} catch(e) {
    send({action: "fatal", error: e.message});
}
"""


def main():
    import frida
    
    print("[RESOLVE] Testing Frida field name resolution...")
    
    import subprocess
    result = subprocess.run(["tasklist", "/FI", "IMAGENAME eq RustClient.exe", "/NH"],
                          capture_output=True, text=True)
    if "RustClient.exe" not in result.stdout:
        print("[ERROR] RustClient.exe not running!")
        return 1
    
    bridge_code = BRIDGE_PATH.read_text(encoding="utf-8", errors="ignore")
    targets_json = json.dumps(FIELDS_TO_RESOLVE)
    agent_code = bridge_code + "\n\n" + FRIDA_AGENT.replace("%TARGETS%", targets_json)
    
    final_results = {}
    done = [False]
    
    def on_message(msg, data):
        if msg["type"] == "error":
            print(f"[ERROR] {msg}")
            done[0] = True
            return
        if msg["type"] != "send":
            return
        payload = msg.get("payload", {})
        action = payload.get("action", "")
        
        if action == "init":
            print("[OK] Bridge initialized")
        elif action == "results":
            final_results.update(payload.get("data", {}))
            done[0] = True
        elif action in ("error", "fatal"):
            print(f"[ERROR] {payload.get('error', '?')}")
            done[0] = True
    
    session = frida.attach("RustClient.exe")
    script = session.create_script(agent_code)
    script.on("message", on_message)
    script.load()
    
    start = time.time()
    while not done[0]:
        time.sleep(0.5)
        if time.time() - start > 30:
            break
    
    session.detach()
    
    # Analyze results
    print(f"\n{'='*60}")
    print("  FIELD RESOLUTION RESULTS")
    print(f"{'='*60}")
    
    resolved = 0
    failed = 0
    
    for className, fields in final_results.items():
        if "error" in fields:
            print(f"\n  [{className}] CLASS NOT FOUND")
            continue
        
        print(f"\n  [{className}]")
        for fieldKey, result in fields.items():
            if fieldKey == "_all_fields":
                continue
            if result.get("found"):
                print(f"    ✅ {fieldKey} = 0x{result['offset']:X} (via name \"{result['name']}\", type={result['type']})")
                resolved += 1
            else:
                print(f"    ❌ {fieldKey} — NOT FOUND by any name variant")
                failed += 1
    
    print(f"\n{'='*60}")
    print(f"  Resolved: {resolved} | Failed: {failed}")
    print(f"{'='*60}")
    
    # Save results
    OUTPUT_FILE.write_text(json.dumps(final_results, indent=2, default=str), encoding="utf-8")
    print(f"\n  Output: {OUTPUT_FILE}")
    
    return 0


if __name__ == "__main__":
    sys.exit(main())
