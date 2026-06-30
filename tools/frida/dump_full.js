// Frida IL2CPP full dump using frida-il2cpp-bridge (ES module)
// Usage: frida RustClient.exe -l dump_full.js -o dump_full.txt

async function main() {
    await import("C:/frida-il2cpp/dist/index.js");
    
    console.log("=== Frida IL2CPP Full Bridge Dump ===");

    Il2Cpp.perform(() => {
        console.log("Unity Version: " + Il2Cpp.unityVersion);
        console.log("Application: " + Il2Cpp.application);
        console.log("");
        console.log("DUMPING ALL ASSEMBLIES AND CLASSES");
        console.log("========================================");
        console.log("");
        
        Il2Cpp.domain.assemblies.forEach(assembly => {
            console.log("// === Assembly: " + assembly.name + " (" + assembly.image.classCount + " classes) ===");
            console.log("");
            
            assembly.image.classes.forEach(klass => {
                var instanceFields = klass.fields.filter(f => !f.isStatic && !f.isThreadStatic && !f.isLiteral);
                if (instanceFields.length === 0) return;
                
                var ns = klass.namespace || "";
                var fullName = ns ? ns + "." + klass.name : klass.name;
                
                console.log("// " + fullName + " (" + instanceFields.length + " fields)");
                console.log("namespace " + ns.replace(/[^a-zA-Z0-9_]/g, "_") + " { // " + klass.name);
                
                klass.fields.forEach(field => {
                    var marker = field.isStatic ? " [static]" : field.isThreadStatic ? " [thread]" : "";
                    var offset = field.offset !== undefined ? "0x" + field.offset.toString(16).toUpperCase().padStart(4, '0') : "-----";
                    console.log("    " + offset + "  " + field.name + marker + "  // " + field.type.name);
                });
                
                console.log("}");
                console.log("");
            });
        });
        
        console.log("=== DUMP COMPLETE ===");
    }).then(ms => console.log("Done in " + ms + "ms"));
}

main().catch(err => console.log("FATAL: " + err.message));
