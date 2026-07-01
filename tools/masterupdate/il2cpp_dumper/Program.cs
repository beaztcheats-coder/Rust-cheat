using System.Text.Json;
using Il2CppInspector;
using Il2CppInspector.Reflection;
using Inspector = Il2CppInspector.Il2CppInspector;

string binaryPath = args.Length > 0 ? args[0] : @"C:\Program Files (x86)\Steam\steamapps\common\Rust\GameAssembly.dll";
string metadataPath = args.Length > 1 ? args[1] : @"C:\Program Files (x86)\Steam\steamapps\common\Rust\RustClient_Data\il2cpp_data\Metadata\global-metadata.dat";
string outputPath = args.Length > 2 ? args[2] : @"E:\github\Rust-cheat\tools\masterupdate\output\il2cppinspector\offsets.json";

string[] targetClasses = {
    "BasePlayer", "BaseCombatEntity", "BaseNetworkable", "BaseEntity",
    "PlayerEyes", "PlayerInput", "PlayerModel", "PlayerInventory",
    "ItemContainer", "Item", "ItemDefinition", "BaseProjectile",
    "RecoilProperties", "HeldEntity", "Model", "SkinnedMultiMesh",
    "TOD_Sky", "PlayerWalkMovement", "BaseMovement",
    "EffectNetwork", "ModelState", "ViewModel", "WorldItem",
    "FlintStrikeWeapon", "MainCamera", "ConVar_Graphics",
    "ConsoleSystem", "Camera",
};

Console.Error.WriteLine($"[DUMP] Loading {binaryPath} + {metadataPath}...");
var inspectors = Inspector.LoadFromFile(binaryPath, metadataPath, silent: true);
if (inspectors == null || inspectors.Count == 0)
{
    Console.Error.WriteLine("[ERROR] Failed to load IL2CPP data");
    return 1;
}

var il2cpp = inspectors[0];
ulong imageBase = il2cpp.BinaryImage.ImageBase;
Console.Error.WriteLine($"[DUMP] Image base: 0x{imageBase:X}");
Console.Error.WriteLine($"[DUMP] Metadata version: {il2cpp.Metadata.Version}");

Console.Error.WriteLine("[DUMP] Building type model...");
var model = new TypeModel(il2cpp);

var results = new Dictionary<string, object>();
results["metadata_version"] = il2cpp.Metadata.Version.ToString();
results["image_base"] = $"0x{imageBase:X}";

var classData = new Dictionary<string, object>();

foreach (var targetName in targetClasses)
{
    try
    {
        TypeInfo type = null;

        if (model.TypesByFullName.TryGetValue(targetName, out var exactMatch))
        {
            type = exactMatch;
        }
        else
        {
            foreach (var kvp in model.TypesByFullName)
            {
                if (kvp.Key == targetName || kvp.Key.StartsWith(targetName + "_") || kvp.Key.EndsWith("/" + targetName) || kvp.Key.EndsWith("." + targetName))
                {
                    type = kvp.Value;
                    Console.Error.WriteLine($"[DUMP] {targetName} -> matched as {kvp.Key}");
                    break;
                }
            }
        }

        if (type == null)
        {
            Console.Error.WriteLine($"[WARN] Class not found: {targetName}");
            classData[targetName] = new { error = "not found" };
            continue;
        }

        var info = new Dictionary<string, object>();
        info["full_name"] = type.FullName;
        info["base_type"] = type.BaseType?.FullName ?? "";
        info["type_def_index"] = type.Index;

        var allFields = new Dictionary<string, object>();

        var currentType = type;
        while (currentType != null)
        {
            try
            {
                foreach (var field in currentType.DeclaredFields)
                {
                    if (field.IsLiteral) continue;

                    string fieldName = field.Name;
                    long offset = field.Offset;
                    string fieldTypeName = field.FieldType?.Name ?? "UNKNOWN";
                    bool isStatic = field.IsStatic;

                    var fieldInfo = new
                    {
                        offset = offset,
                        offset_hex = $"0x{offset:X}",
                        type = fieldTypeName,
                        @static = isStatic,
                        declaring_type = currentType.FullName,
                    };

                    string key = isStatic ? $"static:{fieldName}" : fieldName;
                    if (!allFields.ContainsKey(key))
                        allFields[key] = fieldInfo;
                }
            }
            catch (Exception ex)
            {
                Console.Error.WriteLine($"[WARN] Error reading fields of {currentType.FullName}: {ex.Message}");
            }
            currentType = currentType.BaseType;
        }

        info["fields"] = allFields;

        try
        {
            var methods = new Dictionary<string, string>();
            foreach (var method in type.DeclaredMethods)
            {
                try
                {
                    string methodName = method.Name;
                    if (method.VirtualAddress.HasValue)
                    {
                        ulong methodVA = method.VirtualAddress.Value.Start;
                        ulong methodRVA = methodVA - imageBase;
                        if (!methods.ContainsKey(methodName))
                            methods[methodName] = $"0x{methodRVA:X}";
                    }
                }
                catch { }
            }
            if (methods.Count > 0)
                info["methods"] = methods;
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine($"[WARN] Error reading methods of {targetName}: {ex.Message}");
        }

        int fieldCount = allFields.Count;
        Console.Error.WriteLine($"[DUMP] {targetName}: fields={fieldCount} base={info["base_type"]}");
        classData[targetName] = info;
    }
    catch (Exception ex)
    {
        Console.Error.WriteLine($"[ERROR] Failed to process {targetName}: {ex.Message}");
        classData[targetName] = new { error = ex.Message };
    }
}

results["classes"] = classData;

var options = new JsonSerializerOptions { WriteIndented = true };
string json = JsonSerializer.Serialize(results, options);

Directory.CreateDirectory(Path.GetDirectoryName(outputPath)!);
File.WriteAllText(outputPath, json);

Console.Error.WriteLine($"[DUMP] Written {json.Length} bytes to {outputPath}");
Console.Error.WriteLine($"[DUMP] Done. {classData.Count} classes dumped.");
return 0;
