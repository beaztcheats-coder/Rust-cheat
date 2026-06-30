// Frida IL2CPP field dumper for Rust (Windows x64)
// Usage: frida -n RustClient.exe -l dump_il2cpp.js

const BASENETWORKABLE_RVA = 0xE334210;
// Field count offsets to try
const FC_OFFSETS = [0x120, 0x11E, 0x122, 0x124, 0x11C, 0x118, 0x128, 0x12A];

// Fields pointer offsets to try (Il2CppClass layout may differ from header)
const FIELDS_OFFSETS = [0x80, 0x88, 0x78, 0x90, 0x70, 0x98];

const IL2CPPCLASS_OFF_NAME = 0x10;
const IL2CPPCLASS_OFF_PARENT = 0x58;
const FIELDINFO_SIZE = 0x20;
const FIELDINFO_OFF_NAME = 0x00;
const FIELDINFO_OFF_OFFSET = 0x18;

function readPtr(addr) { try { return addr.readPointer(); } catch(e) { return null; } }
function readU16(addr) { try { return addr.readU16(); } catch(e) { return 0; } }
function readU32(addr) { try { return addr.readU32(); } catch(e) { return 0; } }
function readS32(addr) { try { return addr.readS32(); } catch(e) { return 0; } }
function readStr(addr) { try { return addr.readCString(128) || "?"; } catch(e) { return "?"; } }

function dumpClassByPtr(klass, label) {
    if (!klass || klass.isNull()) return null;
    var namePtr = readPtr(klass.add(IL2CPPCLASS_OFF_NAME));
    var className = namePtr ? readStr(namePtr) : "?";

    console.log("// ========================================");
    console.log("// " + label + "  TypeInfo=" + klass);

    // DEBUG: dump raw Il2CppClass bytes to find correct offsets
    console.log("// DEBUG raw Il2CppClass bytes:");
    console.log("//   +0x10 name=" + (namePtr ? namePtr.toString(16) : "NULL") + " -> '" + className.substring(0,60) + "'");
    
    // Try all field count offsets
    var fieldCount = 0, fcOff = 0;
    for (var ci = 0; ci < FC_OFFSETS.length; ci++) {
        var c = readU16(klass.add(FC_OFFSETS[ci]));
        console.log("//   +0x" + FC_OFFSETS[ci].toString(16) + " (u16) = " + c);
        if (c > 0 && c < 2000 && fieldCount === 0) { fieldCount = c; fcOff = FC_OFFSETS[ci]; }
    }
    
    // Also read u32 at candidates
    for (var ci = 0; ci < FC_OFFSETS.length; ci++) {
        var c32 = readU32(klass.add(FC_OFFSETS[ci]));
        if (c32 > 0 && c32 < 2000) console.log("//   +0x" + FC_OFFSETS[ci].toString(16) + " (u32) = " + c32 + " <-- possible!");
    }

    // Try all fields pointer offsets
    var fieldsPtr = null, fpOff = 0;
    for (var fi = 0; fi < FIELDS_OFFSETS.length; fi++) {
        var p = readPtr(klass.add(FIELDS_OFFSETS[fi]));
        if (p && !p.isNull()) {
            console.log("//   +0x" + FIELDS_OFFSETS[fi].toString(16) + " fieldsPtr=" + p);
            if (!fieldsPtr) { fieldsPtr = p; fpOff = FIELDS_OFFSETS[fi]; }
        }
    }

    console.log("// Best guess: fieldCount=" + fieldCount + " @+0x" + fcOff.toString(16) + " fieldsPtr=" + (fieldsPtr ? fieldsPtr.toString(16) : "NULL") + " @+0x" + fpOff.toString(16));
    console.log("// ========================================");
    console.log("namespace " + label.replace(/[^a-zA-Z0-9_]/g, '_') + " {");

    if (fieldsPtr && !fieldsPtr.isNull() && fieldCount > 0 && fieldCount < 2000) {
        var printed = 0;
        for (var i = 0; i < fieldCount; i++) {
            var fi = fieldsPtr.add(i * FIELDINFO_SIZE);
            var fnPtr = readPtr(fi.add(FIELDINFO_OFF_NAME));
            var foff = readS32(fi.add(FIELDINFO_OFF_OFFSET));
            
            if (fnPtr && !fnPtr.isNull()) {
                var fname = readStr(fnPtr);
                if (fname && fname.length > 0) {
                    var hex = "0x" + foff.toString(16).toUpperCase().padStart(4, '0');
                    // Print ALL fields, including offset=0 (static) but mark them
                    var staticMark = (foff === 0) ? " [static]" : "";
                    console.log("    " + hex + "  " + fname + staticMark);
                    printed++;
                }
            }
        }
        if (printed === 0) {
            // Dump first field's raw bytes
            var fi0 = fieldsPtr.add(0);
            console.log("    // DEBUG: first FieldInfo raw dump");
            for (var b = 0; b < FIELDINFO_SIZE; b += 8) {
                var p = fi0.add(b);
                try { console.log("    //   +0x" + b.toString(16) + " ptr=" + p.readPointer().toString(16)); } catch(e) {}
            }
            // Try reading field name from other offsets
            var altNameOffsets = [0x00, 0x08, 0x10, 0x18, 0x28, 0x30];
            for (var ai = 0; ai < altNameOffsets.length; ai++) {
                var tp = readPtr(fi0.add(altNameOffsets[ai]));
                if (tp && !tp.isNull()) {
                    try { console.log("    //   fi0+0x" + altNameOffsets[ai].toString(16) + " string='" + tp.readCString(32) + "'"); } catch(e) {}
                }
            }
            // Try reading offset as different sizes
            console.log("    //   fi0+0x18 s32=" + readS32(fi0.add(0x18)));
            console.log("    //   fi0+0x18 u32=" + readU32(fi0.add(0x18)));
            console.log("    //   fi0+0x1C s32=" + readS32(fi0.add(0x1C)));
            console.log("    //   fi0+0x14 s32=" + readS32(fi0.add(0x14)));
            console.log("    //   fi0+0x10 ptr=" + (readPtr(fi0.add(0x10)) ? "valid" : "NULL"));
        }
    } else {
        console.log("    // no fields (count=" + fieldCount + " ptr=" + (fieldsPtr ? "valid" : "NULL") + ")");
    }
    console.log("}");
    console.log("");
    return readPtr(klass.add(IL2CPPCLASS_OFF_PARENT));
}

// Main
console.log("=== Frida IL2CPP Runtime Field Dumper ===");
var ga = null;
Process.enumerateModules().forEach(function(m) {
    if (m.name.toLowerCase().indexOf("gameassembly") !== -1) ga = m.base;
});
if (!ga) { console.log("FATAL: GameAssembly.dll not found!"); } else {
    console.log("GameAssembly: " + ga);
    var tiPtr = readPtr(ga.add(BASENETWORKABLE_RVA));
    if (!tiPtr || tiPtr.isNull()) {
        console.log("BaseNetworkable TypeInfo NULL at RVA 0x" + BASENETWORKABLE_RVA.toString(16));
    } else {
        var klass = tiPtr;
        var label = "BaseNetworkable";
        for (var i = 0; i < 4 && klass && !klass.isNull(); i++) {
            klass = dumpClassByPtr(klass, label);
            label = "Parent" + (i + 1);
            if (klass && !klass.isNull()) {
                var pn = readPtr(klass.add(IL2CPPCLASS_OFF_NAME));
                if (pn) { try { label = readStr(pn).substring(0,50); } catch(e) { label = "Parent" + (i+1); } }
            }
        }
    }
    console.log("=== DUMP COMPLETE ===");
}
