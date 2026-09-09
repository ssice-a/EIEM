// Offline evidence collector; no game DLL execution, schema guessing or resource replacement.
using System.Collections;
using System.Security.Cryptography;
using System.Text.Json;
using AnimeStudio;
using AnimeStudio.GUI;
using AssetObject = AnimeStudio.Object;

internal static class PrefabComponentProbe
{
    private static readonly JsonSerializerOptions JsonOptions = new()
    {
        WriteIndented = true,
        NumberHandling = System.Text.Json.Serialization.JsonNumberHandling.AllowNamedFloatingPointLiterals
    };

    public static int Run(string[] args)
    {
        if (args.Length != 6)
        {
            Console.Error.WriteLine("--inspect-prefab-components VFS_ROOT WORKSPACE LOGICAL_PREFAB NEW_OUTPUT_DIRECTORY SOURCE_INDEX");
            return 2;
        }
        var output = Path.GetFullPath(args[4]);
        if (Directory.Exists(output) || File.Exists(output))
        {
            Console.Error.WriteLine("Evidence output must be a new directory.");
            return 2;
        }
        var archive = EndfieldVfsArchive.Open(args[1]);
        var index = EndfieldIndexStore.LoadPrefabExportIndex(args[5], args[3]);
        if (index.VfsFingerprint != archive.Fingerprint)
            throw new InvalidDataException("Index fingerprint does not match the source VFS.");
        var sources = index.Prefab.Records.Select(r => r.Source).Where(s => !string.IsNullOrEmpty(s))
            .Distinct(StringComparer.OrdinalIgnoreCase).ToArray();
        if (sources.Length != 1)
            throw new InvalidDataException($"Expected one Prefab source bundle, found {sources.Length}.");
        var source = sources[0];
        var closure = index.Dependencies.ResolveBundleClosure(source).ToArray();
        var paths = closure.Select(p => archive.ExtractToCache(p, args[2])).ToArray();
        Console.WriteLine($"Loading {paths.Length} dependency bundles (components only).");
        TypeFlags.SetTypes(new());
        foreach (var type in new[] { ClassIDType.GameObject, ClassIDType.Transform,
                     ClassIDType.MonoBehaviour, ClassIDType.MonoScript, ClassIDType.AssetBundle })
            TypeFlags.SetType(type, true, false);
        Logger.Default = new ConsoleLogger();
        Logger.Flags = LoggerEvent.Error;
        var manager = new AssetsManager
        {
            Game = GameManager.GetGameByType(GameType.ArknightsEndfield),
            SpecifyUnityVersion = "2021.3.34f5", ResolveDependencies = false
        };
        manager.LoadFiles(paths, mergeSplitAssets: false);
        var root = EndfieldPrefabDocument.FindRoot(
            manager.assetsFileList.SelectMany(f => f.Objects).OfType<GameObject>(),
            index.Prefab, index.Dependencies.GetCabNames(source))
            ?? throw new InvalidDataException("Prefab root not found in loaded source.");
        var hierarchy = new HashSet<string>();
        Visit(root, hierarchy);
        Directory.CreateDirectory(output);
        var rows = new List<object>();
        var components = manager.assetsFileList.SelectMany(f => f.Objects).OfType<MonoBehaviour>()
            .OrderBy(m => m.assetsFile.fileName, StringComparer.Ordinal).ThenBy(m => m.m_PathID).ToArray();
        foreach (var component in components)
        {
            var id = rows.Count.ToString("D5");
            component.m_Script.TryGet(out var script);
            component.m_GameObject.TryGet(out var owner);
            var raw = component.GetRawData();
            if (raw.Length != component.byteSize)
                throw new InvalidDataException($"Truncated raw component {Identity(component)}");
            File.WriteAllBytes(Path.Combine(output, id + ".bin"), raw);
            var nodes = component.serializedType?.m_Type?.m_Nodes;
            var hasSchema = nodes?.Count > 0;
            string error = null;
            long? consumed = null;
            var decoded = false;
            var references = new List<object>();
            if (hasSchema)
            {
                WriteJson(Path.Combine(output, id + ".schema.json"), nodes.Select(n => new
                {
                    type = n.m_Type, name = n.m_Name, level = n.m_Level,
                    byteSize = n.m_ByteSize, metaFlags = n.m_MetaFlag
                }));
                var saved = component.reader.Position;
                try
                {
                    var data = component.ToType();
                    consumed = component.reader.Position - component.reader.byteStart;
                    if (data == null || consumed != component.byteSize)
                        throw new InvalidDataException($"TypeTree consumed {consumed}/{component.byteSize} bytes.");
                    WriteJson(Path.Combine(output, id + ".data.json"), Plain(data));
                    CollectReferences(data, "$", component.assetsFile, references);
                    decoded = true;
                }
                catch (Exception ex) { error = ex.GetType().Name + ": " + ex.Message; }
                finally { component.reader.Position = saved; }
            }
            rows.Add(new
            {
                id, identity = Identity(component), cab = component.assetsFile.fileName,
                pathId = component.m_PathID, componentName = component.m_Name,
                owner = owner == null ? null : Identity(owner), ownerPath = OwnerPath(owner),
                inSelectedPrefab = owner != null && hierarchy.Contains(Identity(owner)),
                script = script == null ? null : new
                {
                    identity = Identity(script), assembly = script.m_AssemblyName,
                    ns = script.m_Namespace, type = script.m_ClassName
                },
                scriptPointer = new { fileId = component.m_Script.m_FileID, pathId = component.m_Script.m_PathID },
                byteSize = raw.Length, sha256 = Convert.ToHexString(SHA256.HashData(raw)),
                typeTreeNodes = nodes?.Count ?? 0, decoded, consumed, error, references
            });
        }
        var transforms = manager.assetsFileList.SelectMany(f => f.Objects).OfType<Transform>()
            .Select(t =>
            {
                t.m_GameObject.TryGet(out var owner);
                t.m_Father.TryGet(out var parent);
                return new
                {
                    identity = Identity(t), owner = owner == null ? null : Identity(owner),
                    path = OwnerPath(owner), parent = parent == null ? null : Identity(parent),
                    inSelectedPrefab = owner != null && hierarchy.Contains(Identity(owner)),
                    localPosition = new[] { t.m_LocalPosition.X, t.m_LocalPosition.Y, t.m_LocalPosition.Z },
                    localRotation = new[] { t.m_LocalRotation.X, t.m_LocalRotation.Y, t.m_LocalRotation.Z, t.m_LocalRotation.W },
                    localScale = new[] { t.m_LocalScale.X, t.m_LocalScale.Y, t.m_LocalScale.Z }
                };
            }).ToArray();
        WriteJson(Path.Combine(output, "components.json"), new
        {
            purpose = "raw-component-evidence-not-a-physics-package", prefab = args[3],
            vfsFingerprint = archive.Fingerprint, source, closure,
            files = manager.assetsFileList.Select(f => new
            {
                cab = f.fileName, f.unityVersion,
                externals = f.m_Externals.Select((x, i) => new { fileId = i + 1, cab = x.fileName })
            }),
            components = rows, transforms
        });
        Console.WriteLine($"Saved {rows.Count} MonoBehaviour records to {output}");
        return 0;
    }

    private static string Identity(AssetObject obj) => obj.assetsFile.fileName + ":" + obj.m_PathID;

    private static void Visit(GameObject obj, HashSet<string> seen)
    {
        if (!seen.Add(Identity(obj))) return;
        if (obj.m_Transform == null) return;
        foreach (var pointer in obj.m_Transform.m_Children)
            if (pointer.TryGet(out var child) && child.m_GameObject.TryGet(out var childObject))
                Visit(childObject, seen);
    }

    private static string OwnerPath(GameObject obj)
    {
        var names = new List<string>();
        var seen = new HashSet<string>();
        while (obj != null)
        {
            if (!seen.Add(Identity(obj))) throw new InvalidDataException("Cyclic Transform hierarchy.");
            names.Add(obj.m_Name);
            if (obj.m_Transform?.m_Father.TryGet(out var parent) != true ||
                !parent.m_GameObject.TryGet(out obj)) break;
        }
        names.Reverse();
        return string.Join("/", names);
    }

    private static object Plain(object value)
    {
        if (value is KeyValuePair<object, object> pair)
            return new { Key = Plain(pair.Key), Value = Plain(pair.Value) };
        if (value is IDictionary dictionary)
        {
            var result = new Dictionary<string, object>();
            foreach (DictionaryEntry entry in dictionary) result.Add((string)entry.Key, Plain(entry.Value));
            return result;
        }
        if (value is IEnumerable items && value is not string && value is not byte[])
            return items.Cast<object>().Select(Plain).ToArray();
        return value;
    }

    private static void CollectReferences(object value, string field, SerializedFile file, List<object> result)
    {
        if (value is IDictionary dictionary)
        {
            if (dictionary.Contains("m_FileID") && dictionary.Contains("m_PathID"))
            {
                var fileId = Convert.ToInt32(dictionary["m_FileID"]);
                var pathId = Convert.ToInt64(dictionary["m_PathID"]);
                var pointer = new PPtr<AssetObject>(fileId, pathId, file);
                pointer.TryGet(out var target);
                GameObject owner = target as GameObject;
                if (target is Component component) component.m_GameObject.TryGet(out owner);
                result.Add(new
                {
                    field, fileId, pathId, isNull = pointer.IsNull, resolved = target != null,
                    identity = target == null ? null : Identity(target),
                    type = target?.type.ToString(), ownerPath = OwnerPath(owner)
                });
                return;
            }
            foreach (DictionaryEntry entry in dictionary)
                CollectReferences(entry.Value, field + "." + entry.Key, file, result);
        }
        else if (value is KeyValuePair<object, object> pair)
        {
            CollectReferences(pair.Key, field + ".Key", file, result);
            CollectReferences(pair.Value, field + ".Value", file, result);
        }
        else if (value is IEnumerable sequence && value is not string && value is not byte[])
        {
            var index = 0;
            foreach (var item in sequence) CollectReferences(item, field + $"[{index++}]", file, result);
        }
    }

    private static void WriteJson(string path, object data)
    {
        using var stream = new FileStream(path, FileMode.CreateNew, FileAccess.Write);
        JsonSerializer.Serialize(stream, data, JsonOptions);
    }
}
