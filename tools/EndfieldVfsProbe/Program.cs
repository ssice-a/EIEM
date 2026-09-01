using System.Diagnostics;
using System.Security.Cryptography;
using AnimeStudio;
using AnimeStudio.GUI;

if (args.Length == 3 && args[0] == "--find-index")
{
    var index = EndfieldIndexStore.Load(args[1]).Assets;
    var query = args[2];
    var matches = index.Records.Where(x =>
            x.Container.Contains(query, StringComparison.OrdinalIgnoreCase) ||
            x.Name.Contains(query, StringComparison.OrdinalIgnoreCase))
        .Take(500).ToArray();
    foreach (var record in matches)
        Console.WriteLine($"{record.Type}|{record.PathId}|{record.Name}|{record.Container}|{record.Source}");
    Console.WriteLine($"matches={matches.Length}");
    return matches.Length > 0 ? 0 : 1;
}

if (args.Length == 4 && args[0] == "--find-record")
{
    var index = EndfieldIndexStore.Load(args[1]).Assets;
    var matches = index.Records.Where(x =>
            string.Equals(x.Name, args[2], StringComparison.OrdinalIgnoreCase) &&
            x.Container.Contains(args[3], StringComparison.OrdinalIgnoreCase))
        .ToArray();
    foreach (var record in matches)
        Console.WriteLine($"{record.Type}|{record.PathId}|{record.Name}|{record.Container}|{record.Source}");
    Console.WriteLine($"matches={matches.Length}");
    return matches.Length > 0 ? 0 : 1;
}

if (args.Length == 2 && args[0] == "--vfs-stats")
{
    var statsArchive = EndfieldVfsArchive.Open(args[1]);
    var bundles = statsArchive.Entries.Values.Where(x => x.LogicalPath.EndsWith(".ab", StringComparison.OrdinalIgnoreCase)).ToArray();
    Console.WriteLine($"files={statsArchive.Entries.Count:N0} bundles={bundles.Length:N0} bundleBytes={bundles.Sum(x => x.Length) / 1024d / 1024d / 1024d:N2} GiB fingerprint={statsArchive.Fingerprint}");
    return 0;
}

if (args.Length == 4 && args[0] == "--build-index")
{
    var buildArchive = EndfieldVfsArchive.Open(args[1]);
    var limit = int.Parse(args[3]);
    var progress = new Progress<EndfieldIndexProgress>(value =>
    {
        if (value.ProcessedBundles % 1024 == 0 || value.ProcessedBundles == value.TotalBundles)
            Console.WriteLine($"{value.ProcessedBundles}/{value.TotalBundles} assets={value.AssetCount} cabs={value.CabCount} failed={value.FailedBundles}");
    });
    var path = await EndfieldVfsAssetIndexBuilder.BuildAsync(
        buildArchive, args[2], GameManager.GetGameByType(GameType.ArknightsEndfield),
        progress, CancellationToken.None, limit);
    var built = EndfieldIndexStore.Load(path).Assets;
    Console.WriteLine($"index={path} assets={built.AssetCount} directories={built.DirectoryCount}");
    return built.AssetCount > 0 ? 0 : 1;
}

if (args.Length == 3 && args[0] == "--stream-map")
{
    var streamArchive = EndfieldVfsArchive.Open(args[1]);
    var bytes = streamArchive.ReadPayload(args[2]);
    using var payload = new MemoryStream(bytes, writable: false);
    var entries = AssetsHelper.BuildAssetMapFromStream(payload, args[2], GameManager.GetGameByType(GameType.ArknightsEndfield));
    Console.WriteLine($"entries={entries.Count}");
    foreach (var entry in entries.Take(10))
        Console.WriteLine($"{entry.Type}|{entry.PathID}|{entry.Name}|{entry.Container}|{entry.Source}");
    return entries.Count > 0 ? 0 : 1;
}

if (args.Length == 4 && args[0] == "--extract")
{
    var extractArchive = EndfieldVfsArchive.Open(args[1]);
    var output = extractArchive.ExtractToCache(args[2], args[3]);
    Console.WriteLine(output);
    return File.Exists(output) ? 0 : 1;
}

if (args.Length == 5 && args[0] == "--extract-closure")
{
    var closureArchive = EndfieldVfsArchive.Open(args[1]);
    var dependencyIndex = EndfieldIndexStore.Load(args[2]).Dependencies;
    var bundles = dependencyIndex.ResolveBundleClosure(args[3]);
    foreach (var bundle in bundles)
        Console.WriteLine(closureArchive.ExtractToCache(bundle, args[4]));
    Console.WriteLine($"bundles={bundles.Count}");
    return bundles.Count > 0 ? 0 : 1;
}

if (args.Length == 6 && args[0] == "--inspect-closure")
{
    var closureArchive = EndfieldVfsArchive.Open(args[1]);
    var dependencyIndex = EndfieldIndexStore.Load(args[2]).Dependencies;
    var logicalPath = args[3];
    var targetPathId = long.Parse(args[5]);
    var bundlePaths = dependencyIndex.ResolveBundleClosure(logicalPath);
    var cachePaths = bundlePaths.Select(path => closureArchive.ExtractToCache(path, args[4])).ToArray();
    var manager = new AssetsManager
    {
        Game = GameManager.GetGameByType(GameType.ArknightsEndfield),
        ResolveDependencies = false,
    };
    manager.LoadFiles(cachePaths, mergeSplitAssets: false);
    var sourceCabs = dependencyIndex.GetCabNames(logicalPath).ToHashSet(StringComparer.OrdinalIgnoreCase);
    foreach (var file in manager.assetsFileList.Where(x => sourceCabs.Contains(x.fileName)))
    {
        var gameObject = file.Objects.OfType<GameObject>()
            .FirstOrDefault(x => x.m_PathID == targetPathId);
        if (gameObject == null)
            continue;
        Console.WriteLine($"files={manager.assetsFileList.Count} bundles={bundlePaths.Count} object={gameObject.m_Name} hasModel={gameObject.HasModel()}");
        return gameObject.HasModel() ? 0 : 1;
    }
    Console.Error.WriteLine("target GameObject not found");
    return 1;
}

if (args.Length == 6 && args[0] == "--inspect-prefab")
{
    var prefabArchive = EndfieldVfsArchive.Open(args[1]);
    var loadedIndex = EndfieldIndexStore.Load(args[2]);
    var logicalPath = args[3];
    var prefabName = args[5];
    var bundlePaths = loadedIndex.Dependencies.ResolveBundleClosure(logicalPath);
    var cachePaths = bundlePaths.Select(path => prefabArchive.ExtractToCache(path, args[4])).ToArray();
    var manager = new AssetsManager
    {
        Game = GameManager.GetGameByType(GameType.ArknightsEndfield),
        ResolveDependencies = false,
    };
    manager.LoadFiles(cachePaths, mergeSplitAssets: false);
    var matches = manager.assetsFileList.SelectMany(x => x.Objects).OfType<GameObject>()
        .Where(x => string.Equals(x.m_Name, prefabName, StringComparison.OrdinalIgnoreCase))
        .ToArray();
    foreach (var gameObject in matches)
        Console.WriteLine($"pathId={gameObject.m_PathID} cab={gameObject.assetsFile.fileName} " +
                          $"root={gameObject.m_Transform?.m_Father.IsNull == true} hasModel={gameObject.HasModel()}");
    var records = loadedIndex.Assets.Records.Where(x =>
            string.Equals(x.Source, logicalPath, StringComparison.OrdinalIgnoreCase) &&
            x.Container.EndsWith($"/{prefabName}.prefab", StringComparison.OrdinalIgnoreCase))
        .ToArray();
    if (records.Length == 0)
        records = loadedIndex.Assets.Records.Where(x =>
                x.Container.EndsWith($"/{prefabName}.prefab", StringComparison.OrdinalIgnoreCase))
            .ToArray();
    var file = records.Length == 0 ? null : new VirtualAssetFile(records[0].Container, records);
    var root = file == null ? null : EndfieldPrefabDocument.FindRoot(
        manager.assetsFileList.SelectMany(x => x.Objects).OfType<GameObject>(), file,
        loadedIndex.Dependencies.GetCabNames(logicalPath));
    var structure = root == null ? string.Empty : EndfieldPrefabDocument.Build(file, root);
    Console.WriteLine($"files={manager.assetsFileList.Count} bundles={bundlePaths.Count} matches={matches.Length} " +
                      $"resolvedRoot={root?.m_PathID.ToString() ?? "none"} structureChars={structure.Length} " +
                      $"hasMeshRefs={structure.Contains(" Mesh=")} hasMaterialRefs={structure.Contains(" Materials=[")}");
    return root != null && structure.Contains(" Mesh=") ? 0 : 1;
}

if (args.Length == 7 && args[0] == "--export-model")
{
    var closureArchive = EndfieldVfsArchive.Open(args[1]);
    var dependencyIndex = EndfieldIndexStore.Load(args[2]).Dependencies;
    var logicalPath = args[3];
    var targetPathId = long.Parse(args[5]);
    var bundlePaths = dependencyIndex.ResolveBundleClosure(logicalPath);
    var cachePaths = bundlePaths.Select(path => closureArchive.ExtractToCache(path, args[4])).ToArray();
    var game = GameManager.GetGameByType(GameType.ArknightsEndfield);
    var manager = new AssetsManager { Game = game, ResolveDependencies = false };
    manager.LoadFiles(cachePaths, mergeSplitAssets: false);
    var sourceCabs = dependencyIndex.GetCabNames(logicalPath).ToHashSet(StringComparer.OrdinalIgnoreCase);
    var gameObject = manager.assetsFileList.Where(x => sourceCabs.Contains(x.fileName))
        .SelectMany(x => x.Objects).OfType<GameObject>()
        .FirstOrDefault(x => x.m_PathID == targetPathId);
    if (gameObject == null || !gameObject.HasModel())
    {
        Console.Error.WriteLine("target model was not found");
        return 1;
    }

    var uvs = Enumerable.Range(0, 8).ToDictionary(
        i => $"UV{i}", i => (i < 2, i < 2 ? i : 0));
    var convert = new ModelConverter(gameObject, new ModelConverter.Options
    {
        imageFormat = ImageFormat.Png,
        game = game,
        collectAnimations = false,
        exportMaterials = true,
        materials = new HashSet<Material>(),
        uvs = uvs,
        texs = new Dictionary<string, int>(),
    });
    var output = Path.GetFullPath(args[6]);
    ModelExporter.ExportFbx(output, convert, new Fbx.ExportOptions
    {
        eulerFilter = true,
        filterPrecision = 0.25f,
        exportAllNodes = true,
        exportSkins = true,
        exportAnimations = false,
        exportBlendShape = true,
        castToBone = false,
        boneSize = 10,
        scaleFactor = 1,
        fbxVersion = 3,
        fbxFormat = 0,
    });
    Console.WriteLine($"output={output} meshes={convert.MeshList.Count} materials={convert.MaterialList.Count} textures={convert.TextureList.Count} morphs={convert.MorphList.Count}");
    return File.Exists(output) && new FileInfo(output).Length > 0 ? 0 : 1;
}

if (args.Length == 2 && args[0] == "--assetmap")
{
    var mapStopwatch = Stopwatch.StartNew();
    var map = Path.GetExtension(args[1]).Equals(".eidx", StringComparison.OrdinalIgnoreCase)
        ? EndfieldIndexStore.Load(args[1]).Assets
        : VirtualAssetPathIndex.LoadXml(args[1]);
    GC.Collect();
    Console.WriteLine($"assets={map.AssetCount:N0} directories={map.DirectoryCount:N0} elapsed={mapStopwatch.Elapsed} managed={GC.GetTotalMemory(false) / 1024d / 1024d:N1} MiB");
    return 0;
}

if (args.Length != 4)
{
    Console.Error.WriteLine("usage: EndfieldVfsProbe --assetmap <xml> | --stream-map <VFS root> <logical path> | --extract <VFS root> <logical path> <workspace> | --extract-closure <VFS root> <index XML> <logical path> <workspace> | --inspect-closure <VFS root> <index XML> <logical path> <workspace> <PathID> | --export-model <VFS root> <index XML> <logical path> <workspace> <PathID> <output FBX> | <VFS root> <logical path> <workspace> <known plaintext file>");
    return 2;
}

var stopwatch = Stopwatch.StartNew();
var archive = EndfieldVfsArchive.Open(args[0]);
Console.WriteLine($"index entries={archive.Entries.Count:N0} elapsed={stopwatch.Elapsed}");
var extracted = archive.ExtractToCache(args[1], args[2]);
var actual = Convert.ToHexString(SHA256.HashData(File.ReadAllBytes(extracted)));
var expected = Convert.ToHexString(SHA256.HashData(File.ReadAllBytes(args[3])));
Console.WriteLine($"logical={args[1]}");
Console.WriteLine($"output={extracted}");
Console.WriteLine($"actual={actual}");
Console.WriteLine($"expected={expected}");
Console.WriteLine($"equal={actual == expected}");
return actual == expected ? 0 : 1;
