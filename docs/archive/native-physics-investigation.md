# 鍘熺敓鐗╃悊鎺ュ叆璋冩煡

鐘舵€侊細**authoritative**锛堝師鐢熸帴鍏ヨ瘉鎹€佽瘉鎹檺鍒朵笌褰撳墠楠岃瘉鐘舵€侊級銆備腑闂磋鍒掍笉鑳戒綔涓哄凡楠岃瘉浜嬪疄锛?
鏈夊啿绐佹椂浠ユ渶鏂板疄娴嬬粨鏋滃強鍏堕€傜敤鑼冨洿涓哄噯銆傚綋鍓嶅伐浣滃尯楠岃瘉璁板綍瑙佺 20.25 鑺傘€?

鏈枃鎸夎皟鏌ラ樁娈典繚鐣欓潤鎬佸彇璇併€佸涓诲疄鐜板強褰撴椂楠岃瘉缁撴灉銆傜 1锝? 鑺備负鍘熺敓璋冩煡锛?
绗?10锝?4 鑺傝褰曡瘖鏂櫒涓庨€傞厤鍣ㄨ繘灞曪紱鍚勮妭鈥滄湰杞?灏氭湭鈥濆搴旇鑺傛椂鐐广€?
褰撳墠宸ヤ綔鍖虹姸鎬佷互[鏂囨。绱㈠紩](../README.md)鍜屾湰鏂囨湯灏炬牳瀵硅褰曚负鍑嗭紱璁捐濂戠害瑙乕鐗╃悊璁捐](../physics-authoring-design.md)銆?

褰撳墠璇婃柇鍏ュ彛浠ョ 20.24 鑺傜殑 v70 瀹炵幇涓哄噯锛氱墿鐞嗚瘖鏂凡浠?Dump 椤甸潰鍜屼富鍔熻兘绐楀彛娑堟伅涓Щ闄わ紝
娴嬭瘯 DLL 鍦ㄨ繘鍏ユ父鎴忎富绾跨▼鍏ュ彛鍚庤嚜鍔ㄥ紑濮嬭窡韪紝骞跺湪姝ｅ父鍏抽棴璺緞鍐欏叆鐙珛鐨?
`plugin/physics_diagnostics` 鐩綍銆傜 10锝?7 鑺備腑鍏充簬 Dump 鎸夐挳銆佹墜鍔ㄥ紑濮?鍋滄鍜屾棫 dump
鐩綍鐨勬枃瀛楀彧淇濈暀涓虹増鏈紨杩涜褰曪紝涓嶅啀鎻忚堪褰撳墠 UI 鎴栧綋鍓嶈瘖鏂叆鍙ｃ€?

## 1. 鏈疆缁撹

涓嶆槸缂哄皯涓€涓垜浠嚜宸辩殑鐗╃悊姹傝В鍣ㄣ€傚綋鍓嶆父鎴忕殑 `BeyondDynamicBone.dll` 鍏冩暟鎹拰
GameAssembly 鏂规硶浣撲腑宸茬粡鏈夎繍琛屾椂鏋勫缓銆佷唬鐞?绾︽潫/纰版挒娉ㄥ唽銆佸師鐢?Job 妯℃嫙鍙?Animator 缂撳啿鍐欏洖銆?
EIEM 瑕佽ˉ鐨勬槸**鏁版嵁璇诲啓銆佸疄渚嬪紩鐢ㄩ噸缁戝畾銆佸師鐢熸瀯寤轰笌鐢熷懡鍛ㄦ湡鎺ュ叆**銆?

闈欐€佽皟鐢ㄨ皟鏌ユ妸鈥滄帴鍙ｅ悕绉扮嚎绱⑩€濇帹杩涘埌鈥滃凡瀹氫綅鏂规硶浣撳強瀹為檯璋冪敤鐐光€濓紱
鍚庣画绂荤嚎鍙栬瘉宸茶鍑?Typhoea 鐨?11 涓師鐢熷竷鏂?楠ㄩ妯℃嫙缁勫拰 27 涓鎾炰綋锛岃瑙佺 8 鑺傘€?
v65锝瀡70 宸蹭緷娆″疄娴嬫渶灏忔柊澧為摼銆丄nimator Team 鎺ョ撼銆乀ransform 鍐欏洖銆乁I 娉ㄩ攢銆佺簿纭綔鑰呰妭鐐归摼锛屼互鍙?
鏂板 Skeleton 鑺傜偣瀵规浛鎹?Mesh 鐨勫彲瑙佽挋鐨紱杩欎簺缁撴灉瓒充互缁撴潫瀵光€滆兘鍚︾敤鍘熺敓鍚庣澧炲姞 BoneCloth鈥濈殑瀹芥硾璋冩煡銆?
灏氭湭瀹屾垚鐨勬槸鍏朵綑鍥涗釜鏍囬噺鐨勮涓鸿涔夈€佺鎾炰綋鍜?v2 婧愬浘瀹炰緥鍖栵紝
鍥犳涓嶈兘鎶婃牳蹇冩満鍒跺疄楠屽啓鎴愬畬鏁?Physics 鍔熻兘宸茬粡浜や粯銆?

鏈€閲嶈鐨勬柊璇佹嵁锛歚TeamManager.UpdateTeamAnimatorData` 鐪熸璋冪敤
`Animator.DisableClothBindings 鈫?CreateClothBindings(Transform[]) 鈫?EnableClothBindings`锛?
骞舵寜 Animator 韬唤淇濆瓨杩斿洖鍙ユ焺銆傛柊寤?Transform 涓庤繘鍏ヨ繖涓粦瀹氱郴缁熸槸涓嶅悓姝ラ銆?

## 2. 鏍锋湰涓庡鐜拌竟鐣?

杈撳叆浣嶄簬 `D:\Hypergryph Launcher\games\Endfield Game`锛?

| 鏂囦欢 | SHA256 |
|---|---|
| `GameAssembly.dll`锛?56677352 瀛楄妭 | `C24495E51B406F03B03890C4788EE618AE022C991405BE5D5B8B787CB775AE89` |
| `Endfield_Data/il2cpp_data/Metadata/global-metadata.dat`锛?7021288 瀛楄妭 | `0076743397ACADF03D3B0064343A963C7C88863B8160526D397E4B3EFB96F02E` |

姝ゅ厓鏁版嵁鐗堟湰涓?29锛岀被鍨嬭褰曟闀挎槑纭€?92銆傞€氳繃绋嬪簭闆嗚寖鍥淬€佹柟娉?owner/token 鍜屾寚閽堣〃鑼冨洿鏍稿锛?
`BeyondDynamicBone.dll` 浠ｇ爜鐢熸垚妯″潡 RVA 涓?`0xAC33E10`锛屾柟娉曟寚閽堟Ы鏁?3221銆?
绫绘竻鍗曡杞?641 涓被鍨嬶紝鍖呮嫭鍐呴儴绫诲瀷锛涗笉鑳芥妸 641 鐞嗚В鎴愮敤鎴烽渶瑕侀厤缃殑鐗╃悊绫诲瀷鏁伴噺銆?

浣跨敤宸叉湁鍙宸ュ叿锛?

```powershell
python tools/diagnostics/inspect_il2cpp_type.py '<娓告垙鐩綍>\Endfield_Data\il2cpp_data\Metadata\global-metadata.dat' BeyondBoneCloth --namespace BeyondDynamicBone --type-stride 92 --image '<娓告垙鐩綍>\GameAssembly.dll' --module-rva 0xac33e10
python tools/diagnostics/inspect_native_shapes.py '<娓告垙鐩綍>\GameAssembly.dll' --rva 0x5a2283c --limit 6000 --text
```

鍘熷鏂规硶琛ㄥ強鎸囦护鎶ュ憡淇濆瓨浜?`E:\EIEM_Workspace\physics-diagnostics\20260907-native-contract`锛?
`beyond-bone-cloth.json`銆乣cloth-process.json`銆乣team-manager.json`銆乣native-functions.json`銆?
鎶ュ憡涓烘湰杞柊寤猴紝娌℃湁瑕嗙洊鏃у疄楠屻€傝繖閲屾墍鏈?RVA 鍙敤浜庡鏌ヨ繖浠芥枃浠讹紝涓嶅噯纭紪鐮佽繘 DLL銆?

瀛楁鍚嶅彟鐢卞厓鏁版嵁 fieldStart/field_count 鎻愬彇锛屾牳瀵瑰瓧娈?token 涓?`0x04xxxxxx`锛?
瀹冧滑鏄０鏄庡瓧娈碉紝涓嶇瓑浜?Unity 搴忓垪鍖栧瓧娈垫竻鍗曪紝涔熶笉鎻愪緵璧勪骇鍊笺€佸崟浣嶆垨鏂囦欢鍋忕Щ銆?

鍙嶆眹缂栧伐鍏风殑 unwind range 涓嶆槸鍙潬鐨勫畬鏁存墭绠℃柟娉曡竟鐣岋細
`StartRuntimeBuild` 琚垎鎴愬涓寖鍥达紱`ForceCompleteAllJob` 鍒欎綅浜庡寘鍚涓皬璺虫澘鐨勮寖鍥村唴銆?
鍓嶈€呭彟璇?`0x343B480:216` 鎸囦护绐楀彛锛屽悗鑰呬粎纭 `0x507ECA8` 寮€濮?7 瀛楄妭鐨勫熬璺炽€?
鏈В鏋愮殑闂存帴璋冪敤銆佸喎鍒嗘敮涓嶈涓轰笉瀛樺湪锛涚浉鍚?RVA 鐨勪唬鐮佹姌鍙犱笉鎰忓懗鐫€涓嶅悓鍚嶅瓧灞炰簬鍚屼竴棰嗗煙鏂规硶銆?

## 3. 鍘熺敓鏁版嵁鍒嗗眰宸茬粡鎵惧埌

| 瀵硅薄 | 鏈疆纭鐨勫唴瀹?| 鍚箟 |
|---|---|---|
| `BeyondBoneCloth` | 澹版槑 33 涓瓧娈碉紝鍚?serializeData銆乻erializeData2銆乸rocess銆丱nBuildComplete銆佷紶閫佸鐞嗙姸鎬佸拰鍔ㄧ敾鍙傛暟 | 閰嶇疆銆佽繍琛岀姸鎬佸拰瀹屾垚閫氱煡鍒嗗紑锛涗笉鑳芥暣浣?memcpy |
| `ClothSerializeData` | 澹版槑 42 涓瓧娈碉紝瑙佷笅琛?| 鏄富瑕佷綔鑰呭弬鏁板叆鍙ｏ紱涓嶇瓑浜?42 涓兘鑳界洿鎺ョ儹鏀?|
| `ClothSerializeData2` | selectionData銆乥oneAttributeDict銆乿ertexAttributeList銆乸reBuildData | 鍥哄畾/杩愬姩灞炴€т笌棰勬瀯寤烘暟鎹彟瀛橈紝涓嶈兘鍙鍑洪鏋?TRS |
| `ClothProcess` | 澹版槑 52 涓瓧娈碉紝鍚?State_Build/Running/UsePreBuild銆乀eamId銆乧olliderList銆乧ts銆乴ockObject銆乮sDestory/isDestoryInternal/isBuild | 缁勮韩浠姐€佸紓姝ユ瀯寤哄拰閲婃斁鐘舵€佸睘浜庤繍琛屾椂锛屼笉鑳藉啓鍥炰綔鑰呰祫婧?|
| `ColliderComponent` | center銆乻ize銆乼eamIdSet | 鍑犱綍瀹氫箟涓庡悇妯℃嫙缁勭殑娉ㄥ唽鍏崇郴鍒嗗紑 |
| `BeyondBoneCapsuleCollider` | direction銆乺everseDirection銆乺adiusSeparation銆乤lignedOnCenter | 鑳跺泭涓嶅彧鏄€滀竴涓粺涓€鍗婂緞鈥濓紱绗?8 鑺傜‘璁?size 鍒嗛噺锛岀 20.30 鑺傜‘璁ょ鐞冨績涓庢€婚暱绠楁硶 |

`ClothSerializeData` 瀛楁鎸夌紪杈戠敤閫斿綊绫伙紝浠ヤ笅鍚嶇О鏉ヨ嚜鏈満鍏冩暟鎹€岄潪 RE 瀛楁濂楃敤锛?

| 绫诲埆 | 澹版槑瀛楁 |
|---|---|
| 杈撳叆涓庢嫇鎵?| clothType銆乻ourceRenderers銆乵eshWriteMode銆乸aintMode銆乸aintMaps銆乺ootBones銆乮gnoreFromRootBones銆乧onnectionMode銆乺otationalInterpolation銆乺ootRotation |
| 璋冨害/娣峰悎/LOD | updateMode銆乧lothAnimatorAbilityLODThreshold銆乧lothAnimatorLODThreshold銆乧lothLodFadeTime銆乧lothSimulateWeight銆乺esetSimulationToAnimationPoseWhenWeightLow銆乺esetSimulationToAnimationPoseWeightThreshold銆乤nimationPoseRatio |
| 鏋勫缓涓庢柟鍚?| reductionSetting銆乧ustomSkinningSetting銆乶ormalAlignmentSetting銆乧ullingSettings銆乶ormalAxis |
| 鍩虹鍔ㄥ姏鍙傛暟 | gravity銆乬ravityDirection銆乬ravityFalloff銆乻tablizationTimeAfterReset銆乥lendWeight銆乨amping銆乺adius |
| 绾︽潫涓庣幆澧?| inertiaConstraint銆乼etherConstraint銆乨istanceConstraint銆乼riangleBendingConstraint銆乤ngleRestorationConstraint銆乤ngleLimitConstraint銆乵otionConstraint銆乧olliderCollisionConstraint銆乻elfCollisionConstraint銆亀ind銆乻pringConstraint |
| 鏍￠獙鐘舵€?| verificationResult |

鏈妭璁板綍鐨勬槸鏈€鍒濆厓鏁版嵁璋冩煡锛涚 8 鑺傚凡缁忎粠鐪熷疄璧勪骇灞曞紑绫诲瀷銆佹洸绾垮拰宓屽鍊笺€?
涓嶈兘鎶?verificationResult銆乼eamIdSet 鎴栧紓姝ョ姸鎬佸綋浣滃彲缂栬緫鐗╃悊鍙傛暟銆?

### 鐩爣瑙掕壊璧勬簮璇佹嵁

鐜版湁 `E:\EIEM_Workspace\assets\beyond\dynamicassets\gameplay\actors\postmodels\characters\chr_0034_typhoea_postmodel.prefab.structure.txt`
鏈?`MBC_Typhoea_Cloth_Coat/Skirt/Skirt_Rope/Skirt_Bag` 鑺傜偣锛屼互鍙婂涓?
`Magica Capsule Collider (...)` 鑺傜偣銆備絾瀹冨彧鍒?`[MonoBehaviour] PathID`锛屾湭灞曞紑鑴氭湰绫绘垨灏哄鍙傛暟銆?
鑺傜偣鍚嶅彧鑳芥寚绀鸿皟鏌ュ璞★紝涓嶈兘鍗曢潬鍚嶅瓧璁ゅ畾鍏跺悗绔拰鍙傛暟銆?

宸叉湁绫诲瀷蹇収涓殑 `BoneClothItem` 杩樹繚瀛?boneClothData銆乻electionData銆乺ootBoneList銆?
ignoredFromrootBoneList銆乻kinningBoneList銆乧olliderParentBoneList锛?
`NPCAvatarMeshAssetsSO` 鏈?boneClothItems 鍜?referencedMeshAssets銆?
鍥犳绂荤嚎瀵煎嚭闇€瑕佽拷韪厤缃紩鐢ㄩ棴鍖咃紝涓嶈兘鍙褰撳墠 Renderer 鎴栧彧鏀舵湁鏉冮噸鐨勯楠笺€?
杩欎笉鏄绉?Typhoea 鎵€鏈夌墿鐞嗛兘缁忚繃 NPC 璺緞銆?

## 4. 宸茬‘璁ょ殑鏋勫缓璋冪敤

| 鏈満鏂规硶 RVA | 瀹為檯闈欐€佽瘉鎹?|
|---|---|
| `BeyondBoneCloth.BuildAndRun` `0x59DC2D8` | 璋?DisableAutoBuild锛涙鏌?Process 鐘舵€佸拰 GenerateInitialization锛涙牴鎹暟鎹垎鏀皟鐢?PreBuildDataConstruction 鎴?StartRuntimeBuild |
| `ClothProcess.Init` `0x343AA00` | 鍙傛暟/鐘舵€佹牎楠屻€丟etClothParameters銆乀ransformRecord銆丆reateBoneRenderSetupData锛涗篃鏈夐鏋勫缓鏁版嵁楠岃瘉鍜岀櫥璁?|
| `ClothProcess.StartRuntimeBuild` `0x343B480` | 璁剧疆鏋勫缓鐘舵€侊紝鍙栧緱鍙栨秷浠ょ墝锛宍0x343B4F3` 璋?RuntimeBuildAsync锛岄殢鍚庤繑鍥?true |
| `<RuntimeBuildAsync>d__10.MoveNext` `0x38E31C0` | Task.Run/awaiter銆佸彇娑堟鏌ャ€佹敞鍐屻€乁pdateUse 绛夌湡瀹炰唬鐮侊紱涓嶆槸涓€涓┖鍗犱綅鍏ュ彛 |
| 鏋勫缓宸ヤ綔鍑芥暟 `0x3DDCEC0` | VirtualMesh.ImportFrom銆丼election銆丷eduction銆丱ptimization銆丆onvertProxyMesh銆丮apping |
| 绾︽潫宸ヤ綔鍑芥暟 `0x343A390` | DistanceConstraint.CreateData銆乀riangleBendingConstraint.CreateData銆両nertiaConstraint.CreateData锛屽惈鍙栨秷妫€鏌?|

RuntimeBuildAsync 鐨勭姸鎬佹満涓彲瑙佷互涓嬫敞鍐岃皟鐢ㄧ偣锛?

1. `0x38E56B9`锛欳lothManager.AddCloth銆?
2. `0x38E632F`锛欴ynamicBoneTransformManager.AddTransform銆?
3. `0x38E6925`锛歋imulationManager.RegisterProxyMesh銆?
4. `0x38E6945`锛欳olliderManager.Register銆?
5. `0x38E6A15`锛歋imulationManager.RegisterConstraint銆?
6. `0x38E6C3A`锛歏irtualMeshManager.RegisterMappingMesh銆?
7. `0x38E6E21`锛欳lothProcess.UpdateUse銆?

杩欐槸鎵€妫€鏌ユ柟娉曚腑鐨勮皟鐢ㄤ綅缃憳瑕侊紝涓嶆槸缁曡繃 BuildAndRun 鎵嬪伐涓茶皟搴曞眰鎺ュ彛鐨勬搷浣滄寚鍗楋紱
涓棿鏈夌瓑寰呫€佺姸鎬佹鏌ャ€佹暟鎹搷浣滃拰閿欒璺緞銆傞鏋勫缓鍒嗘敮涔熸壘鍒板搴旂殑浠ｇ悊/纰版挒/绾︽潫鐧昏銆?

**BuildAndRun 杩斿洖 true 涓嶇瓑浜庡紓姝ュ凡鏋勫缓鎴愬姛銆?* StartRuntimeBuild 鐨勫疄鐜板凡缁忚瘉鏄庡惎鍔ㄥ拰瀹屾垚鏄袱涓椂鍒汇€?
OnBuildComplete 瀛楁銆丷esult銆両sRunning 鏄牳瀵瑰畬鎴愯涔夌殑鍏ュ彛銆傜 9 鑺傝繘涓€姝ョ‘璁や簡鍗?Boolean 閫氱煡杞借嵎銆?
鍚屾/寮傛瑙﹀彂璺緞鍜屽彇娑堟椂涓嶉€氱煡鐨勫垎鏀紱瀹為檯娉涘瀷瀛楁绫诲瀷銆佸洖璋冪嚎绋嬪拰璋冨害瀹夊叏鐐瑰皻寰呰繍琛屾椂鏍搁獙銆?
鏃?`trojan.h` 璋冪敤鍚庣洿鎺ュ鍔?rebuilt 璁℃暟锛屼笉鑳戒綔涓烘柊澧炵墿鐞嗘垚鍔熺殑鍒ゆ柇銆?

## 5. 妯℃嫙缁撴灉濡備綍閫佸洖楠ㄦ灦

`TeamManager` 鏈?teamId2AnimatorInstnceId銆乤nimatorID2RWHandler銆乼ransformID2RWHandlerID銆?
teamId2Animator 鍜?dirtyAnimatorTransformTeams锛沗DynamicBoneTransformManager` 鏈夊悇绫讳綅缃?鏃嬭浆鏁扮粍銆?
transformAccessArray銆乤nimatorTransformMap 鍜?teamIdArray銆?

### 娉ㄥ唽鍒?Animator 鐨勭湡瀹炶皟鐢ㄧ偣

`TeamManager.UpdateTeamAnimatorData`锛歚0x5A2283C`銆?

- `0x5A231A4`锛氬宸叉湁缁戝畾璋冪敤 Animator.DisableClothBindings銆?
- `0x5A231C9`锛氬皢鏀堕泦鍒楄〃杞垚鏁扮粍鍚庯紝璋冪敤 Animator.CreateClothBindings銆?
- 闅忓悗鎸?Animator 鐨?GetInstanceID 淇濆瓨杩斿洖鐨?AnimationTransformRWBufferHandle銆?
- `0x5A2325F`锛欰nimator.EnableClothBindings銆?

`AddAnimatorTransform(teamId, t)`锛坄0x34408A0`锛夋湰韬笉鏄畬鏁寸殑鏂板楠ㄩ鏋勫缓鍣細
鎵€璇绘柟娉曟鏌?Transform 鍚庯紝鍚戠鐞嗗櫒闆嗗悎鐧昏 teamId锛涙病鏈夊湪杩欎釜鏂规硶閲岀洿鎺ヨ皟鐢?CreateClothBindings銆?
涓嶈兘鍑悕绉拌皟鐢ㄤ竴娆″氨澹扮О宸茶幏寰楁柊楠ㄩ缂撳啿妲姐€?

鐢辨寰楀埌鐨勬帴鍏ョ害鏉燂細涓€涓?Animator 鐨勫鏉＄墿鐞嗛摼蹇呴』鐢卞師鐢熺鐞嗗櫒鍏卞悓绠＄悊缁戝畾锛?
涓嶈兘璁╂瘡涓?Mod/Renderer 鍚勮嚜 DestroyClothBindings 鎴栧彧鐢ㄨ嚜宸辩殑楠ㄩ鏁扮粍瑕嗙洊鍏变韩鍙ユ焺銆?
娓告垙浼氭€庢牱澶勭悊鏈湪鍘?Avatar 鐨勬柊澧?Transform锛屼粛闇€纭鍘熺敓 CreateClothBindings 瀹炵幇鍜屽疄渚嬬粨鏋溿€?

### 姣忓抚鍘熺敓鏇存柊鐨勮皟鐢ㄨ瘉鎹?

`ClothManager.OnAfterLateUpdate` 灏捐烦 ClothUpdate锛坄0x32AD500`锛夈€傚悗鑰呭寘鍚細

- Team/鏃堕棿/椋庢洿鏂般€丷eadTransform 鍜?ReadAnimatorBufferData銆?
- PreProxyMeshUpdate銆丳reSimulationUpdate銆丼imulationStepUpdate銆?
- CalcDisplayPosition銆丳ostProxyMeshUpdate銆丳ostMappingMeshUpdate銆?
- WriteTransform銆丆opyDoubleBuffer銆乄riteAnimatorBufferData銆丳ostTeamUpdate銆丆ompleteMasterJob銆?

鍏朵腑 Animator 缂撳啿璇诲啓璋冪敤鐐逛负 `0x32AD88C`銆乣0x32ADED2`锛?
SimulationStepUpdate 璋冪敤鐐逛负 `0x32ADB31`銆?
鏈夋潯浠躲€佸墧闄ゅ拰璺ㄥ抚璺緞锛屼笉浠ｈ〃鎵€鏈夋潯鐩瘡甯у鎵€鏈夊璞￠兘鎵ц锛屼篃灏氭湭杩樺師鏁翠釜寮曟搸鐨?PlayerLoop銆?

鐗╃悊璁＄畻涓?GPU 钂欑毊鏄袱涓樁娈点€備笂闈㈣瘉瀹炲師鐢熺墿鐞?Job 鐨勭粨鏋滀細杩涘叆 Animator 缂撳啿锛?
EIEM 涓嶉渶瑕佸洜姝よ嚜宸卞疄鐜扮墿鐞嗭紝涔熶笉鑳界敤姣忓抚鏅氭湡瑕嗙洊 Transform 浠ｆ浛鍘熺敓鍐欏洖娉ㄥ唽銆?

## 6. 鍋滅敤涓嶆槸閿€姣侊紝鍚姩浠诲姟涓嶆槸鍙珛鍗抽噴鏀?

- BeyondBoneCloth.OnEnable `0x343B560` 鈫?ClothProcess.StartUse銆?
- BeyondBoneCloth.OnDisable `0x44C3640` 鈫?ClothProcess.EndUse銆?
- BeyondBoneCloth.OnDestroy `0x59DCF04` 鈫?DisposeTeleportResources 鈫?Process.Dispose銆?
- Process.Dispose `0x59DFE8C` 涓湁鍔犻攣銆佸け鏁?閿€姣佺姸鎬併€佸彇娑堢浉鍏宠皟鐢紝闅忓悗杩涘叆 DisposeInternal銆?
- DisposeInternal `0x59DEEBC` 鍏堟鏌ラ噴鏀?鏋勫缓鐘舵€侊紝鍐嶉€€鍑轰唬鐞嗐€乀ransform銆丮apping銆佺鎾炪€丆loth锛?
  閲婃斁鏋勫缓鏁版嵁鍜?Renderer 璁板綍銆佹敞閿€棰勬瀯寤鸿祫婧愩€佺Щ闄ょ洃瑙嗗叧绯汇€傚瓨鍦ㄦ瀯寤轰腑涓嶇珛鍗虫墽琛屽叏閮ㄩ噴鏀剧殑鍒嗘敮銆?
- TeamManager.RemoveTeam `0x5A22470` 涓湁 RemoveComponentTransform 鍜?ClearTeamAnimatorData銆?
- ForceCompleteAllJob `0x507ECA8` 鏄浆鍒?CompleteMasterJob 鐨勭煭璺虫澘锛屼笉鏄彟涓€涓竾鑳芥竻鐞嗗叆鍙ｃ€?

杩欓噷鍙‘璁や唬鐮佷腑瀛樺湪杩欎簺琛屼负锛涘皻鏈瘉鏄?DLL 浠庝换鎰忕嚎绋嬭皟鐢?Dispose 鎴?CompleteMasterJob 閮藉畨鍏紝
涔熸湭璇佹槑涓€涓富 Job 瀹屾垚灏变唬琛ㄥ悗鍙版瀯寤?Task 鍜屽叏閮ㄨ法甯х紦鍐插凡閫€鍑恒€?
F10/鎹㈠浘鏃堕』鏈嶄粠鍘熺敓瀹屾垚/鍙栨秷濂戠害锛屼笉鑳芥姠鍏堥攢姣佷粛琚换鍔″紩鐢ㄧ殑 Mod 楠ㄩ銆?

## 7. 涓嬩竴姝ュ叿浣撹ˉ浠€涔?

1. **鐪熷疄閰嶇疆鏍锋湰 鈫?浣滆€呮暟鎹绾?*锛氱 8 鑺傚凡瀹屾垚 Typhoea 鐨勮剼鏈€乀ypeTree銆乻election銆?
   preBuild銆佺鎾炰綋鍜岄楠煎紩鐢ㄨ鍙栥€傛帴鐫€纭 selection 鐐瑰簭鍒?Transform 鐨勬槧灏勩€佽兌鍥婄鐐?缂╂斁锛?
   鍐嶅埗浣?Blender 杈呭姪浣擄紱涓嶆妸璇诲彇瀹屾垚褰撴垚宸茬粡鏀寔淇敼鍚庡啓鍥炪€?
2. **娉ㄥ唽杈圭晫**锛氱户缁牳瀵?OnBuildComplete銆佸彇娑堝悗瀹屾垚鍜屽師鐢熺鐞嗗櫒鏇存柊瀹夊叏鐐癸紱
   鏌?CreateClothBindings 瀵规柊澧?Transform 鐨勫鐞嗗強杩斿洖妲芥槧灏勶紝涓嶇洿鎺ラ噸缁戞暣涓?Animator 璇曢敊銆?
3. **涓夌瀹炵幇**锛氬湪涓婅堪濂戠害娓呮鍚庯紝瀹炵幇鍙傛暟/寮曠敤鏃犳崯瀵煎叆瀵煎嚭銆佷慨鏀瑰師鐢熺粍浠跺強鏂板鍘熺敓閾俱€?
   杩愯鐘舵€佸拰缂撳啿鍙ユ焺涓嶈繘璧勬簮鏂囦欢锛屽叡浜鏋朵笉鎸?Mesh 澶嶅埗銆?
4. **闆嗕腑楠屾敹**锛氫竴娆¤褰曢厤缃簮銆侀楠艰矾寰勩€乼eam/Animator/缂撳啿鏄犲皠銆佹瀯寤哄紑濮?缁撴潫鍙婃敞閿€锛?
   瑕嗙洊涓栫晫銆佸睍绀?UI銆丗10 鍜屾崲鍥撅紱涓嶉潬鐪嬪埌缃戞牸浼氬姩灏辫涓虹墿鐞嗘纭€?

鏈疆娌℃湁澧炲姞 Hook銆佹ā鎷熷櫒鎴栧洖閫€锛屾病鏈夐儴缃叉柊 DLL銆傚綋鍓嶈兘纭畾鏂瑰悜鍜屽叧閿皟鐢紝
杩樹笉鑳芥妸鈥滃凡鑳藉鍏ュ師鐢熺墿鐞嗐€佹柊澧炵墿鐞嗛摼骞舵纭儹閲嶈浇鈥濇爣涓哄畬鎴愩€?

## 8. 2026-09-07 杩藉姞锛氱湡瀹?Prefab 缁勪欢涓庡紩鐢ㄩ棴鍖?

### 8.1 鍙栬瘉鏂规硶涓庣粨鏋?

鏂板绂荤嚎鍏ュ彛 [PrefabComponentProbe](../../tools/EndfieldVfsProbe/PrefabComponentProbe.cs)锛?
閫氳繃鐜版湁 VFS 绱㈠紩瀹氫綅閫昏緫 Prefab锛屽苟鍔犺浇婧?Bundle 鐨勪緷璧栭棴鍖呫€傚畠鍙鍙栬祫婧愶紝涓嶅姞杞芥垨鎵ц娓告垙 DLL銆?
鎸?`MonoScript` 璇嗗埆瀹為檯缁勪欢绫伙紝鎸夎祫浜ц嚜甯?TypeTree 璇诲彇瀛楁锛?*娌℃湁灏?IL2CPP 鍐呭瓨鍋忕Щ褰撲綔搴忓垪鍖栧亸绉?*銆?
寮曠敤韬唤浣跨敤 CAB + PathID锛屽彟淇濆瓨瀹屾暣 Transform 璺緞鍜?local TRS锛涗笉鑳藉崟鐢?PathID 璺?CAB 鍚堝苟銆?

杈撳叆锛?

- `assets/beyond/dynamicassets/gameplay/actors/postmodels/characters/chr_0034_typhoea_postmodel.prefab`銆?
- 绱㈠紩 `E:\EIEM_Workspace\index\endfield_assets.eidx`銆?
- 婧?Bundle `Bundles/Windows/main/f96b038ea799224659a99f83.ab`锛屼緷璧栭棴鍖?103 涓?Bundle銆?
- VFS fingerprint锛歚D6631362E327706C9DF00AE92D2E794209BAD6C49302F92BA6ED94C801526B50`銆?

鏈€鏂拌瘉鎹洰褰曪細`E:\EIEM_Workspace\physics-diagnostics\20260907-typhoea-reference-graph`銆?
鍏朵腑 `components.json` 鍖呭惈缁勪欢韬唤銆佽剼鏈€佸綊灞炪€佸紩鐢ㄤ笌 Transform锛涙瘡涓紪鍙峰彟鏈夊師濮?`.bin`銆?
`.schema.json`锛屽畬鏁磋В鐮佹垚鍔熸椂鎵嶇敓鎴?`.data.json`銆傛枃浠?SHA256 鍜屽師濮嬪瓧鑺傛暟閫愰」澶嶆牳閫氳繃銆?
杩欐槸**璇婃柇杈撳嚭锛屼笉鏄彲鍔犺浇鐨?Physics Mod 鍖?*銆?

澶嶇幇鍛戒护锛堣緭鍑虹洰褰曞繀椤诲皻涓嶅瓨鍦紝閬垮厤瑕嗙洊鏃у疄楠岋級锛?

```powershell
dotnet build tools/EndfieldVfsProbe/EndfieldVfsProbe.csproj -c Release --nologo
dotnet tools/EndfieldVfsProbe/bin/Release/net9.0/EndfieldVfsProbe.dll --inspect-prefab-components `
  '<娓告垙鐩綍>\Endfield_Data\StreamingAssets\VFS' 'E:\EIEM_Workspace' `
  'assets/beyond/dynamicassets/gameplay/actors/postmodels/characters/chr_0034_typhoea_postmodel.prefab' `
  '<鏂扮殑璇婃柇鐩綍>' 'E:\EIEM_Workspace\index\endfield_assets.eidx'
```

璇ュ伐鍏疯姹?VFS 涓庣储寮曟寚绾瑰畬鍏ㄧ浉绗︼紝杩囨湡绱㈠紩浼氭姤閿欙紝涓嶆贩鐢ㄤ笉鍚岀増鏈殑寮曠敤銆?

| 瀹為檯鑴氭湰绫诲瀷 | 鎵€閫?Prefab 鍐呯粍浠舵暟 | TypeTree 璇诲彇 |
|---|---:|---|
| BeyondBoneCloth | 11 | 鍏ㄩ儴瀹屾暣娑堣垂鍘熷缁勪欢瀛楄妭 |
| BeyondBoneCapsuleCollider | 25 | 鍏ㄩ儴瀹屾暣娑堣垂鍘熷缁勪欢瀛楄妭 |
| BeyondBonePlaneCollider | 1 | 瀹屾暣 |
| BeyondBoneSphereCollider | 1 | 瀹屾暣 |

杩?38 涓粍浠剁殑 317 涓潪绌?PPtr 鍏ㄩ儴瑙ｆ瀽鍒扮洰鏍囥€傝鏁板寘鍚剼鏈€佸綊灞炪€侀楠笺€佺鎾炰笌棰勬瀯寤哄紩鐢紝
**涓嶆槸 317 鏍圭墿鐞嗛楠?*銆傞棴鍖呮€诲叡璇诲埌 49 涓?MonoBehaviour銆?56 涓?Transform锛?
閫氳繃 `inSelectedPrefab` 鍖哄垎鐩爣涓庝緷璧栵紝涓嶈兘鎶婁緷璧栭噷鐨勫璞″叏绠楀埌瑙掕壊涓娿€?

宸叉槑纭繚鐣欎竴涓潪鐗╃悊瑙ｆ瀽澶辫触锛歚00046` 鐨?`AnimatorMono` 鍙秷璐?`336/448` 瀛楄妭锛?
鏈爣璁颁负鎴愬姛锛屾湭鐢熸垚璇ョ粍浠剁殑鏁版嵁 JSON锛涘師濮嬪瓧鑺傚拰 TypeTree 浠嶄繚鐣欍€?
鍏朵粬 48 涓粍浠惰В鐮侀暱搴﹀畬鏁淬€傞暱搴﹀惢鍚堟槸缁撴瀯璇诲彇妫€鏌ワ紝**涓嶇瓑浜庢墍鏈夊瓧娈佃涔夊凡鐭ワ紝涔熶笉绛変簬淇敼鍚庡彲鏃犳崯鍐欏洖**銆?

### 8.2 妯℃嫙缁勭殑鐪熷疄閰嶇疆

涓嬭〃缁勫悕鐪佺暐 `MBC_Typhoea_` 鍓嶇紑銆備竴涓?BeyondBoneCloth 鍙互鏈夊涓?rootBones锛?
11 涓粍浠朵笉鑳界О浣滃彧鏈?11 鏉″崟鏍归摼銆傛鏍锋湰 rootBones 鏁扮粍鍚堣 31 椤广€?
鈥滈€夋嫨鐐光€濇槸 `selectionData` 鏁扮粍闀垮害锛屼笉鐩存帴褰撲綔鍏变韩楠ㄦ灦楠ㄩ鎬绘暟銆?

| 缁勫悕 | 鏍瑰紩鐢?| 纰版挒浣撳紩鐢?| 閫夋嫨鐐?| gravity | damping.value | radius.value |
|---|---:|---:|---:|---:|---:|---:|
| Hair_Front_Bangs_Short | 5 | 1 | 15 | 5 | 0.05 | 0.006 |
| Acc_Back_Left_Bag | 1 | 2 | 4 | 10 | 0.05 | 0.038 |
| Cloth_Skirt | 7 | 5 | 37 | 5 | 0.05 | 0.065 |
| Cloth_Skirt_Bag | 2 | 3 | 6 | 5 | 0.05 | 0.031 |
| Cloth_Coat | 6 | 1 | 12 | 5 | 0.05 | 0.020 |
| Hair_Back_Ponytail_Knot | 2 | 2 | 24 | 0 | 0.20 | 0.020 |
| Hair_Back_Ponytail_Long | 2 | 8 | 38 | 0 | 0.30 | 0.125 |
| Acc_Back_Right_Lantern | 1 | 1 | 4 | 2 | 0.20 | 0.055 |
| Hair_Front_Side_Long | 2 | 10 | 10 | 6 | 0.10 | 0.020 |
| Cloth_Skirt_Rope | 2 | 2 | 6 | 8 | 0.01 | 0.045 |
| Tail | 1 | 0 | 8 | 0 | 0.08 | 0.020 |

杩欎簺鏄簭鍒楀寲鍘熷€硷紝涓嶆搮鑷崲鎴?RE 鍗曚綅銆俤amping/radius 杩樻湁鏇茬嚎缁撴瀯锛屼笉鑳戒粎淇濈暀琛ㄤ腑鐨?value銆?
鍏ㄩ儴 11 涓粍鐨?`clothType=1`銆乣sourceRenderers=[]`锛屼笖 `preBuildData.enabled=0`锛?
浣嗘枃浠跺唴浠嶆湁棰勬瀯寤烘暟鎹€?*涓嶈兘鍥犱负棰勬瀯寤哄潡瀛樺湪灏遍€夐鏋勫缓璺緞锛屾洿涓嶈兘鐢ㄧ鐢ㄧ殑鏃у潡瑕嗙洊褰撳墠閫夋嫨鏁版嵁**銆?
Prefab 鍘熷€间篃涓嶈兘璇佹槑鎵€鏈夊疄渚嬪垵濮嬪寲鍚庨兘涓嶅啀鏀硅繖浜涜缃€?

渚嬪闀垮彂缁勫綋鍓?selection 鏈?38 鐐癸紝鑰岀鐢ㄧ殑棰勬瀯寤哄睘鎬ф暟缁?count 涓?16锛涗袱鑰呬笉鑳戒簰鐩告浛浠ｃ€?
`gravityProperty=0` 涓嶄唬琛ㄩ噸鍔涗负闆讹細渚嬪 `00016` 鍚屾椂鏈?`gravityProperty=0` 鍜?
`serializeData.gravity=10`銆傝鍖哄垎鍔ㄧ敾灞炴€х粦瀹氬瓧娈典笌瀹為檯鐗╃悊閰嶇疆銆?

### 8.3 閫夋嫨灞炴€т笌纰版挒浣撲笉鑳介潬鍚嶇О鐚?

鏈満鏂规硶浣撹繘涓€姝ョ‘璁?`VertexAttribute` 鐨勫垽瀹氾細

- `IsFixed` `0x5A01E7C`锛氳鍙?Value 鐨?`0x01` 浣嶃€?
- `IsMove` `0x5A01F44`锛氭鏌?`0x02` 浣嶆槸鍚﹂潪闆躲€?
- `IsInvalid` `0x5A01EC4`锛氭鏌?`(Value & 0x03) == 0`銆?

瀹冧滑鏄爣蹇椾綅锛屼笉鏄彲闅忔剰閲嶆柊缂栧彿鐨勪笁涓灇涓撅紱鍏朵粬浣嶅繀椤讳繚鐣欍€?
闀垮彂缁勭殑瀹為檯閫夋嫨鏄?2 涓浐瀹氱偣銆?2 涓繍鍔ㄧ偣銆?4 涓棤鏁堢偣锛沬gnoreFromRootBones 涔熸湁 24 涓紩鐢ㄣ€?
鏁伴噺鐩哥瓑灏氫笉璇佹槑鐐瑰簭鍜岄楠奸『搴忕浉鍚岋紝涓嶈兘鐩存帴 zip 瀵瑰簲锛屾洿涓嶈兘鎶婃牴涓嬫墍鏈夊瓙楠ㄩ兘褰撴垚杩愬姩鐐广€?
`ClothSerializeData2` 鐨勮繍琛屾椂澹版槑閲屾湁 boneAttributeDict/vertexAttributeList锛?
浣嗘娆?TypeTree 瀹為檯鍙簭鍒楀寲 selectionData/preBuildData锛涗笉鑳界敤杩愯鏃跺瓧娈垫竻鍗曞啋鍏呮枃浠跺瓧娈点€?

纰版挒浣撳弬鏁伴€氳繃鍏冩暟鎹弬鏁板悕涓庣煭鏂规硶浣撶浉浜掓牳瀵癸細

- 鑳跺泭 `SetSize(startRadius, endRadius, length)` `0x59DC0E4`锛氫緷娆″啓鍏?size.x/y/z锛?
  骞舵寜涓ょ鍗婂緞鏄惁涓嶅悓璁剧疆 radiusSeparation銆?
- 鐞冧綋 `SetSize(radius)` `0x4A46EC0`锛氬啓 size.x锛宻ize.y/z 娓呴浂銆?
- 绗?20.30 鑺傚凡浠?`GetColliderType`銆乣GetSize` 鍜?`StartSimulationStepJob.Execute` 鐨勬湰鏈烘柟娉曚綋纭锛?
  `size.z` 鏄寘鍚袱绔崐鐞冪殑澶栭儴闀匡紝`alignedOnCenter` 鍐冲畾鏃嬭浆涓績锛宍reverseDirection` 缈昏浆绔偣杞达紱
  鏈妭鏃╂湡浠呭嚟鍙傛暟鍚嶄笉鑳界‘璁ょ鐐圭殑闄愬埗鐜板凡琛ラ綈銆?

鐪熷疄鍙嶄緥锛歚00002` 鐨?GameObject 鍚嶇О鍚?`Magica Capsule Collider (skirt_base_R_c_02_jnt)`锛?
鑴氭湰鍗存槸 **BeyondBonePlaneCollider**锛宑enter=(0,0,-0.05)銆乻ize=(0,0,0)銆?
涓嶈兘鎸夊悕瀛楀垱寤鸿兌鍥婏紝涔熶笉鑳芥妸闆?size 鐨勫钩闈㈠垽涓烘棤鏁堢鎾炰綋銆?

11 涓粍鍏辨湁 35 娆＄鎾炲紩鐢紝鎸囧悜 25 涓笉鍚岀鎾炵粍浠讹紱鍏朵腑涓€涓 4 涓粍鍏辩敤銆?
27 涓鎾炵粍浠朵腑杩樻湁 2 涓湭琚繖 11 涓粍寮曠敤锛屽簲淇濈暀骞舵爣璁板叧绯伙紝涓嶈嚜鍔ㄥ垹闄ゆ垨澶嶅埗缁欐瘡鏉￠摼銆?
`Hair_Front_Side_Long` 鏈?10 涓鎾炲紩鐢紝宸茬粡瀹為檯瓒呰繃鏃?`cloth.h` 鐨?8 涓笂闄愶紱
鏂板鍏ュ櫒/鍚庣蹇呴』鎸夎祫婧愮湡瀹炴暟缁勯暱搴﹀鐞嗭紝涓嶇户鎵挎鏃ч檺鍒躲€?

### 8.4 楠岃瘉涓庝笅涓€椤瑰疄鏂?

- 绂荤嚎宸ュ叿 net9.0 Release 缂栬瘧閫氳繃锛? warning / 0 error銆?
- 涓ゆ鐪熷疄璧勬簮鍙栬瘉鍧囦繚鐣欏師濮嬪瓧鑺傦紱鏈€鏂版姤鍛婄殑 49 涓師濮嬬粍浠?hash/闀垮害澶嶆牳閫氳繃銆?
- 38 涓師鐢熺墿鐞嗙粍浠跺叏閲忚В鐮佷笖闈炵┖寮曠敤宸茶В鏋愶紝11 缁?position/attribute 鏁伴噺涓€鑷淬€?
- 鍏冩暟鎹鏌ュ伐鍏锋柊澧炲弬鏁板悕/绫诲瀷绱㈠紩璇诲彇涓庤寖鍥淬€乼oken 妫€鏌ワ紝7 椤逛腑鎬ф祴璇曢€氳繃銆?

鎺ヤ笅鏉ヤ紭鍏堢‘璁?**selection 鐐瑰簭 鈫?瀹屾暣楠ㄩ璺緞** 鍜?**纰版挒浣撶鐐?缂╂斁**锛?
鐒跺悗鎶婃鍥剧撼鍏ユ寮忓叡浜?Physics/Skeleton 璧勬簮锛岃€岄潪璁╃敤鎴风紪杈戣繖浜涜瘖鏂?JSON銆?
Blender 鍙垱寤轰竴浠藉叡浜鎾炶緟鍔╀綋锛岀敱澶氫釜缁勫紩鐢紱鍘熷鏈煡瀛楁鍜屾洸绾夸繚鐣欍€?
杩愯绔粛閬靛畧绗?4鈥? 鑺傜殑鍘熺敓鏋勫缓銆佸畬鎴愩€佹敞閿€濂戠害锛屼笉鑷啓姹傝В鍣紝涔熶笉閫?Mesh 澶嶅埗妯℃嫙缁勩€?
鏈杩藉姞娌℃湁閮ㄧ讲 DLL銆佽皟鐢ㄦ父鎴忔瀯寤烘帴鍙ｆ垨淇敼鐢ㄦ埛鐨?Mod/Blender 宸ョ▼銆?

## 9. 2026-09-07 杩藉姞锛氬垵濮嬪寲銆佸畬鎴愬拰寤惰繜閲婃斁

璇佹嵁娌跨敤绗?2 鑺傚悓涓€ PE/metadata hash锛屾柊澧炲彧璇绘姤鍛婄洰褰曪細
`E:\EIEM_Workspace\physics-diagnostics\20260907-native-lifecycle`銆?
鍖呭惈 `entry-dispose.json`銆乣startup-callback-tail.json`銆乣runtime-build-completion.json`銆?
`async-finally.json`銆乣result-code.json`銆備互涓嬪湴鍧€浠呬緵澶嶆煡锛屼笉杩涘叆鐢熶骇鍋忕Щ琛ㄣ€?

### 9.1 涓嶈兘鍏?AddComponent锛屽啀鍋囧畾鏈夊厖瓒虫椂闂撮厤缃?

- `Awake` `0x45D82E0` 妫€鏌ュ叏灞€妯″紡锛涘€间负 1 鏃惰繘鍏ュ喎鍧?`0x5000C2E`锛?
  璋冪敤 get_Process銆丳rocess.Init 鍜岀鐞嗗櫒鎿嶄綔 `0x343A2E0`銆?
- `Start` `0x343A1F0` 鍦ㄥ悓涓€妯″紡涓?0 鏃舵墽琛屽垵濮嬪寲锛涢殢鍚庤繘鍏?`AutoBuild`銆?
- 妯″紡瀛楁鐨勯鍩熷悕绉?鏋氫妇鍚箟灏氭湭纭锛屼笉鎿呰嚜鍛藉悕鎴愨€滅紪杈?杩愯妯″紡鈥濄€?
  `0x343A2E0` 宸插湪绗?12 鑺傝拷鍔犵‘璁ゆ槸 `TeamManager.RemoveMonitoringProcess`锛屼笉鏄ā鎷熶换鍔″畬鎴愭帴鍙ｃ€?
- `get_Process` `0x32B6860` **涓嶇函鍙**锛氬彇 process 鍚庢妸褰撳墠缁勪欢鍐欏洖 process 寮曠敤銆?
  璋冩煡宸ュ叿璇诲彇鍏冩暟鎹В鏋愬埌鐨?`process` 瀛楁锛屼笉涓衡€滅湅鐪嬬姸鎬佲€濊皟鐢ㄨ繖涓?getter銆?

缁勪欢宸ュ巶蹇呴』鏄庣‘闃绘閰嶇疆瀹屾垚鍓嶈嚜鍔ㄥ垵濮嬪寲/鏋勫缓锛涗笉鑳藉彧闈?BuildAndRun 鍐呴儴鐨?DisableAutoBuild锛?
鍥犱负 Awake/Start 鍙兘宸茬粡鍙戠敓銆傚垱寤烘柟寮忋€佸仠鐢ㄦ椂搴忓皻鏈疄娴嬶紝涓嶈兘鎶娾€滄斁鍒?inactive 瀵硅薄涓娾€濆啓鎴愬凡楠岃瘉鎺ュ叆鏂规銆?

### 9.2 鏋勫缓閫氱煡鏈夊悓姝ヨ矾寰勶紝涔熸湁瀹屽叏涓嶉€氱煡鐨勫彇娑堣矾寰?

- BuildAndRun 鐨勫畬鎴愭竻鐞嗗潡 `0xBF20C0` 璇诲彇缁勪欢 OnBuildComplete锛?
  缁?`0x3596EC0` 璋冪敤甯︿竴涓?Boolean 杞借嵎鐨勫鎵樸€傚悓姝ュけ璐?棰勬瀯寤鸿矾寰勫彲鍦?BuildAndRun 杩斿洖鍓嶉€氱煡銆?
- StartRuntimeBuild 杩斿洖鎴愬姛浠呬唬琛ㄦ帴鍙楀惎鍔紱寮傛鐘舵€佹満杩涘叆鏋勫缓鐘舵€侊紝闅忓悗鎵ц娉ㄥ唽銆?
- 寮傛 finally `0xBF0D30..0xBF205C` 鍦?`0xBF1E10` 娓?isBuild锛屼箣鍚庢鏌?isDestory銆?
  鑻ユ湭閿€姣佷笖缁勪欢鏈夋晥锛屽垯鍦?`0xBF1F9C` 閫氱煡 Boolean 缁撴灉锛涘叾鎴愬姛鍒ゆ柇姣旇緝 ResultCode 鐨勬垚鍔熺姸鎬併€?
- **鑻ヨ姹傞攢姣侊紝鍒嗘敮鍦?`0xBF1FCB` 璋?DisposeInternal锛岀洿鎺ラ€€鍑猴紝涓嶉€氱煡 OnBuildComplete銆?*
  鍥犺€屽彇娑堝畬鎴愪笉鑳藉彧绛夊緟杩欎釜浜嬩欢锛屽惁鍒欏彲鑳芥案杩滀繚鐣欐棫楠ㄦ灦銆?

鍗?Boolean 璋冪敤绾﹀畾鏄湰鏈烘寚浠よ瘉鎹紝涓嶇瓑浜庡凡楠岃瘉瀛楁涓€瀹氭槸鏌愪釜鏍囧噯 `Action<bool>` 绫诲瀷銆?
杩愯鏃惰瘖鏂細杈撳嚭鐪熷疄瀛楁绫诲瀷锛涗笉鑳藉叏灞€ Hook 鍏变韩濮旀墭 Invoke锛堝彲鑳借鍏朵粬濮旀墭鍏辩敤锛夈€?
寮傛 continuation 鎵€鍦ㄧ嚎绋嬪皻鏈‘璁わ紝鍥炶皟閲屼笉鑳界洿鎺ユ搷浣?Unity 瀵硅薄銆?
姝ｅ紡閫傞厤鍣ㄩ渶瑕佸鐞嗗洖璋冩棭浜庤繑鍥炪€佸彇娑堟棤鍥炶皟浠ュ強杩囨湡 program 浠ｉ檯锛屼笉鑳芥寜鈥滄瘡娆℃濂戒竴娆″紓姝ュ洖璋冣€濊璁°€?

### 9.3 Dispose 杩斿洖涓嶆槸鍏佽閲婃斁鏂板楠ㄩ鐨勫嚟璇?

`Dispose` `0x59DFE8C` 鍦?lockObject 淇濇姢涓嬫爣璁伴攢姣併€佹竻鏈夋晥鐘舵€?缁撴灉锛屽彇娑?CTS锛?
闅忓悗杩涘叆 DisposeInternal銆傚悗鑰咃細

1. 宸插畬鎴愬唴閮ㄩ噴鏀惧垯閫€鍑恒€?
2. **浠嶅湪鏋勫缓鍒欐殏涓嶆竻鐞嗭紝鐩存帴杩斿洖**锛涘紓姝?finally 浼氬湪閫€鍑烘瀯寤虹姸鎬佸悗鍐嶆杩涘叆娓呯悊銆?
3. 闈炴瀯寤虹姸鎬佹墠鎵ц鍘熺敓娉ㄩ攢涓庤祫婧愭竻鐞嗐€?
4. `0x59DFC55` 鏍囪 isDestoryInternal 鍚庯紝閲婃斁閿侊紝浠嶈璋冪敤绠＄悊鍣?`0x343A2E0` 鍐嶈繑鍥炪€?

鍥犳锛岃鍒?isDestoryInternal=true 涔熶笉绛変簬璇?native 璋冪敤鏍堝凡缁忛€€鍑恒€?
涓嶈兘鎶婁笂杩板瓧娈垫垨 IsRunning=false 鐢ㄤ綔閲婃斁楠ㄦ灦鐨勫厖鍒嗘潯浠讹紱杩橀渶纭鍘熺敓 Task/Job 涓庣鐞嗗櫒瀹夊叏杈圭晫銆?
鍘熺敓 `ResultCode` 鏈?IsSuccess/IsProcess/IsCancel 绛夋柟娉曪紝姝ｅ紡瀹炵幇搴斾娇鐢ㄥ凡瑙ｆ瀽鐨勮涔夋帴鍙ｏ紝
鑰屼笉鏄妸姝ゆ鎴愬姛鏁板€?2銆佹爣蹇椾綅鎴栧瓧娈靛亸绉诲啓姝汇€?

## 10. DLL 渚ф寜闇€鍘熺敓鐗╃悊璇婃柇锛坴52 鍙婁互鍓嶇殑鍘嗗彶瀹炵幇锛?

鏂板 [eiem_native_physics_probe.h](../../src/eiem_native_physics_probe.h)锛岄€氳繃 Dump 椤电殑
**鍘熺敓鐗╃悊璇婃柇** 鎸夐挳鍙戦€佺嫭绔?WM_APP 璇锋眰锛屽湪鏃㈡湁 Unity 绾跨▼閫氶亾鎵ц銆?
杈撳嚭鍒伴厤缃殑 dump 鐩綍锛屾枃浠跺悕 `physics_runtime_<pid>_<tick>.json`锛屼娇鐢?CREATE_NEW 淇濈暀鍓嶆璇佹嵁銆?
杩欎笉鏄?Physics 璧勬簮鍖咃紝涓嶆敼鍙?INI銆佹簮閰嶇疆銆丮esh銆侀鏋舵垨鍘熺敓姹傝В鍣ㄣ€?

瀹為檯璁板綍锛?

- BeyondDynamicBone 绋嬪簭闆嗕腑鐨?10 涓浉鍏崇被鍨嬶細瀛楁鍚?绫诲瀷/flags銆佹柟娉曡繑鍥炵被鍨?鍙傛暟绫诲瀷/flags銆?
  鍖呭惈 OnBuildComplete 鐨勭湡瀹炲瓧娈电被鍨嬶紝涓嶆妸闈欐€佹帹鏂啋鍏呰繍琛屾椂缁撴灉銆?
- 宸插姞杞界殑 BeyondBoneCloth 缁勪欢锛堝寘鎷潪婵€娲诲璞?缂撳瓨璧勪骇锛夛細瀹炰緥 ID銆佹樉绀哄眰绾с€佹椿鍔ㄧ姸鎬併€?
  鐙珛 process銆乼eamId锛屼互鍙?isBuild/isDestory/isDestoryInternal/IsValid/IsRunning銆?
- 闈炴縺娲讳笉鑷姩鍒や綔 PFB锛屾樉绀哄眰绾т粎鐢ㄤ簬鏌ユ壘锛?*涓嶆槸绋冲畾璧勬簮韬唤**銆?
  姣忎釜瀹為檯缁勪欢鍒嗗埆璁板綍锛屼笉浠庨€変腑 Mesh 鎺ㄦ柇鈥滀竴 Mesh 涓€濂楃墿鐞嗏€濄€?

绾︽潫锛?

- 鍙湪鐐瑰嚮鏃舵灇涓撅紝涓嶉€愬抚鎵弿锛屼笉澧炲姞鐗╃悊 Hook锛屼笉璋冪敤 get_Process銆丅uildAndRun銆丏ispose 鎴栦换浣曞弬鏁?setter銆?
- 瀛楁鎸夊疄闄呭悕绉般€佺簿纭被鍨嬪拰瀹炰緥/static 灞炴€у尮閰嶏紱鏂规硶鎸夎繑鍥炲€笺€佸弬鏁般€佸疄渚?static 鍖归厤銆?
  缂哄け/姝т箟涓嶆寜鍚屽弬鏁版暟鐩寽閲嶈浇锛屼笉鍥為€€纭紪鐮佸亸绉伙紝涓嶆妸寮傚父/缂哄け瑙ｈ鎴?false銆?
- 浣跨敤 GC 寮哄紩鐢ㄤ繚鎸佹湰娆¤鍙栫殑鏁扮粍銆佺粍浠跺拰鎵樼 process 鍙揪锛岃鍙栫粨鏉熷嵆閲婃斁锛?
  **寮哄紩鐢ㄤ笉淇濊瘉 Unity native 瀵硅薄瀛樻椿**锛岀粍浠跺彟鍋氬師鐢熸湁鏁堟€ф鏌ャ€?
- 鐘舵€佹槸鍚勫瓧娈电嫭绔嬮噰鏍凤紝鏄庣‘鏍囪 `observed-not-atomic`锛涙棦涓嶆槸鏁寸粍鍘熷瓙蹇収锛屼篃涓嶆槸閲婃斁鏍呮爮銆?
  璇婃柇涓嶆壙鎷呭疄渚嬬姸鎬佹満/閲婃斁鏉冮檺鍒ゆ柇锛屼笉鑳界敤涓€娆?dump 璇佹槑鎵€鏈夋瀯寤?娉ㄩ攢浜嬩欢宸茬粡鍙戠敓銆?
- 鐢ㄦ墭绠?Array.GetValue 鍜屽疄闄呮暟缁勯暱搴︼紝涓嶇户鎵挎棫瑁欐憜浠ｇ爜鐨?8 涓鎾?缁勪欢闄愬埗銆?

楠岃瘉锛歚build.bat` 鍏ㄩ噺鏋勫缓閫氳繃锛汳SVC 涓嬪疄闄呯紪璇?probe 澶存枃浠剁殑 7 椤逛腑鎬у涓绘祴璇曢€氳繃锛?
瑕嗙洊澶氬疄渚嬨€佺己 API銆佸瓧娈?杩斿洖绫诲瀷涓嶇銆佺┖ process銆佹灇涓惧け璐ュ拰闈?Unity 绾跨▼锛涘悓鏃舵鏌ユ棤淇敼璋冪敤銆?
GC 寮曠敤閲婃斁鍜岀被鍨嬪悕鍐呭瓨閲婃斁銆備笌 Unity lifetime銆丼keleton銆丼kin銆丮od controls 涓€璧锋墽琛屽叡 16 椤归€氳繃锛屾棤璺宠繃銆?
杩欓獙璇佷唬鐮佸绾︼紝涓嶇瓑鍚屼簬娓告垙涓師鐢熺墿鐞嗘敞鍐屾垚鍔熴€?
鏈鏈€缁堟湰鍦?`bin/eiem.dll` SHA256锛?
`DDD495ABB2084CF3537DE175D61A953BFEA33A91FDA3C2876FB5A16DA6442468`銆?
**Render.physics 鍘熺敓瑁呴厤灏氭湭瀹炵幇**锛屾柊澧為摼銆佸叡浜鎾炵紪杈戙€丄nimator 鏂拌妭鐐瑰啓鍥炲拰瀹夊叏娉ㄩ攢浠嶆槸涓嬩竴姝ャ€?
鏈疆娌℃湁閮ㄧ讲 DLL锛屼篃娌℃湁淇敼娓告垙 Mod/Blender 宸ョ▼銆?

## 11. 2026-09-07 杩藉姞锛氬弬鏁板壇鏈€侀€氱煡鍜屽叡浜墍鏈夋潈

鑼冨洿鏇存锛氳嚜鍒剁墿鐞嗛摼闇€瑕佸垱寤洪澶栭鏋惰妭鐐癸紱闅愯棌鎴栧垹闄?Mesh 涓嶅簲椹卞姩杩欎簺楠ㄩ鐨勫鍒犮€?
褰撳墠鍏堝疄鐜板凡鏈夋ā鎷熺粍鐨勫弬鏁版洿鏂板熀纭€锛屽悓鏃剁户缁皢鏂板楠ㄩ銆佸師鐢熸瀯寤?娉ㄥ唽銆佸姩鐢荤紦鍐插啓鍥炵撼鍏ュ繀闇€鑼冨洿銆?
浠ヤ笅涓嶆槸鏂板鐗╃悊閾惧凡鍙敤鐨勭粨璁猴紝涔熶笉鏄鐜版湁娓告垙鐑噸杞介棶棰樼殑閮ㄧ讲淇銆?

### 11.1 鏈満鍘熺敓鍙傛暟鏇存柊涓庡鍒?

娌跨敤绗?2 鑺傚悓涓€娓告垙鏂囦欢锛屾姤鍛婁粛浣嶄簬绗?9 鑺傜洰褰曘€傛柊澧炰繚鐣欙細
`serialize-hot-update.json`銆乣serialize-import-body.json`銆乣collider-hot-update.json`銆?
鍦板潃鍙敤浜庢湰鏈洪潤鎬佸鏍革紝涓嶅啓鍏ヨ繍琛屾椂鍒嗘敮銆?

- `BeyondBoneCloth.SetParameterChange`锛坄0x59DE748`锛夎繘鍏?`ClothProcess.DataUpdate`锛坄0x59DED38`锛夈€?
  鍚庤€呬細瀵?serializeData 璋?`DataValidate`锛坄0x59E2084`锛夛紝鍐嶈繘鍏ョ鐞嗗櫒鐨勬洿鏂扮櫥璁拌矾寰勩€?
  鎵€浠ラ€氱煡涓嶅彧鏄€滄妸涓€涓竷灏旀爣蹇楃疆鐪熲€濓紱涓嶈兘璁╁瀹炰緥鍏变韩鐨勬簮閰嶇疆琚繖鏉¤矾寰勮繛甯︿慨鏀广€?
- `set_SerializeData`锛坄0x5697544`锛夋槸甯?GC 鍐欏睆闅滅殑寮曠敤璧嬪€硷紝涓嶄細鏇挎垜浠鍒舵垨闅旂閰嶇疆銆?
- `ClothSerializeData.Import(ClothSerializeData, Boolean)`锛坄0x59E2CC0`锛夌‘瀹炴鏌?deepCopy锛?
  娣卞鍒跺垎鏀垱寤哄垪琛ㄥ強澶氫釜宓屽鍙傛暟瀵硅薄锛涘洜姝ら€傞厤鍣ㄤ娇鐢ㄥ師鐢熸瀯閫犲嚱鏁板拰 `Import(source, true)`锛?
  涓嶇敤缁撴瀯浣?memcpy 鎴?MemberwiseClone 鍐掑厖娣卞鍒躲€?
- **灏氭湭閫愬瓧娈佃瘉鏄?Import 淇濈暀璇ユ父鎴忔墍鏈夋墿灞曞弬鏁板拰寮曠敤銆?* 鍘熺敓瀛樺湪 deepCopy 鍒嗘敮锛屼笉绛変簬浠绘剰瀛楁鍧囧凡鏃犳崯楠屾敹銆?
  鍚敤鐪熷疄瑕嗗啓鍓嶏紝杩橀』姣旇緝瀹為檯缁勪欢鐨勬簮/鍓湰鍥撅紝灏ゅ叾鏄洸绾裤€佹牴楠ㄥ垪琛ㄥ拰娓告垙鑷畾涔夊瓧娈点€?

涓婃父 API 涔熻姹?SerializeData 鍙樺寲鍚庨€氱煡锛屼絾鍙兘浣滀负鍙傜収锛涘疄闄呯鍚嶅強琛屼负浠ユ湰鏈鸿瘉鎹负鍑嗐€?
[MagicaCloth2 缁勪欢 API](https://magicasoft.jp/en/mc2_api_magicacloth/)銆?

### 11.2 纰版挒鏇存柊鐩存帴鍐欏叡浜暟缁勶紝涓嶈兘闅忔剰璋冪敤

`ColliderComponent.UpdateParameters`锛坄0x59E36C0`锛夊湪 DataValidate 鍚庨亶鍘嗗叾 teamIdSet锛?
閫愮粍璋冪敤 `ColliderManager.UpdateParameters`锛坄0x5A5D2F0`锛夈€?
鍚庤€呮寜妯℃嫙缁勭殑 collider chunk 鍜屽眬閮ㄥ簭鍙凤紝**鐩存帴鍐?flag銆乧enter銆乻ize 鏁扮粍**锛屼笉鏄彧鎺掗槦涓€涓姹傘€?
涓€涓鎾炰綋琚缁勫紩鐢ㄦ椂锛屽簲閫氳繃缁勪欢鐨勫師鐢熸洿鏂版搷浣滆鐩栧叾娉ㄥ唽缁勶紝涓嶈兘澶嶅埗鍑烘瘡缁勪竴浠界殑鍋囧叡浜粍浠躲€?

杩欎篃鎰忓懗鐫€鈥滃凡缁忓湪 Unity 涓荤嚎绋嬧€濇湰韬笉璇佹槑涓庡師鐢熸ā鎷?Job 鍚屾瀹屾垚銆?
鍙傛暟涓庣鎾炴洿鏂颁笉鑳藉叡鐢ㄤ竴涓寽娴嬬殑浠绘剰 WndProc 鍐欏叆鏃舵満锛涙湰娆￠€傞厤鍣ㄤ笉璋冪敤纰版挒 setter 鎴栨洿鏂板嚱鏁般€?
鍘熺敓 SetSize 鐨勫叿浣撳垎閲忚瘉鎹粛瑙佺 8.3 鑺傦紱娌℃湁淇敼鏃㈡湁瑁欐憜 UI/浠ｇ爜銆?

### 11.3 宸插啓鍏ョ殑浠ｇ爜鍙婅竟鐣?

[鍏冩暟鎹?API](../../src/eiem_native_physics_api.h) 缁熶竴璇婃柇涓庨€傞厤鍣ㄧ殑绮剧‘鏌ユ壘锛?
绋嬪簭闆嗐€佺被銆佸瓧娈电被鍨嬶紝浠ュ強鏂规硶杩斿洖绫诲瀷銆佸弬鏁扮被鍨嬨€侀潤鎬佹€ч兘蹇呴』鐩哥銆?
鍙屽弬鏁?Import 涓庡崟鍙傛暟閲嶈浇鍒嗗紑鍖归厤锛涗笉鎸夊弬鏁版暟閲忔垨鍥哄畾鍦板潃鍙栫涓€涓€欓€夈€?

[鍙傛暟閫傞厤鍣╙(../../src/eiem_native_physics_parameters.h) 鐩墠鍙帴鍙?5 涓凡鍛藉悕鐨勬爣閲忓瓧娈碉細
`gravity`銆乣stablizationTimeAfterReset`锛堟湁闄愰潪璐熷€硷級锛?
`gravityFalloff`銆乣blendWeight`銆乣animationPoseRatio`锛堟湁闄愮殑 0锝? 鍊硷級銆?
杩欐槸鏈鍙楁敮鎸佺紪杈戣寖鍥达紝涓嶆槸瀹屾暣 Physics schema锛涙嫆缁濆叾浠栧瓧娈点€侀噸澶嶅瓧娈点€佺┖缂栬緫鍜屾棤鏁堟暟鍊硷紝涓嶆埅鏂簮鍙傛暟銆?

鎿嶄綔鍒嗕负鍑嗗銆佸簲鐢ㄣ€佹仮澶嶏細

1. 鍑嗗闃舵寮烘寔鏈夊師閰嶇疆鍜岀粍浠讹紝寤虹珛鑷繁鐨勯厤缃壇鏈紱鍙慨鏀瑰壇鏈苟鍥炶锛屽皻涓嶆敼缁勪欢缁戝畾銆?
2. 搴旂敤鏃剁‘璁ょ粍浠堕潪鏋勫缓/閿€姣佺姸鎬併€佸師鐢熸湁鏁堛€佸綋鍓嶇粦瀹氫粛鍙楁湰娆℃墍鏈夋潈绾︽潫锛岀劧鍚庤祴鍊煎苟閫氱煡銆?
   杩欎簺鐘舵€佸彧鏄弬鏁拌皟鐢ㄧ殑鍑嗗叆妫€鏌ワ紝**涓嶆槸 Task/Job 閲婃斁鏍呮爮**銆?
3. setter 鍙兘鏀逛簡寮曠敤鍚庢墠鎶ラ敊锛屽洜姝ゅ湪璋冪敤鍓嶈褰曟仮澶嶈矗浠汇€傚け璐ヤ笉鑳戒涪澶卞師閰嶇疆銆?
4. 鎭㈠鏃跺啓鍥炰繚鐣欑殑鍘熼厤缃苟閲嶆柊閫氱煡銆傛仮澶嶅け璐ヤ繚鐣欒处鏈紱纭缁勪欢鍘熺敓姝讳骸鍚庡彲浠ュ彧娓呰处鏈紝涓嶈皟鐢?setter銆?
   杩欐潯瑙勫垯鍙€傜敤浜庡凡鏈夌粍浠剁殑鍙傛暟寮曠敤锛屼笉鑳芥帹骞挎垚鈥滅粍浠舵浜″氨鍙洿鎺ュ垹鏂板鐗╃悊楠ㄩ鈥濄€?

`EiemPhysicsParameters` 璐︽湰鎸夊疄闄呯粍浠剁鐞嗚褰曪紝褰掓ā鍨?鐗╃悊瀛愮郴缁燂紝涓嶈兘姣忎釜 Renderer 鍚勫缓涓€浠斤細

- 鍚屼竴璧勬簮閲嶅寮曠敤鍚屼竴缁勪欢鍏辩敤涓€浠芥簮鍩虹嚎锛屽凡搴旂敤鐘舵€佸洖璇绘垚鍔熸椂涓嶉噸澶嶈祴鍊?閫氱煡銆?
- 涓や釜瀹炰緥鍗充娇鍏变韩鍚屼竴浠芥簮閰嶇疆锛屼篃鍒嗗埆鎸佹湁 Mod 鍓湰锛涙仮澶嶄竴涓笉褰卞搷鍙︿竴涓€?
- 涓嶅悓璧勬簮浜夌敤鍚屼竴缁勪欢鏄庣‘鎷掔粷锛涗笉鎸?Render 閬嶅巻椤哄簭浜掔浉瑕嗙洊銆?
- 鍊煎彉鍖栬姹傚厛鎭㈠鏃ц褰曪紝鍐嶅噯澶囨柊璁板綍銆傛仮澶嶅け璐ユ湡闂寸姝㈠啀娆℃崟鑾烽厤缃綔鏂版簮銆?
- 鎵归噺鎭㈠鍙竻闄ゆ垚鍔熼」锛屽け璐ラ」淇濇寔寮曠敤涓庡師鍥狅紝涓嶅洜涓嬩竴娆?F10 涓㈣处鏈€?
- 涓嶅湪鏋愭瀯鍑芥暟涓皟鐢?Unity锛涙墍灞炲瓙绯荤粺蹇呴』娲诲埌鏄惧紡鎭㈠瀹屾垚锛屽啀閿€姣佽处鏈垨鍙戝竷鏂颁唬闄呫€?

**灏氭湭鎺ュ叆 INI銆丷ender 鎵ц鍣ㄦ垨 F10**锛氬綋鍓嶅寘鍚ご鏂囦欢鍙繚璇?DLL 缂栬瘧锛屼絾娌℃湁鍒涘缓鐢熶骇璐︽湰鎴栬皟鐢ㄥ弬鏁板啓鍏ャ€?
鎴嚦鏈妭璁板綍鏃讹紝灏氭湭瀹炵幇 Physics 鏂囦欢璇诲彇銆佹柊缁勪欢宸ュ巶銆佸師鐢熸柊閾炬敞鍐屻€佺墿鐞嗘寔鏈?Skeleton 鐨勬帴绾垮強瀹夊叏娉ㄩ攢銆?
鏂囦欢璇诲彇鍚庣画瑙佺 13 鑺傚強绗?15 鑺傦紱鍘熺敓缁勪欢宸ュ巶鍜屽畨鍏ㄦ敞閿€鎺ョ嚎浠嶆湭瀹屾垚銆?
涓嶅緱灏嗘湰娆￠€傞厤鍣ㄥ綋浣滀竴涓彲鐩存帴閫氳繃 `physics=...` 浣跨敤鐨勫姛鑳斤紝涔熶笉闇€瑕佺敤鎴风幇鍦ㄤ慨鏀?INI 鎴栭噸鍚父鎴忔祴璇曞畠銆?

### 11.4 楠岃瘉璁板綍鍜屼笅涓€鎺ュ叆鏉′欢

瀹屾暣 `build.bat` 鏋勫缓鎴愬姛銆傛湰娆℃湰鍦?`bin/eiem.dll` SHA256锛?
`F1882AC90EE627157B5AE21D4CC611FFBF67B32AE6C75BB86B0C532A86C76F9F`銆?

鍦?MSVC 鐜鎵ц锛?

```text
python -m unittest test_native_physics_parameters test_native_physics_probe test_unity_lifetime test_skeleton_runtime test_skin_runtime test_mod_controls -v
```

32 椤归€氳繃锛屾棤璺宠繃锛涘叾涓弬鏁拌处鏈?17 椤广€佸彧璇?probe 7 椤广€?
鍙傛暟娴嬭瘯瀹為檯缂栬瘧鐢熶骇澶存枃浠讹紝浣跨敤涓€?IL2CPP 瀹夸富妯℃嫙 API锛涜鐩栧悓缁勪欢澶嶇敤銆佷袱涓疄渚嬪叡浜簮銆?
浜夌敤銆侀噸鏂板噯澶囥€佺嚎绋嬨€乻etter 鏀圭粦瀹氬悗鎶涢敊銆侀€氱煡澶辫触銆侀儴鍒嗘仮澶嶃€佸師鐢熸浜″拰寮曠敤閲婃斁銆?
杩欐槸閫傞厤鍣ㄧ殑绂荤嚎濂戠害楠岃瘉锛屼笉鏄父鎴忔眰瑙ｃ€佹繁澶嶅埗瀹屾暣鎬ф垨鐢婚潰鏁堟灉楠屾敹銆?

涓嬩竴鎺ュ叆蹇呴』鍏堣ˉ榻愶細鐪熷疄婧?鍓湰鍥炬瘮杈冿紱鍙傛暟涓庣鎾炴洿鏂扮殑鍘熺敓瀹夊叏鏃舵満锛?
鏂扮粍浠堕厤缃墠鐨勮嚜鍔ㄦ瀯寤烘姂鍒讹紱鏂板鑺傜偣鐨勯€夋嫨搴?Animator 鍐欏洖锛涙瀯寤哄彇娑堝強妯℃嫙浠诲姟閫€鍑哄悗鐨勯鏋堕噴鏀俱€?
鍐嶆妸鐗╃悊瀹炰緥鎵€鏈夋潈鎺ュ埌鍏变韩楠ㄦ灦涓?Reconcile/F10锛屾渶鍚庡仛姝ｅ紡璧勬簮涓?Blender 鍙鍖栧線杩斻€?
鏈娌℃湁閮ㄧ讲 DLL锛屾病鏈夌紪杈戞父鎴?Mod/Blender 宸ョ▼锛屼篃娌℃湁鍔犲叆鏂版眰瑙ｅ櫒鎴栦换鎰忓瓧娈靛啓鍏ユ帴鍙ｃ€?

## 12. 2026-09-07 杩藉姞锛氶閾捐緭鍏ラ『搴忎笌 Animator 鍐欏洖鏄犲皠

### 12.1 璇佹嵁鍙婇€傜敤杈圭晫

浠嶄娇鐢ㄧ 2 鑺傚悓涓€ GameAssembly/metadata銆傜 9 鑺傜洰褰曟柊澧炰繚鐣欙細
`render-setup-fields.json`銆乣process-binding-fields.json`銆乣team-binding-fields.json`銆?
`vertex-attribute-fields.json`銆乣bone-input-mapping.json`銆乣animator-binding-map.json`銆?

闈欐€?metadata 宸ュ叿鐜板湪杈撳嚭瀛楁鍚嶇О銆乼ype index 鍜?token锛屽苟妫€鏌ュ瓧娈佃寖鍥村強 token锛?
**type index 涓嶆槸瀛楁鍋忕Щ锛屼篃涓嶆槸宸茶В鏋愬嚭鐨勭被鍨嬪悕绉?*銆傝繍琛屾椂绮剧‘绫诲瀷鏉ヨ嚜 DLL 鍏冩暟鎹?API锛?
涓嶆妸闈欐€佽〃搴忓彿鍐欒繘鐢熶骇鍐呭瓨璁块棶銆?

鏈妭鍦板潃浠呬緵鏈満鍙嶆眹缂栧鏍革紝涓嶅姞鍏?Hook 鍋忕Щ琛ㄣ€傚凡鏈夊疄鏃剁被鍨?dump 涓殑
`UnityEngine.AnimationTransformRWBufferHandle` 鐢ㄤ簬鏍稿瀛楁鍚箟锛涘叾 boxed 瀛楁鍋忕Щ涓嶈兘鐩存帴褰撲綔鏈绠辩粨鏋勫竷灞€銆?

### 12.2 鍘熺敓 BoneCloth 濡備綍浠?Transform 鐢熸垚閫夋嫨鐐?

`ClothProcess.CreateBoneRenderSetupData`锛坄0x3439960`锛夊湪 `0x3439A1C` 璋冪敤涓冨弬鏁?
`RenderSetupData` 鏋勯€犲嚱鏁帮紙`0x38D4A50`锛夛紝杈撳叆鍖呭惈缁勪欢 Transform銆乺ootTransforms銆?
ignoreFromRootBones銆乧ollisionBones銆乧onnectionMode銆傛敞鎰忓叆鍙?unwind 鑼冨洿鍙鐩栫煭鍓嶆锛?
鏈鍙︿繚鐣欏悗缁寚浠ょ獥鍙ｏ紱涓嶆妸鐭?unwind 鑼冨洿褰撲綔瀹屾暣鍑芥暟銆?

鍦ㄦ娆℃鏌ョ殑楠ㄩ摼鏋勯€犺矾寰勫唴锛?

1. 浠庢牴鍒楄〃閬嶅巻 Transform 瀛愬眰绾э紝缁忓幓閲嶅拰蹇界暐鍒ゆ柇锛屽缓绔?`transformList`銆?
   涓嶆槸浠?Mesh 鐨勯《鐐圭粍鎴栧師灞€閮?bones 鏁扮粍澶嶅埗绱㈠紩銆?
2. 璁板綍姝ゆ椂鍒楄〃闀垮害涓?`skinBoneCount`锛屽苟灏嗗悓涓€浣嶇疆璁颁綔 `renderTransformIndex`銆?
3. **闅忓悗杩樹細鎶婄粍浠?render Transform 浣滀负绌洪棿閿氱偣鍔犲叆鍒楄〃**锛屽啀璇?Transform 淇℃伅銆?
   鎵€浠ヨ繖閲岀殑 transformCount 涓嶅簲鐩存帴瑙ｉ噴涓衡€滃叏閮ㄧ墿鐞嗗彲鍔ㄩ楠兼暟鈥濄€傚瓧娈?skinBoneCount 鐨勫悕瀛椾篃涓嶈兘鎷挎潵鍐掑厖 Mesh palette 闀垮害銆?
4. `ReadTransformInformation` 寤虹珛瀵瑰簲鐨?Transform ID銆佺埗 ID 绛夋暟鎹€?

`GenerateBoneClothSelection`锛坄0x59DFF84`锛夋寜 `skinBoneCount` 鍒涘缓閫夋嫨鏁版嵁锛?
鎶婄偣浣嶇疆杞崲鍒扮粍浠剁┖闂达紝骞跺～榛樿杩愬姩灞炴€э紱闅忓悗瀵瑰簭鍒楀寲 rootBones 璋?
`Object.GetInstanceID 鈫?RenderSetupData.GetTransformIndexFromId`锛屽湪瀹為檯鏌ユ壘浣嶇疆鍐欏浐瀹氬睘鎬с€?
`VertexAttribute` 鐨勯潤鎬佸瓧娈靛垎鍒湁 `Invalid / Fixed / Move / DisableCollision`锛涗笉鍦ㄦ彃浠堕噷纭紪鐮佸叾瀛楄妭鍊笺€?

`GetTransformIndexFromId`锛坄0x5A51EF0`锛夎鍙?`transformIdList` 鍚庤皟鐢?IndexOf銆?
`GetParentTransformIndex`锛坄0x346FF20`锛夊悓鏍烽€氳繃鐖?ID 鏌ヨ〃锛屽苟鍙帓闄ょ粍浠堕敋鐐广€?
鍥犳鎸夆€滈楠肩煭鍚嶇浉鍚屸€濇垨鈥滅储寮曟伆濂戒粠 0 寮€濮嬧€濇帹鏂槧灏勯兘涓嶆垚绔嬨€?

**灏氫笉鑳藉皢浠ヤ笂榛樿鐢熸垚瑙勫垯鐩存帴濂楀埌鎵€鏈夊凡缂栬緫 selectionData銆?* 婧愰€夋嫨鏁版嵁鍙兘缁忚繃杞崲銆佷唬鐞嗙畝鍖栨垨閲嶆槧灏勶紱
鏈鏈瘉鏄庝换鎰忕绾?selection 鏁扮粍閮藉彲涓庤嚜琛岄亶鍘嗛鏋剁殑椤哄簭鐩存帴 zip銆?
Blender 姝ｅ紡瀵煎叆浠嶉渶璁板綍宸茶瘉瀹炵殑鐐光€擳ransform 瀵瑰簲锛屼笉鑳界寽瀹屽氨鎶婃湭鐭ョ偣褰掑埌鏍归銆?

### 12.3 鍘熺敓鍐欏洖瀛樺湪绗簩娆＄储寮曢噸鏄犲皠

`TeamManager.UpdateTeamAnimatorData`锛坄0x5A2283C`锛変笉鏄皢姣忎釜缁勭殑楠ㄥ簭鐩存帴浜ょ粰 Animator锛?

- 姹囨€诲悓 Animator 涓嬬殑鐩稿叧 Transform/妯℃嫙妲斤紝澶勭悊閲嶅寮曠敤銆?
- 鍘熺粦瀹氶渶瑕佹洿鏂版椂璧?DisableClothBindings锛岄殢鍚庡皢姹囨€荤殑 Transform 鏁扮粍浼犵粰
  `Animator.CreateClothBindings`锛堣皟鐢ㄧ偣 `0x5A231C9`锛夈€傝繑鍥炲彞鏌勬寜 Animator 瀹炰緥 ID 淇濆瓨锛屽啀 EnableClothBindings銆?
- 鍙ユ焺鍖呭惈 `count`銆乣invalidCount`銆乣validTransformIndexsPtr`銆乣invalidTransformIndexsPtr` 鍙婂绉嶈鍐欑紦鍐叉寚閽堛€?
- `0x5A23293` 璇诲彇鏈夋晥 Transform 绱㈠紩鍒楄〃锛岀敤瀹冩壘鍒拌緭鍏?Transform 瀵瑰簲鐨勬ā鎷熸Ы锛?
  `0x5A23347` 灏嗚繖浜涙ā鎷熸Ы鏄犲皠鍒版湰杞?Animator 鍐欏洖搴忓彿銆?

闇€瑕佷弗鏍煎尯鍒嗭細

| 绱㈠紩/韬唤 | 琛ㄨ揪浠€涔?| 涓嶈兘褰撲綔浠€涔?|
|---|---|---|
| 楠ㄩ摼 setupIndex | 鏈楠ㄩ摼鏋勫缓鍒楄〃鐨勪綅缃?| Mesh 鏉冮噸閲岀殑灞€閮ㄩ绱㈠紩 |
| Transform instance ID | 褰撳墠鍘熺敓瀵硅薄韬唤锛岀敤鏉ユ煡琛?| 璺ㄩ噸鍚ǔ瀹氳祫婧愯韩浠?|
| 妯℃嫙 Transform 鍏ㄥ眬妲?| 鍘熺敓绠＄悊鍣ㄥ叡浜暟缁勪腑鐨勪綅缃?| 褰撳墠缁勭殑灞€閮ㄧ偣搴?|
| Animator 鍐欏洖妲?| 寮曟搸鎺ョ撼銆侀噸鎺掑悗鐨勭紦鍐插簭鍙?| 鍘熻緭鍏?Transform 鏁扮粍搴忓彿 |
| Mesh palette 绱㈠紩 | 椤剁偣鏉冮噸寮曠敤璇?Mesh 鐨勫眬閮?bones/bind poses | 鍏ㄨ鑹查鏋跺簭鍙?|

鍥犳鏂板鐗╃悊缁勫簲璧版父鎴忓師鐢熺粍娉ㄥ唽锛岃 TeamManager 鏇存柊鍏变韩 Animator 缁戝畾锛?
鑰屼笉鏄瘡涓?Mod/Renderer 鑷繁 CreateClothBindings銆佽鐩栧悓涓€涓?Animator 鐨勬暣濂楀啓鍥炵姸鎬併€?
杩欎笉鏄姹傛瘡涓鑹茬被鍨嬪姞涓€濂?Hook锛岃€屾槸鍚屼竴鍘熺敓娉ㄥ唽閾捐矾鍐呴儴蹇呴』閬靛畧鐨勬槧灏勫叧绯汇€?

**寮曟搸鏄惁鎺ョ撼鏂板姞銆佸師 Avatar 涓病鏈夌殑 Transform 浠嶅緟纭銆?*
GameAssembly 鐨?CreateClothBindings/Injected 鍖呰鍙В鏋?icall 骞惰浆鍙戯紝涓嶈兘鎹鏂█浠绘剰鑺傜偣閮戒細鎴愬姛銆?
宸插畾浣?UnityPlayer 涓搴?icall 鍚嶇О锛屼絾灏氭湭纭鏈€缁堝疄鐜板拰绛涢€夋潯浠讹紱鏈尮閰嶅埌闈欐€?xref 涓嶄唬琛ㄦ帴鍙ｄ笉瀛樺湪銆?
鏈妫€鏌ョ殑 UnityPlayer SHA256 涓?
`BEE7BE52370ADDDD67BA61E4937CA51B7F272656841D187E95E505496DA798D1`銆?
鏈夋晥/鏃犳晥璁℃暟蹇呴』鎴愪负鍚庣画娉ㄥ唽楠屾敹璇佹嵁锛屼笉鑳戒互鎸囬拡闈炵┖鏇夸唬銆?

鍙︼細metadata token/RVA 宸茬‘璁ょ 9 鑺傜殑 `0x343A2E0` 涓?`TeamManager.RemoveMonitoringProcess`銆?
瀹冪Щ闄ょ洃鎺х櫥璁帮紝涓嶇瓑浜庢ā鎷?Job 鎴栧姩鐢诲啓鍥炰换鍔″凡缁忛€€鍑猴紝绗?9 鑺傜殑瀹夊叏閲婃斁闄愬埗涓嶅彉銆?

### 12.4 鏈疆宸插疄鐜扮殑 DLL 璇婃柇

[鍘熺敓鐗╃悊璇婃柇](../../src/eiem_native_physics_probe.h) 鍦ㄥ師鎸夐挳涓婂鍔狅細

- 鍘熺敓绫诲瀷濂戠害鐢?10 涓鑷?14 涓紝骞跺彟澶栬緭鍑?Animator 鍜?AnimationTransformRWBufferHandle 鐨勫瓧娈?鏂规硶濂戠害銆?
- 姣忎釜 process 鐨?`interlockingAnimatorId`銆?
- 闈炴瀯寤?闈為攢姣佺姸鎬佷笅锛岃鍙?boneClothSetupData 鐨勬墭绠?Transform銆両D銆佺埗 ID 鍜屾牴 ID 鍒楄〃銆?
- 杈撳嚭 setupIndex銆佸疄闄?Transform instance ID銆佽褰?ID 鏄惁鐩稿悓銆佸師鐢熸煡鎵捐繑鍥炵储寮曘€佺粍浠堕敋鐐圭储寮曞拰 skinBoneCount銆?
- 涓嶅悓瀹炰緥鍗充究鏄剧ず灞傜骇鐩稿悓锛屼篃涓嶅悎骞躲€傛樉绀哄眰绾у彲鑳芥埅鏂紝浠呬緵闃呰锛屼笉浣滆妭鐐硅韩浠姐€?
- 鏁扮粍闀垮害涓嶇涓嶅～鍏呫€佷笉 zip锛涙煡鎵剧己澶辫緭鍑?null锛岀湡瀹炶繑鍥?-1 鍘熸牱淇濈暀锛屼笉鍏滃簳涓?0銆?
- 缁撴潫鏃堕噸鏂版鏌ュ垪琛ㄥ紩鐢?闀垮害銆乻etup 寮曠敤鍙婃瀯寤?閿€姣佺姸鎬併€?
  `sameBindingsAndLengthsAtEnd=true` **涔熶笉鏄師瀛愬揩鐓?*锛屽洜涓哄垪琛ㄥ唴瀹瑰彲鍦ㄧ浉鍚岄暱搴︿笅鍙樺寲銆?

璇婃柇鍙鎵樼鍒楄〃鍙婂凡纭鐨勬煡鎵惧嚱鏁帮紝涓嶈皟鐢ㄦ瀯寤恒€佹敞鍐屻€佸弬鏁板啓鍏ユ垨缁戝畾鏇存柊锛?
涓嶄細瑙ｅ紩鐢?NativeArray/Job/RW buffer 鐨勮８鎸囬拡锛屼篃涓嶆妸涓€娆?dump 褰撲綔鍏佽閲婃斁鏂板楠ㄩ鐨勪緷鎹€?
鎵樼寮曠敤鍦ㄨ鍙栨湡闂村己鎸佹湁锛孶nity native 鏈夋晥鎬у崟鐙鏌ワ紱缁撴灉涓嶈兘璇佹槑涓嬩竴鏃跺埢瀵硅薄浠嶇劧瀛樻椿銆?

### 12.5 楠岃瘉涓庡綋鍓嶅畬鎴愮▼搴?

瀹屾暣 `build.bat` 鏋勫缓閫氳繃锛涙湰娆?`bin/eiem.dll` SHA256锛?
`79B61A5956E7A92EAF7B2218C1DA8703DDA0010F81E996DEED029A54503C2A28`銆?

MSVC 涓嬫墽琛屼互涓嬫祴璇曪紝**44 椤归€氳繃锛屾棤璺宠繃**锛?

```text
python -m unittest test_native_physics_bone_probe test_native_physics_probe test_native_physics_parameters test_unity_lifetime test_skeleton_runtime test_skin_runtime test_mod_controls -v
```

鍏朵腑鏂伴鏄犲皠璇婃柇 12 椤癸紝瀹為檯缂栬瘧鐢熶骇澶存枃浠讹紝瑕嗙洊缁勪欢閿氱偣銆佸瀹炰緥閲嶅悕銆佺被鍨嬩笉绗︺€?
闀垮害涓嶇銆両D 涓嶇銆佺己鏌ユ壘銆佹浜¤妭鐐广€佹瀯寤?閿€姣佹湡闂存嫆璇汇€佸垪琛?瀵硅薄琚崲銆佽皟鐢ㄥ紓甯稿強绾跨▼杈圭晫銆?
绫诲瀷鍚嶅垎閰嶄笌 GC 鍙ユ焺鍧囨鏌ラ噴鏀撅紝鏃犲啓鍏ヨ皟鐢ㄣ€傞杞祴璇曞涓绘妸宓屽叆鎴愬憳鍦板潃鍜屾墍灞炲璞″湴鍧€娣峰悓锛?
瀵艰嚧 setup 绫诲瀷妫€鏌ユ嫆缁濓紱淇瀹夸富鐨勭嫭绔嬪璞″缓妯″悗閫氳繃锛屾湭鏀惧鐢熶骇绫诲瀷妫€鏌ユ潵杩庡悎娴嬭瘯銆?
闈欐€?metadata 瑙ｆ瀽鍙﹀ **8 椤归€氳繃**锛屽寘鎷柊澧炲瓧娈佃寖鍥村強 token 鏍￠獙銆?

鏈疆瀹屾垚鐨勬槸楠ㄩ摼/鍔ㄧ敾鏄犲皠鍙栬瘉銆佹寜闇€璇婃柇鍜岀绾垮洖褰掞紝**涓嶆槸鏂板鐗╃悊閾惧凡鎺ラ€?*銆?
灏氭湭鎺ュ叆 Render.physics銆佹湭閮ㄧ讲 DLL銆佹湭淇敼娓告垙 Mod 鎴?Blender 宸ョ▼銆?
涓嬩竴姝ラ泦涓‘璁ゅ師鐢熸柊澧炶妭鐐规帴绾炽€佹瀯寤?妯℃嫙/鍔ㄧ敾缁戝畾閫€鍑虹殑瀹夊叏杈圭晫锛屽啀鎺ュ叡浜?Skeleton 鎸佹湁鏉冨強娉ㄥ唽/娉ㄩ攢鎵ц鍣紱
淇濇寔鈥滆嚜鍒剁墿鐞嗛摼瑕佹柊澧為楠笺€侀殣钘?Mesh 涓嶅垹闄ら楠笺€佺敱娓告垙鍘熺敓绯荤粺瑙ｇ畻鈥濈殑绾﹀畾銆?

## 13. 2026-09-07锛歅hysics 浣滆€呰祫婧愮嫭绔嬩氦浠?

Blender 0.11 宸插疄鐜版柊澧炵墿鐞嗙粍銆佸叡浜悆/绛夊崐寰勮兌鍥娿€佷簲涓爣閲忓弬鏁板強鐙珛 `.physics`/`.skeleton`
璇诲啓锛岃鎯呰 [Physics 浣滆€呰祫婧?v1](../physics-authoring-v1.md)銆傛柊澧?Python 缂栬В鐮併€丆++ reader/validator
鍙婄湡瀹?Blender 缂栬緫/淇濆瓨/寰€杩旀祴璇曪紝杩炲悓鏃㈡湁 Blender 鍥炲綊鍏?14 椤归€氳繃锛屾棤璺宠繃銆?

杩欐槸浣滆€呯涓庤祫婧愭牸寮忕殑浜や粯锛屼笉鍖呭惈 `Render.physics` 鎺ョ嚎锛屼篃涓嶆柊澧炲師鐢熸瀯寤?鍐欏叆璋冪敤銆?
鏂囦欢涓?`purpose=authoring`锛涜兌鍥?span 涓轰綔鑰呭畾涔夌殑鐞冨績闂磋窛锛岃妭鐐硅鑹蹭负浣滆€呮灇涓撅紝
涓嶈兘鐩存帴褰撲綔鍘熺敓 SetSize 闀垮害銆乂ertexAttribute 浣嶅€兼垨 Animator 绱㈠紩銆?
婧愮墿鐞嗘洸绾裤€佹湭鐭ュ瓧娈靛強棰勬瀯寤烘暟鎹皻鏈浆鎹紝涓嶈兘绉颁綔鍘熺敓鐗╃悊瀹屾暣瀵煎叆瀵煎嚭銆?
绗?9銆?1銆?2 鑺備腑鐨勬柊澧炶妭鐐规帴绾冲拰 Task/Job 瀹夊叏杈圭晫浠嶆湭瀹屾垚娓告垙楠岃瘉銆?

鏈疆鍙慨鏀逛粨搴撴枃浠跺苟鐢熸垚鏈湴寮€鍙戝寘锛涙病鏈夐儴缃叉父鎴?DLL銆佷慨鏀圭敤鎴?Mod銆佸紑鍙戠洰褰曟彃浠舵垨宸叉湁 `.blend`銆?

寮€鍙戝寘锛歚bin/EIEM_Blender-0.11.0-physics-authoring.zip`锛?5,019 瀛楄妭锛孲HA256锛?
`30b2ccd354e582a91137b2d9089babd65ba9385939dc504463fe5e00d943c29e`銆?
宸蹭粠 ZIP 瑙ｅ帇鑷充复鏃剁洰褰曪紝楠岃瘉鍏ㄩ儴鍒嗗彂婧愮爜涓庝粨搴撲竴鑷达紝骞堕€氳繃鍖呮敞鍐?涓夋閲嶈浇鍙婁綔鑰呭線杩旀祴璇曘€?

## 14. 2026-09-07锛氬洖鍒?DLL 楠岃瘉锛屽鍔犳寜闇€璋冪敤璺熻釜

鎸夌敤鎴疯姹傦紝鍏堣ˉ榻愬苟璇佹槑 DLL 鏈夋晥锛屽啀琛?Blender銆傜 13 鑺傜殑浣滆€呰祫婧愭祴璇曚笉鏋勬垚鍘熺敓杩愯鏃惰瘉鎹紝
鏈疆鏈户缁慨鏀?Blender銆傛柊澧為摼鐨?Animator 鎺ョ撼銆佺綉鏍奸┍鍔ㄥ拰 Task/Job 瀹夊叏娉ㄩ攢渚濈劧娌℃湁瀹炴満缁撹銆?

### 14.1 鏈疆瀹炵幇鍙婇檺鍒?

[璋冪敤璺熻釜](../../src/eiem_native_physics_trace.h) 鍦?Dump 椤垫彁渚涒€滃紑濮嬪師鐢熺墿鐞嗚窡韪€濆拰鈥滃仠姝㈠苟瀵煎嚭璺熻釜鈥濄€?
浠呮墜鍔ㄥ紑濮嬫椂瀹夎 Hook锛屾寜杩愯鏃?assembly/type銆佸畬鏁村弬鏁般€佽繑鍥炵被鍨嬪拰瀹炰緥鏂规硶灞炴€цВ鏋愪笅鍒楀叆鍙ｏ細

- `BeyondBoneCloth.BuildAndRun`
- `ClothProcess.StartRuntimeBuild / Init / Dispose / DisposeInternal`
- `TeamManager.RemoveMonitoringProcess`
- `ClothManager.CompleteMasterJob`

涓嶄娇鐢ㄩ潤鎬?RVA锛涘厓鏁版嵁缂哄け銆佺鍚嶄笉绗︺€佸叆鍙ｇ┖鎸囬拡鎴栦笌鍏朵粬鍙灇涓炬柟娉曞叡鐢ㄥ湴鍧€鏃舵嫆缁濆紑濮嬨€?
鍚屼竴鍦板潃鍙兘鏉ヨ嚜璺ㄧ▼搴忛泦浠ｇ爜鎶樺彔锛屽洜姝ゆ鏌ユ墍鏈夊凡鍔犺浇绋嬪簭闆嗭紝鑰岄潪鍙鏌ュ緟璺熻釜鍒楄〃銆?
Hook 閮ㄥ垎瀹夎澶辫触鏃朵笉鍚姩璁板綍锛屼繚鐣欏凡鍒涘缓鐨勮鍔ㄨ浆鍙戝叆鍙ｄ緵閲嶈瘯锛涘仠姝㈣褰曚笉鍗歌浇 trampoline銆?
杩欓伩鍏嶅湪鍙兘浠嶆湁璋冪敤缁忚繃鏃跺洖鏀惰浆鍙戜唬鐮侊紝浣嗕笉鎰忓懗鐫€鎻掍欢鏀寔杩愯涓嵏杞?DLL銆?

鍖呰鍣ㄥ師鏍疯浆鍙戝弬鏁般€佽繑鍥炲€煎拰寮傚父锛屽彧璁板綍鏍囬噺锛屼笉瑙ｅ紩鐢ㄥ璞°€佷笉璋冪敤 Unity API銆佷笉鍒嗛厤鍫嗗唴瀛樸€佷笉鍐欐枃浠躲€?
[璁板綍鍣╙(../../src/eiem_native_physics_events.h) 浣跨敤鏈夌晫缂撳啿淇濆瓨杩涘叆銆佹甯歌繑鍥炴垨寮傚父閫€鍑恒€佽皟鐢ㄩ厤瀵圭紪鍙枫€?
绾跨▼ ID銆乼ick 鍜屽璞″湴鍧€銆傚湴鍧€鍙澶嶇敤锛屼笉鏄ǔ瀹氬疄渚嬭韩浠斤紱`BuildAndRun=true` 浠嶅彧琛ㄧず鎺ュ彈寮€濮嬫瀯寤恒€?

鍋滄鍚庣户缁褰曞凡杩涘叆璋冪敤鐨勮繑鍥烇紝涓嶇瓑寰呭師鐢熷伐浣滃畬鎴愩€傚皻鏈夎皟鐢ㄦ湭杩斿洖鎴栨柊浜嬩欢鏈垚鍔熷鍑烘椂锛?
鎷掔粷鐢ㄦ柊涓€杞褰曡鐩栨棫璇佹嵁銆傜紦鍐叉弧鍚庝涪寮冩柊浜嬩欢骞舵槑纭疮璁?`dropped`锛屼笉浼鎴愬畬鏁磋褰曘€?
瀵煎嚭鍦ㄦ棦鏈夎瘖鏂?JSON 涓鍔?`lifecycle`锛涘彧鏈夊啓鍏ャ€乫lush銆乧lose 鍏ㄩ儴鎴愬姛鎵嶇‘璁ゅ凡瀵煎嚭銆?
濡傛灉鍋滄鏃?`inFlightCalls>0`锛屼箣鍚庡啀娆′娇鐢ㄨ瘖鏂?瀵煎嚭鎸夐挳淇濆瓨杩熷埌鐨勮繑鍥炪€?

**`inFlightCalls=0` 鍙唬琛ㄦ湰杞瀵熷埌鐨勫嚱鏁拌皟鐢ㄥ凡杩斿洖锛屼笉浠ｈ〃鍏跺惎鍔ㄧ殑 Task銆丣ob 鎴栧姩鐢诲啓鍥炲凡閫€鍑恒€?*
杩欎簺鍑芥暟鐨勮皟鐢ㄦ搴忎篃涓嶈兘璇佹槑鏂板鑺傜偣琚?Animator 鎺ョ撼锛屼粛闇€鏋勫缓鍜屽啓鍥炵殑鐙珛瀹炴満璇佹嵁銆?

闆嗘垚妫€鏌ュ彂鐜版棦鏈夌墿鐞嗚瘖鏂?`WM_APP+0x316` 涓?Mod reconcile 鍐茬獊锛屾柊璺熻釜鍏ュ彛鍘熷厛閫夋嫨鐨?`0x317`
涔熶笌 Mod key 鍐茬獊锛涚墿鐞嗚瘖鏂拰鍚仠鍛戒护宸茬粺涓€绉昏嚦 `0x320..0x322`锛屽姞鍏ョ紪璇戞湡鍙婂洖褰掓鏌ャ€?
鍚﹀垯娓告垙绐楀彛浼氬厛鍖归厤璇婃柇鍒嗘敮锛岃鍚炴甯哥殑 Mod 鏇存柊/杈撳叆娑堟伅銆?

### 14.2 宸叉墽琛岄獙璇佷笌涓嬩竴姝?

瀹屾暣 `build.bat` 鏋勫缓閫氳繃锛屾瀯寤烘爣璇?`resource-runtime-v49-physics-lifecycle-trace`銆?
`bin/eiem.dll`锛?,900,864 瀛楄妭锛孲HA256锛?
`A5A2757540A0EAC4846AD0D950DDF312D78EED7CF8D1BD56F056994B820E968F`銆?
棣栨鍛戒护閲嶅鍒濆鍖?MSVC 鐜瀵艰嚧鑴氭湰瑙ｆ瀽澶辫触锛岀洿鎺ヨ繍琛屾瀯寤鸿剼鏈悗閫氳繃锛涙湭淇敼绯荤粺鐜鎴栨瀯寤鸿剼鏈€?

MSVC 瀹夸富娴嬭瘯鍏?**56 椤归€氳繃锛屾棤璺宠繃**锛氬師鏈夌墿鐞?楠ㄦ灦/钂欑毊/Mod 娴嬭瘯 44 椤癸紝
鏂板璺熻釜娴嬭瘯 11 椤瑰強鐑敭宸ヤ綔绾跨▼鍥炲綊 1 椤广€傝窡韪祴璇曠紪璇戠敓浜уご鏂囦欢锛?
瑕嗙洊鍙傛暟鍜岃繑鍥炲€艰浆鍙戙€佸紓甯镐紶鎾€佸祵濂楀強澶氱嚎绋嬮厤瀵广€佸仠姝㈠悗鐨勮繜鍒拌繑鍥炪€佺紦鍐叉孩鍑恒€佸鍑虹‘璁ゃ€?
绛惧悕鍙婂叡浜湴鍧€鎷掔粷銆丠ook 閮ㄥ垎澶辫触鍚庨噸璇曘€佺洰鏍囧彉鏇村拰绐楀彛娑堟伅缂栧彿鍐茬獊銆?
娴嬭瘯涓殑 MinHook 瀹夎鐢卞涓绘ā鎷燂紝涓嶄唬琛ㄧ湡瀹炴父鎴忎唬鐮佸凡琚垚鍔?Hook銆?

鏈疆浠呯敓鎴愭湰鍦?DLL锛屾湭閮ㄧ讲鍒版父鎴忕洰褰曪紱妫€鏌ユ椂鏈彂鐜拌繍琛屼腑鐨?Endfield锛屽洜姝ゆ病鏈夋父鎴忔棩蹇楁垨鐢婚潰楠屾敹銆?
涓嬩竴姝ュ厛閲囬泦鍘熺敓缁勪欢涓€娆℃瀯寤?閿€姣佺殑瀹為檯璋冪敤璁板綍锛屽苟鏍稿疄瀹夊叏杈圭晫锛屽啀鍦?DLL 鍐呭疄鐜颁笉渚濊禆 Blender 鐨勬渶灏忔柊澧為摼瀹為獙锛?
蹇呴』鍚屾椂璁板綍鏂板 Transform 韬唤銆丄nimator 鏈夋晥/鏃犳晥缁戝畾銆侀楠奸殢妯℃嫙鍙樺寲鍙婄綉鏍艰〃鐜帮紝
骞惰鐩栨瀯寤轰腑鍙栨秷銆丗10 鍜屽疄渚嬮攢姣併€備笂杩拌瘉鎹綈鍏ㄥ悗鎵嶆帴瀹屾暣 `Render.physics` 涓庝綔鑰呯娴佺▼銆?

## 15. 2026-09-07 鏂囨。鏍稿鏃剁殑宸ヤ綔鍖虹姸鎬?

鏈妭淇濈暀娓呯悊鏂囨。鏃剁殑蹇収锛涢殢鍚庤繘琛岀殑婧愮爜鏁寸悊銆佹祴璇曞拰鏋勫缓瑙佺 16 鑺傘€?

- `eiem_mod_document.h`銆乣eiem_physics_asset.h` 鏂板浜?Physics 澹版槑銆佷笉鍙彉鏂囦欢蹇収銆丼keleton 寮曠敤鏍￠獙鑽夌銆?
  鍚姩鏍囪瘑宸叉敼涓?v50锛屼絾杩欎簺鏂版敼鍔ㄥ皻鏈紪璇戞垨娴嬭瘯锛屼笉鑳界О涓哄凡浜や粯 v50銆?
- `eiem_mods.h` 鏄庣‘鎷掔粷鍙戝竷鍚潪绌?Physics 鍔ㄤ綔鐨?Mod锛涙病鏈夊師鐢熺粍浠跺伐鍘傘€佹敞鍐屽拰瀹夊叏娉ㄩ攢鐨勭敓浜ф帴绾裤€?
- 姝ゅ墠澶嶉獙鐨?61 椤规槸涓婅堪鏀瑰姩涔嬪墠鐨勫涓?鏍煎紡娴嬭瘯锛涙病鏈夋柊澧為摼鐨勫疄鏈洪獙鏀惰瘉鎹紝涓嶈兘璇佹槑 v50 鑽夌姝ｇ‘銆?
- 鏇炬柊澧炵殑鐙珛璺嚎鍥惧惈鏈夋湭缁忚瘉鏄庣殑缁勫悎閫€鍑哄垽鎹紝鐜板凡鍒犻櫎锛屼笉鍐嶄綔涓哄疄鏂戒緷鎹€?
  DisposeInternal / CompleteMasterJob 杩斿洖涓?inFlightCalls=0銆乨ropped=0 鐨勭粍鍚堜粛涓嶈兘璇佹槑
  鏋勫缓浠诲姟銆佹ā鎷?Job 鍜屽姩鐢诲啓鍥炲叏閮ㄥ仠姝紱绗?9銆?4 鑺傜殑璇佹嵁杈圭晫缁х画鏈夋晥銆?
- 鏂囨。鏁寸悊涓嶄慨鏀圭敓浜ц皟鐢紝涔熸湭鏋勫缓鎴栭儴缃叉柊鐨?DLL锛涘姛鑳芥帴鍏ユ殏鏃舵殏鍋滐紝寰呮枃妗ｆ牳瀵圭粨鏉熷悗鍐嶇户缁€?

## 16. 2026-09-07锛歷50 璧勬簮鑽夌鐨勭嫭绔嬪鏌ヤ笌鏈湴楠岃瘉

鏈疆缁х画涓嶄緷璧栧師鐢熼€€鍑虹粨璁虹殑宸ヤ綔銆?*object lifetime / teardown 灏氭湭楠岃瘉**锛?
鏅€氬嚱鏁版垨 hook 杩斿洖銆乼race 璁℃暟褰掗浂鍙婂涓绘祴璇曟垚鍔燂紝閮戒笉鏋勬垚鍘熺敓 Tasks銆丣obs 鎴?Animator 鍐欏洖瀹屾垚鐨勮瘉鏄庛€?

### 16.1 婧愮爜涓庢祴璇曡寖鍥?

- 澶嶆牳 `eiem_physics_asset.h` 鐨勬枃浠跺揩鐓с€佽矾寰勮В鏋愩€丼keleton 瑙ｇ爜鍙婇楠煎紩鐢ㄦ牎楠岋紝
  浠ュ強 `eiem_mod_document.h` / `eiem_mods.h` 鐨勯厤缃噯澶囧拰鍙戝竷鍒嗘敮銆?
- 灏嗘祦瑙ｆ瀽涓庢枃浠惰В鏋愮殑 Mod 鍚堝苟缁熶竴鍒?`EiemAppendModDocument`锛屽叡鐢ㄧ姸鎬佺储寮曡皟鏁撮€昏緫銆?
- 鏂板 [Physics 璧勬簮娴嬭瘯](../../tests/test_physics_resources.py)锛岀紪璇戝苟鎵ц瀹為檯 C++ 璇诲彇鍣ㄥ拰 Mod 鍔犺浇鍣ㄣ€?
  鏂囦欢鍏ㄩ儴鍦ㄦ祴璇曚复鏃剁洰褰曠敓鎴愶紝涓嶅垱寤?Unity 瀵硅薄锛屼篃涓嶈繍琛屽師鐢熺墿鐞嗐€?
- 12 椤规柊澧炴祴璇曡鐩栵細Python 浣滆€呮枃浠跺埌 C++ 鐨勮鍙栥€丼keleton v1/v2銆佷竷浣嶉暱搴﹀瓧绗︿覆銆乁TF-8 璺緞銆?
  涓嶅彲鍙樺揩鐓с€佺己澶遍楠笺€佹崯鍧?瓒呴檺璧勬簮銆佽矾寰勭┛瓒婁笌鐩綍 junction銆佽祫婧愬埆鍚嶃€佹湭婵€娲诲垎鏀€?
  Skeleton 涓嶄竴鑷淬€佽法 Mod 鍚屽悕璧勬簮闅旂锛屼互鍙婂畬鏁村姞杞?閲嶅鍔犺浇銆?
- 瀹炴祴鍔犺浇鍣ㄦ嫆缁濆惈闈炵┖ Physics 鍔ㄤ綔鐨勬暣浠?Mod锛屽寘鍚湭婵€娲绘垨闅忓悗娓呯┖鐨勫姩浣滐紱鍏惰鍒欍€佸彉閲忋€?
  蹇嵎閿拰 UI 鍧囦笉鍙戝竷銆傛櫘閫?Mod 涓庝粎澹版槑璧勬簮銆佹病鏈夐潪绌?Physics 鍔ㄤ綔鐨?Mod 浠嶅彲鍙戝竷銆?

### 16.2 楠岃瘉璁板綍

鍦?MSVC x64 寮€鍙戠幆澧冧腑锛屾簮鐮佸悎骞舵暣鐞嗗悗鎵ц涓嬪垪涓ょ粍妫€鏌ワ紝鍒嗗埆 29 椤逛笌 35 椤癸紝鍏ㄩ€氳繃銆佹棤璺宠繃锛?

```text
cd tests
python -m unittest test_physics_resources test_mod_program test_mod_controls test_hotkey_worker test_persistent_state -v
python -m unittest test_model_reload_lifecycle test_mesh_resource_cache test_material_resource_cache test_lua_ui test_resource_pipeline_contracts -v
```

鍚堣 64 椤瑰寘鍚涓诲彲鎵ц娴嬭瘯鍜岄潤鎬佸绾︽鏌ワ紝涓嶆槸鏂板鐗╃悊閾剧殑娓告垙楠屾敹銆?
姝ゅ墠鍏堣繍琛岀殑 43 椤圭浉鍏冲熀绾挎鏌ヤ篃閫氳繃锛屼絾涓庝笂杩版鏌ユ湁閲嶅锛屼笉鐩稿姞璁＄畻瑕嗙洊鏁伴噺銆?

`build.bat` 瀹屾暣鏈湴鏋勫缓鎴愬姛锛屽寘鍚?EIEM 鍙婁袱涓唬鐞?DLL銆備骇鐗╀娇鐢ㄥ綋鍓嶆暣涓伐浣滃尯婧愮爜锛?
涓嶈兘灏嗗叾涓凡鏈夌殑鍏朵粬淇敼褰掍负鏈疆瀹炵幇锛屼篃涓嶈〃绀鸿繖浜涘姛鑳藉叏閮ㄥ畬鎴愬疄鏈洪獙鏀躲€?
`bin/eiem.dll`锛?,950,528 瀛楄妭锛汼HA256锛?
`4cefb9e6cf1e547b4c89c4218c9ca327d0e70d112ab39297e030516994cd8671`銆?
鏈湴鏋勫缓鏃ュ織锛歚bin/diagnostics/v50-resource-contract/build.log`銆傛湰杞病鏈夐儴缃叉垨娓告垙鐩綍鍐欏叆銆?

### 16.3 浠嶇己灏戠殑璇佹嵁涓庡彲缁х画鐨勫伐浣?

鏂板閾剧殑 Animator 鎺ョ撼銆佹ā鎷熷埌缃戞牸鐨勯┍鍔ㄣ€佹瀯寤哄彇娑堝拰娉ㄩ攢鍚庣殑鍏ㄩ儴鍘熺敓寮曠敤閫€鍑轰粛鏈疄鏈洪獙璇併€?
鍏佽閲婃斁鏂板 Skeleton 鑺傜偣锛岄渶瑕佹槑纭鐩栨瀯寤?Task銆佹ā鎷?Job銆丄nimator/璺ㄥ抚鍐欏洖鍙婂叾浠栨秷璐硅€呯殑
completion fence 鎴栧搴旂殑鐪熷疄杩愯鏃惰瘉鎹紱褰撳墠璺熻釜涓嶆彁渚涜鍑瘉銆?

渚濊禆璇ュ亣璁剧殑鑷姩楠ㄦ灦閲婃斁鍜屽畬鏁村師鐢熻閰?娉ㄩ攢鎺ョ嚎锛屽繀椤诲厛琛ラ綈鍏跺叿浣撳墠缃潯浠躲€?
涓嶄緷璧栬鍋囪鐨勮祫婧愭牸寮忋€丅lender 缂栬緫銆佽瘖鏂€佹簮鐮佸疄鐜般€佺紪璇戙€侀潤鎬佸垎鏋愬拰娴嬭瘯鍙互缁х画銆?
杩欐鏋勫缓涓庢祴璇曟帹杩涗簡璧勬簮灞傞獙璇侊紝娌℃湁鍏抽棴 Physics 鎵ц鎷掔粷鍒嗘敮锛屼篃娌℃湁琛ュ嚭鍘熺敓瀹屾垚鏍呮爮銆?

## 17. 2026-09-07锛氬鏌ョ籂姝ｄ笌 v51 璇婃柇鏋勫缓

### 17.1 褰撳墠鐘舵€佸強姝ゅ墠璇垽

姝ゅ墠瀵硅瘽澹扮О鈥淒LL 杩愯鍩虹宸茬粡灏辩华锛屽彧闇€杩炴帴 Render.physics鈥濓紝骞舵嵁姝ゅ缓璁厛鍋?Blender锛?
杩欎竴鍒ゆ柇涓嶅彈浠撳簱璇佹嵁鏀寔锛岀幇鏄庣‘鎾ゅ洖銆倂50 鍙獙璇佷簡璧勬簮璇诲彇涓庝緷璧栨鏌ワ紱
鍘熺敓缁勪欢鍒涘缓銆丄nimator 鎺ュ叆銆佹墽琛屻€佸彇娑堛€佸疄渚嬫墍鏈夋潈鍙婂畨鍏ㄦ敞閿€灏氭湭鎺ラ€氭垨楠岃瘉銆?
`eiem_mods.h` 瀵归潪绌?Physics 鍔ㄤ綔鐨勬暣浠?Mod 鎷掔粷浠嶇劧瀛樺湪銆?
杩欎笉鏄彧鍓╀笅涓€澶勫瓧娈佃祴鍊兼垨鎺ュ彛杞彂鐨勭姸鎬併€?

鎸夌敤鎴锋渶鏂板喅瀹氾紝鍚庣画浠?DLL 鍘熺敓鏈哄埗鍜屾渶灏忛摼瀹炶瘉涓轰紭鍏堛€傛鍓嶈幏鎺堟潈鍐欏叆鐨?Blender v2 宸ヤ綔淇濈暀涓虹绾夸綔鑰呭疄鐜帮細

- 鏂板婧愬浘銆佸畬鏁村瓧娈?鏇茬嚎/棰勬瀯寤哄瓧鑺備繚鐣欍€佸叡浜鎾炲紩鐢ㄣ€佸彲閫夋簮瀵煎叆銆佺嫭绔嬩綔鑰呭鍑哄拰 C++ v2 鏍戣鍙栥€?
  瀹屾暣鑼冨洿鍜岄檺鍒惰[婧愭暟鎹綔鑰?v2](../physics-authoring-v2.md)銆傜绾挎牱鏈線杩斾笉绛変簬鍘熺敓閰嶇疆瀹炰緥鍖栨垚鍔熴€?
- 鍒犻櫎鏈粡楠岃瘉鐨勮兌鍥婄鐐?闀垮害/鏂瑰悜鎺ㄥ锛涘師鐢熺鎾炰綋鏆傚彧鏄剧ず婧愪腑蹇冩爣璁帮紝鍙傛暟浠嶅彲淇濈暀/缂栬緫銆?
- 鎾ゅ洖缁勫悎 Mesh+Physics Mod 瀵煎嚭 UI 鍜屽姩浣滅敓鎴愩€傞潪绌虹粍鍚堝弬鏁板湪鍐欏叆鍓嶆姤閿欙紱鐙珛 `.physics` 瀵煎嚭鍜?Mesh-only 淇濈暀銆?
  鍥炲綊娴嬭瘯纭鎷掔粷鏃朵笉瑕嗙洊鍘?`mod.ini`銆傛湭鍒犻櫎鐢熶骇鍔犺浇鍣ㄧ殑鎷掔粷鍒嗘敮銆?
- 鏃╂湡璇曚綔鐢熸垚鐨?`bin/diagnostics/blender-physics-v2/mesh-physics` 鏄棫娴嬭瘯杈撳嚭锛?
  涓嶄綔涓哄綋鍓嶅彲鐢?Mod 鎴栦氦浠樻牱渚嬶紱璇ユ祴璇曠洰褰曞鍔犺鏄庝繚鐣欏巻鍙茶瘉鎹€?

### 17.2 DLL 鏂板鐨勬寜闇€瑙傛祴

[濂戠害璇婃柇](../../src/eiem_native_physics_contract_probe.h) 鎺ュ叆鏃㈡湁鈥滃師鐢熺墿鐞嗚瘖鏂€濇寜閽細

1. `engineEntryPoints`锛氭鏌?Animator 鍏冩暟鎹‘鏈夊搴?InternalCall 鍚庯紝閫氳繃杩愯鏃?
   `il2cpp_resolve_icall` 瑙ｆ瀽 Create/CreateByName/Enable/Disable/DestroyClothBindings 鐨勫疄闄呭湴鍧€锛?
   璁板綍鎵€灞炴ā鍧椼€丷VA銆佹槸鍚︿綅浜庡彲鎵ц椤点€?*涓嶈皟鐢ㄨ繖浜涘湴鍧€锛屼笉鍒涘缓缁戝畾锛屼笉淇濆瓨涓虹敓浜у垎娲捐〃**銆?
   鍏冩暟鎹己澶?姝т箟銆佽В鏋?API 缂哄け銆佺┖杩斿洖鍜屼笉鍙墽琛屽湴鍧€鍧囨槑纭姤鍛婏紱涓嶉€€鍥炲浐瀹?RVA銆?
2. `buildResult`锛氱簿纭尮閰?`ClothProcess.get_Result 鈫?BeyondDynamicBone.ResultCode`锛?
   浠庤繑鍥炵殑鍊肩被鍨嬪壇鏈鍙?IsSuccess/IsProcess/IsCancel/IsError/IsWarning銆?
   鍓湰浣跨敤 pinned GC handle锛涘湪瑙ｇ鏁版嵁涓婅皟鐢ㄧ簿纭尮閰嶇殑 Boolean 璋撹瘝锛屼笉鍐?process 瀛楁銆?
   缂哄け銆佸紓甯告垨绫诲瀷閿欒杈撳嚭 unavailable/null锛屼笉浼涓?false 鎴栨垚鍔熴€?

杩欎簺瑙傛祴鐢ㄦ潵鍖哄垎鈥滃彂璧锋瀯寤衡€濆拰鈥滅粨鏋滅姸鎬佲€濓紝骞朵负闈欐€佸垎鏋愭彁渚涚湡瀹炲紩鎿庡湴鍧€銆?
缁撴灉鍓湰銆丅uildAndRun 杩斿洖鍊笺€佸嚱鏁拌窡韪繑鍥炲拰璁℃暟褰掗浂鍧?*涓嶆槸 Task/Job/Animator 瀹屾垚鏍呮爮**銆?
鐜版湁缁勪欢鐨?ResultCode 涔熶笉鑳借瘉鏄庢柊澧為楠艰繘鍏?Animator 鎴栭┍鍔ㄧ綉鏍笺€?
鏈疆鏈姞鍏ヨ嚜鍔ㄧ粍浠跺伐鍘傘€侀噸缁?Animator銆佺鎾?setter 鎴栭楠奸噴鏀捐皟鐢ㄣ€?

### 17.3 UnityPlayer 闈欐€佺嚎绱紙绛夊緟杩愯鏃跺湴鍧€浜ゅ弶纭锛?

鍙妫€鏌ユ湰鏈?UnityPlayer.dll锛屽ぇ灏?33069624 瀛楄妭锛孲HA256锛?
`BEE7BE52370ADDDD67BA61E4937CA51B7F272656841D187E95E505496DA798D1`銆?
璇佹嵁淇濆瓨浜?`bin/diagnostics/v51-physics-contract-probe/engine-binding-candidates.json`銆?
闈欐€佸瓧涓插拰鍑芥暟鎸囬拡鏁扮粍鎸?Animation 瀛愯〃閰嶅寰楀埌鍊欓€夊叆鍙ｏ紱**灏氭湭浠ヨ繍琛屾椂 resolver 鏍稿**锛?
涓嶈兘灏嗗€欓€夊湴鍧€鍐欏叆鐢熶骇 Hook/璋冪敤琛ㄣ€傝繛缁瓧绗︿覆鍖鸿繕鍖呭惈 Android 瀛愯〃锛屼笉鑳芥妸鏁翠釜杩炵画鍖哄綋鎴愬悓涓€寮犻厤瀵硅〃銆?

鍊欓€?CreateClothBindings_Injected 涓?`0xFDBA20`锛岃浆鍏?`0x132E8E8`锛涘寘瑁呭鍒?112 瀛楄妭鐨勮繑鍥炴暟鎹€?
鍚庣画鍊欓€夎矾寰?`0x327E70 鈫?0x2231F0` 瀵硅緭鍏ラ€愰」鏋勯€?32 浣嶅瓧绗︿覆鍝堝笇骞舵煡鍙︿竴浠界幇鏈夎〃锛?
鏈懡涓垎鏀?`0x2236E4` 灏嗗師杈撳叆搴忓彿鍔犲叆鍙︿竴鍒楄〃銆俙0x328530` 姹囬泦涓や釜 16 浣嶆暟閲忓強澶氱粍缂撳啿鎸囬拡銆?
杩欎笌绗?12 鑺傚凡鐭ョ殑鏈夋晥/鏃犳晥杈撳叆绱㈠紩鏈哄埗鐩哥锛屼絾杈撳叆瀛楃涓叉潵婧愩€佺幇鏈夎〃鐨勭敓鎴?鎵╁睍浠ュ強鏂板楠ㄩ鎺ョ撼鏉′欢浠嶉渶鏍稿疄銆?
涓嶈兘鐢辫繖浜涚嚎绱㈡柇瑷€浠绘剰鏂板 Transform 鑳借鎺ョ撼锛屾垨鏂█鏂板鑺傜偣涓€瀹氫笉鍙楁敮鎸併€?

鍊欓€?Disable 鍏ュ彛 `0xFDBE44` 鍙啓鍚敤鏍囧織锛涘€欓€?Destroy 璺緞
`0xFDBDA0 鈫?0x2E4E30 鈫?0xEFC680` 杩涘叆瀵硅薄娓呯悊銆?
杩欎簺闈欐€佺墖娈垫病鏈夊缓绔嬭法鏋勫缓 Task銆佹ā鎷?Job 鍜?Animator 鍐欏洖鐨勫畬鎴愯瘉鏄庛€?

### 17.4 鏈疆瀹為檯楠岃瘉涓庝骇鐗?

MSVC x64 鐜鎵ц涓ょ粍妫€鏌ワ紝**54 + 28 = 82 椤归€氳繃锛屾棤璺宠繃**锛?

```text
python -m unittest test_native_physics_contract_probe test_native_physics_probe test_native_physics_bone_probe test_native_physics_parameters test_native_physics_trace -v
python -m unittest test_physics_native_document test_physics_document test_physics_resources test_blender_physics test_blender_native_physics test_blender_selection_export test_blender_registration -v
```

绗簩缁勪娇鐢?Blender 5.0.1锛屾樉寮忔寚瀹?`EIEM_BLENDER` 鍜岀湡瀹?Typhoea 寮曠敤鍥?`EIEM_PHYSICS_EVIDENCE`銆?
娴嬭瘯鍦ㄧ嫭绔嬪悗鍙拌繘绋嬪拰涓存椂鐩綍杩愯锛屼笉淇敼鐢ㄦ埛鎵撳紑鐨?Blender 宸ョ▼銆?
鏂板 7 椤?DLL 濂戠害璇婃柇娴嬭瘯缂栬瘧鐢熶骇澶存枃浠讹紱娴嬭瘯涓殑鍙嶅皠銆佽繑鍥炲璞″拰 resolver 鐢卞涓绘ā鎷燂紝
涓嶆瀯鎴愮湡瀹炴父鎴?API 鎴愬姛璇佹嵁銆傚疄闄?Blender 娴嬭瘯楠岃瘉 11 缁勩€?7 纰版挒浣撶殑婧愬浘鍜岀紪杈戝線杩斿強淇鍚庣殑瀵煎嚭杈圭晫銆?

瀹屾暣 `build.bat` 宸查€氳繃锛屽寘鍚?EIEM 鍜屼袱浠戒唬鐞?DLL銆傛瀯寤烘爣璇嗭細
`resource-runtime-v51-physics-contract-probe`銆?
`bin/eiem.dll`锛?976128 瀛楄妭锛孲HA256锛?
`40B89B6B0BFC413EF7611EDC440D2DD938072A653A08F50B80FEA6ACF2A8B67E`銆?
鏋勫缓鏃ュ織锛歚bin/diagnostics/v51-physics-contract-probe/build.log`锛涢獙璇佹憳瑕佸湪鍚岀洰褰?`validation.json`銆?
鏋勫缓鍖呭惈鏁翠釜鏃㈡湁宸ヤ綔鍖虹殑鏀瑰姩锛屼笉鑳藉皢鍏朵粬宸插瓨鍦ㄥ姛鑳藉叏閮ㄧ畻浣滄湰杞柊澧炴垨娓告垙楠屾敹銆?

**鏈儴缃诧紝鏈啓鍏ユ父鎴忕洰褰曪紝鏈繘琛屾父鎴忕敾闈㈡垨鐢熷懡鍛ㄦ湡楠屾敹锛涙鏌ユ椂娌℃湁杩愯涓殑 Endfield銆?*
褰撳墠鍙氦浠樼殑鏄湰鍦拌瘖鏂?DLL 鍜岀绾夸綔鑰呮暟鎹疄鐜帮紝浠嶄笉鏄彲杩愯涓斿彲瀹夊叏娉ㄩ攢鐨勬父鎴?Physics 闆嗘垚銆?
涓嬩竴瀹炴満璇佹嵁闇€瑕佽褰曞疄闄?resolver 鍦板潃銆佸師鐢熺粍浠舵瀯寤?鍙栨秷/閿€姣併€佽緭鍏?Transform 韬唤鍙婃湁鏁?鏃犳晥缁戝畾銆?
妯℃嫙濮挎€佷笌缃戞牸椹卞姩锛屽苟鏍稿疄娑堣垂鑰呭叏閮ㄩ€€鍑虹殑鐪熷疄鏉′欢锛涘畬鎴愬悗鎵嶈繛鎺ョ敓浜у垱寤恒€佽繍琛屽強娉ㄩ攢璺緞銆?

## 18. 2026-09-07锛欴LL 宸ュ巶鐨勭嫭绔嬮厤缃噯澶?

### 18.1 绾犳鎺ュ叆椤哄簭鍜屾瀯閫犵姸鎬佸垽鏂?

绗?17 鑺備箣鍚庯紝瀵硅瘽鍐嶆灏嗙绾挎祴璇曡В閲婁负鈥滃師鐢熻繍琛屽熀纭€宸查獙璇佲€濓紝骞跺缓璁垹闄ゅ姞杞藉櫒淇濇姢銆?
浠?DisposeInternal 杩斿洖浣滀负楠ㄩ閲婃斁鏉′欢銆?*杩欎袱鏉″缓璁敊璇笖宸叉挙鍥烇紱鏈疆娌℃湁鎵ц瀹冧滑銆?*
鐢ㄦ埛闅忓悗鏄庣‘瑕佹眰淇濈暀杩欎簺杈圭晫銆傛湰鑺傚彧瀹炵幇灏氭湭琚粍浠舵垨鍘熺敓浠诲姟娑堣垂鐨勯厤缃崏绋匡紝
涓嶆嵁娴嬭瘯鏁伴噺銆佽繑鍥炵姸鎬併€佽瘖鏂鏁版垨鏈湴鏋勫缓鎺ㄥ鍘熺敓宸ヤ綔宸查€€鍑恒€?

鍙澶嶆牳鍚屼竴 GameAssembly 鐨?`BeyondBoneCloth..ctor`锛坄0x33F8C20`锛夊強
`ClothProcess..ctor`锛坄0x33F8EF0`锛夛紝璇佹嵁鍦?`bin/diagnostics/v52-physics-config-draft/constructors.json`銆?
缁勪欢鏋勯€犲嚱鏁板凡鍒嗗埆鏋勯€?serializeData銆乻erializeData2 鍜?process锛涙墍浠?**process 闈炵┖涓嶈瘉鏄?Awake/Init 宸插彂鐢?*锛?
涔熶笉鑳界敤鈥滄柊缁勪欢鐨?process 搴斾负绌衡€濇潵妫€娴嬪垵濮嬪寲鏄惁鍙楀埌鎶戝埗銆?
杩欎簺鍦板潃浠呯敤浜庨潤鎬佸鏌ワ紝娌℃湁鍐欏叆杩愯鏃跺垎娲捐〃銆?

### 18.2 瀹炵幇鑼冨洿

[鐙珛閰嶇疆鑽夌](../../src/eiem_native_physics_config.h) 宸茬撼鍏?DLL 缂栬瘧锛?

- 鎸夌▼搴忛泦銆佺被銆佸畬鏁存柟娉曠鍚嶄笌瀛楁绫诲瀷瑙ｆ瀽鍘熺敓 ClothSerializeData / ClothSerializeData2 鏋勯€犲櫒銆?
  `List<Transform>` 鏋勯€?娣诲姞/璇诲彇鏂规硶銆佷簲涓?Single 瀛楁鍜?Transform 鐖剁骇 getter銆?
  缂哄け鎴栨涔夋椂鎷掔粷鍑嗗锛屼笉鎸夊悓鍙傛暟鏁伴噺鐚滈噸杞姐€?
- 褰撳墠浠呭鐞?**v1銆佹棤纰版挒浣撶殑浣滆€呯粍**銆倂2 婧愬浘鏋勯€犲拰鍘熺敓纰版挒澶栧舰杞崲鏄惧紡鎷掔粷锛屼笉闄嶇骇鎴栦涪瀛楁銆?
- 鍑嗗鍓嶆牎楠屽叏閮ㄤ綔鑰呰妭鐐圭殑璺緞鏄犲皠銆佸疄闄?Transform 绫诲瀷銆佸師鐢熷瓨娲诲強缁勫唴鐪熷疄鐖跺瓙鍏崇郴锛?
  涓嶅洜浼犲叆鏁扮粍棣栭」鎭板ソ鍦ㄧ储寮?0 灏辨妸瀹冨綋鎴愭牴銆?
- 鏂板缓褰兼鐙珛鐨勬墭绠￠厤缃拰鏍归楠煎垪琛紝濉啓浜斾釜鏍囬噺骞跺洖璇伙紱鍒楄〃鏁伴噺銆佹垚鍛樹互鍙婂瓧娈靛紩鐢ㄤ篃鍥炶銆?
  浣滆€?FIXED/MOVE/IGNORE 鏁版嵁鍗曠嫭淇濈暀锛?*灏氭湭杞崲鎴愬師鐢?SelectionData**銆?
- 鍙湁鏁翠釜璇锋眰鎴愬姛鎵嶆浛鎹㈡棫鑽夌锛涗换鎰忕粍澶辫触閮戒繚鐣欏凡鏈夐厤缃紝閲婃斁姝ゆ鏈彁浜よ崏绋跨殑鎵樼寮曠敤銆?
  杩欎簺 Transform 寮曠敤浠呮彁渚涙墭绠″彲杈炬€э紝涓嶆彁渚?Unity native 瀛樻椿淇濊瘉鎴栭楠奸噴鏀炬巿鏉冦€?

娌℃湁璋冪敤 AddComponent銆丅uildAndRun銆両nit銆丏isposeInternal銆丏estroy 鎴?Animator 缁戝畾鎺ュ彛锛?
娌℃湁淇敼鐜版湁婧愮粍浠讹紝娌℃湁鎺ュ叆 Render/F10锛屼篃娌℃湁鍒涘缓鐢熶骇閰嶇疆鑽夌瀹炰緥銆?
鍥犳鏈疆鏄粍浠跺伐鍘傜殑**閰嶇疆鍑嗗瀹炵幇**锛屼笉鏄凡瀹屾垚鐨?Unity 缁勪欢宸ュ巶鎴栧彲鎵ц鐗╃悊閾俱€?
榛樿鏋勯€犻厤缃€佽妭鐐硅鑹层€侀€夋嫨鐐归『搴忓拰娓告垙鎵╁睍瀛楁浠嶉渶杩涗竴姝ユ槧灏勫強瀹炴満楠屾敹銆?

### 18.3 楠岃瘉璁板綍

鏂板 11 椤瑰涓绘祴璇曠紪璇戝苟鎵ц鐢熶骇閰嶇疆澶存枃浠讹紝瑕嗙洊鏍硅妭鐐逛贡搴忋€侀厤缃嫭绔嬨€佽矾寰?鐖剁骇/绫诲瀷/瀛樻椿妫€鏌ャ€?
涓嶆敮鎸佹牸寮忋€佹瀯閫?鍐欏叆寮傚父銆佸洖璇讳笉绗︺€佸悗缁粍澶辫触銆佸噯澶囦腑鑺傜偣澶辨晥鍙婇潪 Unity 绾跨▼銆?
娴嬭瘯棣栨缂栬瘧鍥犲涓诲彉閲忎笌 MSVC 鐨?`unexpected` 鍑芥暟鍚屽悕澶辫触锛涢噸鍛藉悕娴嬭瘯鍙橀噺鍚庡楠岄€氳繃銆?
鐩稿叧鍙傛暟閫傞厤鍣?17 椤瑰拰濂戠害璇婃柇 7 椤逛篃閫氳繃锛屾湰杞悎璁?35 椤规垚鍔熸鏌ワ紝鏃犺烦杩囷紱涓嶆槸娓告垙璋冪敤璇佹嵁銆?

鏈€缁堥厤缃祴璇曟棩蹇楋細`bin/diagnostics/v52-physics-config-draft/tests.log`銆?
瀹屾暣鏋勫缓鍜屼骇鐗╄褰曡鍚岀洰褰?`build.log` 涓?`validation.json`锛涘惎鍔ㄦ爣璇嗕负
`resource-runtime-v52-physics-config-draft`銆傛湰杞湭閮ㄧ讲銆佹湭鍐欏叆娓告垙鐩綍銆佹湭鎵ц瀹炴満鐗╃悊銆?
瀹屾暣鏋勫缓鎴愬姛锛宍bin/eiem.dll` 涓?4976128 瀛楄妭锛孲HA256锛?
`40E04FAFABE85C6B353BACCDB85C0F96B62FDEF48394565C2B5AB6BD6C72C360`銆?
鐢熶骇鍔犺浇鍣ㄧ殑闈炵┖ Physics 鎷掔粷鍒嗘敮浠ュ強楠ㄩ閲婃斁璺緞淇濇寔鍘熺姸銆?

鍚庣画浼樺厛纭閰嶇疆瀹屾垚鍓嶆姂鍒跺師鐢熷垵濮嬪寲鐨勭湡瀹炴満鍒躲€侀粯璁ら€夋嫨鏁版嵁鐢熸垚鍙?Animator 鎺ョ撼鏉′欢锛?
鍘熺敓缁勪欢瀹為獙鍜岀敓浜ф帴绾夸粛闇€鍦ㄨ繖浜涘叿浣撴潯浠跺緱鍒拌瘉鏄庡悗鎺ㄨ繘锛屼笉鎶婃湰鑺傝崏绋夸綔涓哄畠浠殑瀹屾垚鍑瘉銆?

## 19. 2026-09-07锛歷52 璇婃柇 DLL 棣栨閮ㄧ讲涓庡惎鍔ㄦ牳瀵?

纭 Endfield銆乁nityCrashHandler64 鍜?PlatformProcess 鍧囨湭杩愯鍚庯紝灏嗘湰鍦?v52 `bin/eiem.dll`
閮ㄧ讲鍒?`D:\Hypergryph Launcher\games\Endfield Game\plugin\eiem.dll`銆傚畨瑁呮枃浠朵负 4976128 瀛楄妭锛?
SHA256 涓?`40E04FAFABE85C6B353BACCDB85C0F96B62FDEF48394565C2B5AB6BD6C72C360`锛屼笌鏋勫缓浜х墿涓€鑷淬€?
鍘熷畨瑁?DLL 鍜屾湭淇敼鐨?`eiem.ini` 澶囦唤鍒?
`E:\EIEM_Workspace\plugin-releases\before-v52-physics-diagnostic-20260907-183816`锛涙棫 DLL SHA256 涓?
`2DC3590245DB6ED7A19C14F2C1EF5E3905A47078DB5AC7D61347D78AD7B74031`銆?

闅忓悗鍚姩 Endfield锛宍plugin/eiem_log.txt` 绗?2 琛岃褰?
`resource-runtime-v52-physics-config-draft`锛岃瘉鏄庢湰娆″畨瑁呯殑 DLL 宸茶娓告垙杩涚▼鍔犺浇銆傜幇鏈?Mesh Mod 姝ｅ父瑙ｆ瀽锛?
鐢熶骇鍔犺浇鍣ㄧ殑 Physics 鎷掔粷鍒嗘敮鏈慨鏀癸紝涔熸病鏈夋墽琛岃嚜鍒?Physics 鍔ㄤ綔銆?

鏈疆娌℃湁鍙栧緱鍘熺敓鐗╃悊杩愯璁板綍锛氫粠褰撳墠缁堢鍚戞父鎴忕獥鍙ｆ姇閫?`WM_EIEM_PHYSICS_TRACE_START` 鍜?
`WM_EIEM_PHYSICS_PROBE` 鍧囪繑鍥炲け璐ワ紱鏃ュ織涓病鏈?`PHYSICS-TRACE` / `PHYSICS-PROBE` 寮€濮嬭褰曪紝
`plugin/dumps` 涔熸病鏈夋柊澧?`physics_runtime_*.json`銆傝瘖鏂叆鍙ｆ湰韬嫭绔嬩簬 Mod 鍔犺浇鍣紝澶辫触涓嶅簲褰掑洜浜?
闈炵┖ Physics 鍔ㄤ綔鐨勬嫆缁濆垎鏀紱涓嬩竴娆＄敱娓告垙鍐?EIEM Dump 椤垫寜閽惎鍔ㄨ窡韪拰蹇収銆?

娓告垙闅忓悗鏀跺埌 `WM_CLOSE`锛屾棩蹇楄褰曠幇鏈夋ā鍨嬪疄渚嬬殑鍗歌浇銆傚悓鏈?Windows Application 鏃ュ織娌℃湁
Endfield Application Error锛屼篃娌℃湁鏂扮殑鏈湴 Endfield 宕╂簝杞偍銆侰rashSight 鏃ュ織鍑虹幇
`reportException`锛屼絾璇ヨ褰曚笉鍚冻浠ュ垽瀹氶€€鍑哄師鍥犵殑寮傚父璇︽儏锛屼笉鑳芥嵁姝ゅ０绉板彂鐢熸垨鏈彂鐢熸彃浠跺穿婧冦€?

鎴嚦鏈妭锛屽彧鑳界‘璁?DLL 閮ㄧ讲鍙婂姞杞姐€傚師鐢熺粍浠跺垱寤恒€佹ā鎷熴€丄nimator 瀵规柊澧?Transform 鐨勬帴绾炽€?
鏋勫缓鍙栨秷銆佸叏閮ㄦ秷璐硅€呴€€鍑哄拰瀹夊叏閲婃斁浠嶆湭楠岃瘉锛涙湰杞病鏈夊垹闄ゅ姞杞戒繚鎶ゃ€佽皟鐢ㄩ楠奸噴鏀炬垨鏀瑰姩 Mod銆?
Mesh銆丳hysics 浣滆€呮枃浠跺拰 Blender 宸ョ▼銆傛満鍣ㄥ彲璇昏褰曡
`bin/diagnostics/v52-physics-config-draft/deployment.json`銆?

## 20. 2026-09-07锛氳繍琛屾椂蹇収涓?v53 鑷姩璺熻釜

### 20.1 v52 鎵嬪姩蹇収瀹為檯璁板綍浜嗕粈涔?

鐢ㄦ埛閲嶆柊鍚姩娓告垙鍚庯紝閫氳繃褰撴椂鐨勬墜鍔ㄥ叆鍙ｇ敓鎴愪袱浠借繍琛屾椂蹇収銆備袱浠芥枃浠跺唴瀹瑰畬鍏ㄧ浉鍚岋紝SHA256 鍧囦负
`9AA21B4029ADC814A2D079A5E99994D88FB5D5C06328DC470391906414A085E3`锛屽凡澶嶅埗鍒?
`bin/diagnostics/v53-physics-auto-trace/v52-runtime-snapshot.json` 鍜?
`v52-runtime-snapshot-repeat.json`銆?

杩愯鏃?`il2cpp_resolve_icall` 灏嗕簲涓?Animator cloth 鎺ュ彛鍏ㄩ儴瑙ｆ瀽鍒板綋鍓?`UnityPlayer.dll` 鍙墽琛岄〉锛?

| 鎺ュ彛 | 杩愯鏃?RVA |
|---|---:|
| `CreateClothBindings_Injected` | `0xFDBA20` |
| `CreateClothBindingsByNameLst_Injected` | `0xFDB8E8` |
| `EnableClothBindings` | `0xFDBFA0` |
| `DisableClothBindings` | `0xFDBE44` |
| `DestroyClothBindings` | `0xFDBDA0` |

杩欑‘璁や簡绗?17.3 鑺傚搴旈潤鎬佸€欓€夌殑鍦板潃閰嶅锛屼絾蹇収娌℃湁璋冪敤杩欎簺鎺ュ彛锛屼粛鏈獙璇佹柊澧?Transform 鐨勬帴绾炽€?

鏈鏋氫妇鍒?96 涓凡鍔犺浇 BeyondBoneCloth 缁勪欢锛屽叾涓?46 涓?`activeAndEnabled/IsValid/IsRunning`
鍧囦负 true 涓?Result 涓?success锛?0 涓潎涓?false銆?5 涓繍琛岀粍浠舵湁鍙 BoneCloth setup锛?
Yvonne 鐨?`MC_ultMachine` 杩愯鏈夋晥浣嗘病鏈?BoneCloth setup锛岃鏄庝笉鑳芥妸鎵€鏈夋湁鏁?ClothProcess 閮藉綋浣滈閾俱€?

Typhoea 褰撳墠瀹炰緥鐨?11 涓粍浣跨敤鍚屼竴 Animator 瀹炰緥 ID `-115842`锛宼eam ID 杩炵画涓?17锝?7銆?
姣忎釜 BoneCloth setup 鐨?`transformCount = skinBoneCount + 1`锛屾渶鍚庝竴椤瑰搴旀鍓嶉潤鎬佸垎鏋愬彂鐜扮殑缁勪欢閿氱偣銆?
灏ゅ叾 `MBC_Typhoea_Hair_Back_Ponytail_Long` 鐨勭绾?selection 鐐规暟涓?38锛岃€岃繍琛屾椂
`skinBoneCount=14`銆乣transformCount=15`銆傝繖鏄笉鑳芥寜浣滆€呰妭鐐?selection 鏁扮粍鍜岃繍琛屾椂 Transform 鍒楄〃鐩存帴 zip
鐨勫疄闄呭弽渚嬶紱鍏朵粬缁勬暟閲忓伓鐒剁浉绛変篃涓嶈兘鎺ㄧ炕璇ュ弽渚嬨€?

璋冪敤璺熻釜鍙褰曞埌鍚屼竴绾跨▼涓婄殑 88 娆?`ClothManager.CompleteMasterJob` 杩涘叆/杩斿洖锛屽叡 176 涓簨浠躲€?
鏃犱涪澶便€佹棤鏈繑鍥炶皟鐢紝浜嬩欢璺ㄥ害 31 ms銆傛病鏈夎褰?Init銆丅uildAndRun銆丼tartRuntimeBuild銆丏ispose 鎴?
DisposeInternal锛岃鏄庢墜鍔ㄥ紑濮嬪彂鐢熷湪鐩稿叧鏋勫缓涔嬪悗锛涜繖浠藉揩鐓т笉鑳藉洖绛斿垱寤恒€佸彇娑堟垨娉ㄩ攢鏃跺簭銆?

### 20.2 v53 鏀逛负娴嬭瘯鏋勫缓鑷姩閲囬泦

鎸夌敤鎴疯姹傦紝鐗╃悊璇婃柇涓嶅啀浣滀负 Dump 椤典富鍔熻兘锛氬垹闄も€滃師鐢熺墿鐞嗚瘖鏂€濃€滃紑濮嬪師鐢熺墿鐞嗚窡韪€?
鈥滃仠姝㈠苟瀵煎嚭璺熻釜鈥濅笁涓帶浠跺強鍏朵笁鏉＄獥鍙ｆ秷鎭€倂53 鍦ㄦ父鎴忕獥鍙ｈ繘鍏?EIEM 涓荤嚎绋嬪叆鍙ｅ悗鑷姩瀹夎骞跺紑濮?
涓€娆″師鐢熻皟鐢ㄨ窡韪紱姝ｅ父鍏抽棴棣栨杩涘叆鍏抽棴鍒嗘敮鏃跺仠姝㈣褰曪紝骞跺皢缁勪欢蹇収鍜岃皟鐢ㄤ簨浠跺啓鍒扮嫭绔嬬洰褰?
`plugin/physics_diagnostics/physics_runtime_<pid>_<tick>.json`銆傚畠涓嶉渶瑕?Physics Mod锛屼篃涓嶅垱寤虹粍浠躲€?
璋冪敤 BuildAndRun銆佷慨鏀瑰弬鏁版垨閲婃斁楠ㄩ銆?

璇婃柇瀹炵幇宸蹭粠 `scene_dump.h` 绉诲埌鐙珛鐨?`eiem_native_physics_diagnostic.h`锛汥ump 妯″潡鍜岄〉闈㈠潎涓嶅啀鎵挎媴
鐗╃悊璇婃柇鍏ュ彛銆俙test_native_physics_trace` 涓?`test_hotkey_worker` 鍏?13 椤归€氳繃锛涘畬鏁?`build.bat` 鏋勫缓鎴愬姛銆?
鏈€缁?v53 `bin/eiem.dll` 涓?4974592 瀛楄妭锛孲HA256锛?
`1403CADBB80F61A5EDA31556B49832AE13FF33328B00309214E983840BD4649B`銆?
纭娓告垙杩涚▼閫€鍑哄悗宸查儴缃诧紝瀹夎鏂囦欢鍝堝笇涓庢瀯寤轰骇鐗╀竴鑷达紝鏈慨鏀?`eiem.ini`銆丮od 鎴栦綔鑰呰祫婧愩€?

閮ㄧ讲鍓嶅垱寤?v52 澶囦唤鐨勫懡浠ら敊璇娇鐢ㄤ簡鏈満 PowerShell 涓嶆敮鎸佺殑 `New-Item -LiteralPath`锛岀洰褰曞拰澶囦唤鏈垱寤猴紱
鍛戒护鍚庣画浠嶈鐩栦簡瀹夎 DLL銆傜幇鏈夊彂甯冨綊妗ｄ腑鏈壘鍒?v52 鍝堝笇
`40E04FAFABE85C6B353BACCDB85C0F96B62FDEF48394565C2B5AB6BD6C72C360` 鐨勫壇鏈紱
绗?19 鑺傚浠界洰褰曚腑浠嶄繚鐣欓儴缃?v52 涔嬪墠鐨勬棫 DLL銆倂53 瀹夎鏂囦欢鍜屼粨搴撴瀯寤轰骇鐗╁凡缁忛噸鏂版牳瀵逛竴鑷淬€?

闅忓悗浣跨敤鍔熻兘鐩稿悓銆佽瘖鏂唬鐮佸皻鏈媶鍑虹嫭绔嬪ご鏂囦欢鐨勯涓?v53 鏋勫缓鍚姩 PID 40436銆傛棩蹇楄褰曟瀯寤烘爣璇?
`resource-runtime-v53-physics-auto-trace`锛屽苟鍦ㄦ父鎴忕獥鍙ｅ畬鎴愬瓙绫诲寲鍚庤褰?
`[PHYSICS-DIAGNOSTIC] automatic native physics trace started`銆傛甯稿叧闂椂鑷姩鍐欏嚭
`plugin/physics_diagnostics/physics_runtime_40436_31457484.json`锛岃瘉鏄庤嚜鍔ㄥ紑濮嬪拰鍏抽棴瀵煎嚭鍧囧凡瀹為檯鎵ц銆?
璇ヨ繍琛?DLL SHA256 涓?`A42B802E53C3B626893D56C45F7DFF04D107CDE81CF1A3A475191E7FE8698220`锛?
鐜板浠戒簬 `E:\EIEM_Workspace\plugin-releases\before-v53-diagnostic-header-split-20260907-191207`銆?
鏈€缁堝畨瑁呯増浠呭皢鐩稿悓璇婃柇閫昏緫绉诲叆鐙珛澶存枃浠讹紱鍏跺悗缁疄鏈哄姞杞界粨鏋滆绗?20.3 鑺傘€?

鑷姩璁板綍涓?492974 瀛楄妭锛孲HA256
`F3D2724C44A5BA7D97FBEC419F8BE2E7C2911F91C5E39904E2667B33FA561079`锛屼粨搴撳壇鏈负
`bin/diagnostics/v53-physics-auto-trace/runtime-auto-shutdown.json`銆傝褰曡鐩?357016 ms锛?34 涓簨浠讹紝
`dropped=0`銆乣inFlightCalls=0`锛?6 娆?Init銆?6 娆?RemoveMonitoringProcess銆?5 娆?StartRuntimeBuild
鍜?180 娆?CompleteMasterJob 鍧囨湁杩涘叆/杩斿洖锛?5 娆?StartRuntimeBuild 鍏ㄩ儴杩斿洖 true銆?

瀵?45 鏉?BoneCloth 閾撅紝瀹炴満椤哄簭鍧囦负 `Init 鈫?RemoveMonitoringProcess 鈫?StartRuntimeBuild`锛涘敮涓€娌℃湁
BoneCloth setup 鐨?`MC_ultMachine` 鍙湁 Init 鍜?RemoveMonitoringProcess銆傝繖璇佹槑鍦ㄦ湰娆¤矾寰勯噷
RemoveMonitoringProcess 鏄垵濮嬪寲/閲嶅缓搴忓垪鐨勪竴閮ㄥ垎锛屼笉鑳芥寜鍚嶅瓧瑙ｉ噴鎴愭敞閿€瀹屾垚閫氱煡銆傚叧闂墠蹇収浠嶆湁
46 涓湁鏁堣繍琛?process锛岃褰曟病鏈?Dispose 鎴?DisposeInternal锛涘鍑哄彂鐢熷湪杞彂 `WM_CLOSE` 涔嬪墠锛?
鎵€浠ュ畠娌℃湁瑙傚療娓告垙鍚庣画鍏抽棴闃舵鐨勫師鐢熸竻鐞嗐€?

鍗充娇鏈璋冪敤鍏ㄩ儴鎴愬涓旀棤涓㈠け锛屼篃鍙鏄庢墍 Hook 鍑芥暟鐨勮瀵熺粨鏋滐紝涓嶄細鑷姩鎴愪负 Task銆丣ob銆丄nimator
鍐欏洖鎴栭楠奸噴鏀剧殑瀹屾垚鏍呮爮銆傛柊澧?Transform 鎺ョ撼銆佸彇娑堝拰瀹屾暣娉ㄩ攢浠嶉渶鍚庣画鏈夐拡瀵规€х殑杩愯璁板綍銆?

### 20.3 鏈€缁?v53 鏋勫缓鐨勪腑閫旀敞閿€璁板綍

鏈€缁堟媶鍒嗙増 v53 DLL锛圫HA256
`1403CADBB80F61A5EDA31556B49832AE13FF33328B00309214E983840BD4649B`锛夐殢鍚庣敱 PID 31592 瀹為檯鍔犺浇銆?
姝ｅ父閫€鍑虹敓鎴?`plugin/physics_diagnostics/physics_runtime_31592_32160296.json`锛涗粨搴撳壇鏈负
`bin/diagnostics/v53-physics-auto-trace/runtime-final-header-split.json`锛?061368 瀛楄妭锛孲HA256
`802C6EE9DF8462A8ECF62ECB2DCC410DFE332BD53B6E5E221125D3537054C35A`銆?

璁板綍瑕嗙洊 52125 ms锛屽叡 3402 涓簨浠躲€?701 娆¤皟鐢ㄣ€傚叏閮ㄤ簨浠舵潵鑷嚎绋?20672锛?701 娆¤皟鐢ㄥ潎鏈夐厤瀵圭殑
杩涘叆鍜岃繑鍥烇紝`dropped=0`銆乣inFlightCalls=0`銆傛搷浣滅粺璁″涓嬶細

| 鎿嶄綔 | 璋冪敤鏁?| 瑙傚療鍒扮殑杩斿洖 |
|---|---:|---|
| `ClothProcess.Init` | 298 | 298 娆¤繑鍥?|
| `TeamManager.RemoveMonitoringProcess` | 293 | 293 娆¤繑鍥?|
| `ClothProcess.StartRuntimeBuild` | 137 | 137 娆″潎涓?true |
| `ClothManager.CompleteMasterJob` | 735 | 735 娆¤繑鍥?|
| `ClothProcess.Dispose` | 119 | 119 娆¤繑鍥?|
| `ClothProcess.DisposeInternal` | 119 | 119 娆¤繑鍥?|

119 涓娉ㄩ攢鐨?process 鍧囧憟鐜板悓涓€宓屽椤哄簭锛?

`Dispose enter 鈫?DisposeInternal enter 鈫?RemoveMonitoringProcess enter/return 鈫?DisposeInternal return 鈫?Dispose return`

娉ㄩ攢闆嗕腑鍦?tick 32137562锝?2137718銆傛鍚?tick 32150796 鍐嶆鍑虹幇 `CompleteMasterJob`锛宼ick 32154156
鍙堝嚭鐜版柊鐨?Init 鍜?StartRuntimeBuild锛岄殢鍚庢墠鍋滄璁板綍骞跺鍑恒€傚洜姝ゆ湰娆℃崟鑾风殑鏄繍琛屼腑閫旂殑鐢熷懡鍛ㄦ湡鍒囨崲锛?
涓嶆槸杞彂 `WM_CLOSE` 涔嬪悗鐨勮繘绋嬬粓姝㈡竻鐞嗐€傚畠纭浜嗙幇鏈夋父鎴忓璞″湪璇ヨ矾寰勪腑鐨勫悓姝ヨ皟鐢ㄥ祵濂楋紱鍚屾椂涔熻繘涓€姝?
璇存槑 `RemoveMonitoringProcess` 鍚屾椂鍑虹幇鍦ㄥ垵濮嬪寲/閲嶅缓鍜屾敞閿€涓婁笅鏂囷紝涓嶈兘鑴辩璋冪敤鏍堟寜鍚嶇О瑙ｉ噴鍏惰涔夈€?

瀵煎嚭鏃舵灇涓惧埌 123 涓粍浠讹細55 涓?`activeAndEnabled/IsValid/IsRunning` 鍧囦负 true 涓?Result 涓?success锛?
68 涓笁椤瑰潎涓?false銆?4 涓繍琛岀粍浠舵湁 BoneCloth setup锛涜繍琛屼腑鐨?`MC_ultMachine` 浠嶆病鏈夎 setup銆?
鏈渚濇棫鍙瀵熸父鎴忓師鏈夌粍浠讹紝娌℃湁鐢?EIEM 鍒涘缓鏂扮粍浠舵垨鎻愪氦浣滆€呯墿鐞嗘暟鎹€?

`DisposeInternal` 鍜屽灞?`Dispose` 杩斿洖鐜板湪灞炰簬宸茶瀵熶簨瀹烇紝浣?Hook 娌℃湁瑕嗙洊鍘熺敓 Task/Job 瀹屾垚閫氱煡銆?
Animator 鍐欏洖缁撴潫鎴栫鐞嗗櫒鍐呴儴寮曠敤閫€浼戙€傚洜姝よ繖浜涜繑鍥炰粛涓嶈兘鍗曠嫭浣滀负閲婃斁 EIEM Transform/楠ㄩ鐨勫畬鎴愭爡鏍忥紱
鏂板 Transform 鎺ョ撼銆佹瀯寤哄彇娑堝拰 EIEM 鑷缓缁勪欢鐨勫畬鏁存敞閿€浠嶆湭楠岃瘉銆?

### 20.4 v54 鑷姩璺熻釜 Animator binding锛堝疄鏈哄惎鍔ㄥけ璐ワ紝鏈畨瑁?Hook锛?

涓嬩竴杞瘖鏂户缁暀鍦ㄧ嫭绔嬫祴璇曟ā鍧楋紝涓嶆仮澶?Dump 鎺т欢鎴栫獥鍙ｆ秷鎭€倂54 鍦ㄦ棦鏈夎嚜鍔ㄨ窡韪腑鍔犲叆浜斾釜宸茬敱
杩愯鏃?resolver 鍜屽厓鏁版嵁鍏卞悓纭鐨?Animator InternalCall锛?

- `CreateClothBindings_Injected`
- `CreateClothBindingsByNameLst_Injected`
- `EnableClothBindings`
- `DisableClothBindings`
- `DestroyClothBindings`

瀹夎 Hook 鍓嶉€愰」鏍稿绋嬪簭闆嗐€佺被鍨嬨€佸畬鏁村弬鏁颁笌杩斿洖绫诲瀷銆両nternalCall 鏍囧織銆乺esolver 杩斿洖鍊煎強鍙墽琛岄〉锛?
涓嶄娇鐢ㄥ浐瀹?RVA銆侶ook 鍙浆鍙戞父鎴忓師璋冪敤骞惰褰曡繘鍏?杩斿洖銆備袱涓?Create 鍏ュ彛鍦ㄥ師璋冪敤姝ｅ父杩斿洖鍚庤鍙?
`AnimationTransformRWBufferHandle` 宸茬敱鏈鍏冩暟鎹‘璁ょ殑鍓嶄袱涓?`UInt16` 瀛楁 `count/invalidCount`锛?
瀹冧笉閬嶅巻杈撳叆鏁扮粍銆佹湁鏁堢储寮曡〃鎴?Transform锛屼篃涓嶈嚜琛岃皟鐢ㄥ垱寤恒€佸惎鐢ㄣ€佺鐢ㄥ拰閿€姣佹帴鍙ｃ€?

`test_native_physics_trace`銆乣test_native_physics_contract_probe` 鍜?`test_hotkey_worker` 鍏?20 椤归€氳繃锛?
瑕嗙洊瀹屾暣鍙傛暟杞彂銆佷袱涓?Create 杩斿洖璁℃暟銆両nternalCall 濂戠害澶辫触銆丠ook 閮ㄥ垎瀹夎澶辫触銆佸紓甯稿睍寮€銆佸苟鍙戝強
鑷姩鍏ュ彛浠嶄笌 Dump 闅旂銆傚畬鏁?`build.bat` 鏋勫缓鎴愬姛銆倂54 `bin/eiem.dll` 涓?5505536 瀛楄妭锛孲HA256锛?
`937448ACD0D3B778FD887B5D1ACB42AB920A2A8A7F9B8A6207E48B8690B573BF`銆?

纭娓告垙閫€鍑哄悗宸插皢 v53 澶囦唤鍒?
`E:\EIEM_Workspace\plugin-releases\before-v54-physics-binding-trace-20260907-193945`锛岄殢鍚庨儴缃?v54锛?
娓告垙鐩綍 DLL 涓庢瀯寤轰骇鐗╁搱甯屼竴鑷淬€?

PID 41236 闅忓悗瀹為檯鍔犺浇 v54锛屼絾鏃ュ織鍦ㄥ畨瑁呬换浣曠墿鐞?Hook 鍓嶆姤鍛?
`Physics Animator icall signature unavailable: CreateClothBindings_Injected`銆傞€€鍑哄揩鐓?
`bin/diagnostics/v54-physics-binding-trace/runtime-start-failure.json` 涓?375931 瀛楄妭锛孲HA256
`8F32C9BF2078331A66DCDC608C3CFFDB9EAD0AAA24A2EF4035665F455F7DCA6C`锛涘叾涓?lifecycle
`session=0/sequence=0/events=[]`锛岃瘉鏄庢湰杞病鏈?binding 鎴?ClothProcess 璺熻釜浜嬩欢銆?

鏍瑰洜鏄?v54 Hook 瀹夎鍣ㄦ妸 Animator 绋嬪簭闆嗚鍐欐垚 `UnityEngine.CoreModule.dll`锛岃€屽疄闄呮帰閽堜竴鐩翠粠
`UnityEngine.AnimationModule.dll` 璇诲彇 Animator銆傚悓涓€澶辫触蹇収浠嶄粠姝ｇ‘绋嬪簭闆嗚褰曞埌绮剧‘鐨?
`CreateClothBindings_Injected(Transform[], AnimationTransformRWBufferHandle&)` 鍏冩暟鎹紝骞舵妸浜斾釜 icall
鍏ㄩ儴瑙ｆ瀽鍒?UnityPlayer 鍙墽琛岄〉銆傝繖鏄瘖鏂疄鐜伴敊璇紝涓嶆槸娓告垙缂哄皯 binding 鎺ュ彛锛泇54 涓嶈兘浣滀负杩欎簺鎺ュ彛
娌℃湁琚父鎴忚皟鐢ㄧ殑璇佹嵁銆?

### 20.5 v55 缁熶竴 Animator 绋嬪簭闆嗗绾︼紙宸叉瀯寤洪儴缃插苟鎴愬姛鍚姩璺熻釜锛?

v55 鍦?`eiem_native_physics_api.h` 瀹氫箟鍞竴鐨?`EiemPhysicsAnimatorImage` 甯搁噺锛屾帰閽堜笌 Hook 瀹夎鍣ㄥ叡鍚屼娇鐢?
`UnityEngine.AnimationModule.dll`锛屽垹闄や袱澶勭嫭绔嬪瓧绗︿覆閫犳垚鐨勬紓绉汇€傚涓绘祴璇曚篃浣跨敤鍚屼竴甯搁噺锛屽苟闈欐€佹嫆缁?
鐗╃悊鎺㈤拡鎴栬窡韪櫒鍐嶆寮曠敤 `UnityEngine.CoreModule.dll`銆?

淇鍚庣 20.4 鑺傜殑 20 椤规鏌ュ啀娆″叏閮ㄩ€氳繃锛屽畬鏁?`build.bat` 鏋勫缓鎴愬姛銆倂55 `bin/eiem.dll` 涓?
5505024 瀛楄妭锛孲HA256锛歚514A9E661E8C274BB891A545A6C2E41786706CC15F54D0A69ACDB1C8067869BB`銆?
娓告垙閫€鍑哄悗灏?v54 澶囦唤鍒?
`E:\EIEM_Workspace\plugin-releases\before-v55-physics-binding-trace-20260907-194722`锛岄殢鍚庨儴缃?v55锛涘畨瑁呮枃浠?
涓庢瀯寤轰骇鐗╁搱甯屼竴鑷淬€?

鐢ㄦ埛闅忓悗閲嶅惎娓告垙锛孭ID 37728 瀹為檯鍔犺浇璇?v55锛沗plugin/eiem_log.txt` 鍚屾椂璁板綍鏋勫缓鏍囪瘑鍜?
`[PHYSICS-DIAGNOSTIC] automatic native physics trace started`锛岀‘璁ょ▼搴忛泦淇鍚庤窡韪畨瑁呮垚鍔熴€?
姝ｅ父閫€鍑哄悗鐢熸垚鐨勫揩鐓у凡澶嶅埗涓?
`bin/diagnostics/v55-physics-binding-trace/runtime.json`锛?082130 瀛楄妭锛孲HA256锛?
`022893B718A4A14CEED022C1620A148A8CC3D1DD246C78BF45DF03A6472EA5F9`锛涙満鍣ㄥ彲璇荤粺璁¤鍚岀洰褰?
`summary.json`銆傝褰曡法搴?1164453 ms锛屽叡 32236 涓簨浠躲€?6118 娆¤皟鐢紝鍏ㄩ儴鏉ヨ嚜绾跨▼ 24856锛屽叏閮ㄨ皟鐢?
杩涘叆/杩斿洖鎴愬锛宍dropped=0`銆乣inFlightCalls=0`銆?

鍏朵腑 `CompleteMasterJob=14592`銆乣Init=446`銆乣StartRuntimeBuild=181`锛堝叏閮ㄨ繑鍥?true锛夈€?
`RemoveMonitoringProcess=465`銆乣Dispose/DisposeInternal=217/217`銆?17 娆?Dispose 浠嶉兘鏄?
`Dispose 鈫?DisposeInternal 鈫?RemoveMonitoringProcess 鈫?return` 鐨勫悓姝ュ祵濂楋紱瀹冧粛涓嶆槸寮傛閫€浼戞爡鏍忋€?
蹇収鏋氫妇鍒?138 涓粍浠讹紝鍏朵腑 61 涓?IsValid/IsRunning 涓?true锛屽苟鍏宠仈 6 涓潪闆?Animator 韬唤銆?

浜斾釜 Animator binding Hook 鐨勮皟鐢ㄦ暟鍏ㄩ儴涓?**0**銆傝繖涓嶆槸鈥滄柊澧?Transform 宸茶鎺ョ撼鈥濈殑璇佹嵁锛屼篃涓嶈兘璇佹槑
鎺ュ彛鍦ㄨ窡韪獥鍙ｅ浠庢湭浣跨敤锛涘畠璇存槑鏈闀挎椂闂磋褰曞嵆浣胯鐩?181 娆¤繍琛屾椂鏋勫缓鍜?217 娆℃敞閿€锛屼粛娌℃湁瑙﹀彂
`Create/Enable/Disable/DestroyClothBindings`銆傚洜姝や笉鑳芥妸闈欐€佸畾浣嶅埌鐨?`TeamManager.UpdateTeamAnimatorData`
鍒嗘敮褰撴垚姣忎釜 ClothProcess 鏋勫缓蹇呯粡姝ラ锛屼笅涓€杞渶瑕佺洿鎺ヨ瀵?TeamManager 鐨勭櫥璁般€乨irty 鍜屾眹鎬绘洿鏂板叆鍙ｃ€?

### 20.6 v56 宸ヤ綔鍖猴細Render 鍛戒腑涓庤鑹?UI 鐨勬ā鍨嬬骇 Physics 鎰忓浘

鐢ㄦ埛鎸囧嚭鐗╃悊楠ㄩ鐨勫鍒犲繀椤绘部鐢?Mesh 宸叉湁 INI 鍛戒腑璇硶锛屽苟瑕佹眰鏍稿瑙掕壊 UI銆傛簮鐮佸鏍哥粨鏋滃涓嬶細

- `EiemSetRenderField` 宸茶 `physics=` 杩涘叆 Render 鏉′欢璇彞鐨勬櫘閫氭眰鍊硷紱`asset`銆佺浉瀵?`path` 鍜?Mesh
  褰㈢姸鏉′欢浠嶇敱 `EiemRenderRuleMatches` 缁熶竴鍒ゆ柇锛岄涓尮閰嶈鍒欎粛浼樺厛銆?
- `CharUIModelMono` 鑳戒互鑷韩 GameObject 娉ㄥ唽鍏蜂綋 UI 妯″瀷锛屼絾娌℃湁 PFB 璺緞锛涘洜姝ら《灞?`asset=` Render
  鏄鑹插睍绀?UI 鐨勫繀瑕佸叆鍙ｃ€俙OnRelease` 鍦ㄦ父鎴忓師閲婃斁鍓嶇Щ闄?owner锛宍SetVisible(false)` 姝ゅ墠娌℃湁鐙珛鐘舵€併€?
- Renderer setter Hook 鍙互鍙嬁鍒板崟涓?Renderer/Mesh锛屾湭蹇呰兘寰楀埌妯″瀷鏍癸紱瀹冧笉鑳芥壙鎷呮ā鍨嬬骇鐗╃悊鍒涘缓鎴栭攢姣併€?

v56 宸ヤ綔鍖烘嵁姝ゅ鍔犱簡涓嶆墽琛屽師鐢熻皟鐢ㄧ殑 Physics 鎰忓浘灞傘€俁enderer 鍦ㄦ敞鍐屾ā鍨嬫牴閬嶅巻涓耽寰楃幇鏈夐涓懡涓悗锛?
灏嗚鍒欎腑鐨?Physics 蹇収鍔犲叆 `EiemModelInstanceState`锛涘悓涓€ Mod 鍐呭紩鐢ㄥ悓涓€鍑嗗蹇収鐨勫涓祫婧愬埆鍚嶃€丷enderer
鎴?LOD 鎸夊揩鐓ц韩浠藉幓閲嶃€傛瘡娆℃敞鍐屻€佹睜鍖栧鐢ㄣ€丗10 鎴栨潯浠?UI 浜嬪姟 reconcile 閮介噸寤哄綋鍓嶆ā鍨嬭鍒掞紱鏀堕泦鍙戠敓鍦?
鍙楀奖鍝?Mod 杩囨护涔嬪墠锛屾墍浠ヨ緝楂樹紭鍏堢骇鐨勬湭鏀瑰姩瑙勫垯浠嶄細闃绘浣庝紭鍏堢骇瑙勫垯琚敊璇彁鍗囥€俙physics=` 琚竻绌烘垨鏉′欢
鍒囨崲鍚庯紝鎰忓浘浠庢柊璁″垝娑堝け锛屼絾褰撳墠浠ｇ爜娌℃湁鎶婅繖涓€鍙樺寲褰撲綔鍙洿鎺ラ噴鏀?Transform 鐨勫畬鎴愬嚟璇併€?

瑙掕壊 UI owner 鐜板彟瀛?active 鐘舵€侊細`OnAwake` / `SetVisible(true)` 婵€娲诲苟澶嶇敤璁″垝锛宍SetVisible(false)` 淇濈暀璁″垝浣?
鏍囪涓嶆椿璺冿紝`OnRelease` 鎵嶇Щ闄?owner銆傛ā鍨嬫槸鍚︿粛鏈夋椿璺?owner 鐙珛璁＄畻锛岀粰鍚庣画 native adapter 鐨?
enable/disable/retire 鐘舵€佹満鎻愪緵杈撳叆锛涙湰杞病鏈夊疄鐜拌鐘舵€佹満銆?

閽堝鎬ч獙璇佷负 Physics 璧勬簮/鎰忓浘 13 椤瑰拰 Render/瑙掕壊 UI 濂戠害 33 椤癸紝鍏?46 椤归€氳繃锛涘畬鏁?`build.bat` 閫氳繃銆?
鏈湴 v56 `bin/eiem.dll` 涓?5515264 瀛楄妭锛孲HA256锛?
`34203F39C109222223C60D1C680231606BAE583A25F01C8240762AAB1611EA16`銆傝鐙珛 v56 鏋勫缓娌℃湁閮ㄧ讲锛涘叾瑙勫垝浠ｇ爜
闅忓悗淇濈暀鍦ㄧ 20.7 鑺傚凡閮ㄧ讲鐨?v57 涓€傜敱浜庣敓浜у姞杞藉櫒浠嶆暣浠芥嫆缁濆甫 Physics 鍔ㄤ綔鐨?Mod锛岄儴缃插寘鍚繖浜涗唬鐮?
涓嶇瓑浜庤鍒掑眰宸插湪娓告垙涓帴鏀?Physics Mod 鎴栨墽琛屽師鐢熺墿鐞嗐€?

鐢熶骇鍔犺浇鍣ㄧ殑鏁?Mod Physics 鎷掔粷鍒嗘敮淇濇寔涓嶅彉锛涙湰杞病鏈夊垱寤虹粍浠躲€佸惎鍔ㄦā鎷熴€佽皟鐢?Animator binding銆?
淇敼楠ㄩ閲婃斁鎴栬瘉鏄庡師鐢熼€€浼戣竟鐣屻€倂55 鐨勯浂 binding 缁撴灉鐩存帴浜х敓绗?20.7 鑺傜殑涓嬩竴杞鐞嗗櫒璺熻釜锛涙ā鍨嬫剰鍥?
浠嶉渶绛夊緟鍙獙璇佺殑 native adapter 鐘舵€佹満锛屼笉鑳借秺杩囪璇佹嵁缂哄彛鐩存帴鎵ц銆?

### 20.7 v57 鑷姩璺熻釜 TeamManager Animator 姹囨€诲叆鍙ｏ紙宸查儴缃诧紝寰呭疄鏈哄姞杞斤級

鏍规嵁 v55 鐨勯浂 binding 缁撴灉鍜岄潤鎬佹柟娉曟槧灏勶紝v57 鍦ㄥ師 12 涓?Hook 涓婂鍔犱簲涓簿纭鍚嶇殑 TeamManager 鍏ュ彛锛?

- `UpdateTeamAnimatorData(ExNativeArray<Int16>, TransformAccessArray)`锛?
- `ClearTeamAnimatorData(Int32)`锛?
- `AddTeamAnimatorData(Int32, ClothProcess)`锛?
- `AddAnimatorTransform(Int32, Transform)`锛?
- `MarkAnimatorTransformDirty(Int32, Transform)`銆?

杩欎簺 wrapper 鍙師鏍疯浆鍙戝弬鏁板苟璁板綍 manager銆佸叧鑱斿璞″拰 teamId锛涗笉璇诲彇鍊肩被鍨嬪唴瀹癸紝涓嶈皟鐢?Animator锛?
涔熶笉鏀瑰彉鍥㈤槦銆乀ransform 鎴?binding 鐘舵€併€傝繖鏍蜂笅涓€娆¤褰曞彲浠ュ尯鍒嗏€滃洟闃熶粠鏈繘鍏ユ眹鎬绘洿鏂扳€濄€?
鈥滆繘鍏ユ洿鏂颁絾鏃犻渶閲嶅缓 binding鈥濆拰鈥滆繘鍏ユ洿鏂颁笖瀹為檯璋冪敤 Animator icall鈥濅笁绉嶆儏鍐点€?

`test_native_physics_trace`銆乣test_native_physics_contract_probe` 涓?`test_hotkey_worker` 鍏?20 椤归€氳繃锛屽畬鏁?
`build.bat` 鎴愬姛銆倂57 DLL 涓?6043136 瀛楄妭锛孲HA256锛?
`3AB2482DCF33270A0D4165B6E8B508709E864A68A8EB0FAC5381D72BF9BBE542`銆傜‘璁ゆ父鎴忚繘绋嬪叏閮ㄩ€€鍑哄悗锛屽凡鎶?v55
澶囦唤鍒?`E:\EIEM_Workspace\plugin-releases\before-v57-physics-team-animator-trace-20260907-202605` 骞堕儴缃?v57锛?
娓告垙鐩綍涓庢瀯寤轰骇鐗╁搱甯屼竴鑷达紝鏈慨鏀?Mod 鎴栧叏灞€閰嶇疆銆倂57 灏氬緟涓嬩竴娆℃父鎴忓惎鍔ㄤ笌姝ｅ父閫€鍑虹敓鎴愯褰曘€?

璇ラ儴缃蹭粛鏄鍔ㄦ祴璇?DLL銆傜敓浜?Physics Mod 鎷掔粷鍒嗘敮銆佸師鐢熺粍浠跺垱寤恒€丼electionData 杞崲銆佹ā鎷熷惎鍋滃拰楠ㄩ
閲婃斁鍧囨湭杩炴帴鎴栨敼鍙樸€?

### 20.8 v58 缁熶竴 Mesh 娑堣垂鑰呰涔夊苟琛ラ綈 NPC 鏈€缁?Render 杈圭晫锛堝凡閮ㄧ讲锛屽緟瀹炴満鍔犺浇锛?

鐜拌棰嗗煙绾﹀畾鏄?Render 鎸?Mesh 韬唤浣滅敤浜庢墍鏈夋秷璐硅€咃紝PFB 鍙繚瀛樿祫婧愬叧绯诲拰瀹炰緥鏉ユ簮銆傚鏍稿彂鐜?
`EiemCompileModProgram` 鏇炬妸 `Prefab.render.N` 涓?`partner.N` 涓€璧疯涓烘ā鏉垮紩鐢紝鍓嶈€呬細鎶婃簮 Render 鎺掗櫎鍑?
鍏ㄥ眬瑙勫垯锛涜繍琛屾椂闅忓悗鍙堝彧鍦ㄧ簿纭?PFB path 鍛戒腑鏃舵墽琛屽畠銆傝繖涓庝笂杩扮害瀹氬啿绐併€倂58 鏀逛负鍙湁 `partner.N`
鎸囧悜鐨勯澶?Renderer 妯℃澘涓嶅弬涓庢簮 Mesh 鍖归厤锛汸FB 寮曠敤鐨?Render 浠嶈繘鍏ュ叏灞€瑙勫垯锛屽苟鍒犻櫎鎸?PFB path 鐨?
绗簩娆?Render 鎵ц锛岄伩鍏嶅悓涓€娑堣垂鑰呴噸澶嶅簲鐢ㄣ€俙physics=` 鐨勮鍒掍娇鐢ㄥ悓涓€ Render 鍛戒腑锛屽洜姝ゅ凡娉ㄥ唽妯″瀷鏍逛笂
涔熼噰鐢ㄨ鍏ㄥ眬璇箟銆?

瀹炴満鏃ф棩蹇楀悓鏃剁‘璁?NPC 鐨?`CreateSMSGO 鈫?AssignSkin 鈫?SetSMRRootBone` 椤哄簭鍜屽悗鑰呮敹鍒扮殑鍏蜂綋 Renderer 鏁扮粍銆?
v58 鍦ㄦ父鎴忓師 `SetSMRRootBone` 杩斿洖鍚庯紝瀵硅鏁扮粍閲嶆斁 mesh銆乵aterial銆乻kip銆乸artner 绛夊叏灞€ Render 鍔ㄤ綔锛?
鐢ㄤ簬瑕嗙洊 NPC 鏈€缁堢粍瑁呴樁娈点€傝鏁扮粍娌℃湁鍙潬妯″瀷鏍癸紝鎵€浠ユ鍏ュ彛涓嶅垱寤烘垨鍒犻櫎 Physics锛屼篃涓嶅崟鐙啓鍏ユā鍨嬬骇
Physics 鎰忓浘锛汵PC 鐨?Physics 鎵€鏈夋潈浠嶉渶鍚庣画鐢ㄦ槑纭ā鍨?owner 鎺ュ叆銆?

瑙ｆ瀽/缂栬瘧銆佺湡瀹炲妯″瀷鐧昏銆侀噸杞界敓鍛藉懆鏈熴€丷ender 鍚堝悓鍜?Physics 璧勬簮妫€鏌ュ叡 62 椤瑰湪 MSVC 鐜閫氳繃锛?
瀹屾暣 `build.bat` 閫氳繃銆倂58 DLL 涓?6040064 瀛楄妭锛孲HA256锛?
`CF0AFAA0972E12C0583332438A27514DD11E6F153F88BF150CD022161FC3EF39`銆傜‘璁ゆ父鎴忚繘绋嬫湭杩愯鍚庯紝v57 澶囦唤鍒?
`E:\EIEM_Workspace\plugin-releases\before-v58-global-mesh-consumers-20260907-210846`锛寁58 宸查儴缃蹭笖娓告垙鐩綍
鍝堝笇涓€鑷淬€傞儴缃插墠鐨勬渶鏂版父鎴忔棩蹇椾粛鏍囪 v55锛岃鏄?v57 娌℃湁鑾峰緱瀹炴満鍔犺浇璁板綍锛泇58 淇濈暀 v57 鐨勮嚜鍔ㄧ墿鐞嗚窡韪€?

鏈妭娌℃湁绉婚櫎鐢熶骇 Physics Mod 鎷掔粷鍒嗘敮锛屾病鏈夎繛鎺ュ師鐢熺粍浠跺垱寤恒€佹墽琛屻€佸彇娑堟垨楠ㄩ閲婃斁銆倂58 鐨?Mesh/NPC
琛屼负鍜屼繚鐣欑殑鐗╃悊璺熻釜閮藉皻寰呬笅涓€娆℃父鎴忓惎鍔ㄩ獙璇併€?

### 20.9 v58 NPC 鍥炴斁琚疄鏈哄惁瀹氾紝v59 鏀圭敤鍏蜂綋 Renderer 鍒濆鍖栧叆鍙ｏ紙瀹炴満楠岃瘉锛?

v58 宸茬敱娓告垙杩涚▼鍔犺浇銆侾FB 寮曠敤涓嶅啀闄愬埗 Render 鐨勫叏灞€ Mesh 韬唤璇箟锛岃繖閮ㄥ垎鐢卞惎鍔ㄦ椂 4 鏉℃簮瑙勫垯浠ュ強
鏅€氭ā鍨嬨€佽鑹?UI 鐨?Typhoea 鍛戒腑璁板綍纭銆備絾 52 娆?`SetSMRRootBone` 璁板綍鐨?`applied` 鍧囦负 0锛?0.8
鎻愬嚭鐨勬渶缁堥楠兼暟缁勫洖鏀惧湪鏈瀹炴満涓病鏈夎鐩栫洰鏍囧疄渚嬶紝涓嶈兘缁х画浣滀负 NPC 鎺ュ叆渚濇嵁銆?

鏃ュ織涓殑 Renderer `00000011E4D09F40` 鍜?`0000001022369BA0` 鍧囦互 917 椤剁偣鐨勫師 Typhoea body Mesh
杩涘叆 `EntityRenderHelperMaterialController.RendererInfo._Init`锛涘搴?cloth 浠嶅惎鐢ㄣ€傚畠浠病鏈夌粡杩囧叕寮€
`SkinnedMeshRenderer.set_sharedMesh` setter锛屼篃娌℃湁杩涘叆宸茬櫥璁扮殑妯″瀷鏍规墽琛屽櫒鎴栦换浣曟垚鍔熺殑鏈€缁堥楠煎洖鏀俱€?
鍥犳鏈 NPC 婕忓簲鐢ㄧ殑瀹炶瘉鍘熷洜鏄叿浣?Renderer 鏋勯€犳梺璺己灏戞墽琛屽叆鍙ｏ紝涓嶆槸 PFB 鎴?INI 鍖归厤閿欒銆?

v59 灏?`SetSMRRootBone` 鎭㈠涓洪『搴忚瀵熴€傚湪娓告垙 `_Init` 淇濆瓨骞插噣婧愭潗璐ㄤ箣鍚庯紝鍏蜂綋
SkinnedMeshRenderer 杩涘叆鐜版湁鍏ㄥ眬 Mesh 韬唤鎵ц鍣紱涓嶆寜 NPC 绫诲瀷銆佽鑹茶韩浠芥垨 PFB 鍒嗗弶瑙勫垯銆?
鐩稿叧 63 椤?MSVC/瀹夸富妫€鏌ュ強瀹屾暣鏋勫缓閫氳繃銆侱LL 涓?6040064 瀛楄妭锛孲HA256锛?
`E5E11A9A51FDDBC1CD2BF8EBF7A0D9A39392F0268E1608367CFBFFA4813DA890`锛泇58 澶囦唤鍒?
`E:\EIEM_Workspace\plugin-releases\before-v59-npc-renderer-init-20260907-213635` 鍚庡凡閮ㄧ讲锛屽皻寰呮柊杩涚▼楠岃瘉
`[MOD-RENDERER-INIT]`銆?4664 椤剁偣璇诲洖鍜?NPC 鐢婚潰銆?

2026-09-08 鐨勬柊杩涚▼宸插姞杞?`[BUILD] resource-runtime-v59-npc-renderer-init`銆俆yphoea body 鍦ㄥ涓洿鎺ユ瀯閫犲疄渚嬩笂
璁板綍 `resource mesh replaced ... applied=true`锛岀揣闅忓叾鍚庣殑 `[MOD-RENDERER-INIT]` 鏄庣‘缁欏嚭鍚?Renderer锛?
cloth 01/02 涔熷湪鐩稿悓鍏ュ彛鎵ц `skip`銆傜敤鎴峰悓鏃剁‘璁?NPC 鐢婚潰宸茬粡搴旂敤銆傚洜姝ゆ湰鑺傜殑 NPC Mesh/skip 鍏ュ彛瀹屾垚
瀹炴満楠屾敹锛涜缁撹涓嶆墿澶у埌鏈０鏄庣殑 lod1/2/3锛屼篃涓嶆墿澶у埌涓嬫鍒楀嚭鐨勫師鐢?Physics 鐘舵€併€?

鏈妭鍚屾牱娌℃湁绉婚櫎鐢熶骇 Physics Mod 鎷掔粷鍒嗘敮锛屼篃娌℃湁杩炴帴鍘熺敓缁勪欢鍒涘缓銆佹墽琛屻€佸彇娑堛€丄nimator 鍐欏洖鎴栭楠奸噴鏀俱€?

### 20.10 v60 Mesh 鍛戒腑瀹炰緥鐨?Physics owner/Animator 瀹氬悜璋冩煡锛堝凡閮ㄧ讲锛屽緟瀹炴満閲囨牱锛?

閽堝鈥滃悓涓€ Mesh 鍛戒腑鍚庣殑闄勫姞 Physics 鏄惁鑳借鐩栦富瑙掋€丯PC 涓庤鑹?UI鈥濓紝鏈疆鍏堥噸鏂版牳瀵瑰綋鍓?
GameAssembly 鍏冩暟鎹€侼PC 璺緞宸茬粡瀛樺湪鏄庣‘瀵硅薄鍏崇郴锛屼笉闇€瑕佺户缁敱 Renderer 鐖堕摼鐚滄祴鎵€鏈夎€咃細

- `Beyond.NPC.Avatar.NPCAvatarManager._BuildBeyondCloth` 鐨勫弬鏁板悓鏃跺寘鍚?
  `NPCAvatarMeshAssetsSO`銆乣UnityEngine.Animator`銆乣FNPCAvatarGOReference&` 涓庢ā鍨?
  `GameObject`锛涜繖鏄父鎴忚嚜韬负 NPC 寤虹珛 BeyondCloth 鐨勭簿纭瀯寤鸿竟鐣屻€?
- `NPCAvatar.avatarGoRef` 鏄唴宓屽€肩被鍨嬶紱鍏朵腑淇濆瓨 `animator`銆乣go` 涓?
  `List<BeyondBoneCloth> boneCloths`銆傚叾鎴愬憳鍋忕Щ蹇呴』鎸?IL2CPP 闈炶绠卞€肩被鍨嬭В閲婏紝涓嶈兘鎶?
  `avatarGoRef` 瀛楁鏈韩褰撴垚鎵樼瀵硅薄鎸囬拡銆?
- `NPCAvatar.StartNPC(NPCCrowdEntityComponent)` 鍙妸宸叉瀯寤烘ā鍨嬬粦瀹氬埌绋冲畾 Avatar owner锛?
  `NPCAvatarManager.ReleaseAvatar(NPCCrowdEntityComponent)` 涓?
  `NPCCrowdEntityComponent.OnRelease()` 鎻愪緵 NPC 姹犲寲閲婃斁杈圭晫銆俙NPCCrowdEntityComponent.avatar`
  鑳芥妸閲婃斁鍙傛暟鏄犲皠鍥炲悓涓€ `NPCAvatar`銆?

鎹鏂板鐙珛娴嬭瘯鎺㈤拡锛堢幇浣嶄簬 `eiem_npc_model_owner.h`锛屽綋鏃跺悕涓?`eiem_physics_owner_probe.h`锛夈€傚畠闅忚祫婧?Hook 鑷姩瀹夎锛屾病鏈?Dump/ImGui/
鐑敭鍏ュ彛锛屽彧鍦ㄧ幇鏈?Render 瑙勫垯宸茬粡鍛戒腑 Mesh 鍚庤瀵燂細娉ㄥ唽妯″瀷涓殑 Animator 涓庣幇鏈?
BeyondBoneCloth 鏁伴噺銆丯PC 鏋勫缓鏃舵父鎴忓疄闄呬紶鍏ョ殑 Animator/妯″瀷鏍广€丼tartNPC owner锛屼互鍙婂搴旈噴鏀惧叆鍙ｃ€?
鎺㈤拡鍏堣皟鐢ㄦ父鎴忓師 `_BuildBeyondCloth`/`StartNPC`锛屽啀璇诲彇杩斿洖鍚庣殑鍏宠仈锛涗笉璋冪敤 AddComponent銆?
BuildAndRun銆丄nimator binding銆丏ispose 鎴?Destroy锛屼篃涓嶆敹闆?鎵ц Physics intent銆?

瀹屾暣 `build.bat` 宸查€氳繃锛沗python -m unittest discover -s tests` 鍙戠幇 80 椤癸紝鍏朵腑 35 椤规墽琛岄€氳繃銆?
45 椤瑰洜褰撳墠鐜鏉′欢璺宠繃銆傚彟涓€娆″畾鍚戣皟鐢ㄨ鐩?42 椤癸紝鍏朵腑 37 椤归€氳繃銆? 椤硅烦杩囥€備袱娆￠泦鍚堥噸鍙狅紝
涓嶇浉鍔犺鏁般€倂60 `bin/eiem.dll` 涓?6201344 瀛楄妭锛孲HA256锛?
`9DAB7BEE2ECFCACEDCB7FEF20C5B8530AE12480D7CB18560B6F14C92A476A4F5`銆傜‘璁ゆ父鎴忚繘绋嬮€€鍑哄悗锛?
v59 宸插浠藉埌
`E:\EIEM_Workspace\plugin-releases\before-v60-physics-owner-probe-20260908-021212`锛岄殢鍚庨儴缃?v60锛?
瀹夎鏂囦欢涓庢瀯寤轰骇鐗╁搱甯屼竴鑷淬€?

v60 闅忓悗鐢辨柊娓告垙杩涚▼瀹為檯鍔犺浇銆傚洓涓?NPC Hook 鍧囧畨瑁呮垚鍔燂紝鍏冩暟鎹В鏋愬緱鍒?
`avatarGoRef=0x120`銆乣component.avatar=0x140`銆乣goRef.animator=0x10`銆乣goRef.go=0x30` 鍜?
`goRef.boneCloths=0x48`銆? 娆?`StartNPC` 璁板綍鍧囨弧瓒?`component.avatar == avatar`銆?
`GetModelGo() == avatarGoRef.go`锛屼笖鍐呭祵 Animator 闈炵┖锛岀‘璁よ繖鏉?owner 鏄犲皠鍙洿鎺ヤ娇鐢ㄣ€?

涓诲満鏅腑鍛戒腑鐨?Wulfa 涓?Typhoea 妯″瀷鍚勬灇涓惧埌 1 涓?Animator 鍜?11 涓幇鏈?BeyondBoneCloth銆?
Typhoea 瑙掕壊 UI 鍦?`OnAwake`/棣栨鍙鏃舵灇涓惧埌 1 涓?Animator锛岄殢鍚庝竴娆?`SetVisible` 鏋氫妇鍒?
5 涓?Animator锛涗袱涓?UI 瀹炰緥鍧囦负 0 涓幇鏈?BeyondBoneCloth銆傛棩蹇楀悓鏃惰褰曞埌
`SetVisible(false)` 鍜?`UIModelLoader.UnloadModel`銆傚洜姝?UI 涓嶈兘澶嶇敤鍦烘櫙妯″瀷宸叉湁鐨?Cloth锛屼笖涓嶈兘鍦?
5 涓?Animator 涓换鍙栦竴涓紱蹇呴』鎸夊懡涓?Renderer 鐨勫疄闄呯鍏堝叧绯婚€夊嚭瀵瑰簲 Animator銆?

鑷姩鐢熷懡鍛ㄦ湡鏂囦欢璁板綍 2510 涓垚瀵逛簨浠躲€?255 娆¤皟鐢紝`dropped=0`銆乣inFlightCalls=0`锛?
`AddAnimatorTransform=902`銆乣AddTeamAnimatorData=50`銆乣ClothProcess.Init=60`銆?
`StartRuntimeBuild=47`銆乣RemoveMonitoringProcess=50`銆乣CompleteMasterJob=146`銆傝繖璇佹槑娓告垙鍘熸湁 Cloth
璺緞浼氳繘鍏?TeamManager 鐨?Animator 鏁版嵁鍏ュ彛锛涜璁板綍娌℃湁 EIEM 鑷缓缁勪欢锛屼篃娌℃湁 Animator binding
icall 浜嬩欢锛屼笉鑳芥嵁姝ゅ０绉版柊澧為楠煎凡琚帴绾炽€?

v60 鐨?`_BuildBeyondCloth` wrapper 琚疄鏈哄惁瀹氾細瀹冩寜瀹炰緥鏂规硶澶氬０鏄庝簡涓€涓?`self`锛屽鑷村敮涓€涓€鏉℃棩蹇楃殑
鍙傛暟鏁翠綋閿欎綅銆傚 GameAssembly 涓鏂规硶鍏ュ彛鐨勫弽姹囩紪鏄剧ず瀹冩槸闈欐€佹柟娉曪紝鐪熷疄鍘熺敓鍙傛暟渚濇涓?
`meshConfig, animator, goRef&, model, MethodInfo*`銆傚洜姝?v60 鐨?Build 鏃ュ織涓嶄綔涓烘纭?ABI 璇佹嵁锛?
杩愯鏃ュ織鍜岀敓鍛藉懆鏈?JSON 宸插綊妗ｅ埌 `bin/diagnostics/v60-physics-owner-probe/`銆?

### 20.11 v61 淇 NPC 鏋勫缓 ABI 涓?Renderer/Animator 鍏宠仈锛堝凡瀹炴満楠岃瘉锛?

v61 灏?`_BuildBeyondCloth` detour 鏀逛负涓婅堪 5 鍙傛暟闈欐€佸嚱鏁板竷灞€銆俙StartNPC` 鐜板湪鍗充娇姝ゅ墠娌℃湁鍙戠敓
`_BuildBeyondCloth`锛屼篃浼氱敤宸茬粡瀹炴祴涓€鑷寸殑 `GetModelGo()`銆佸唴宓?Animator 鍜?owner 寤虹珛 NPC 璁板綍锛?
鍐嶆妸姝ゅ墠鍛戒腑鐨?Renderer 涓庢ā鍨嬫牴鍏宠仈銆傝繖鏍蜂笉浼氬啀鎶娾€滆 NPC 娌℃湁 BuildCloth 璋冪敤鈥濊鍐欐垚
鈥滆 Renderer 涓嶆槸 NPC鈥濄€?

鍚屼竴鎺㈤拡杩樹細鍦ㄦ櫘閫氬満鏅ā鍨嬪拰瑙掕壊 UI 妯″瀷涓灇涓?Animator锛屽苟娌垮懡涓?Renderer 鐨?Transform 鐖堕摼璁＄畻
姣忎釜 Animator 鏄惁涓虹鍏堬紝璁板綍绁栧厛鏁般€佹渶杩?Animator 鍜屽眰绾ц窛绂汇€傝繖涓粨鏋滅敤浜庡洖绛斾富瑙掋€丯PC 涓?UI
鍒嗗埆搴旀妸鏂板 Physics 鎺ュ埌鍝釜 Animator锛涘畠涓嶉璁?UI 鐨勭涓€涓?Animator 灏辨槸姝ｇ‘瀵硅薄銆?

鏈疆鍚屾椂淇妯″瀷閲婃斁瑙傚療锛氳皟鐢ㄦ柟宸茬粡缁欏嚭 owner 鏃跺彧鎸?owner 绉婚櫎璁板綍锛屽彧鏈夋病鏈?owner 鏃舵墠鎸夋ā鍨嬫牴
绉婚櫎锛岄伩鍏嶅悓涓€妯″瀷鐢?`PrefabProxy` 涓?`CharUIModel` 鍙岄噸鐧昏鏃朵竴娆￠噴鏀捐鍒犱袱椤广€傛柊澧炴帰閽堝绾﹀叡 9 椤?
閫氳繃锛涘畬鏁?`build.bat` 鏋勫缓閫氳繃锛涙櫘閫氭祴璇曞彂鐜?83 椤癸紝鍏朵腑 38 椤规墽琛岄€氳繃銆?5 椤瑰洜褰撳墠鐜鏉′欢璺宠繃銆?
v61 `bin/eiem.dll` 涓?6203904 瀛楄妭锛孲HA256锛?
`DC6C1BE763C613B981CDFBAE7DDB3F8785DACCD2205512AE6CDA0191006280CE`銆?
纭鐩稿叧娓告垙杩涚▼閫€鍑哄悗锛屽凡灏?v60 澶囦唤鍒?
`E:\EIEM_Workspace\plugin-releases\before-v61-physics-owner-correlation-20260908-024409` 骞堕儴缃?v61锛?
娓告垙鐩綍 DLL 涓庢瀯寤轰骇鐗╁搱甯屼竴鑷淬€?

PID 23656 宸插疄闄呭姞杞?v61锛屽洓涓?Hook 鍏ㄩ儴瀹夎鎴愬姛銆傛纭?ABI 涓嬭褰曞埌涓€娆?`_BuildBeyondCloth`锛?
`refAnimator == animator`銆乣refModel == model`锛岀‘璁や慨姝ｅ悗鐨?5 鍙傛暟杞彂鍜屽唴宓屽€肩被鍨嬭鍙栧潎姝ｇ‘銆?
璇ュ疄渚嬪湪鍘熻皟鐢ㄨ繑鍥炲悗鐨?`boneCloths=0`銆?

鏃ュ織鍏辫褰?9 娆?`StartNPC`銆傚叾涓袱涓?Typhoea 鐩爣 NPC 鍚勮嚜鎶?3 涓懡涓?Renderer 鍏宠仈鍒板敮涓€鐨勬ā鍨嬫牴銆?
`NPCAvatar`銆乣NPCCrowdEntityComponent` 鍜?`avatarGoRef.animator`锛屽叡 6 鏉?`npc-render-match`锛涗袱鑰呴兘鏄?
`buildSeen=0`銆乣embeddedCloths=0`銆傝繖璇佹槑鐩爣 NPC 涓嶄細渚濊禆 `_BuildBeyondCloth` 寤虹珛鍘熸湁 Cloth锛屼絾
`StartNPC` 宸叉彁渚涢檮鍔?Physics 鎵€闇€鐨勭ǔ瀹氭ā鍨嬪拰 Animator 韬唤銆備袱涓洰鏍囬殢鍚庨兘璁板綍鍒板畬鏁村祵濂楅噴鏀鹃『搴忥細
`NPCCrowdEntityComponent.OnRelease enter 鈫?NPCAvatarManager.ReleaseAvatar enter/return 鈫?OnRelease return`銆?

Typhoea UI 鐨勪笁涓懡涓?Renderer 鍦?`OnAwake` 鏃堕兘浣嶄簬鍚屼竴涓?Animator 涓嬶紝璺濈鍧囦负 3锛涚◢鍚庣殑
`SetVisible` 铏界劧鑳戒粠鏁翠釜妯″瀷鏋氫妇鍑?5 涓?Animator锛屼絾姣忎釜鍛戒腑 Renderer 鐨?Animator 绁栧厛鏁颁粛涓?1锛?
鏈€杩?Animator 濮嬬粓鏄?`OnAwake` 宸插嚭鐜扮殑绗竴涓?Animator銆俇I 妯″瀷涓幇鏈?BeyondBoneCloth 濮嬬粓涓?0銆?
`SetVisible(false)` 鍜?`UIModelLoader.UnloadModel` 鍧囧凡璁板綍锛涘悓涓€妯″瀷鐨?`CharUIModel` 涓?`PrefabProxy`
涓や唤 owner 璁板綍鍒嗗埆閲婃斁锛屾病鏈夊啀鍥犳ā鍨嬪湴鍧€鐩稿悓鑰岃涓€娆¤鍒犮€?

鏅€氬満鏅ā鍨嬬殑鍛戒腑 Renderer 鍚屾牱鍚勬湁涓斾粎鏈変竴涓?Animator 绁栧厛锛岃窛绂讳负 3锛汿yphoea 妯″瀷鍚屾椂宸叉湁
11 涓父鎴忓師鐢?BeyondBoneCloth銆傜敱姝ゅ彲鎶婁笁绫诲疄渚嬬粺涓€涓衡€滄寜 Mesh 鍛戒腑 Renderer锛屽悜涓婇€夋嫨鍞竴鏈€杩?
Animator锛屽湪妯″瀷瀹炰緥涓婃寔鏈?Physics鈥濓紝浣?UI 蹇呴』鎸夋ā鍨嬪湴鍧€鍘婚噸锛屽苟浠ラ殣钘忓拰鍗歌浇鍒嗗埆椹卞姩鍋滅敤涓庨攢姣侊紱
NPC 鍒欑敱 `StartNPC` 寤虹珛 owner銆佸湪涓婅堪宓屽閲婃斁鍏ュ彛寮€濮嬫敞閿€銆?

瀹炴椂鏃ュ織蹇収宸蹭繚瀛樹负
`bin/diagnostics/v61-physics-owner-correlation/runtime-live-20260908-024929.log`銆傛甯搁€€鍑哄悗鐨勬渶缁堟棩蹇楀彟瀛樹负鍚岀洰褰?
`runtime.log`锛氬寘鍚?171 鏉?v61 璁板綍銆?7 鏉℃ā鍨?Renderer 鍏崇郴銆?1 娆?NPC Start銆? 娆?NPC Build銆?
12 鏉＄洰鏍?NPC Renderer 鍏崇郴銆? 鏉?UI owner 閲婃斁璁板綍锛屼互鍙婂洓涓洰鏍?NPC 鐨?16 鏉℃垚瀵归噴鏀捐褰曘€?

鏈€缁堣嚜鍔ㄧ敓鍛藉懆鏈熻褰曚负 `physics_runtime.json`锛?502 涓簨浠躲€?751 娆¤皟鐢紝鍏ㄩ儴鏉ヨ嚜 Unity 绾跨▼ 20672锛?
`dropped=0`銆乣inFlightCalls=0`銆傝皟鐢ㄦ暟涓?`AddAnimatorTransform=1537`銆?
`CompleteMasterJob=591`銆乣RemoveMonitoringProcess=130`銆乣MarkAnimatorTransformDirty=104`銆?
`ClothProcess.Init=101`銆乣AddTeamAnimatorData=81`銆乣StartRuntimeBuild=70`銆?
`Dispose/DisposeInternal=49/49`銆乣ClearTeamAnimatorData=39`銆?9 涓笉鍚?ClothProcess 閮芥寜
`Dispose enter 鈫?DisposeInternal enter 鈫?RemoveMonitoringProcess enter/return 鈫?DisposeInternal return 鈫?Dispose return`
鍚屾宓屽銆備簲涓?Animator binding 鍏ュ彛鍧囪В鏋愬埌 UnityPlayer 鍙墽琛屽湴鍧€锛屼絾鏈璋冪敤鏁颁粛涓?0銆?
蹇収鏋氫妇鍒?143 涓父鎴忕幇鏈夌粍浠讹紱涓婅堪璁板綍浠嶅彧鎻忚堪娓告垙鍘熷璞★紝涓嶅惈 EIEM 鑷缓缁勪欢銆?

v61 浠嶆槸鑷姩鍚姩鐨勮瀵熷瀷娴嬭瘯鏋勫缓銆傜敓浜?Physics Mod 鎷掔粷鍒嗘敮銆佸師鐢熺粍浠跺垱寤恒€丼electionData 杞崲銆?
妯℃嫙鍚仠銆佸彇娑堜笌楠ㄩ閲婃斁鍧囨湭杩炴帴鎴栨敼鍙樸€傛湰鑺傜幇鍦ㄧ‘璁ょ殑鏄疄渚?owner 涓?Animator 閫夋嫨瑙勫垯锛屼互鍙婃父鎴?
宸叉湁鐨勫缓绔?閲婃斁杈圭晫锛涘畠涓嶄唬琛?EIEM Physics 宸茬粡鍦ㄦ父鎴忎腑鍒涘缓鎴栬繍琛屻€?

### 20.12 v62 鍘熺敓宸ュ巶闈欐€佺粨璁轰笌涓€娆℃€ф瀯寤烘帰閽堬紙宸查儴缃诧紝寰呭疄鏈猴級

閫€鍑?v61 鍚庣户缁鏍稿綋鍓?`GameAssembly.dll` 涓庡厓鏁版嵁锛岃瘉鎹繚瀛樺湪
`bin/diagnostics/v62-physics-factory-static/`銆傚綋鍓嶆父鎴忔枃浠?SHA256 涓?
`C24495E51B406F03B03890C4788EE618AE022C991405BE5D5B8B787CB775AE89`銆傛湰杞彇寰椾互涓嬮潤鎬佺粨璁猴細

- `BeyondBoneCloth` 鐨勫叧閿叆鍙ｄ负 `DisableAutoBuild`銆乣BuildAndRun`銆乣set_SerializeData` 涓?
  `GetSerializeData2`锛涙瀯閫犲嚱鏁颁細鍒涘缓 `serializeData`銆乣serializeData2` 鍜?`process`銆?
- `BuildAndRun` 鍏堢姝㈣嚜鍔ㄦ瀯寤猴紝鍐嶆墽琛屽垵濮嬪寲銆侭oneCloth 鍦?SelectionData 涓虹┖銆佹棤鏁堟垨鏈爣璁颁负
  鐢ㄦ埛缂栬緫鏃惰皟鐢?`GenerateBoneClothSelection`锛岄殢鍚庤繘鍏ヨ繍琛屾椂鏋勫缓銆?
- `GenerateBoneClothSelection` 浣跨敤 `boneClothSetupData` 鐨勭湡瀹?Transform/鐖剁储寮曠敓鎴愪綅缃笌榛樿 Move
  灞炴€э紝鍐嶆寜 `rootBones` 鐨勫疄渚?ID 鏄犲皠鍥哄畾鐐广€傚洜姝?v1 鏂伴閾炬棤闇€鎶婁綔鑰呰妭鐐归『搴忕洿鎺ュ啓鎴?
  SelectionData锛涗綔鑰?`FIXED` 鏍瑰啓鍏?`rootBones`锛屼綔鑰?`IGNORE` 鑺傜偣鍐欏叆
  `ignoreFromRootBones`锛屽叾浣欒妭鐐圭敱鍘熺敓鎷撴墤鐢熸垚銆?
- Typhoea 瑙ｅ寘鏍锋湰鐨?11 涓粍浠跺潎涓?`clothType=1`銆乣connectionMode=0`銆傞暱鍙戞牱鏈腑 24 涓?
  `ignoreFromRootBones` 寮曠敤涓?24 涓棤鏁堥€夋嫨鐐逛竴鑷达紝鏀寔涓婅堪 IGNORE 鏄犲皠銆倂1 鐩墠鍙帴鍙楀崟涓€鍥哄畾鏍癸紱
  闈炴牴 FIXED 涓嶄細琚倓鎮勯檷绾ф垚 Move銆?
- `DisableAutoBuild` 璁剧疆 `ClothProcess` 鐨勭鐢ㄨ嚜鍔ㄦ瀯寤虹姸鎬侊紱`AutoBuild` 浼氭鏌ヨ鐘舵€併€傛甯歌繍琛屾ā寮忎笅
  `Start` 鎵ц Init/AutoBuild锛屽洜姝ゅ姩鎬?AddComponent 鍙互鍦ㄥ悓涓€ Unity 绾跨▼璋冪敤涓厛绂佺敤鑷姩鏋勫缓銆?
  缁戝畾閰嶇疆锛屽啀涓诲姩 `BuildAndRun`銆傝繖浠嶆槸闈欐€佹帹鏂紝闇€瑕佹湰杞帰閽堝疄鏈洪獙璇佽皟鐢ㄩ『搴忎笌缁撴灉銆?

[閰嶇疆鍑嗗](../../src/eiem_native_physics_config.h) 宸插鍔犲苟鍥炶 `clothType=1`銆?
`connectionMode=0`銆乣rootBones` 鍜?`ignoreFromRootBones`銆侻SVC 瀹夸富娴嬭瘯 11 椤归€氳繃锛岃鐩栧垪琛ㄦ垚鍛樸€佹灇涓俱€?
澶辫触浜嬪姟銆佽妭鐐瑰瓨娲讳笌鐖跺瓙鍏崇郴銆?

鍘嗗彶璁板綍涓殑涓€娆℃€у師鐢熷伐鍘傛帰閽堝凡浠庡綋鍓嶆簮鐮佺Щ闄ゃ€備互涓嬫楠や粎淇濈暀涓哄巻鍙茶瘉鎹紱瀹冪敱鐪熷疄
`S_actor_typhoea_body_01_lod0` Mesh 鍛戒腑鍜屽凡鐧昏妯″瀷鍏卞悓瑙﹀彂锛屽彧閫夋嫨 Renderer 鐨勫敮涓€鏈€杩?Animator锛?
骞惰姹傛ā鍨嬪凡鏈夋父鎴忓師鐢?Cloth锛屼互鎺掗櫎瑙掕壊 UI 棰勮銆傛帰閽堝缓绔嬬嫭绔嬬殑涓夎妭鐐规柊 Transform 閾惧拰鐙珛缁勪欢瀹夸富锛?

1. 瀹夸富璁句负 inactive锛?
2. `AddComponent(BeyondBoneCloth)`锛?
3. `DisableAutoBuild`锛?
4. 鍐欏叆骞跺洖璇讳袱涓厤缃璞★紱
5. 婵€娲诲涓诲苟璋冪敤 `BuildAndRun`锛?
6. 璁板綍 process銆乀eam銆丼electionData銆丅oneCloth setup銆佹柊鑺傜偣鏁伴噺鍙?interlocking Animator銆?

鎺㈤拡涓嶈皟鐢?Dispose銆丏estroy 鎴?Animator binding icall锛屽叏閮ㄦ柊瀵硅薄淇濈暀鍒拌繘绋嬮€€鍑恒€傝繖鏍锋湰杞彲浠ュ厛鍥炵瓟
鈥滄柊澧?Transform 鏄惁杩涘叆 BoneCloth setup銆乀eam 涓?Animator鈥濓紝鑰屼笉鎶婂皻鏈獙璇佺殑杩斿洖鍊煎綋浣滈噴鏀炬爡鏍忋€?
鑷姩鍏抽棴璺緞浼氬湪瀵煎嚭鍘熺敓璋冪敤璁板綍鍓嶅啓鏈€鍚庝竴娆℃帰閽堢姸鎬併€?

褰撳墠瀹夎鐨?`typhoeus/mod.ini` 鍙湁 Mesh銆丮aterial銆乀exture 涓庝笁涓?Render 娈碉紝娌℃湁 Skeleton銆丳hysics
璧勬簮鎴?`Render.physics`銆傛墍浠?v62 瀹為獙浣跨敤鐪熷疄 Mesh 鍛戒腑閫夋嫨瀹炰緥锛屼絾浣跨敤鏄庣‘鏍囪鐨勮瘖鏂笁鑺傜偣閾撅紱
瀹冧笉鏄綔鑰呰祫婧愬姞杞介獙鏀讹紝涔熶笉浼氫慨鏀硅 Mod銆傜敓浜у姞杞藉櫒瀵归潪绌?Physics 鍔ㄤ綔鐨勬暣 Mod 鎷掔粷鍒嗘敮淇濇寔涓嶅彉銆?

鏈湴瀹屾暣 `build.bat` 宸查€氳繃銆俙bin/eiem.dll` 涓?6238720 瀛楄妭锛孲HA256锛?
`5B6FD0EB40FDF82F3244A9E7B6061F1F5DEF457B68F8077A8D86C81B4D42FE43`銆傚伐鍘?owner 闈欐€佹鏌?
15 椤归€氳繃锛涙澶勮褰曠殑鏄湰鍦版瀯寤哄拰瀹夸富濂戠害锛屽皻鏃?v62 娓告垙鍔犺浇銆佺粍浠跺垱寤恒€丅uildAndRun 杩斿洖銆乀eam銆?
Animator 鎺ョ撼鎴栬繍琛屼腑閿€姣佽瘉鎹€?

2026-09-08 03:34:28 宸插湪娓告垙閫€鍑虹姸鎬侀儴缃茶 DLL锛屾父鎴忕洰褰曞壇鏈殑澶у皬涓?SHA256 鍧囦笌鏋勫缓浜х墿涓€鑷淬€?
琚浛鎹㈢殑 v61 DLL 鍙婂綋鏃剁殑閰嶇疆/鏃ュ織澶囦唤鍒?
`E:\EIEM_Workspace\plugin-releases\before-v62-physics-factory-probe-20260908-033428`锛涘叾 SHA256 涓?
`DC6C1BE763C613B981CDFBAE7DDB3F8785DACCD2205512AE6CDA0191006280CE`銆傞儴缃蹭笉绛変簬杩愯鏃堕獙璇侊紱
闇€瑕佹柊娓告垙杩涚▼鍛戒腑 Typhoea 鍦烘櫙妯″瀷骞舵甯搁€€鍑猴紝鎵嶈兘璇诲彇鏈妭鍒楀嚭鐨勭粍浠躲€侀厤缃€乀eam 涓?Animator 璇佹嵁銆?

### 20.13 v62 棣栨瀹炴満缁撴灉涓?v63 NPC 瑙﹀彂淇锛坴63 宸查儴缃诧紝寰呭疄鏈猴級

v62 鐨勬柊杩涚▼ PID 37984 宸叉甯稿鍑鸿嚜鍔ㄨ瘖鏂€傚師濮嬫棩蹇楀拰蹇収淇濆瓨鍦?
`bin/diagnostics/v62-physics-factory-runtime-37984/`銆侱LL 鍔犺浇鏍囪姝ｇ‘锛岃褰曞埌 3 涓?
`S_actor_typhoea_body_01_lod0` Renderer 鍛戒腑锛屼絾娌℃湁 `candidate`銆乣build-return` 鎴?`poll` 璁板綍锛?
鍥犳 v62 娌℃湁鍒涘缓缁勪欢锛屼篃娌℃湁璋冪敤 BuildAndRun銆?

鏈鍛戒腑鐨勫満鏅?Typhoea 鏄?NPC锛宍NPCAvatar.StartNPC` 宸茬簿纭叧鑱?model銆佸敮涓€ Animator 涓?Renderer锛?
浣嗗叾 `embeddedCloths=0`锛涘彟涓€涓懡涓槸瑙掕壊棰勮锛屼篃娌℃湁鍘熺敓 Cloth銆倂62 宸ュ巶瑕佹眰
`ownerKind=PrefabProxy && nativeCloths>0`锛屽師鎰忔槸鎺掗櫎棰勮锛屽嵈鍚屾椂鎺掗櫎浜嗙洰鏍?NPC銆傝繖鏄Е鍙戞潯浠堕敊璇紝
涓嶆槸鍘熺敓 AddComponent 鎴?BuildAndRun 澶辫触銆傝杩涚▼鐨勪富鍦烘櫙鍙帺瑙掕壊鏄?Wulfa锛屽洜姝ゅ叿鏈?11 涓幇鏈?Cloth
鐨勬ā鍨嬪苟涓嶆槸 Typhoea Mesh 鍛戒腑鐩爣銆?

鍚屼竴閫€鍑哄揩鐓ф灇涓惧埌 143 涓父鎴忕粍浠讹紝鍏朵腑 52 涓?`IsValid=true`锛?2 涓?`IsRunning=true`锛涜繖 42 涓兘鏈?
姝?Team ID銆丅one setup 涓?interlocking Animator銆傝皟鐢ㄨ褰曞寘鍚?52 娆?`ClothProcess.Init`銆?4 娆?
`StartRuntimeBuild`銆?2 娆?`AddTeamAnimatorData`銆?11 娆?`AddAnimatorTransform`銆?22 娆?
`CompleteMasterJob`锛屾棤涓㈠け浜嬩欢銆傝繖璇佹槑璇ヨ繘绋嬪瓨鍦ㄨ繍琛屼腑鐨勬父鎴忓師鐢熻瀛?澶村彂绛?Cloth 鍙婂叾 Team銆?
楠ㄩ摼鍜?Animator 鐧昏锛屼絾涓嶈瘉鏄?v62 鑷缓閾炬垚鍔熴€?

v63 淇濈暀鏅€?`PrefabProxy` 鐨?`nativeCloths>0` 鍒ゆ嵁锛屽苟浠庡凡缁忕‘璁ょ殑鐩爣
`NPCAvatar.StartNPC` owner 鍏宠仈鍗曠嫭璋冪敤宸ュ巶瑙傚療鍏ュ彛銆傚彧鏈?Mesh 鍛戒腑銆乵odel/Renderer 褰掑睘涓€鑷翠笖鍞竴鏈€杩?
Animator 鎴愮珛鏃舵墠浼氬缓绔嬩竴娆¤瘖鏂摼銆傚畬鏁存瀯寤洪€氳繃锛涜仛鐒︽鏌?38 椤归€氳繃锛孭hysics 濂椾欢杩愯 108 椤癸紝
106 椤归€氳繃锛? 椤瑰洜娌℃湁鎻愪緵 Blender/澶栭儴鏍锋湰鑰岃烦杩囷紝0 椤瑰け璐ャ€?

v63 DLL 涓?6238720 瀛楄妭锛孲HA256锛?
`A36547F701BEC97DDA31378FD79C0736BCC5BAECEEA154BF221EDE9CC6F4B977`銆?026-09-08 03:47:52 宸插湪娌℃湁
娓告垙杩涚▼鏃堕儴缃诧紝瀹夎鍓湰鍝堝笇涓€鑷淬€倂62 澶囦唤鍦?
`E:\EIEM_Workspace\plugin-releases\before-v63-physics-factory-npc-probe-20260908-034752`銆傜敓浜у姞杞藉櫒淇濇姢銆?
`Render.physics` 鍜?typhoeus Mod 鍧囨湭鏀瑰彉锛泇63 鐨勭粍浠跺垱寤恒€侀厤缃洖璇汇€乀eam 涓?Animator 鎺ョ撼浠嶅緟涓嬩竴娆?
瀹炴満璁板綍銆?

### 20.14 v63 閰嶇疆澶辫触涓?v64 鏋勯€犲垪琛ㄤ慨姝ｏ紙v64 宸查儴缃诧紝寰呭疄鏈猴級

v63 鏂拌繘绋?PID 27012 鐨勬棩蹇椾笌鑷姩蹇収淇濆瓨鍦?
`bin/diagnostics/v63-physics-factory-runtime-27012/`銆傚畠璁板綍鍒扮洰鏍?NPC candidate锛歊enderer銆乵odel銆佸敮涓€鏈€杩?
Animator 涓?`NPCAvatar.StartNPC` owner 鍧囨垚绔嬶紝`animatorAncestors=1`銆佹繁搴︿负 4銆傝繖纭 v63 鐨?NPC 瑙﹀彂
淇鏈夋晥銆傞殢鍚庢帰閽堝湪鍒涘缓缁勪欢鍓嶆姤鍛婏細

`failed stage=prepare error=Cannot bind detached rootBones list`

鍥犳 v63 娌℃湁璋冪敤 AddComponent 鎴?BuildAndRun銆傚け璐ュ彂鐢熷湪鎶婃柊寤?`List<Transform>` 鍐欏叆
`ClothSerializeData.rootBones` 鍚庣殑韬唤鍥炶銆傞噸鏂版牳瀵?`ClothSerializeData::.ctor` 鍙嶆眹缂栧彲瑙侊紝鏋勯€犲嚱鏁板凡缁?
鍒嗗埆鍒涘缓鍒楄〃骞朵繚瀛樺埌寮曠敤瀛楁锛涘杩愯鏃舵柊瀵硅薄涔熷簲娌跨敤杩欎釜鏋勯€犺涔夛紝涓嶉渶瑕佸彟寤哄垪琛ㄥ苟鏇挎崲寮曠敤銆?

v64 鍒犻櫎浜嗘牴/IGNORE 鍒楄〃瀛楁鏇挎崲锛氬畠璇诲彇鏋勯€犲嚱鏁颁骇鐢熺殑涓や釜鍒楄〃锛岀‘璁ら潪绌恒€佸疄闄呮硾鍨嬬被鍨嬫纭€佸郊姝?
鐙珛涓斿垵濮嬩负绌猴紝鍐嶅～鍏?root 鍜?IGNORE Transform锛涘～鍏ュ悗鍐嶆鏍稿瀛楁浠嶆寚鍚戠浉鍚屽垪琛ㄣ€傚涓讳篃鏀逛负鐪熷疄
妯℃嫙璇ユ瀯閫犺涓猴紝閬垮厤缁х画鐢ㄢ€滄瀯閫犲悗鍒楄〃涓虹┖鎸囬拡鈥濈殑閿欒妯″瀷鎺╃洊杩愯鏃跺樊寮傘€?

v64 瀹屾暣鏋勫缓閫氳繃銆傝仛鐒︽祴璇?39 椤归€氳繃锛汸hysics 濂椾欢杩愯 109 椤癸紝107 椤归€氳繃锛? 椤瑰彲閫?Blender/澶栭儴
鏍锋湰闆嗘垚娴嬭瘯璺宠繃锛? 椤瑰け璐ャ€侱LL 涓?6239744 瀛楄妭锛孲HA256锛?
`0CA50D307958924C79389C9BB8563FF8B3CB6B1C12949C521007BC4603014844`銆?026-09-08 03:57:18 宸插湪娌℃湁娓告垙
杩涚▼鏃堕儴缃诧紝瀹夎鍓湰鍝堝笇涓€鑷达紱v63 澶囦唤鍦?
`E:\EIEM_Workspace\plugin-releases\before-v64-physics-constructor-lists-20260908-035718`銆傜敓浜у姞杞藉櫒淇濇姢銆?
`Render.physics` 鍜?typhoeus Mod 浠嶆湭鏀瑰彉锛寁64 鐨勯厤缃€氳繃銆佺粍浠跺垱寤恒€乀eam 涓?Animator 鎺ョ撼绛夊緟涓嬩竴
杩涚▼瀹炶瘉銆?

### 20.15 v64 瀹炴満杈圭晫涓?v65 缁勪欢鑷甫 Data2锛坴65 宸查儴缃诧紝寰呭疄鏈猴級

v64 鏂拌繘绋?PID 33724 鐨勬棩蹇楀拰閫€鍑哄揩鐓т繚瀛樺湪
`bin/diagnostics/v64-physics-factory-runtime-33724/`銆傜洰鏍?NPC銆佸敮涓€鏈€杩?Animator銆侀厤缃?`Prepare` 鍜?
`BeyondBoneCloth` 鐨?`AddComponent` 鍧囧凡閫氳繃锛涚粍浠跺湴鍧€涓?`0x0000000FAE24B000`锛屽叾鏋勯€犱骇鐢熺殑
`ClothProcess` 鍦板潃涓?`0x000000100367EA80`銆傚け璐ョ偣鏄?`config-readback`锛屽洜姝?v64 娌℃湁璋冪敤
`BuildAndRun`銆?6 娆″悗缁疆璇腑缁勪欢淇濇寔瀛樻椿锛屼絾 `isBuild=false`銆乣valid=false`銆乣running=false`銆?
`team=0`銆丄nimator 涓虹┖涓旀病鏈?Selection/Setup銆傝繖浜涚粨鏋滃彧璇佹槑閰嶇疆鍑嗗鍜岀粍浠跺垱寤烘垚绔嬨€?

闈欐€佸弽姹囩紪鍐嶆纭锛歚set_SerializeData` 鍐欏叆缁勪欢鍋忕Щ `0x98` 骞舵墽琛屽啓灞忛殰锛宍get_SerializeData`
璇诲彇璇ュ亸绉伙紝`GetSerializeData2` 鍒欒鍙栧亸绉?`0xa0`銆倂64 鍚屾椂鐢ㄦ寮?setter 鍐欏叆 Data锛屽苟鐢ㄩ€氱敤瀛楁
鍐欏叆鏇挎崲 Data2锛涚粨鍚堝け璐ヤ綅缃紝Data2 鏇挎崲娌℃湁褰㈡垚 getter 杩斿洖韬唤鏄綋鍓嶆帹鏂紝灏氫笉鑳戒綔涓鸿繍琛屾椂浜嬪疄銆?
鍘熺敓缁勪欢鏋勯€犲嚱鏁版湰鏉ュ氨鍒涘缓鑷繁鐨?`ClothSerializeData2`锛岃€?v1 鐨?SelectionData 搴旂敱娓告垙鏍规嵁楠ㄩ摼
鐢熸垚锛屽洜姝?v65 涓嶅啀鏇挎崲瀹冿細浣滆€呭弬鏁颁粛閫氳繃 `set_SerializeData` 娉ㄥ叆锛涙帰閽堣鍙栥€佹牎楠屻€佹寔鏈夊苟杞
缁勪欢鏋勯€犱骇鐢熺殑 Data2銆傚け璐ユ棩蹇椾細鍒嗗埆璁板綍棰勬湡/瀹為檯 Data銆乨etached Data2 鍜?component Data2 鍦板潃銆?

v65 鑱氱劍娴嬭瘯 40 椤瑰叏閮ㄩ€氳繃锛汸hysics 濂椾欢杩愯 110 椤癸紝108 椤归€氳繃銆? 椤瑰彲閫?Blender/澶栭儴鏍锋湰
闆嗘垚娴嬭瘯璺宠繃銆? 椤瑰け璐ワ紱瀹屾暣 DLL 鏋勫缓閫氳繃銆侱LL 涓?6240256 瀛楄妭锛孲HA256锛?
`43582FEDF97B0D1BA29EA116ADE92254823BEE3059A95D45C9152CF0E54873FB`銆?026-09-08 04:09:38 鍦ㄦ父鎴?
杩涚▼閫€鍑哄悗閮ㄧ讲锛屽畨瑁呭壇鏈搱甯屼竴鑷达紱v64 澶囦唤浣嶄簬
`E:\EIEM_Workspace\plugin-releases\before-v65-physics-component-data2-20260908-040937`锛岄潤鎬侀獙璇佽褰曚綅浜?
`bin/diagnostics/v65-physics-component-data2/validation.json`銆傜敓浜у姞杞藉櫒 guard銆乣Render.physics` 鍜?
typhoeus Mod 鍧囨湭鏀瑰彉锛涚粍浠?Data2 韬唤銆乣BuildAndRun` 杩斿洖銆乀eam/Selection/Animator 鎺ョ撼浠嶇瓑寰?
涓嬩竴杩涚▼鐨勫疄鏈烘棩蹇椼€?

### 20.16 v65 瀹炴満寤虹珛鏈€灏?BoneCloth Team锛圥ID 15584锛屽凡褰掓。锛?

v65 杩涚▼ PID 15584 鐨勫疄鏃惰瘉鎹繚瀛樺湪
`bin/diagnostics/v65-physics-factory-runtime-15584/`銆傜洰鏍?Renderer 鍦?`NPCAvatar.StartNPC` 涓嬪懡涓紝
妯″瀷鍙湁涓€涓鍏?Animator锛涘師妯″瀷娌℃湁鍐呯疆 Cloth銆傛帰閽堥€氳繃姝ｅ紡 setter 缁戝畾
`SerializeData=0x000000100B53E6C0`锛岀粍浠舵瀯閫犵殑
`Data2=0x000000100BBD9EA0` 涓庨厤缃崏绋夸腑鏈娇鐢ㄧ殑 detached Data2 鍦板潃涓嶅悓锛岄厤缃洖璇婚€氳繃銆?

`BuildAndRun` 杩斿洖 1銆傜揣闅忚繑鍥炵殑绗竴娆¤疆璇㈣褰曟瀯寤轰粛鍦ㄨ繘琛岋細`isBuild=1`銆乣valid=1`銆?
`running=0`銆乣team=0`锛涙父鎴忓凡缁忕敓鎴?3 鐐?Selection锛坄userEdit=1`锛夈€? 涓?skin bone 鍜?4 涓?setup
Transform锛屽叾涓?3 涓氨鏄帰閽堟柊澧炶妭鐐广€備笅涓€娆?NPC 浜嬩欢鏃舵瀯寤哄凡缁忓畬鎴愶細`isBuild=0`銆乣valid=1`銆?
`running=1`銆乣team=39`锛宍interlockingAnimator` 绛変簬棰勬湡鏈€杩?Animator銆傛鍚?14 娆?settled 杞淇濇寔
鐩稿悓鐘舵€併€?

杩欒瘉鏄庝簡褰撳墠娓告垙鐗堟湰涓渶灏忔柊澧?BoneCloth 鐨勫垱寤洪摼锛氬湪鐩爣妯″瀷/Animator 涓嬪缓绔嬭繛缁?Transform锛?
鏋勯€犲苟濉啓 `ClothSerializeData`锛屾妸缁勪欢鑷韩鐨?`ClothSerializeData2` 鐣欑粰娓告垙鐢熸垚 Selection锛岄殢鍚庢縺娲?
瀹夸富骞惰皟鐢?`BuildAndRun`锛涘紓姝ユ瀯寤哄畬鎴愬悗缁勪欢杩涘叆鏈夋晥杩愯 Team锛屽苟鎺ュ叆鐩爣 Animator銆傚畠灏氭湭璇佹槑
楠ㄩ Transform 瀹為檯閫愬抚浣嶇Щ銆丮esh 钂欑毊浜х敓鍙鍙樺舰銆佺鎾炰綋瀹炰緥鍖栨垨鐢熶骇鐢熷懡鍛ㄦ湡娉ㄩ攢銆?

### 20.17 v65 涓撳睘 trace 搴忓垪涓?v66 鍚堝苟楠岃瘉锛坴66 宸查儴缃诧紝寰呭疄鏈猴級

PID 15584 閫€鍑哄悗鑷姩瀵煎嚭鐨?2230 涓簨浠跺叏閮ㄦ垚瀵癸紝`dropped=0`銆乣inFlightCalls=0`銆傚叾涓叡鏈?
1115 娆¤皟鐢細`Init=54`銆乣RemoveMonitoringProcess=43`銆乣StartRuntimeBuild=35`銆?
`CompleteMasterJob=124`銆乣AddTeamAnimatorData=43`銆乣AddAnimatorTransform=815`锛屼互鍙婂叏杩涚▼鍞竴涓€娆?
`BeyondBoneCloth.BuildAndRun`銆傝鍞竴璋冪敤鐨勫璞″氨鏄?EIEM 缁勪欢 `0x000000100B1D3960`銆?

鎸夌粍浠躲€乸rocess `0x000000100BBDAA80` 鍜?team 39 杩囨护鍚庯紝涓撳睘搴忓垪涓猴細

1. `BuildAndRun enter 鈫?Init 鈫?StartRuntimeBuild(true) 鈫?BuildAndRun(true)`锛?
2. 31 ms 鍚庝簩娆?`Init 鈫?RemoveMonitoringProcess`锛?
3. 鍐嶈繃 78 ms锛宍CompleteMasterJob 鈫?AddTeamAnimatorData(team 39)`锛?
4. 鍚屼竴 tick 涓?team 39 璋冪敤鍥涙 `AddAnimatorTransform`锛屼笌 setup 涓笁涓閾捐妭鐐瑰姞缁勪欢瀹夸富鍏卞洓涓?
   Transform 涓€鑷达紱
5. 鍏抽棴蹇収浠嶈褰曠粍浠跺瓨娲汇€佹湁鏁堛€佽繍琛屼腑锛宼eam 39銆佹瀯寤烘垚鍔熶笖鏃犻攢姣佹爣璁帮紝楠ㄩ摼寮曠敤鍜岄暱搴﹀潎鏈彉鍖栥€?

娓告垙绐楀彛鍏抽棴鏃舵帰閽堝厛鍋滄骞跺鍑?trace锛岄殢鍚庢墠璁板綍鐩爣 NPC 鐨?`OnRelease/ReleaseAvatar`锛屾墍浠ヨ繖娆?
蹇収娌℃湁鎹曡幏鐩爣 process 鐨?Dispose锛涗笉鑳芥嵁姝ゅ垽鏂噴鏀捐涓恒€傚畬鏁村師濮嬪揩鐓с€佽繃婊や簨浠跺拰缁撴瀯鍖栫粨璁轰綅浜?
`bin/diagnostics/v65-physics-factory-runtime-15584/physics_runtime.json` 涓?`trace-summary.json`銆?

涓轰簡鐢ㄤ笅涓€娆¤繘绋嬪悓鏃跺洖绛旇繍鍔ㄤ笌閲婃斁闂锛寁66 澧炲姞涓ら」鑷姩璇婃柇锛氱獥鍙ｄ富绾跨▼姣?500 ms 璇诲彇涓変釜鏂板
鑺傜偣鐨勫眬閮?涓栫晫浣嶇疆鍜屾棆杞紝鏈€澶?32 娆★紝骞朵笌 `BuildAndRun` 鍓嶅熀绾挎瘮杈冿紱绮剧‘鐩爣 NPC 鐨?
`NPCCrowdEntityComponent.OnRelease` 鍜?`NPCAvatarManager.ReleaseAvatar` 鍧囧湪鍘熷嚱鏁拌繘鍏ャ€佽繑鍥炰袱渚ц疆璇?
缁勪欢銆乸rocess銆乀eam 鍜屽Э鎬併€傚畠娌℃湁鏂板 Hook锛屼篃娌℃湁鎺ュ叆 Dump UI銆?

v66 鑱氱劍娴嬭瘯 42 椤瑰叏閮ㄩ€氳繃锛汸hysics 濂椾欢杩愯 112 椤癸紝110 椤归€氳繃銆? 椤瑰彲閫?Blender/澶栭儴鏍锋湰娴嬭瘯
璺宠繃銆? 椤瑰け璐ワ紱瀹屾暣 DLL 鏋勫缓閫氳繃銆侱LL 涓?6243840 瀛楄妭锛孲HA256锛?
`2D36DB8CCF6659C1AEA940CA211739EBF296C82FE02B29C3B10020A1B820E62E`銆?026-09-08 04:30:46 鍦ㄦ父鎴?
閫€鍑哄悗閮ㄧ讲涓斿畨瑁呭搱甯屼竴鑷达紱v65 澶囦唤浣嶄簬
`E:\EIEM_Workspace\plugin-releases\before-v66-physics-motion-release-20260908-043046`銆傜敓浜у姞杞藉櫒 guard銆?
`Render.physics` 鍜?typhoeus Mod 浠嶆湭鏀瑰彉銆?

### 20.18 v66 瀹炴満纭鏂板鑺傜偣鍙戠敓灞€閮ㄨ繍鍔紙PID 32784锛?

v66 鍐嶆瀹屾垚鐩稿悓鐨勬渶灏忓伐鍘傞摼骞惰繘鍏?team 39銆俙BuildAndRun` 杩斿洖鏃朵笁涓妭鐐圭浉瀵规瀯寤哄墠鍩虹嚎鍧囨湭鍙樺寲锛?
寮傛鏋勫缓瀹屾垚鍚庣殑杩炵画閲囨牱涓紝涓変釜鑺傜偣鍧囧彲璇诲彇锛屽浐瀹氭牴淇濇寔涓嶅彉锛屼袱涓?MOVE 鑺傜偣鍚屾椂鍑虹幇灞€閮ㄤ綅缃拰
涓栫晫浣嶇疆鍙樺寲锛氭渶澶у眬閮ㄤ綅缃樊骞虫柟涓?`6.29432179e-07`锛屾渶澶т笘鐣屼綅缃樊骞虫柟涓?
`1.2407545e-06`锛屾棆杞樊涓?0銆傚洜涓哄垽鎹寘鍚浉瀵圭埗鑺傜偣鐨勫眬閮ㄤ綅缃紝缁撹涓嶄緷璧栬鑹叉暣浣撲笘鐣屼綅绉汇€?

杩欒瘉鏄庢父鎴忚繍琛屼腑鐨?BoneCloth Team 宸茬粡瀵逛袱涓柊澧炲彲鍔ㄧ墿鐞嗚妭鐐逛骇鐢?Transform 鍐欏洖銆傚綋鍓嶄笁鐐圭珫鐩撮摼鍦?
闈欐鍚庢敹鏁涘埌鐩稿悓浣嶇疆锛屽洜姝よ繖浜涢噰鏍疯繕娌℃湁鎻忚堪杩炵画鎽嗗姩杞ㄨ抗锛屼篃娌℃湁鎶婅妭鐐硅鍏ュ彲瑙?Mesh 鐨勯楠艰皟鑹叉澘銆?
瀹炴椂璇佹嵁浣嶄簬 `bin/diagnostics/v66-physics-motion-release-runtime-32784/`銆?

### 20.19 v66 鍦烘櫙鍗歌浇瑙﹀彂鍘熺敓閲婃斁锛圥ID 32784锛?

鍒囨崲鍦烘櫙鏃讹紝鐩爣妯″瀷 `0000000FCD7D4520` 鐨?NPC owner 杩涘叆骞惰繑鍥?`ReleaseAvatar/OnRelease`銆傚畬鏁?trace
鏄剧ず鐩爣宸ュ巶缁勪欢 `0000001022A34E10` 瀵瑰簲鐨?process `0000001022EB1A80` 鍦ㄥ垱寤哄悗杩愯绾?166 绉掞紝骞朵簬
tick 65441546 鍙戠敓浠ヤ笅宓屽璋冪敤锛?

1. `ClothProcess.Dispose` enter锛?
2. `ClothProcess.DisposeInternal` enter锛?
3. `TeamManager.ClearTeamAnimatorData` enter/return锛岃繑鍥?team 39锛?
4. `TeamManager.RemoveMonitoringProcess` enter/return锛屽弬鏁颁负鐩爣 process锛?
5. `DisposeInternal` return锛?
6. `Dispose` return銆?

閲婃斁涔嬪悗 trace 鍙堣褰?6388 涓簨浠讹紝鐩爣 component/process 鍦板潃娌℃湁鍐嶆鍑虹幇锛涢€€鍑烘椂鏋氫妇鐨?401 涓?
BoneCloth 缁勪欢涓篃娌℃湁鐩爣 component 鎴?process銆傚洜姝ゅ彲浠ョ‘璁わ紝鎶婃柊澧?BoneCloth 缁勪欢鍙婃柊澧?Transform
鎸傚湪鍛戒腑妯″瀷涓嬮潰鏃讹紝妯″瀷鐨勮嚜鐒跺満鏅嵏杞戒細杩涘叆娓告垙鑷韩鐨勭粍浠堕噴鏀鹃摼锛屽苟娉ㄩ攢鐩爣 Team 鐨?Animator 鏁版嵁鍜?
鐩戞帶 process銆?

team 鏁板瓧涓嶈兘浣滀负闀挎湡瀹炰緥鏍囪瘑銆傜洰鏍?team 39 娓呴櫎鍚庝粎 4719 ms锛屽彟涓€涓?process
`0000001023046A80` 鍗宠幏寰?team 39锛涢€€鍑哄揩鐓т腑鐨?team 39 灞炰簬 `MC_Hair` 缁勪欢
`0000001022A34960`銆傚悗缁疄渚嬭〃蹇呴』浣跨敤 component/process 鍦板潃骞剁淮鎶や唬娆★紝涓嶈兘璺ㄩ噴鏀炬湡浠呭嚟 team 缂栧彿鍏宠仈銆?

杩欐缁撴灉璇佹槑鐨勬槸鑷劧妯″瀷鍗歌浇涓嬬殑缁勪欢娉ㄩ攢锛屼笉鎶?`DisposeInternal` 鎴?`Dispose` 鐨勮繑鍥炲€兼彁鍗囦负浠绘剰寮傛
浠诲姟銆丣ob 鎴?Animator 鍐欏洖鍧囧凡闈欐鐨勯€氱敤鏍呮爮銆傜敓浜у疄鐜板簲璁╂柊澧炶妭鐐硅窡闅忔ā鍨嬪眰绾ч攢姣侊紝骞跺湪缁勪欢/妯″瀷閿€姣?
杈圭晫绉婚櫎鑷繁鐨勫疄渚嬭褰曪紝涓嶅簲鍦ㄨ瀵熷埌鍗曟 `Dispose` 杩斿洖鍚庣嫭绔嬮攢姣佷粛鍙兘琚師鐢熺粍浠跺紩鐢ㄧ殑鑺傜偣銆傜粨鏋勫寲
璇佹嵁瑙?`trace-summary.json`锛涜娆?trace 涓?8772 涓簨浠躲€? dropped銆佸鍑烘椂 0 涓?hook 璋冪敤浠嶅湪鏍堝唴銆?

鑷虫锛屾渶灏?v1 BoneCloth 鐨勭粍浠跺垱寤恒€丏ata 閰嶇疆銆佸紓姝?Team 鏋勫缓銆丄nimator 鎺ョ撼銆佺墿鐞嗗眬閮ㄤ綅缃啓鍥烇紝浠ュ強
鑷劧鍦烘櫙鍗歌浇涓嬬殑鍘熺敓娉ㄩ攢鍧囧凡鏈夊悓涓€鏉″疄鏈洪摼璺瘉鎹€傚皻鏈獙璇佺殑鏄柊澧炶妭鐐硅繘鍏ユ浛鎹?Mesh 鐨勯楠艰皟鑹叉澘鍚庤兘鍚?
浜х敓鍙鍙樺舰銆佷綔鑰呭弬鏁伴€愰」瀵硅繍鍔ㄧ殑褰卞搷銆佺鎾炰綋鏋勯€狅紝浠ュ強 `Render.physics` 鐨勭敓浜у疄渚嬪寲涓庡瀹炰緥鐘舵€佺鐞嗐€?

### 20.20 v67 鎺ュ叆 `Render.physics` 鐢熶骇瀹炰緥閫傞厤鍣紙宸叉瀯寤洪儴缃诧紝寰呭疄鏈猴級

v67 涓嶅啀鎵ц v62-v66 鐨?Typhoea 纭紪鐮佷竴娆℃€у伐鍘傘€傛棫宸ュ巶澶存枃浠朵繚鐣欎负鍘嗗彶瀹為獙婧愮爜锛屼絾宸蹭粠 DLL include
閾剧Щ闄ゃ€傜敓浜у姞杞藉櫒鐜板湪鍙戝竷鍖呭惈鏈夋晥 Physics 鍔ㄤ綔鐨?Mod锛汸hysics 鎰忓浘浠嶇敱鏅€?Render 绗竴鍛戒腑瑙勫垯浜х敓锛?
骞舵柊澧炰繚瀛樺疄闄呭懡涓殑 Renderer銆傞€傞厤鍣ㄦ寜鈥滄ā鍨嬪疄渚?+ Mod + 涓嶅彲鍙?Physics 璧勬簮蹇収鈥濆幓閲嶏紝鍥犳鐩稿悓 Mesh
韬唤鐨勪富瑙掋€丯PC 鍜岃鑹?UI 鏄笉鍚屽疄渚嬶紝鍚屼竴妯″瀷鐨勫涓?LOD/Renderer 鍛戒腑涓嶄細閲嶅鍒涘缓鍚屼竴浠?Physics銆?

褰撳墠 v1 鏃犵鎾炰綋鎵ц璺緞濡備笅锛?

1. 鍦ㄥ懡涓ā鍨嬪唴楠岃瘉 Renderer 浠嶅瓨娲伙紝骞惰姹傚畠鍙湁涓€涓渶杩?Animator 绁栧厛锛?
2. 閫氳繃 Physics 寮曠敤鐨?Skeleton 鏂囨。瑙ｆ瀽璇ユā鍨嬭嚜宸辩殑 Transform銆傝嫢 Render 鐨?Mesh 鍚屾椂浣跨敤鐩稿悓 Skeleton
   璧勬簮锛岄€氱敤 Skeleton 瀹炰緥閿細澶嶇敤鍚屼竴鎵规柊澧炶妭鐐癸紱
3. 鍏堝噯澶囨瀯閫犲嚱鏁拌嚜甯︾殑 root/IGNORE 鍒楄〃鍜屼簲涓爣閲忥紝鍐嶅垱寤烘ā鍨嬪瓙绾х殑闈炴縺娲?
   `EIEM_Physics_<generation>` 瀹夸富锛?
4. 瀵规瘡缁勬坊鍔犱竴涓?`BeyondBoneCloth`锛屾墽琛?`DisableAutoBuild`銆佸啓鍏?璇诲洖 SerializeData銆佷繚鐣欑粍浠惰嚜宸辩殑
   Data2锛屾縺娲诲涓诲悗璋冪敤 `BuildAndRun`锛?
5. 寮傛杞瑕佹眰 process 鍚屾椂婊¤冻 valid銆乺unning銆佹湁鏁?team锛屼笖 `interlockingAnimator` 涓烘楠?1 鐨?
   Animator锛屼箣鍚庢墠璁板綍 ready锛?
6. 瑙勫垯绉婚櫎銆佽祫婧愭崲浠ｆ垨妯″瀷 owner 閲婃斁鏃跺彧閿€姣?EIEM 鑷繁鐨勫涓汇€傝嫢鍥炶皟涓嶅湪 Unity 绾跨▼锛岄攢姣佽姹傜暀鍒?
   涓嬩竴娆＄獥鍙ｄ富绾跨▼杞锛涘疄渚嬬户缁繚鐣欓厤缃拰 Skeleton锛岀洿鍒板涓诲強鍏ㄩ儴缁勪欢鐨?Unity native 鐘舵€佸潎涓?0銆?
   璇ュ疄鐜颁笉璋冪敤 `DisposeInternal`锛屼篃涓嶆妸瀹冪殑杩斿洖褰撲綔瀹屾垚鏍呮爮銆?

涓哄尯鍒嗏€淧hysics 寤虹珛浜嗏€濅笌鈥滃懡涓簡 Mesh 瀹為檯浣跨敤鐨勯楠尖€濓紝v67 鍦ㄦ瀯寤哄墠璁板綍 Renderer palette 妲芥暟銆佺粍鑺傜偣鏁?
鍜屾寚閽堢浉绛夌殑 `paletteHits`銆俙typhoeus` 瀹炴満澶瑰叿浣跨敤 body Mesh 鍘熸湁鐨勫乏椋熸寚
`Bip001_L_Finger0 鈫?Finger01 鈫?Finger02`锛屼笁鑰呭湪瀵煎嚭 Mesh 鐨?76 妲介楠艰〃鍐咃紱Physics 鍙傛暟涓?v66 宸茶繍琛?
閰嶇疆涓€鑷达紝涓?gravity 10銆乻tabilization 0.1銆乫alloff 0銆乥lend 1銆乤nimation pose 1锛屾棤纰版挒浣撱€傚す鍏风敓鎴愬櫒浣嶄簬
`tools/diagnostics/make_v67_typhoea_physics_fixture.py`锛岀敓鎴?781 瀛楄妭 Physics 鍜?1774 瀛楄妭 source-only
Skeleton锛涚敓浜?C++ 璇诲彇鍣ㄥ凡璇诲彇璇ュ噯纭緭鍑恒€?

鐩稿叧 Physics/Skeleton/Skin 濂椾欢鍏辫繍琛?118 椤癸紝鍏ㄩ儴閫氳繃锛屽叾涓?4 椤瑰洜褰撳墠鍛戒护琛岀幆澧冩病鏈?Blender 鑰岃烦杩囷紱
瀹屾暣 `build.bat` 閫氳繃銆侱LL SHA256 涓?
`D4237678B72483E532DC33CA81379A9DBAA123488CC8225118A61AB97FA2D715`锛?251008 瀛楄妭锛夛紝宸插湪娓告垙閫€鍑哄悗瀹夎锛屽苟缁?
`typhoeus` 鐨?body Render 澧炲姞 `physics=PhysicsTyphoeaLeftIndexV67`銆傞儴缃插墠澶囦唤浣嶄簬
`E:\EIEM_Workspace\plugin-releases\before-v67-physics-resource-adapter-20260908-052503`銆?

鏈妭灏氭棤 v67 娓告垙杩涚▼缁撴灉銆傚緟楠岃瘉椤规槸锛歁od 瑙ｆ瀽銆佹瘡绉嶅疄闄呮ā鍨嬫秷璐硅€呯殑 build/ready銆?
`selected=3/paletteHits=3`銆佸乏椋熸寚 Transform/鍙钂欑毊鍝嶅簲锛屼互鍙婅鍒欐挙閿€鍜屽満鏅嵏杞藉悗鐨?retire/鍘熺敓娉ㄩ攢銆?
纰版挒浣撳拰 Physics v2 浠嶇敱閰嶇疆鍑嗗鍣ㄦ槑纭嫆缁濓紝涓嶅睘浜庢湰娆￠儴缃层€?

### 20.21 v67 `Render.physics` 瀹炴満缁撴灉涓?v68 绮剧‘浣滆€呰妭鐐硅竟鐣?

v67 杩涚▼ PID 4292 棣栨璧伴€氱敓浜ц祫婧愰摼銆傚姞杞藉櫒瑙ｆ瀽浜嗗寘鍚?Physics 鍔ㄤ綔鐨?`typhoeus` Mod锛涘悓涓€鏉?body Mesh
瑙勫垯鍏堝悗涓轰袱涓?`chr_0034_typhoea_uimodel(Clone)` 鍜屼竴涓?
`chr_0034_typhoea_postmodel(Clone)#160` 寤虹珛鐙珛瀹炰緥銆傚墠涓よ€呯敱 `CharUIModelMono.OnAwake` 鍛戒腑锛岀涓夎€呭厛鐢?
`PrefabInstantiateProxy.OnCompleted` 鍛戒腑锛岄殢鍚庝篃鍦?`BaseModelViewPart.OnLoadFinish` 涓嬭璇嗗埆锛涙渶鍚庝竴涓疄渚嬪洜姝?
鍙兘绉颁负 `PrefabProxy + BaseModelPart` 娑堣垂鑰咃紝鏈疆璁板綍涓嶈冻浠ユ妸瀹冭繘涓€姝ヨ瀹氫负涓昏鎴?NPC銆?

涓夋鏋勫缓鍧囪褰?`palette=76 selected=3 paletteHits=3`锛宍BuildAndRun` 鍜屽唴閮?`StartRuntimeBuild` 鍧囪繑鍥?true锛?
骞跺垎鍒繘鍏?team 43銆?3銆?9锛涗笁涓?process 鐨?`interlockingAnimator` 閮界瓑浜庡懡涓?Renderer 鐨勫敮涓€鏈€杩?Animator銆?
姣忎釜 team 娉ㄥ唽浜嗕簲涓?Animator Transform銆傜涓変釜瀹炰緥鍦ㄨ嚜鍔?trace 瀵煎嚭鏃朵粛涓?valid/running锛屽師鐢?
`boneClothSetupData` 鏄庣‘缁欏嚭鍥涗釜 skin bones锛氫綔鑰呭０鏄庣殑
`Bip001_L_Finger0 鈫?Finger01 鈫?Finger02`锛屼互鍙婂疄闄呮ā鍨嬩腑鏈嚭鐜板湪浣滆€呮枃浠跺唴鐨勫瓙鑺傜偣
`Bip001_L_Finger0Nub`锛涚浜斾釜 Transform 鏄?`EIEM_Physics_3` 瀹夸富銆?

杩欎唤璇佹嵁淇浜?v67 鐨勪竴涓槧灏勫亣璁俱€倂1 浣滆€呯害瀹氭槑纭鏈€夋嫨鐨勫瓙楠ㄩ涓嶈繘鍏ラ摼锛屼絾鍙妸浣滆€?FIXED 鑺傜偣鍐欏叆
`rootBones` 浼氳鍘熺敓 BoneCloth 閫掑綊灞曞紑璇ヨ妭鐐逛笅鐨勫畬鏁村疄闄呭眰绾э紝鍥犳 `paletteHits=3/3` 鍙兘璇佹槑涓変釜浣滆€呰妭鐐?
閮藉睘浜?Mesh palette锛屼笉鑳借瘉鏄庡師鐢?Selection 鍙惈杩欎笁涓妭鐐广€倂68 鍦ㄥ噯澶囬厤缃椂閬嶅巻姣忎釜闈?IGNORE 浣滆€呰妭鐐?
鐨勭洿鎺ュ瓙鑺傜偣锛涘嚒涓嶅湪鍚岀粍浣滆€呰妭鐐归泦鍚堝唴鐨勫瓙鑺傜偣锛屽潎浣滀负杈圭晫鏍硅拷鍔犲埌 `ignoreFromRootBones`銆傛樉寮?IGNORE
鑺傜偣浠嶆寜鍘熻涔夊啓鍏ワ紝涓斿畠鑷韩宸叉帓闄ゆ暣涓瓙鏍戯紝涓嶉噸澶嶆灇涓惧瓙绾с€傚鏈疆椋熸寚澶瑰叿锛岄鏈熻嚜鍔ㄨ竟鐣?IGNORE 涓?
`Finger0Nub` 涓€椤癸紱杩欎釜棰勬湡灏氶渶涓嬩竴娓告垙杩涚▼璇诲彇 Selection 灞炴€ч獙璇併€?

涓や釜 UI 瀹炰緥閮藉湪 `UIModelLoader.UnloadModel` 瑙﹀彂閫傞厤鍣ㄩ攢姣佽嚜鏈夊涓汇€傚搴?process 闅忓悗鍒嗗埆鎵ц瀹屾暣鐨?
`Dispose 鈫?DisposeInternal 鈫?ClearTeamAnimatorData(team 43) 鈫?RemoveMonitoringProcess 鈫?return`锛屼笖鍦ㄥ悇鑷?
Dispose 杩斿洖涔嬪悗鐩村埌 trace 缁撴潫鍧囨病鏈夊啀娆″紩鐢ㄨ process锛涚粍浠朵篃涓嶅啀浠ュ師韬唤鍑虹幇鍦ㄦ渶缁堟灇涓句腑銆傜浜屼釜
EIEM 缁勪欢鐨勮８鍦板潃鍚庢潵琚?`MBC_Lizhiyan_Ear_Upper_01` 澶嶇敤锛屽叾 process 鍜?team 宸插彉涓哄彟涓€缁勫€硷紝杩欏啀娆″疄璇?
team 缂栧彿鍜岃８瀵硅薄鍦板潃閮戒笉鑳借法閫€浼戞湡浣滀负瀹炰緥韬唤銆?

绐楀彛姝ｅ父鍏抽棴鏃讹紝鑷姩 trace 鍏堝鍑猴紝涔嬪悗绗笁涓疄渚嬫墠鍦?`PrefabInstantiateProxy.Unload` 璁板綍 retire 璇锋眰锛?
鎵€浠ョ幇鏈?trace 鍙互璇佹槑瀹冨湪瀵煎嚭鐐逛粛杩愯锛屼笉鑳借瘉鏄庤娆″叧闂湡闂寸殑鏈€缁堝師鐢?Dispose锛涜繖涓嶆槸宕╂簝璁板綍銆?
瀹屾暣杩愯鏃ュ織銆?2934 涓浂涓㈠け/闆跺湪鏍堣皟鐢ㄤ簨浠剁殑 trace 鍜岀粨鏋勫寲缁撹淇濆瓨鍦?
`bin/diagnostics/v67-physics-resource-runtime-4292/`銆倂68 鍙﹀璁╄嚜鍔?trace 璇诲彇 managed
`SelectionData.attributes`锛屾寜鍘熺敓 `VertexAttribute.IsFixed/IsMove/IsInvalid` 鏂规硶缁熻锛屽苟浠呬负 EIEM 缁勪欢杈撳嚭
閫愮偣鐘舵€侊紱璇ュ姛鑳戒粛涓嶅湪 Dump UI 鎴栫敓浜т氦浜掑叆鍙ｅ唴銆?

v68 鐨?Native Physics 瀹夸富濂椾欢 75 椤瑰叏閮ㄩ€氳繃锛汸hysics銆丼keleton銆丼kin 鍚堝苟濂椾欢鍏辫繍琛?118 椤癸紝114 椤归€氳繃銆?
4 椤瑰洜褰撳墠鍛戒护琛岀幆澧冩病鏈?Blender/澶栭儴鏍锋湰鑰岃烦杩囷紝0 椤瑰け璐ワ紱瀹屾暣 `build.bat` 閫氳繃銆傛父鎴忛€€鍑虹姸鎬佷笅宸查儴缃?
6257664 瀛楄妭 DLL锛屾瀯寤轰笌瀹夎 SHA256 鍧囦负
`E92837AA6788F951E16E54EA452F6C4A1664004EFC5C642F68BCAE1A27AA5C28`锛泇67 澶囦唤浣嶄簬
`E:\EIEM_Workspace\plugin-releases\before-v68-physics-selection-boundary-20260908-133817`銆備互涓婃槸 v68 閮ㄧ讲鏃剁殑
寰呴獙璇佺姸鎬侊紱瀹為檯杩愯缁撴灉鍜?Selection 璇婃柇閿欒瑙佷笅涓€鑺傦紝涓嶈兘缁х画寮曠敤鏈鍘熼鏈熶綔涓哄疄鏈虹粨鏋溿€?

### 20.22 v68 绮剧‘鎷撴墤瀹炶瘉銆丼election 璇婃柇绾犻敊涓?v69

v68 杩涚▼ PID 22888 鍛戒腑浜嗕竴涓?`PrefabInstantiateProxy.OnCompleted` / `BaseModelPart` 娑堣垂鑰呭拰涓€涓?
`CharUIModelMono.OnAwake` 娑堣垂鑰呫€備袱鑰呭潎璁板綍 `palette=76 selected=3 paletteHits=3 boundaryIgnores=1`锛?
鍒嗗埆杩涘叆 team 17 鍜?team 48锛屼笖 process 鐨?Animator 涓庣洰鏍?Renderer 鐨勬渶杩?Animator 涓€鑷淬€?
鏈€缁堣嚜鍔ㄥ揩鐓т腑鐨勭敓浜х粍浠?`EIEM_Physics_1` 浠嶄负 valid/running锛涘叾鍘熺敓 `boneClothSetupData` 鍙湁涓変釜
skin bones锛歚Bip001_L_Finger0 鈫?Finger01 鈫?Finger02`锛岀粍浠跺涓讳綅浜庣鍥涗釜 Transform 妲姐€?
v67 澶氶€夌殑 `Finger0Nub` 宸蹭笉鍐嶅嚭鐜般€傝繖鐩存帴楠岃瘉浜嗚竟鐣?IGNORE 鑳芥妸鍘熺敓閫掑綊鎷撴墤鏀舵暃鍒颁綔鑰呰妭鐐归泦鍚堛€?

瑙掕壊 UI 瀹炰緥鍦?`UIModelLoader.UnloadModel` 鍚庡畬鏁磋褰?
`ClothProcess.Dispose 鈫?DisposeInternal 鈫?ClearTeamAnimatorData(team 48) 鈫?RemoveMonitoringProcess`锛?
闅忓悗閫傞厤鍣ㄨ褰?`retired`銆備富妯″瀷瀹炰緥鏄湪鑷姩蹇収瀵煎嚭鍚庢墠鏀跺埌 `PrefabInstantiateProxy.Unload`锛屾棩蹇楀彧璁板綍
retire 璇锋眰锛屾病鏈夊湪璇ュ揩鐓т腑璁板綍鏈€缁?`retired`锛屽洜姝や笉鎶婅繖娆′富妯″瀷鍏抽棴鍐欐垚瀹屾暣娉ㄩ攢瀹炶瘉銆?

v68 蹇収鏈€鍒濇妸鐢熶骇缁勪欢鐨勪笁涓?Selection 鐐归兘缁熻涓?invalid銆備氦鍙夋鏌ュ彂鐜板悓涓€璇婃柇涔熸妸鍏朵綑 148 涓父鎴忓師鐢?
缁勪欢鐨勫叏閮ㄧ偣閮界粺璁′负 invalid锛岄棶棰樻潵鑷瘖鏂櫒鏈韩锛歚System.Array.GetValue` 杩斿洖瑁呯鐨勫€肩被鍨?
`VertexAttribute`锛寁68 鍗寸洿鎺ユ妸瑁呯瀵硅薄浼犵粰瀹炰緥璋撹瘝锛涘璞″ご琚敊璇綋浣滅粨鏋勪綋 `this`銆傚悓鏂囦欢璇诲彇
`ResultCode` 鏃跺凡缁忎娇鐢ㄤ簡姝ｇ‘鐨?pinned box 鈫?unboxed value 娴佺▼銆傚涓诲洖褰掔幇宸叉敼涓烘ā鎷熺湡瀹炲璞″ご锛屾棫浠ｇ爜
绋冲畾澶辫触锛泇69 鍥哄畾姣忎釜瑁呯鍊煎苟鍦?unboxed 鍦板潃涓婅皟鐢?`IsFixed/IsMove/IsInvalid`锛岃鍥炲綊鎭㈠閫氳繃銆?
鎵€浠?v68 鐨勭簿纭楠兼嫇鎵戠粨鏋滄湁鏁堬紝鍏?Selection 灞炴€х粺璁℃棤鏁堬紝涓嶈兘鎹鍒ゆ柇鍙傛暟娌℃湁杩涘叆姹傝В鍣ㄣ€?

v68 瀹屾暣鏃ュ織銆佽嚜鍔ㄥ揩鐓у拰缁撴瀯鍖栫粨璁轰繚瀛樺湪
`bin/diagnostics/v68-physics-selection-runtime-22888/`銆倂69 鐨勫師鐢熺墿鐞嗘祴璇?75 椤广€乷wner 濂戠害 10 椤瑰潎閫氳繃锛?
瀹屾暣 `build.bat` 閫氳繃锛涙父鎴忛€€鍑虹姸鎬佷笅宸查儴缃?6257664 瀛楄妭 DLL锛屾瀯寤轰笌瀹夎 SHA256 鍧囦负
`C39752928198493E10AE0F3DDFD99A28FC677A01A472CC6A830C548A6FC7B2FA`銆倂68 瀹夎澶囦唤浣嶄簬
`E:\EIEM_Workspace\plugin-releases\before-v69-physics-selection-value-unbox-20260908-140521`銆?
v69 闅忓悗鍦?PID 10760 杩愯銆備竴涓?`BaseModelPart` 鍜屼竴涓鑹?UI 瀹炰緥鍧囧啀娆′互
`palette=76 selected=3 paletteHits=3 boundaryIgnores=1` 杩涘叆杩愯 Team锛屽垎鍒负 19 鍜?52銆傝嚜鍔ㄥ揩鐓т腑鐨?
`EIEM_Physics_1` 涓?valid/running锛屽師鐢?setup 浠嶇簿纭寘鍚笁涓綔鑰呰妭鐐癸紱Selection 鍙俊璇诲洖涓?
`count=3 fixed=1 move=2 invalid=0 other=0 unreadable=0`锛岄€愮偣椤哄簭涓?`fixed / move / move`銆?
杈圭晫澶栫殑 Nub 琚粠 setup 鍜?Selection 涓€璧锋帓闄わ紝鑰屼笉鏄繚鐣欎负涓€涓?invalid 鐐广€?

浣滀负璇婃柇浜ゅ弶鏍稿锛屾父鎴忓師鐢?`MBC_Typhoea_Hair_Back_Ponytail_Long` 鍚屾椂璇诲洖
`count=38 fixed=2 move=12 invalid=24`锛屼笌绗?8 鑺傜殑绂荤嚎鏍锋湰瀹屽叏涓€鑷达紱杩欒瘉鏄?v69 鐨勫€肩被鍨嬭鍙栦慨姝ｆ湁鏁堬紝
涔熻瘉鏄?v1 鐨?FIXED/MOVE 浣滆€呰鑹插凡閫氳繃 `rootBones + ignoreFromRootBones` 杩涘叆鍘熺敓 Selection 鐢熸垚杩囩▼銆?
瑙掕壊 UI 鐨?team 52 瀹屾暣鎵ц Dispose銆丆learTeamAnimatorData 鍜?RemoveMonitoringProcess 鍚庤閫傞厤鍣ㄧЩ闄ゃ€?
涓绘ā鍨嬩粛鍦ㄨ嚜鍔ㄥ揩鐓у鍑哄悗鎵嶈褰?retire 璇锋眰锛屾晠鏈€缁?Dispose 缁х画鍙噰鐢ㄦ鍓?UI/v66 鐨勫疄璇佽寖鍥淬€?
瀹屾暣 v69 鏃ュ織銆佸揩鐓у拰缁撴瀯鍖栫粨璁轰繚瀛樺湪 `bin/diagnostics/v69-physics-selection-runtime-10760/`銆?

褰撳墠瀹屾垚搴﹀簲鍒嗗眰琛ㄨ堪锛氬師鐢熷悗绔€佹渶灏?BoneCloth銆乀eam/Animator銆乀ransform 鍐欏洖銆佺簿纭綔鑰呮嫇鎵戙€?
FIXED/MOVE Selection 鐢熸垚鍜屼竴涓?
瑙掕壊 UI 鐨勫畬鏁撮€€褰规満鍒跺凡缁忓疄璇侊紝瓒充互缁х画鍋氫骇鍝佸疄鐜帮紱鍦?v70 鍙缁撴灉鍑虹幇鍓嶏紝瀹屾暣鍔熻兘浠嶇己灏戞柊澧?Skeleton 鑺傜偣椹卞姩鏇挎崲 Mesh 鐨?
鍙缁撴灉銆佷綔鑰呮爣閲忓弬鏁扮殑鍙楁帶 A/B 鍝嶅簲銆佺鎾炰綋杞崲涓庡搷搴斻€丳hysics v2 婧愬浘瀹炰緥鍖栵紝浠ュ強鏄庣‘鏍囪瘑涓?NPC 鐨?
鐢熶骇瀹炰緥銆備笉鑳芥妸 `BaseModelPart` 鑷姩绉颁负 NPC銆?

### 20.23 v70 鏂板楠ㄩ鍙钂欑毊澶瑰叿锛堣鑹?UI 宸插疄鏈洪獙璇侊級

涓嬩竴椤瑰疄楠屽彧楠岃瘉涓€涓棶棰橈細Skeleton 鏂板缓鐨?Transform 鑳藉惁鍚屾椂杩涘叆 BoneCloth 鍜屾浛鎹?Mesh 鐨勯楠兼暟缁勶紝杩涜€?
璁╁疄闄呴《鐐归殢鐗╃悊鍐欏洖鍙樺舰銆傚畠娌℃湁淇敼 DLL锛屼篃娌℃湁鍚?Dump 鎴栦富鍔熻兘 UI 澧炲姞鍏ュ彛锛涗粛鐢?v69 鐨勮嚜鍔ㄨ繍琛岃褰?
瑙傚療缁撴灉銆?

`tools/diagnostics/make_v70_typhoea_new_bone_fixture.py` 浠ュ綋鍓?Typhoea Mod 涓鸿緭鍏ワ紝鍦ㄧ嫭绔嬬洰褰曞鍒跺苟鑱斿姩鏀瑰啓
Mesh銆丼keleton銆丳hysics 涓?INI銆傚師 body Mesh 鏈?14664 涓《鐐瑰拰 76 涓楠兼Ы锛屾Ы 48 鏄?
`Bip001_L_Finger02`锛屽叡鏈?202 涓鏉冮噸褰卞搷銆傚す鍏峰湪璇ラ楠间笅杩藉姞
`EIEM_PhysicsTip`锛堝眬閮ㄤ綅缃?`(-0.015, 0, 0)`锛夛紝灏嗚繖 202 涓奖鍝嶄繚鎸佸師鏉冮噸鏀规寚鍚戞柊妲?76锛屽苟鎸?
`inverse(newLocal) @ Finger02Bindpose` 鐢熸垚鏂?bindpose銆傝繖鏍烽潤姝㈠Э鎬佷笉鍥犳敼缁戞湰韬亸绉伙紝鍚庣画鐢婚潰鍙樺寲鎵嶅彲褰掑洜
浜庢柊澧?Transform 鐨勫Э鎬併€?

瀵瑰簲 Skeleton v2 鍚?80 涓幇鏈夋簮鑺傜偣鍜屼竴涓?`source=false` 鏂拌妭鐐癸紝瑕嗙洊 Mesh 鐨勫叏閮ㄥ師楠ㄩ璺緞鍙婂繀瑕佺鍏堬紱
Physics v1 閾句负 `Finger0 FIXED 鈫?Finger01 MOVE 鈫?Finger02 MOVE 鈫?EIEM_PhysicsTip MOVE`銆傚悓涓€ Render 鍚屾椂澹版槑
`mesh=`銆乣skeleton=` 涓?`physics=`锛孭hysics 鍐呯殑鐩稿 Skeleton 璺緞鍜?INI Skeleton 璧勬簮鏈€缁堟寚鍚戝悓涓€涓枃浠躲€?

绂荤嚎缁撴灉涓?palette `76 鈫?77`銆乺emapped vertices/influences `202/202`銆佹柊 bindpose 鏈€澶ч噸缂栫爜璇樊
`1.4305114759416426e-08`锛屽叾浣?Mesh 瀛楁閫愰」涓€鑷淬€傛柊澧炵敓鎴愬櫒鍥炲綊 3 椤归€氳繃锛涚幇鏈夌敓浜?C++ Physics/Skeleton/
Mod 璇诲彇濂椾欢 14 椤归€氳繃锛屽疄闄呯敓鎴愮殑 v70 `mod.ini` 涔熺敱鍚屼竴 C++ 璇诲彇鍣ㄤ互 exit 0 鎺ュ彈銆傚す鍏峰拰鏈哄櫒鍙璁板綍浣嶄簬
`bin/diagnostics/v70-new-bone-visible-fixture/`銆?

娓告垙閫€鍑哄悗宸叉妸鍘?Mod 澶囦唤鍒?
`E:\EIEM_Workspace\plugin-releases\before-v70-visible-new-bone-20260908-142630`锛岄殢鍚庨儴缃插す鍏凤紱7 涓簮鏂囦欢涓?
瀹夎鏂囦欢閫愪竴鏍稿鏃犲搱甯屽樊寮傦紝瀹夎 Mesh SHA256 涓?
`E40B2C0B4A025B81DA08F8AB1BB78CDF0A65599DF329893F4E7559CD86868E4B`銆?

PID 7100 闅忓悗鍔犺浇 v69 DLL 鍜?v70 璧勬簮銆備富妯″瀷 generation 1 涓庤鑹?UI generation 2 鍧囪褰?
`Skeleton nodes=81 added=1`銆乣palette=77 selected=4 paletteHits=4 boundaryIgnores=1`锛屽垎鍒繘鍏?team 36 鍜?
team 52銆傝嚜鍔ㄥ揩鐓т腑 generation 1 鐨勫師鐢?setup 绮剧‘鍚?`Finger0 鈫?Finger01 鈫?Finger02 鈫?EIEM_Bone_1` 鍥涗釜
skin bones锛孲election 涓?`fixed/move/move/move`锛屾病鏈?invalid/unreadable 鐐广€傜浜斾釜 Transform 鏄粍浠跺涓汇€?
瑙掕壊 UI 鐨?team 52 鍦?`UIModelLoader.UnloadModel` 瀹屾暣鎵ц
`Dispose 鈫?ClearTeamAnimatorData 鈫?RemoveMonitoringProcess` 骞惰褰?`retired`锛涗富妯″瀷鍙湪蹇収瀵煎嚭鍚庤褰?
retire 璇锋眰锛屾晠鏈浠嶄笉鎶婂叾鏈€缁堥€€褰瑰啓鎴愬凡瑙傚療浜嬪疄銆?

鐢ㄦ埛鍦ㄨ鑹?UI 椤甸潰鏄庣‘瑙傚療鍒板乏鎵嬫媷鎸囧姩浣滆鏀瑰彉銆備綔鑰呴摼鐨勬父鎴忓悕绉版槸 `Bip001_L_Finger0/...`锛涚粨鍚堜笂杩?
鏂板鑺傜偣銆佸洓涓?palette 鍛戒腑銆佸師鐢熷啓鍥炲拰鏀圭粦鐨?202 涓《鐐癸紝杩欎竴缁撴灉楠岃瘉浜嗗畬鏁村彲瑙侀€氳矾锛?
**Skeleton 鏂板缓 Transform 鈫?BoneCloth/Animator 鍐欏洖 鈫?鏇挎崲 Mesh 钂欑毊鍙樺舰**銆傝繖涓嶆槸浠呭嚟鏃ュ織鎺ㄦ柇鐨勭敾闈㈢粨璁恒€?

鍚屼竴杩愯杩樼簿纭懡涓袱涓?Typhoea NPC 妯″瀷锛屽悇鏈変笁涓?Renderer 瀹屾垚 Mesh/Skeleton 搴旂敤锛屼笖
`StartNPC target=1`銆佸敮涓€ Animator 鍜岄噴鏀捐竟鐣屽潎鎴愮珛锛涗絾娌℃湁浠讳綍浠?NPC owner 涓?stage 鐨?`PHYSICS-PLAN`銆?
build 鎴?ready銆傚洜鑰屾湰娆?NPC 鐨勪笉纭畾涓嶆槸瑙嗚瑙傚療涓嶈冻锛氬姞杞戒腑鐨?v69 纭疄娌℃湁涓?NPC 鍒涘缓闄勫姞 Physics銆?
瀹屾暣鏃ュ織銆佸揩鐓у拰缁撴瀯鍖栫粨璁轰綅浜?`bin/diagnostics/v70-new-bone-visible-runtime-7100/`銆?

### 20.24 v70 灏嗙簿纭?NPC owner 鎺ュ叆鍏变韩 Physics 鎵ц鍣紙NPC 宸插疄鏈洪獙璇侊級

20.23 鐨?NPC 缂哄彛鏉ヨ嚜涓や釜宸插瓨鍦ㄤ絾鏈繛鎺ョ殑鍏ュ彛锛歚RendererInfo._Init` 宸插鍏蜂綋 Renderer 鎵ц Mesh/Skeleton锛?
鑰屾ā鍨嬬骇 Physics 鍙兘鍦ㄦā鍨嬫牴鍜?Animator 閮界ǔ瀹氬悗寤虹珛銆倂70 鍦ㄦ父鎴忓師 `NPCAvatar.StartNPC` 杩斿洖鍚庤鍙栧叾宸茬‘璁ょ殑
model/component 鍏崇郴锛屽苟浠?`EiemModelOwnerKind::NpcAvatar` 璋冪敤鐜版湁
`EiemRegisterAndApplyModelInstance`銆傝繖閲屼粛鐢辨櫘閫?Render 鐨?Mesh 韬唤瑙勫垯鍐冲畾鏄惁鍛戒腑锛涙病鏈?NPC 涓撶敤璧勬簮鏍煎紡銆?
瑙掕壊鍚嶅垽鏂垨绗簩濂楀師鐢熺粍浠跺伐鍘傘€?

瀵瑰簲閲婃斁鍦ㄦ父鎴忓師 `NPCAvatarManager.ReleaseAvatar` 涓?`NPCCrowdEntityComponent.OnRelease` 璋冪敤鍓嶏紝鍧囦互鍚屼竴
component owner 璋冪敤 `EiemForgetModelOwner`銆傚祵濂楅噸澶嶉€氱煡鐢卞凡鏈?owner 琛ㄥ箓绛夊鐞嗭紝鐗╃悊缁勪欢鍜?Skeleton 鑺傜偣浠嶇敱
鐜版湁妯″瀷瀹炰緥閫€褰硅矾寰勭鐞嗐€傛湰鏀瑰姩娌℃湁鎶?`DisposeInternal` 杩斿洖銆佽鏁版垨 trace 璋撹瘝褰撲綔寮傛瀹屾垚鏍呮爮銆?
閫傞厤鍣ㄧ幇浣嶄簬 `src/eiem_npc_model_owner.h`锛涜嚜鍔ㄨ瘖鏂户缁殢鍚姩杩愯骞跺啓鐙珛鐩綍锛屾病鏈夎繘鍏?Dump 鎴栦富鍔熻兘 UI銆?

NPC owner銆佽祫婧愩€丼keleton銆丼kin銆佺敓鍛藉懆鏈熷拰鍘熺敓鎺㈤拡瀹氬悜濂椾欢鍏?79 椤归€氳繃锛? 椤瑰け璐ワ紱瀹屾暣 `build.bat` 閫氳繃銆?
娴嬭瘯澶瑰叿鍚屾椂淇浜?MSVC 涓枃杈撳嚭鐨勮В鐮佹柟寮忥紝鍙奖鍝嶆祴璇曡繘绋嬫崟鑾烽敊璇枃鏈€傛父鎴忛€€鍑哄悗锛寁69 DLL 宸插浠藉埌
`E:\EIEM_Workspace\plugin-releases\before-v70-physics-npc-owner-20260908-151543`锛屽苟閮ㄧ讲 6257664 瀛楄妭 v70 DLL锛?
鏋勫缓涓庡畨瑁?SHA256 鍧囦负 `439251AFF50BFB44541A348F7F8381AF797575410F1128B4821845279F3B41CD`銆?
PID 5048 闅忓悗鍔犺浇涓婅堪 v70 DLL銆備袱涓簿纭?Typhoea NPC 鍒嗗埆鍦?`NPCAvatar.StartNPC` 浠?generation 2/3 寤虹珛
Physics锛氭瘡涓ā鍨嬮兘鏈変笁涓懡涓?Renderer銆佸敮涓€ owner/model/Animator 鍏崇郴锛宐inding 鍧囦负
`palette=77 selected=4 paletteHits=4 boundaryIgnores=1`銆俫eneration 2 鏄庣‘璁板綍 ready team 48锛沢eneration 3
鍦ㄥ揩鐓т腑涓?valid/running team 49銆備袱涓?team 鍧囨敞鍐屼簲涓?Animator Transform锛岃嚜鍔ㄥ揩鐓т腑鐨?setup 閮芥槸鍥涗釜
skin bones 鍔犱竴涓粍浠跺涓伙紝Selection 閮芥槸 `fixed/move/move/move`銆傜敤鎴峰悓鏃剁‘璁?NPC 鐢婚潰鐨勫乏鎵嬫媷鎸囩墿鐞嗗凡缁?
鐢熸晥锛屽洜姝?NPC 鐢熶骇鍒涘缓涓庡彲瑙佽挋鐨矾寰勫畬鎴愬疄鏈洪獙璇併€?

涓や釜 NPC 闅忓悗閮藉湪 `NPCCrowdEntityComponent.OnRelease` 娉ㄩ攢 owner 骞惰褰?retire 璇锋眰銆備笉杩囧叧闂椂鐨勮嚜鍔ㄥ揩鐓?
浠嶆灇涓惧埌杩欎袱涓?valid/running 缁勪欢锛屾棩蹇楁病鏈夊嚭鐜板搴?`retired`锛涜繖鍙兘璇佹槑 owner/retire 鍏ュ彛宸茶Е鍙戯紝涓嶈兘鎶?
鏈杩涚▼鍏抽棴褰撲綔寤惰繜 Destroy 鍜屽師鐢熶换鍔℃渶缁堝畬鎴愮殑璇佹嵁銆傝闄愬埗涓嶆帹缈诲凡缁忚瀵熷埌鐨?NPC 鍒涘缓銆乀eam 鎺ョ撼鍜?
鍙鏁堟灉銆傚畬鏁存棩蹇椼€佸揩鐓у拰缁撴瀯鍖栫粨璁轰綅浜?`bin/diagnostics/v70-physics-npc-runtime-5048/`銆?

鑷虫锛屸€滆兘鍚︽寜 Mesh 韬唤鍦ㄤ富妯″瀷銆佽鑹?UI 鍜?NPC 涓婃柊澧為楠煎苟鐢ㄦ父鎴忓師鐢?BoneCloth 椹卞姩鏇挎崲 Mesh鈥濈殑鏈哄埗
瀹為獙宸茬粡瀹屾垚銆備笅涓€椤圭嫭绔嬪疄楠屽簲楠岃瘉浜斾釜浣滆€呮爣閲忔槸鍚﹀疄闄呮敼鍙樻眰瑙ｇ粨鏋滐紱浼樺厛浠ュ悓涓€璧勬簮鐨?
`blendWeight=1 鈫?0` 鍋氱儹閲嶈浇 A/B锛屼繚鎸?Mesh銆丼keleton銆佽妭鐐归€夋嫨鍜屽叾浣欏弬鏁颁笉鍙樸€傞€氳繃鍚庡啀鎺ュ叆鐞冧綋纰版挒浣擄紝
閬垮厤鎶婂弬鏁版槧灏勪笌纰版挒杞崲鍚屾椂寮曞叆涓€涓疄楠屻€?

### 20.25 v71 `blendWeight` 鐑噸杞藉弻鍚?A/B 涓?NPC 鏈€缁堥€€褰?

PID 35576 鍏堜互鍘熻祫婧?`blendWeight=1` 寤虹珛涓绘ā鍨?generation 1/team 17 鍜?NPC generation 2/team 48銆?
瀹為獙浠庡悓涓€ Physics v1 鏂囨。鐢熸垚涓や釜 994 瀛楄妭鍙樹綋锛屽彧鏀瑰彉涓€涓?float锛欰 涓?`blendWeight=1`锛孊 涓?
`blendWeight=0`锛沝ocument identity銆丼keleton銆佺粍銆佽妭鐐广€佺鎾炰綋鍒楄〃銆乬ravity銆乻tabilization銆乫alloff 鍜?
animation pose ratio 鍧囦繚鎸佷竴鑷淬€侫/B SHA256 鍒嗗埆涓?
`B7B4A70F5995A8AA6B32275A1C83BBB7BDB00615B3FCE16B6C1311C4E0E654BA` 涓?
`9ECA7935FC14F523483C028E0D4C7B39D5CF45B1AF283E81654634B674DEF0FF`銆?

绗竴娆?F10 璇诲彇 B 鍚庯紝鏃?generation 1/2/3 鍏ㄩ儴璁板綍 `retired`锛屽啀寤虹珛闆舵潈閲嶇殑涓绘ā鍨?generation 4/team 54銆?
NPC generation 5/team 55 鍜岃鑹?UI generation 6/team 56銆傜敤鎴疯瀵熷埌宸︽墜鎷囨寚鎭㈠鍘熷姩鐢汇€侼PC generation 5
鑷劧鍗歌浇鏃朵緷娆¤褰?retire銆佹渶缁?`retired` 鍜?Skeleton 鑷湁鑺傜偣閫€浼戯紱UI generation 6 涔熷畬鏁撮€€浼戙€傝繖琛ラ綈浜?
20.24 鍏抽棴蹇収鏈鐩栫殑 NPC 寤惰繜閿€姣佺粨鏋溿€?

闅忓悗纾佺洏璧勬簮鎭㈠ A锛岀浜屾 F10 璁╂椿璺?generation 4/7 瀹屾暣閫€浼戝苟寤虹珛 generation 8/9锛涚敤鎴疯瀵熷埌鐗╃悊鍋忕Щ
鎸夐鏈熼噸鏂板嚭鐜般€傜敱姝ょ‘璁?`blendWeight` 浠?Physics v1 浣滆€呮枃浠剁粡杩?F10 璧勬簮蹇収銆侀厤缃瀯閫犮€佸師鐢熼噸寤虹洿鑷?
鏈€缁?Animator/Mesh 杈撳嚭鐨勫弻鍚戣涓烘湁鏁堛€傝瀹為獙娌℃湁璇佹槑鍏朵綑鍥涗釜鏍囬噺鐨勫畬鏁村崟浣嶆垨鏇茬嚎璇箟銆?
褰撳墠娓告垙鐩綍宸蹭繚鎸?A锛屽嵆 `blendWeight=1`銆傚彉浣撱€佹棩蹇楁憳褰曞拰缁撴瀯鍖栫粨璁轰綅浜?
`bin/diagnostics/v71-physics-parameter-ab/`銆?

鍘熻鍒掔殑鐞冨舰纰版挒浣撳疄楠屽凡鎸夌敤鎴峰畨鎺掓殏鍋滐紱鍚庣画鍏堝畬鍠勭湡瀹炶В鍖呯墿鐞嗘暟鎹湪 Blender 涓殑瀵煎叆銆侀厤缃拰澧為噺瀵煎嚭锛?
寰呮湁瀹為檯鐗╃悊楠ㄩ銆丮esh銆佺鎾炰綋鍜屽弬鏁扮粍鍚堝悗鍐嶅洖鍒?DLL 纰版挒楠岃瘉銆?

### 20.26 Blender 0.13.0 鍘熺敓閰嶇疆鏉ユ簮涓?v1 澧為噺缁勫悎瀵煎嚭

鏈疆娌℃湁淇敼鎴栭儴缃?DLL銆傝В鍖呭櫒鐜版湁 `components.json`銆侀€愮粍浠跺師濮嬪瓧鑺傘€乀ypeTree schema銆佽В鐮佹暟鎹€佸畬鏁?
Transform 琛ㄥ拰 PPtr 寮曠敤鍥剧户缁綔涓?v2 鏉ユ簮锛汢lender 涓嶅啀鎶?SelectionData 鐐瑰簭鍙风寽鎴愰楠奸『搴忋€傛瘡涓師鐢?
BoneCloth 鐨勭墿鐞?Transform 鐢辩湡瀹?`rootBones` 鍚庝唬鍑忓幓 `ignoreFromRootBones` 鍒嗘敮寰楀埌锛岄€夋嫨鐐瑰強鍏跺睘鎬т粛淇濇寔
鍘熸暟缁勯厤瀵癸紝浜岃€呭湪闈㈡澘鍒嗗埆璁℃暟銆?

鍘熺敓纰版挒浣撴寜婧愮被鍨嬫仮澶嶅彲瑙嗗寲锛氱悆浣跨敤 center/radius锛岃兌鍥婁娇鐢?direction銆乺everseDirection銆?
alignedOnCenter銆乺adiusSeparation銆佷袱绔崐寰勫拰 length锛屽钩闈㈡樉绀哄眬閮?+Y 娉曠嚎骞舵爣鏄庢棤闄愯涔夈€傚嚑浣曡В閲婁緷鎹?
MagicaCloth 鍏叡 API 鍚堝悓锛屼粛涓嶆妸瀹冨啓鎴愮粓鏈湴娓告垙涓殑纰版挒鍝嶅簲瀹炶瘉銆俆yphoea 鏍锋湰鐨?27 涓粍浠跺潎淇濈暀锛?
25 鑳跺泭銆? 鐞冦€? 骞抽潰锛?9 涓綅浜庨鐩嗐€佽剨鏌便€佽兏銆侀銆佸ご銆佹墜鑷傚拰澶ц吙绛夎韩浣撳眰绾э紝8 涓綅浜庨檮浠跺眰绾с€?
11 涓墿鐞嗙粍寮曠敤鍏朵腑 25 涓紝Neck 涓?Spine1 鑳跺泭鏈紩鐢紝鍚屼竴纰版挒浣撴渶澶氳 4 缁勫叡浜€侭lender 闈㈡澘鐩存帴鏄剧ず
缁戝畾楠ㄩ銆佸紩鐢ㄦ鏁板拰缁勫悕锛屽洜姝よ函骞茬鎾炰綋涓庨檮浠剁鎾炰綋閮借兘浠庡師閰嶇疆璇嗗埆銆?

鍙傛暟宸ヤ綔娴佸垎涓ゅ眰銆傚師鐢?v2 缁勪箣闂村彲澶嶅埗鍏ㄩ儴鍖归厤鐨勫彲缂栬緫鏁板€煎拰鏇茬嚎瀛楁锛屼絾涓嶅鍒剁粍浠惰韩浠姐€佹牴楠ㄣ€?
SelectionData 鎴栫鎾炲紩鐢紱鏂板 v1 缁勫彲浠庡綋鍓嶅師鐢熸ā鏉垮鍒朵簲涓凡缁忔槧灏勭殑鏍囬噺銆傛柊澧為楠间娇鐢ㄢ€滃鍒舵墍閫夐閾句负
鏂板鐗╃悊閾锯€濓細澶嶅埗杩炵画鏍戠殑闈欐灞傜骇骞舵妸鍓湰鏍规帴鍥炲師澶栭儴鐖剁骇锛屾墍鏈夊壇鏈爣璁颁负 Skeleton `source=false`锛?
缃戞牸鏉冮噸涓嶈嚜鍔ㄥ鍒讹紝閬垮厤婧愰涓庡壇鏈悓鏃跺奖鍝嶅悓涓€椤剁偣锛岀敱浣滆€呮槑纭浆绉婚渶瑕佺墿鐞嗛┍鍔ㄧ殑鏉冮噸銆?

Mesh Mod 瀵煎嚭淇濇寔閫夋嫨寮忓閲忚涔夈€傚彧閫?Mesh 鏃朵粛涓?Mesh-only锛涘悓鏃堕€変腑鏂板 v1 鏃犵鎾炰綋缁勬椂锛屽鍑哄櫒鎸?
鍏变韩 Rig 姹囨€讳竴浠?Physics锛岃嚜鍔ㄥ啓鍏ュ悓涓€ Skeleton锛屽苟缁欐墍鏈変娇鐢ㄨ Rig 鐨勬墍閫?Render 鍐欏叆鐩稿悓 `physics=`銆?
鐢辨涓绘ā鍨嬨€丯PC 鍜岃鑹?UI 缁х画鍏变韩 DLL 鐜版湁鐨?Mesh 韬唤鍛戒腑瑙勫垯锛孭FB 涓嶆垚涓虹墿鐞嗚寖鍥存潯浠躲€傚師鐢?v2銆佸甫
纰版挒浣撶殑 v1銆佺己澶卞悓 Rig 鍙 Mesh 鐨勭墿鐞嗙粍閮藉湪淇敼鐩爣鐩綍鍓嶆嫆缁濓紝淇濈暀鐙珛浣滆€呭鍑哄叆鍙ｃ€?

瀹炵幇浣嶄簬 `tools/Blender/eiem_physics_source.py`銆乣eiem_physics_native.py`銆?
`eiem_physics_authoring.py` 鍜?`eiem_blender_addon.py`銆侭lender 5.0.1 涓?MSVC 鐜杩愯 33 椤归拡瀵规€ф祴璇曞叏閮ㄩ€氳繃锛?
瑕嗙洊鐪熷疄 Typhoea 婧愬浘銆佷笁绉嶇鎾炰綋銆佹牴/蹇界暐闂寘銆佸畬鏁村弬鏁版ā鏉裤€佸鍒舵柊澧為閾俱€佷繚瀛橀噸寮€銆乿1 缁勫悎鍖呫€?
鐢熶骇 C++ Physics/Mod 璇诲彇鍣ㄥ強 v2/纰版挒浣撴嫆缁濄€傛湰缁撴灉楠岃瘉浣滆€呮暟鎹笌褰撳墠 v1 鏃犵鎾炰綋杈撳嚭锛屼笉楠岃瘉 v2 鎴栫鎾炰綋
鍘熺敓瀹炰緥鍖栥€?

婧愮爜鐗堟湰宸插崌鑷?0.13.0锛屼竷涓繍琛屾枃浠堕€愪竴鍝堝笇鍚屾鍒?`E:\vscode\EIEM_Blender`銆傚畨瑁呭寘涓?
`bin/EIEM_Blender-0.13.0-native-physics-authoring.zip`锛屽ぇ灏?70941 瀛楄妭锛孲HA256锛?
`387B7CBD57FD2FB2056714C1117DC908BBF928A80AC6248630109902206533E0`銆傛湰杞湭鍚戞父鎴忔彃浠剁洰褰曞啓鏂囦欢銆?

### 20.27 姝ｅ父瑙ｅ寘鍖呭唴宓屽師鐢熺墿鐞嗘簮鍥惧苟涓庡叡浜?Rig 鍚堝苟

AnimeStudio 鐨勬甯?`Export Prefab as EIEM mod package` 宸叉帴鍏ョ墿鐞嗘簮鍐欏叆鍣ㄣ€傛墍閫?Prefab 瀛樺湪
`BeyondBoneCloth`銆佺悆浣撱€佽兌鍥婃垨骞抽潰缁勪欢鏃讹紝杈撳嚭鐩綍澧炲姞 `physics/components.json` 鍙婇€愮粍浠?
`.bin`銆乣.schema.json`銆乣.data.json`锛涘畠浠繚鐣?VFS 鎸囩汗銆丆AB/PathID 韬唤銆佸紩鐢ㄩ棴鍖呫€佸師濮嬪瓧鑺傘€?
TypeTree銆佽В鐮佸瓧娈靛拰 Prefab Transform 鍥俱€俙mod.ini` 涓嶅紩鐢ㄨ鐩綍锛屽洜姝よВ鍖呭緱鍒扮殑婧愬浘涓嶄細鑷鎴愪负
杩愯鏃?Physics 鍔ㄤ綔銆傞噸澶嶅鍑轰細鍏堟竻鐞嗚鍖呰嚜宸辩殑 `physics` 瀛愮洰褰曪紝閬垮厤鏃?Prefab 鏁版嵁娈嬬暀銆?

鐪熷疄 Typhoea 姝ｅ父鍖呭鍑烘垚鍔燂紝`components.json` 涓?646496 瀛楄妭锛屽寘鍚?38 涓粍浠跺拰 556 涓簮
Transform锛?1 涓?BoneCloth銆?5 涓兌鍥娿€? 涓悆浣撱€? 涓钩闈€侭lender 鏁村寘瀵煎叆瀹炴祴寰楀埌 62 涓?Mesh銆?
1 涓叡浜?Rig銆?1 涓師鐢熺墿鐞嗙粍鍜?27 涓鎾炰綋銆傛覆鏌?Skeleton 鍘熸湰鐪佺暐浜?48 涓墿鐞嗛摼鏈鑺傜偣浠ュ強
缁勪欢/纰版挒浣?owner Transform锛涘鍏ュ櫒鐜板湪鎶婄粍浠?owner銆佹樉寮?Transform 寮曠敤銆佹寜
`rootBones - ignoreFromRootBones` 灞曞紑鐨勭墿鐞嗛摼鍙婂叾绁栧厛鍚堝苟杩涘叡浜?Rig锛屽叡 192 涓墿鐞嗙浉鍏?Transform銆?
`GrounderIK`銆乣Mesh_all`銆乂FX 绛夋棤鍏?Prefab 鑺傜偣涓嶈繘鍏?Rig銆傚悎骞跺悗浠嶆牎楠屾簮璺緞銆佺洿鎺ョ埗绾у拰灞€閮?TRS銆?

AnimeStudio GUI 鐨?.NET 9 Release 鏋勫缓涓?0 閿欒锛屽苟鍙戝竷鍒?`tools/AnimeStudio/dist/win-x64-vfs-next`锛?
鍙戝竷鐗?`AnimeStudio.GUI.exe` 涓?168960 瀛楄妭锛孲HA256
`9B6ACC8139CDB3EF8B9DD15E11FC08DE57330CA58EA64DCF376CFA933670F004`銆侭lender 5.0.1 鐨勭湡瀹炴簮鍥惧線杩旀祴璇?
鍜屾甯告暣鍖呭鍏ユ祴璇曞潎閫氳繃銆備互涓婇獙璇佽鐩栨甯歌В鍖呫€佸師鐢熷弬鏁?纰版挒浣撲繚鐣欏強 Blender 鍦烘櫙寤虹珛锛涙湰鑺傛病鏈?
淇敼鎴栭儴缃?DLL锛屾病鏈夋妸 v2/纰版挒浣撳啓鍏ユ父鎴忚繍琛屾椂銆?

### 20.28 Blender 0.14.0 鐗╃悊宸ヤ綔鍖恒€侀泦鍚堥殧绂讳笌鍙傛暟闈㈡澘

鏈疆鍙慨鏀?Blender 浣滆€呭伐鍏峰拰鏂囨。锛屾病鏈変慨鏀广€佹瀯寤烘垨閮ㄧ讲 DLL锛屼篃娌℃湁鍚戞父鎴忕洰褰曞啓鏂囦欢銆傞棶棰樻潵婧愬凡缁忓垎鍒畾浣嶏細
鏁村寘瀵煎叆涓€鐩村鐢ㄥ叏灞€ `EIEM` 鍙婂悓鍚?LOD 闆嗗悎锛屽鑷村涓?package 娣峰湪涓€璧凤紱鍘熺敓鐗╃悊瀵硅薄鍜屽叏閮ㄧ嚎妗嗗彲瑙嗗寲鍚屾椂鏄剧ず锛?
瀵艰嚧 11 涓粍銆?7 涓鎾炰綋鍜岃緟鍔╂洸绾垮湪瑙嗗浘涓彔鍔狅紱闈㈡澘姣忔閲嶇粯閮戒細閲嶆柊璇诲彇骞惰В鏋愬寘鍚?556 涓?Transform 鐨?
`components.json` 鏂囨湰锛屾櫘閫氬弬鏁颁慨鏀硅繕浼氶噸寤哄彲瑙嗗寲鍑犱綍锛屽洜姝ゅ弬鏁版搷浣滃嚭鐜板崱椤裤€?

0.14.0 鎶婁竴娆″鍏ュ缓绔嬩负鐙珛 package 宸ヤ綔鍖猴細`EIEM / EIEM <package> / Meshes銆丼keletons銆丳hysics`銆?
鍚屼竴璺緞鍐嶆瀵煎叆浼氱敓鎴愪笉鍚岀殑 import id 鍜屽敮涓€闆嗗悎鍚嶏紝涓嶅啀鎶婂璞″苟鍏ュ墠涓€娆″鍏ャ€傛瘡涓?Physics 宸ヤ綔鍖虹户缁垎涓?
`Groups`銆乣Colliders`銆乣Visuals`锛涢泦鍚堝彧璐熻矗鍦烘櫙鏁寸悊鍜岄€夋嫨锛屼笉鎵挎媴鐗╃悊鎷撴墤璇箟銆傛棫鍦烘櫙鍙湪鐗╃悊闈㈡澘鎵ц
鈥滄暣鐞嗗綋鍓?Rig鈥濓紝鎶婂凡鏈夌墿鐞嗚緟鍔╁璞¤縼鍏ヨ繖涓変釜鍒嗙被锛涜鑾峰緱瀹屾暣鐨勯€?package Mesh/Skeleton 灞傜骇锛屽簲浣跨敤 0.14.0
閲嶆柊瀵煎叆 package銆?

鐗╃悊鍏崇郴浠嶄互鐪熷疄婧愭暟鎹负鍑嗐€備竴涓?BoneCloth 缁?Empty 淇濆瓨缁勫弬鏁般€乣rootBones`銆乣ignoreFromRootBones`銆?
SelectionData 鍜岀鎾炰綋寮曠敤锛涘畠鍙互灞曞紑鍑哄涓牴鍜屽垎鏀紝涓嶅己鍒惰В閲婃垚鍗曟潯绾挎€ч摼銆侰ollider Empty 鐙珛淇濆瓨绫诲瀷銆?
缁戝畾 Transform銆佸舰鐘跺弬鏁板拰婧愮粍浠惰韩浠斤紝鍚屼竴涓鎾炰綋鍙互琚涓粍寮曠敤銆傞楠兼湰韬笉澶嶅埗涓€濂椻€滄墍灞炵墿鐞嗏€濆弬鏁帮紱
楠ㄩ銆佺粍鍜岀鎾炰綋閫氳繃涓婅堪寮曠敤鍏宠仈銆俆ransform 灞傜骇鍜?SelectionData 鐐规暟缁勭户缁垎鍒睍绀猴紝鍦ㄦ病鏈夊疄璇佹槧灏勬椂涓嶇寽娴?
鈥滅 N 涓偣绛変簬绗?N 鏍归鈥濄€?

鍙傛暟鐨勫敮涓€鍙紪杈戞潵婧愮户缁斁鍦ㄧ粍鎴栫鎾炰綋 Empty 鐨?Blender RNA PropertyGroup 涓紝鑰屼笉鏄啀闀滃儚涓€浠?ID 鑷畾涔夊睘鎬с€?
闈㈡澘澧炲姞浜斾釜甯哥敤缁勫弬鏁扮殑鐩存帴缂栬緫鍖猴紝瀹屾暣鍘熺敓鍙傛暟鏀寔鎼滅储骞舵瘡椤垫樉绀?32 椤广€傛簮鏂囨。瑙ｆ瀽缁撴灉鎸?Text 鏁版嵁鍧楀唴瀹圭紦瀛橈紝
鏅€氭眰瑙ｅ弬鏁颁慨鏀逛笉鍐嶉攢姣佸苟閲嶅缓鏇茬嚎锛涘彧鏈?center銆乺adius銆乴ength銆乨irection 绛夊奖鍝嶇鎾炰綋褰㈢姸鐨勫瓧娈垫墠鏇存柊鍑犱綍銆?
杩欐牱淇濆瓨銆佹挙閿€鍜屽鍑轰粛浣跨敤鍚屼竴浠界粨鏋勫寲鏁版嵁锛屽悓鏃舵秷闄ら噸澶?JSON 瑙ｆ瀽鍜屾棤鍏冲嚑浣曢噸寤恒€?

榛樿瑙嗗浘鍙樉绀哄綋鍓嶇墿鐞嗙粍鍙婂叾寮曠敤鐨勭鎾炰綋锛屽苟鍏抽棴绌块€忔樉绀猴紱杩樺彲鍒囨崲涓轰粎缁勩€佷粎纰版挒浣撱€佸叏閮ㄦ垨鍏ㄩ儴闅愯棌銆?
缁勯潰鏉挎樉绀烘牴鏁般€佺墿鐞?Transform 鏁板拰 Selection 鐐规暟锛岀鎾炰綋闈㈡澘鏄剧ず缁戝畾楠ㄩ銆佽鍝簺缁勫叡浜紝骞跺彲浠庣鎾炰綋璺宠浆鍒?
涓€涓紩鐢ㄧ粍銆俆yphoea 鏍锋湰涓殑 19 涓函骞?韬綋灞傜骇纰版挒浣撲笌 8 涓檮浠跺眰绾х鎾炰綋閮戒繚鐣欏湪 `Colliders` 涓紱11 涓?
妯℃嫙缁勫睘浜庡ご鍙戙€佽。鐗┿€侀檮浠跺拰灏鹃儴锛屾病鏈夌嫭绔嬬殑鈥滆函骞叉ā鎷熺粍鈥濄€傝函骞插湪杩欎釜鏍锋湰涓富瑕佹彁渚涘姩鐢婚敋鐐瑰拰鍏变韩纰版挒鐜锛?
鍏剁鎾炰綋鍙傛暟浠嶅彲鍦ㄥ搴?Collider Empty 涓婄洿鎺ヤ慨鏀广€?

浜や簰缁勭粐鍙傝€冧簡寮€婧?[RE Chain Editor](https://github.com/NSACloud/RE-Chain-Editor) 鐨勫仛娉曪細娲诲姩閾炬枃浠躲€佺鎾炰綋闆嗗悎銆?
Empty 鍙傛暟闈㈡澘銆佹寜绫诲埆闅愯棌鍜岀嫭绔嬬┛閫忓紑鍏炽€傝繖閲屽彧閲囩敤宸ヤ綔鍖轰笌鍙鎬ф€濊矾锛涙病鏈夊鍒?RE Engine Chain2 瀛楁銆佽妭鐐瑰嚑浣?
鎴栫墿鐞嗗崟浣嶏紝缁堟湯鍦版暟鎹涔変粛鏉ヨ嚜褰撳墠 `components.json`銆乀ypeTree 鍜屽凡缁忛獙璇佺殑 EIEM 浣滆€?杩愯鏃跺悎鍚屻€?

Blender 5.0.1 鐨勫畬鏁?Blender 鍖呰娴嬭瘯杩愯 12 椤癸紝鍏朵腑 8 椤归€氳繃锛? 椤瑰洜褰撳墠娴嬭瘯鍏ュ彛鏈厤缃?MSVC/澶栭儴鏍锋湰鑰岃烦杩囷紝
0 椤瑰け璐ャ€傞拡瀵规€ф祴璇曡鐩栫湡瀹?Typhoea 鐗╃悊婧愩€侀粯璁ゅ綋鍓嶇粍鍙鎬с€佺紦瀛橀殧绂汇€佹櫘閫氬弬鏁颁笉閲嶅缓鍑犱綍銆侀€?package 闆嗗悎闅旂銆?
閲嶅瀵煎叆涓嶆贩鍚堝璞★紝浠ュ強鎻掍欢娉ㄥ唽銆佸嵏杞藉拰涓夎疆閲嶈浇銆備竷涓繍琛屾枃浠跺凡閫愪竴鍝堝笇鍚屾鍒?
`E:\vscode\EIEM_Blender`銆傚畨瑁呭寘涓?`bin/EIEM_Blender-0.14.0-physics-authoring-ui.zip`锛屽ぇ灏?75252 瀛楄妭锛?
SHA256锛歚AA5CF4B674404D14E4C22AEEFBDF76EC49A607ED1BE2B7DE511FFDC67B5151C9`锛涘帇缂╁寘瑙ｅ帇鍚庝篃閫氳繃 Blender 娉ㄥ唽涓庝笁杞噸杞芥祴璇曘€?

### 20.29 褰撳墠 Typhoeus 宸ョ▼鐨勯楠兼湞鍚戝璁?

浣跨敤 Blender 5.0.1 鐙珛鍚庡彴杩涚▼鍙鍔犺浇鐢ㄦ埛褰撳墠鎵撳紑骞跺凡淇濆瓨鐨?
`G:\zmd\typhoeus\Typhoeus_1.0.blend`锛屾病鏈変繚瀛樻垨淇敼璇ュ伐绋嬨€傚満鏅腑鏈変袱浠?EIEM Rig锛?
`Skeletonchr_0034_typhoea_postmodel_0` 涓?316 鏍归锛岀己灏戞簮鐗╃悊鍥捐姹傜殑 48 涓湯绔墿鐞嗚妭鐐癸紱
`Skeletonchr_0034_typhoea_postmodel_0.001` 涓?397 鏍归锛屽畬鏁磋鐩?140 涓疄闄呮ā鎷?Transform 鍜?
192 涓墿鐞嗕綔鑰呬緷璧?Transform銆傚悗缁煡鐪嬫垨鍒涘缓鐗╃悊搴斾娇鐢ㄧ浜屼唤瀹屾暣 Rig锛岀洿鍒版棫鐨勯噸澶嶅鍏ヨ娓呯悊銆?

瀹屾暣 Rig 鐨?397 鏍归閫愪竴鐢ㄥ祵鍏?`.blend` 鐨?`native-authoring` 婧愬浘閲嶅缓娓告垙涓栫晫鐭╅樀锛屽啀閫氳繃 EIEM 鐨?
Unity 宸︽墜 Y-up 鍒?Blender Z-up 鍩哄彉鎹㈡瘮杈冦€俬ead 浣嶇疆鏈€澶ц宸负 0锛岀煩闃垫棆杞渶澶ц宸负 0锛涗繚瀛樼殑
`eiem_rest_display` 涓庡綋鍓?`matrix_local` 鐨勬渶澶ц宸篃涓?0銆傚洜姝ゅ綋鍓嶉鏋朵笉瀛樺湪鍧愭爣杞翠氦鎹€佹墜鎬ф垨鍥涘厓鏁?
杞崲閿欒銆?

Blender 澶栧舰纭疄瀹规槗琚璁や负鏈濆悜閿欒锛?97 鏍瑰鍏ラ楠煎叏閮ㄤ娇鐢ㄧ害 0.05 鐨勬樉绀洪暱搴︼紝鍧囨湭璁剧疆 connected銆?
140 涓ā鎷?Transform 鍐呮湁 112 鏉″疄闄呯埗瀛愯竟锛沚one tail 涓庡瓙鑺傜偣 head 鏂瑰悜鐨勪腑浣嶅す瑙掍负
`89.99997掳`锛屽叾涓?110 鏉¤秴杩?45掳銆傝繖鏄簮 Transform 灞€閮?`+Y` 杞翠笌鑺傜偣灞傜骇鏂瑰悜骞堕潪鍚屼竴姒傚康閫犳垚鐨勶紝
涓嶆槸瀵煎叆璇樊銆傜豢鑹测€滃眰绾ц繛绾库€濊繛鎺ョ埗瀛?Transform 鍘熺偣锛屾墠浠ｈ〃鐗╃悊閾鹃潤姝㈡柟鍚戯紱bone tail/roll 淇濈暀灞€閮?
鏃嬭浆鍧愭爣绯汇€?

杩欎竴鍖哄埆瀵逛綔鑰呮搷浣滄湁瀹為檯褰卞搷銆傚師鐢熸簮楠ㄩ瀵煎嚭浣跨敤淇濆瓨鐨勫眬閮?TRS锛屽苟鎷掔粷宸茬粡鏀瑰姩鐨勭粦瀹氬Э鎬侊紝鎵€浠ュ綋鍓嶅褰?
涓嶄細鐮村潖瀵煎叆鐨勫師鐢熺粍銆丼election 鎴栫鎾炰綋銆傛柊澧為楠煎垯浠?Blender `matrix_local` 鐢熸垚鏂?Transform锛歨ead 鍜岀埗绾?
鍐冲畾灞€閮ㄥ钩绉伙紝tail 鏂瑰悜鍙?roll 鍐冲畾灞€閮ㄦ棆杞紱鏄剧ず闀垮害鏈韩涓嶅啓鍏?Physics 鍙傛暟銆傛妸婧愰楠煎己鍒?connect銆佺粺涓€
閲嶇畻 roll锛屾垨涓轰簡璁╁叓闈綋鎸囧悜瀛愯妭鐐硅€岀Щ鍔?tail锛屼細鏀瑰彉灞€閮ㄦ棆杞苟浣挎簮楠ㄦ灦瀵煎嚭琚嫆缁濄€傚鍒舵柊澧炵墿鐞嗛摼淇濈暀
鍘熼楠肩殑 head/tail/roll锛屽洜姝や笉浼氳嚜琛屽紩鍏ユ湞鍚戝彉鍖栥€?

0.14.0 闈㈡澘鎹澧炲姞鈥淏lender 楠ㄩ澶栧舰鈥濊鍥鹃€夋嫨锛屽苟鍦ㄥ師鐢熺粍涓槑纭爣鍑衡€滅豢鑹插眰绾ц繛绾?鐗╃悊閾炬柟鍚戙€?
bone tail=婧?Transform 灞€閮?+Y鈥濓紱鏂板缁勪篃鎻愮ず head/鐖剁骇涓?tail/roll 鐨勪笉鍚屽鍑轰綔鐢ㄣ€傝閫夋嫨鍙敼鍙?Blender
瑙嗗浘鐢绘硶锛屼笉淇敼 Skeleton 鎴?Physics 鏁版嵁銆傛湰鑺傛病鏈変慨鏀规垨閮ㄧ讲 DLL锛屼篃娌℃湁鍐欏叆娓告垙鐩綍銆?

### 20.30 鍘熺敓鑳跺泭绔偣鍏紡涓?Blender 0.14.1 淇

鐢ㄦ埛鍦?Blender 涓彂鐜?`Magica Capsule Collider (Bip001_L_Thigh)` 浠庨珛閮ㄥ悜涓婁几鍏ヨ函骞层€傚鏍哥‘璁ゅ師濮?
`center=(-0.02,0,0)`銆乣size=(0.1,0.105,0.497)`銆乣direction=X`銆乣reverseDirection=false`銆?
`alignedOnCenter=false` 鍧囪鍙栨纭紝Collider owner 鐨勬簮 Transform 涓?Blender 缁戝畾鐭╅樀涔熶竴鑷达紱閿欒鏉ヨ嚜
0.13.0/0.14.0 棰勮鎶?`size.z` 鐚滄垚涓ょ鐞冨績璺濈锛屽苟鎶婇潪灞呬腑鑳跺泭鐢绘垚 `center 鈫?center+axis*length`銆?

鏈満 `GameAssembly.dll` 鐨?BeyondDynamicBone 鏂规硶浣撶粰鍑轰簡瀹屾暣鍏紡銆俙GetColliderType` `0x4563880` 鎶?
灞呬腑鐨?X/Y/Z 鑳跺泭缂栫爜涓?2/3/4锛岄潪灞呬腑缂栫爜涓?5/6/7锛沗GetSize` `0x44BFB70` 鍦?
`radiusSeparation=false` 鏃剁敤 startRadius 鍚屾椂鏇夸唬 endRadius锛沗GetLocalDir` `0x59DC020` 閫夋嫨灞€閮?
X/Y/Z锛屽苟鍦?`reverseDirection=true` 鏃跺彇鍙嶃€俙StartSimulationStepJob.Execute(int)` `0x5A66A3C`
杩涗竴姝ヨ绠椾袱绔悆蹇冿細

```text
axis = reverseDirection ? -localAxis : localAxis
if alignedOnCenter:
    start = center + axis * max(length / 2 - startRadius, 0)
    end   = center - axis * max(length / 2 - endRadius, 0)
else:
    start = center
    end   = center - axis * max(length - startRadius - endRadius, 0)
```

杩欒瘉鏄?`length` 鏄寘鍚袱绔渾澶寸殑鑳跺泭鎬婚暱锛宻tart/end 淇濆瓨鐨勬槸绔儴鐞冨績锛屽苟鍦ㄥ崐寰勪箣鍜岃秴杩囬暱搴︽椂鎶婄悆蹇冮棿璺?
閽冲埗涓?0銆傚畼鏂?MagicaCloth2 鏂囨。涔熸槑纭妸 `size` 璁颁负 `(start radius, end radius, length)`锛屽苟璇存槑鍏抽棴
Aligned On Center 鍚庝互鑳跺泭璧风偣浣滀负鏃嬭浆涓績锛涙湰鏈烘柟娉曚綋琛ヨ冻浜嗗叕寮€鏂囨。娌℃湁鍒楀嚭鐨勫叿浣撶畻寮忋€?

Typhoea 鐨勫叏閮?19 涓潪灞呬腑鑳跺泭涔熸彁渚涗簡鐙珛鍑犱綍浜ゅ弶妫€鏌ワ細榛樿鏂瑰悜鐨勬棫棰勮閮借儗绂诲涓婚楠肩殑鍚庝唬锛涙寜鍘熺敓
璐熺鍏紡鍚庯紝宸﹀彸澶ц吙鏈濆皬鑵裤€佸乏鍙充笂鑷傛湞鍓嶈噦銆佸乏鍙冲墠鑷傛湞鎵嬨€佽剨鏌辨湞涓婄骇韬共銆侀鏈濆ご銆佷袱渚у彂鏉熸湞鍚庣画鍙戦銆?
鍏朵腑宸﹀ぇ鑵跨殑 start 鐞冨績浠嶅湪楂嬮儴闄勮繎锛宔nd 鐞冨績娌胯吙鍚戜笅绾?`0.497-0.1-0.105=0.292`锛屼笉鍐嶅悜涓婅繘鍏ヨ函骞层€?

Blender 0.14.1 宸叉寜涓婅堪鐞冨績鍏紡鐢熸垚鑳跺泭绾挎锛屼繚鐣欏師瀛楁銆佺粦瀹氥€佸鍑哄瓧鑺傚拰澧為噺瑙勫垯銆傝淇鍙敼鍙樺師鐢?
纰版挒浣撶殑浣滆€呴瑙堬紝涓嶄慨鏀?DLL銆丳hysics v2 鏂囦欢鎴栨父鎴忕洰褰曪紱纰版挒鍝嶅簲浠嶉渶鍦ㄤ互鍚庢仮澶?DLL 纰版挒瀹為獙鏃堕獙璇併€?
涓冧釜鎻掍欢鏂囦欢宸插悓姝ュ埌寮€鍙戠洰褰曞拰 Blender 5.0 鐢ㄦ埛鎻掍欢鐩綍锛涘畨瑁呭寘涓?
`bin/EIEM_Blender-0.14.1-native-collider-geometry.zip`锛屽ぇ灏?75502 瀛楄妭锛孲HA256锛?
`6F4961983C18CC82FEFA5A4E1C2686650D9689B9D5589A5F655021859C44ABFD`銆?

### 20.31 Blender 0.15.0 鐗╃悊宸ヤ綔鍖恒€佹洸绾夸笌瀹炰綋鏄剧ず

鎸夊綋鍓嶄綔鑰呮祦绋嬬户缁畬鍠?Blender 渚э紝娌℃湁淇敼 Physics v1/v2 浜岃繘鍒跺悎鍚屻€丏LL 鎴栨父鎴忕洰褰曘€侴roup 涓?Collider
Empty 鐨?RNA PropertyGroup 浠嶆槸鍞竴鍙傛暟鐪熷€硷紱楠ㄩ鍙壙鎷呯ǔ瀹氳韩浠姐€佺埗瀛愬眰绾у拰闈欐鍙樻崲銆傚師鐢?Group 鐜板湪鎶?
姣忎釜 `rootBones` 椤逛綔涓轰竴鏉″彲鍗曠嫭閫夋嫨鐨勬牴鍒嗘敮鍒楀嚭锛屽苟鍦ㄥ悓涓€闈㈡澘鏄庣‘鍒楀嚭婧愮粍瀹為檯寮曠敤鐨?Collider Empty銆?
杩欎笌婧愭暟鎹殑鐪熷疄鍏崇郴涓€鑷达細涓€涓?BeyondBoneCloth 缁勫彲鍚涓牴鍜屽垎鏀紝缁勫唴鍏变韩涓€濂楀弬鏁般€佹洸绾夸笌纰版挒浣撳垪琛紱
鍚屼竴涓鎾炰綋浠嶅彲琚涓粍寮曠敤銆傞渶瑕佷笉鍚屽弬鏁版垨纰版挒闆嗗悎鐨勯閾惧簲鍒掑垎涓轰笉鍚岀墿鐞嗙粍銆?

鍘熺敓缁勫鍔犱節绫荤粨鏋勫寲鏇茬嚎缂栬緫鍏ュ彛锛氶樆灏笺€佽妭鐐瑰崐寰勩€佽窛绂荤害鏉熷己搴︺€佽搴︽仮澶嶅己搴︺€佽搴﹂檺鍒躲€佹渶澶ц繍鍔ㄨ窛绂汇€?
鍥炴尅璺濈銆佺鎾為檺鍒惰窛绂诲拰鑷鎾炶〃闈㈠帤搴︺€傜晫闈㈡寜婧愬瓧娈电洿鎺ョ紪杈戝熀纭€鍊笺€乣useCurve`銆佸叧閿抚鐨勯摼浣嶇疆/鍊嶇巼锛?
浠ュ強鍙睍寮€鐨勫垏绾裤€佹潈閲嶆ā寮忎笌鏉冮噸锛涗笉寤虹珛绗簩浠借嚜瀹氫箟灞炴€э紝涔熶笉鎶?Selection 鐐归『搴忕寽鎴愰楠奸『搴忋€?
褰撳墠瀹炵幇鏄暟鍊煎叧閿抚缂栬緫鍣紝灏氭湭鎻愪緵鍥惧舰鏇茬嚎鐢诲竷鎴栨寜楠ㄩ鐫€鑹茬殑鏇茬嚎閲囨牱棰勮銆備氦浜掔粍缁囧弬鑰冨浐瀹氱増鏈殑
[RE Chain Editor 鍑犱綍鑺傜偣瀹炵幇](https://github.com/NSACloud/RE-Chain-Editor/blob/54ed5d41a6360511b4b314c80e9b459032b32688/modules/re_chain_geoNodes.py)锛?
鍙€熼壌瀹炰綋鑺傜偣銆佽繛鎺ヤ綋涓庡垎绫绘樉绀烘柟寮忥紝涓嶅鐢?RE Engine Chain2 瀛楁鎴栫墿鐞嗗崟浣嶃€?

杈呭姪鏄剧ず榛樿浠庝笁鐜嚎妗嗘敼涓轰綆闈㈡暟瀹炰綋锛歋election 鑺傜偣涓哄僵鑹茬悆锛岀埗瀛愬眰绾ц竟涓哄疄浣撶锛岀悆/鑳跺泭/骞抽潰纰版挒浣?
涓哄崐閫忔槑缃戞牸锛涒€滆鍥炬樉绀?鈫?鏍峰紡鈥濆彲闅忔椂鍒囧洖绾挎銆傞粯璁よ寖鍥翠粛涓衡€滃綋鍓嶇粍鈥濓紝鍙樉绀鸿缁勫強鍏跺紩鐢ㄧ殑鍏变韩纰版挒浣擄紱
绌块€忔樉绀洪粯璁ゅ叧闂€備綔鑰?v1 缁勫悓鏍蜂娇鐢ㄥ疄浣撹妭鐐逛笌閾炬锛岀悆/绛夊崐寰勮兌鍥婁娇鐢ㄥ悓涓€鏄剧ず鍣ㄣ€侭lender Armature 鑷韩鐨?
榛戣壊 bone tail/roll 浠嶇敱鈥淏lender 楠ㄩ澶栧舰鈥濇帶鍒讹紝瀹冧笉鏄墿鐞嗛摼杩炴帴浣擄紱缁胯壊瀹炰綋杩炴帴鎵嶈〃绀虹埗瀛?head 鏂瑰悜銆?

Blender 5.0.1 閽堝鎬ч獙璇侀€氳繃鎻掍欢鍙戠幇鍙婁笁杞敞鍐?鍗歌浇銆乿1 浣滆€呭垱寤?淇濆瓨/閲嶅紑銆佺湡瀹?Typhoea v2 婧愭暟鎹線杩旓紝
浠ュ強姝ｅ父 package 鏁翠綋瀵煎叆銆傛甯稿寘缁撴灉浠嶄负 62 Mesh銆? 鍏变韩 Rig銆?1 缁勩€?7 纰版挒浣撳拰 192 涓墿鐞嗕緷璧?Transform銆?
鐪熷疄婧愭暟鎹殑 11 涓粍鍧囪瘑鍒埌涔濈被鏇茬嚎瀛楁锛屾樉寮?Collider 寮曠敤涓庢簮鍒楄〃涓€鑷达紱瀹炰綋/绾挎鍙屽悜鍒囨崲鍜屼笁绉嶅師鐢?
纰版挒浣撳潎鏈夋柇瑷€銆傚綋鍓嶆墦寮€鐨勬湭淇濆瓨 Blender 鍦烘櫙宸茬儹閲嶈浇鍒?0.15.0锛屼繚鐣?11 缁勫拰 27 纰版挒浣擄紝閲嶅缓涓?61 涓?
Mesh 杈呭姪浣撱€? 涓棫 Curve 杈呭姪浣擄紝瀵煎叆涓庡鍑鸿彍鍗曞洖璋冨悇淇濈暀涓€涓紱褰撳墠瑙嗗浘璁句负鈥滃綋鍓嶇粍鈥濄€佸疄浣撱€佸叧闂┛閫忋€?

涓冧釜杩愯鏂囦欢宸查€愪竴鍝堝笇鍚屾鍒?`E:\vscode\EIEM_Blender`銆傚畨瑁呭寘涓?
`bin/EIEM_Blender-0.15.0-physics-solid-workspace.zip`锛屽ぇ灏?80272 瀛楄妭锛孲HA256锛?
`884121E2634B166A9E06FDD0FD085ACC398144497E7C1D0E54A94ABB56A5FA52`锛涘帇缂╁寘瀹屾暣鎬у強瑙ｅ帇鍚庣殑涓夎疆娉ㄥ唽娴嬭瘯閫氳繃銆?

### 20.32 Blender 0.16.0锛欵mpty 鍙傛暟銆丗-Curve 涓庨摼鍒锋柊

0.15.0 鐨勬暟鍊兼洸绾垮尯鍜屼晶鏍忓悓鏃舵壙鎷呭弬鏁般€佹嫇鎵戙€侀€夋嫨銆佹枃浠跺拰鏄剧ず鎿嶄綔锛屽鑷村父鐢ㄥ叆鍙ｈ繃澶氾紱鏂板缓缁勫張鍙湪鍒涘缓
鐬棿璇诲彇楠ㄩ閫夋嫨锛屽悗缁柊楠ㄩ娌℃湁鏄庣‘鍔犲叆鍔ㄤ綔锛岄瑙堝埛鏂拌竟鐣屼篃涓嶆竻妤氥€?.16.0 灏嗕綔鑰呮暟鎹墍鏈夋潈鍜屽揩鎹峰叆鍙?
鍒嗗紑锛欸roup/Collider Empty 缁х画淇濆瓨鍞竴 RNA 婧愬弬鏁帮紝閫変腑 Empty 鍚庡湪瀵硅薄灞炴€х殑 **EIEM 鐗╃悊鍙傛暟**涓紪杈戯紱
N 渚ф爮棣栧眰鍙樉绀?*褰撳墠缁勩€佹柊寤恒€佸鍒躲€佹煡鐪?*锛岄摼缁撴瀯銆佹樉绀哄拰鏂囦欢宸ュ叿榛樿鎶樺彔銆傞€変腑鍙︿竴涓?Group Empty 鏃讹紝
瀹冮€氳繃 Blender RNA 娑堟伅鎬荤嚎鎴愪负褰撳墠缁勶紝涓嶄娇鐢ㄦ瘡甯ц疆璇€?

鍘熺敓缁勭殑涔濈被閾句綅缃€嶇巼鏇茬嚎鐜板湪鎶曞奖涓鸿 Group Empty 鑷繁鐨?Blender Action/F-Curve銆傛í杞?0锝?00 瀵瑰簲婧愪綅缃?
0锝?锛岀旱杞翠负鍊嶇巼锛屾洸绾块潤闊冲搴?`useCurve=false`锛涙簮鍒囩嚎杞崲涓?Bezier 鎵嬫焺銆傜敤鎴峰彲浠庡璞″睘鎬т竴閿墦寮€ Graph
Editor锛屾棩甯告洸绾垮悕浣跨敤鐭腑鏂囷紝鍘熷 TypeTree 璺緞鍙湪鎶樺彔鐨勯珮绾ф簮瀛楁涓繚鐣欍€侫ction 璁板綍鏇茬嚎绛惧悕锛涘彧鏈夌敤鎴?
瀹為檯淇敼鏇茬嚎鍚庯紝搴旂敤鎴栧鍑烘墠鍐欏洖婧愬瓧娈碉紝鏈敼鏁版嵁涓嶄細鍥?Blender float32 鎴栨墜鏌勮〃绀鸿閲嶅啓銆傚綋鍓嶅啓鍥炰繚鎸佸師
鍏抽敭甯ф暟閲忥紝鏂板鎴栧垹闄ゅ叧閿抚浼氭槑纭嫆缁濓紝閬垮厤浼€犲皻鏈畾涔夌殑 Unity 鏇茬嚎瀛楁銆?

鏂板 v1 鎷撴墤缁х画閬靛畧绾挎牸寮忓悎鍚岋細涓€涓粍鏄竴妫靛叿鏈夊崟涓€鍥哄畾鏍圭殑杩為€氭爲銆傚垱寤烘椂閫夋嫨瀹屾暣杩炵画楠ㄩ摼锛涘悓涓€閾炬柊澧?
鍚庝唬鍚庣敤**灏嗘墍閫夐楠煎姞鍏ュ綋鍓嶉摼**锛岀嫭绔嬫牴鍙﹀缓缁勩€傚姞鍏ュ墠杩愯瀹屾暣 v1 鎷撴墤鏍￠獙锛岀浜屾牴涓嶄細鐣欎笅鍗婁慨鏀圭姸鎬侊紱
鍔犲叆銆佺Щ闄や笌鍒涘缓绔嬪嵆閲嶅缓棰勮銆侫rmature 闈欐鏁版嵁鍙樺寲鐢?depsgraph 鏍囪锛屽湪閫€鍑虹紪杈戞ā寮忓悗鍚堝苟閲嶅缓璇?Rig 鐨?
浣滆€呯粍锛岄伩鍏嶇紪杈戞湡闂村弽澶嶅埛鏂般€傚師鐢?v2 鎷撴墤浠嶅彧璇伙紝娌℃湁浠?SelectionData 鐚滃啓鏂伴楠兼槧灏勩€?

Blender 5.0.1 鍥炲綊宸查€氳繃鎻掍欢涓夎疆娉ㄥ唽/鍗歌浇銆佹秷鎭€荤嚎涓?depsgraph 娓呯悊銆乿1 閾惧姞鍏?绗簩鏍规嫆缁?绉婚櫎/鍒锋柊銆?
鐪熷疄 Typhoea 11 缁?27 纰版挒浣撳鍏ャ€佷節鏉?F-Curve 缂栬緫鍐欏洖銆佹湭鏀规簮绮剧‘寰€杩斾互鍙?`.blend` 淇濆瓨閲嶅紑銆?
鏈妭鍙慨鏀?Blender 浣滆€呭伐鍏峰拰鏂囨。锛屾病鏈変慨鏀?DLL銆丳hysics 浜岃繘鍒跺悎鍚屾垨娓告垙鐩綍锛泇2 涓庣鎾炰綋鐨勬父鎴忓疄渚嬪寲
杈圭晫淇濇寔涓嶅彉銆備竷涓繍琛屾枃浠跺凡閫愪竴鍝堝笇鍚屾鍒?`E:\vscode\EIEM_Blender`锛屽綋鍓?Blender 浼氳瘽宸茬儹閲嶈浇涓?
0.16.0锛屼粛涓?11 缁勩€?7 纰版挒浣撱€?1 涓疄浣撹緟鍔?Mesh銆? 涓棫 Curve 杈呭姪浣擄紝瀵煎叆/瀵煎嚭鑿滃崟鍥炶皟鍙?
depsgraph 澶勭悊鍣ㄥ悇涓€涓紱娌℃湁淇濆瓨鐢ㄦ埛 `.blend`銆傚畨瑁呭寘涓?
`bin/EIEM_Blender-0.16.0-fcurve-object-ui.zip`锛屽ぇ灏?85941 瀛楄妭锛孲HA256锛?
`CA34E63A2445B6D77E700A9B80988ECE2CF3E89EEACF3311D018E1E13EF142E9`锛涜В鍘嬪悗鐨勫寘宸查€氳繃涓夎疆娉ㄥ唽娴嬭瘯銆?

### 20.33 瑙掑害闄愬埗涓?Chain2 閿ヤ綋鐨勫樊寮?

Blender 0.16.0 灏氭湭鐢熸垚涓夌淮瑙掑害閿ワ紱褰撳墠鍙妸
`serializeData.angleLimitConstraint.limitAngle` 鏄犲皠涓?Group Empty 鐨?Blender F-Curve銆傛洸绾块潤闊虫槧灏勭殑鏄?
`limitAngle.useCurve`锛屼笉浠ｈ〃鏁翠釜绾︽潫鐨?`useAngleLimit`锛屽洜姝ゅ彧鐪?Graph Editor 鍙兘鎶娾€滀繚瀛樹簡鏇茬嚎浣嗘€诲紑鍏?
鍏抽棴鈥濊璁や负瑙掑害闄愬埗姝ｅ湪鐢熸晥銆傚悗缁父鐢ㄥ弬鏁伴潰鏉垮簲鎶?*鍚敤瑙掑害闄愬埗銆侀檺鍒惰銆佹洸绾垮拰杈圭晫鍥炲脊寮哄害**鏀惧湪鍚屼竴鍖哄煙銆?

缁堟湯鍦版簮瀛楁涓?MagicaCloth2 鐨勫叕寮€ `AngleConstraint.LimitSerializeData` 涓€鑷达細缁勭骇閰嶇疆鍖呭惈
`useAngleLimit`銆丆urveSerializeData `limitAngle` 鍜?`stiffness`銆傚畠闄愬埗 baseline 涓瘡鏉¤竟鐩稿鐖惰竟/鍩哄噯濮挎€?
鍙集鏇茬殑瑙掑害锛涙渶缁堣搴︿负鍩虹瑙掍箻浠ヨ椤剁偣 depth 涓婄殑鏇茬嚎鍊嶇巼锛宍stiffness` 鎺у埗瓒婄晫鍚庣殑鍥炲脊杞‖锛?
`animationPoseRatio` 杩樹細褰卞搷浣跨敤鍒濆濮挎€佽繕鏄綋鍓嶅姩鐢诲Э鎬佷綔涓哄熀鍑嗐€傚弬鑰冿細
https://magicasoft.jp/en/mc2_magicacloth_anglelimit/ 涓?https://magicasoft.jp/en/mc2_baseline/ 銆?

Typhoea 鐨?11 涓?BoneCloth 缁勪腑鏈?4 涓惎鐢?`useAngleLimit`锛氳鎽?18掳銆侀暱椹熬 90掳銆佷晶闀垮彂 36掳銆佸熬宸?
60掳锛涘叾浣?7 缁勮櫧鐒朵繚鐣欓粯璁?60掳 鎴栨洸绾匡紝浣嗘€诲紑鍏冲叧闂€傛牱鏈叏閮?164 涓?Selection 浣嶇疆閮借兘鍦ㄧ粍鏍瑰睍寮€鐨?
Transform 鍘熺偣涓壘鍒板嚑浣曞搴旓紝鏈€澶ц宸害 `2.55e-6`锛涢暱椹熬鐨?38 鐐瑰寘鎷?14 涓椿鍔ㄨ妭鐐逛笌 24 涓?IGNORE
鍒嗘敮鐐广€傝繖涓烘寜浣嶇疆寤虹珛 Typhoea 鏍锋湰鏄犲皠鎻愪緵浜嗚瘉鎹紝浣嗘病鏈夋妸鏁扮粍搴忓彿璇佹槑涓洪€氱敤韬唤鍚堝悓锛屼篃灏氭湭鍙栧緱鍘熺敓
姣忕偣 depth 缂撳瓨銆?

鎬寧鑽掗噹 Chain2 鍒欐妸瑙掑害鍗婂緞銆佹ā寮忓拰瑙掑害闄愬埗鏂瑰悜浣滀负鑺傜偣渚ф暟鎹紝骞剁敱鐙珛 `_ANGLE_LIMIT` 瀵硅薄鎺у埗姣忎釜閿ヤ綋
鏈濆悜锛汻E Chain Editor 杩樻彁渚涘榻愭柟鍚戝拰娌块摼璁剧疆鍗婂緞鍧″害鐨勬搷浣溿€傚叾浠撳簱璁板綍涔熸槑纭瓨鍦ㄦ煇浜涢鍚?灞炴€ф爣蹇椾笅閿ヤ綋
鏂瑰悜鐩稿弽鐨勯棶棰橈細https://github.com/NSACloud/RE-Chain-Editor 銆傚洜姝?EIEM 鍙互閲囩敤绫讳技鐨勯敟褰氦浜掞紝浣嗕笉鑳藉鍒?
Chain2 鐨勮妭鐐瑰瓧娈垫垨鏂瑰悜瀵硅薄銆傛纭殑 EIEM 棰勮搴斾粠缁堟湯鍦?baseline 鐖跺瓙杈逛笌鍩哄噯濮挎€佺敓鎴愰敟杞达紝浠?
`limitAngle.value 脳 curve(depth)` 浣滀负鍗婅锛涘叧闂?`useAngleLimit` 鏃堕殣钘忔垨鐏版樉銆傜簿纭樉绀哄墠杩橀渶鏄庣‘ BoneCloth
depth 鐨勭敓鎴愯鍒欙紝骞剁敤鍘熺敓鍦烘櫙 Gizmo 鎴栨眰瑙ｇ粨鏋滀氦鍙夐獙璇侀敟杞达紝涓嶈兘浠呮寜 Blender bone tail 鐚滄柟鍚戙€?

### 20.34 Blender 0.17.0锛氳搴﹂檺鍒堕敟涓庡師鐢?depth 瑙勫垯

鏈疆缁х画闄愬畾鍦?Blender 浣滆€呭伐鍏枫€佸彧璇讳簩杩涘埗璋冩煡鍜屾枃妗ｏ紝娌℃湁淇敼鎴栭儴缃?DLL锛屼篃娌℃湁鍐欏叆娓告垙鐩綍銆?
`VirtualMesh.CreateVertexRootAndDepth`锛堟湰鏈?`GameAssembly.dll` RVA `0x374B590`锛変細瀹夋帓
`BaseLine_CalcMaxBaseLineLengthJob`锛涘悗鑰呯殑 `Execute`锛圧VA `0x313FED0`锛夌粰鍑轰簡 depth 鐨勫疄闄呯畻娉曘€傚畠鍙
MOVE 灞炴€э紙浣?2锛夌殑鐐瑰伐浣滐細浠庡綋鍓嶇偣娌?`vertexParentIndices` 鍚戜笂锛岀疮璁?`localPositions` 鐨勭埗瀛愯窛绂伙紝鍖呭惈鍒?
棣栦釜闈?MOVE 鐖剁偣鐨勬渶鍚庝竴娈碉紱闅忓悗鍙栧叏缁勬渶澶х疮璁￠暱搴︼紝鎶婃墍鏈夌偣鐨勭疮璁″€奸櫎浠ヨ鏈€澶у€煎苟閽冲埗鍒?0锝?銆?
鎵€浠?depth 鏄?*缁勫唴缁熶竴鐨勫綊涓€鍖栫疮璁￠娈甸暱搴?*锛屼笉鏄暟缁勫簭鍙锋垨楠ㄩ灞傛暟锛涘涓牴鍜屽垎鍙変細鍏卞悓浣跨敤鏈€闀胯矾寰?
浣滀负鍒嗘瘝銆?

0.17.0 鎹涓?`useAngleLimit=true` 鐨勫師鐢?BoneCloth 缁勭敓鎴愰粍鑹插崐閫忔槑瑙掑害閿ャ€傝鍥惧厛鐢ㄧ粍灞€閮ㄩ潤姝綅缃妸
Selection 鐐逛笌 `rootBones` 灞曞紑鐨?Transform 鍘熺偣鍋氫竴瀵逛竴鍑犱綍鍖归厤锛屽啀涓烘瘡鏉￠潪闆?MOVE 鐖跺瓙杈瑰缓绔嬮敟闈細閿ュ皷
浣嶄簬鐖剁偣锛岃酱娌块潤姝?baseline 鐖剁偣鈫掑瓙鐐癸紝鐞冮潰鍗婂緞鍙栬楠ㄦ闀垮害锛屽崐瑙掍负
`limitAngle.value 脳 limitAngle.curve(depth)`銆傝繖绉嶇悆闈㈡瀯閫犲湪 90掳 鏃跺舰鎴愭湁闄愬渾鐩橈紝涓嶄細浜х敓鏃犻檺閿ュ簳銆?
Typhoea 瑁欐憜銆侀暱椹熬銆佷晶闀垮彂鍜屽熬宸村洓涓惎鐢ㄧ粍鍧囪兘鐢熸垚閿ヤ綋锛涘叧闂€诲紑鍏崇殑涓冪粍涓嶆樉绀恒€傚嚑浣曞尮閰嶅彧鏈嶅姟瑙嗗浘锛?
娌℃湁鎶?Selection 鏁扮粍涓嬫爣鏀瑰啓鎴愰€氱敤楠ㄩ韬唤銆丮esh palette銆乻etupIndex 鎴?Animator 鍐欏洖妲姐€?

Group Empty 鐨勫璞″睘鎬х幇鍦ㄦ妸**鍚敤瑙掑害闄愬埗銆佸熀纭€瑙掑害銆侀檺鍒跺垰搴?*鏀惧湪鍚屼竴鍖哄煙锛岀洿鎺ヤ唬鐞嗗師瀛楁銆?
`limitAngle.useCurve` 淇濇寔鐙珛锛欸raph Editor 涓搴︽洸绾挎湭闈欓煶锛屽彧鑳借鏄庝娇鐢ㄦ洸绾匡紝涓嶈兘璇存槑鎬荤害鏉熷凡缁忓紑鍚€?
Action/F-Curve 鐨勪慨鏀归€氳繃 depsgraph 鏍囪骞剁煭寤惰繜鍚堝苟鍒锋柊锛屽彧閲嶅缓璇ョ粍鐨勮搴﹂敟锛涙櫘閫氶噸鍔涖€侀樆灏肩瓑鍙傛暟浠嶄笉
閲嶅缓鏁村杈呭姪鍑犱綍銆俇nity 鏇茬嚎鐨勬棤鏉冮噸鍜屽姞鏉?Bezier 鎵嬫焺閮藉弬涓庨瑙堬紝骞跺湪搴旂敤/瀵煎嚭鏃舵妸鍒囩嚎銆?
`weightedMode`銆乣inWeight/outWeight` 鍐欏洖鍘熷瓧娈碉紝鍏抽敭甯ф暟閲忎粛淇濇寔涓嶅彉銆?

閿ヤ綋琛ㄨ揪闈欐浣滆€呭熀鍑嗭紝涓嶆槸 Blender 鍐呯殑鐗╃悊姹傝В銆傜粓鏈湴杩愯鏃朵細渚濇嵁 `animationPoseRatio` 鍦ㄥ垵濮嬪Э鎬佸拰褰撳墠
鍔ㄧ敾濮挎€佷箣闂村舰鎴愬姩鎬?baseline锛屽洜姝よ鑹茶繍鍔ㄦ椂姹傝В鍣ㄤ娇鐢ㄧ殑鐬椂閿ヨ酱涓嶄細鐢遍潤鎬佽緟鍔╃綉鏍奸€愬抚妯℃嫙銆傝闄愬埗涓嶅奖鍝?
缂栬緫鍩虹瑙掋€佹洸绾垮拰鍒氬害锛屼篃閬垮厤涓虹粓鏈湴缁勪吉閫?Chain2 鐙湁鐨勮妭鐐硅搴︽柟鍚戝璞°€?

Blender 5.0.1 鐨勭湡瀹?Typhoea 娴嬭瘯纭 4 涓惎鐢ㄧ粍鍏辩敓鎴?54 涓潪闆堕娈甸敟浣擄細瑁欐憜 29銆侀暱椹熬 12銆佷晶闀垮彂 6銆?
灏惧反 7锛涘熬宸村湪 depth `0.5` 鐨?60掳 鍩虹瑙掔粡绾挎€ф洸绾垮緱鍒?30掳銆傚疄鏃舵祴璇曟妸瑁欑怀鏈鏇茬嚎鍊嶇巼浠?1 鏀逛负 0.5锛?
鏃犻渶鎵嬪姩鍒锋柊锛屽叾閿ヤ綋鏈鍗婅浠?60掳 鏇存柊涓?30掳锛涢殢鍚庡凡鎭㈠鏇茬嚎鍜屽叧闂复鏃舵€诲紑鍏炽€傚綋鍓?Blender 浼氳瘽鐑噸杞戒负
0.17.0锛屼繚鐣?11 缁勩€?7 纰版挒浣擄紝杈呭姪鏄剧ず涓?65 涓?Mesh锛堝叾涓?4 涓敟浣撳璞★級銆? 涓?Curve锛涗袱涓?Physics
depsgraph 澶勭悊鍣ㄥ悇涓€涓紝娌℃湁淇濆瓨鐢ㄦ埛 `.blend`銆?

瀹屾暣 Blender 娴嬭瘯杩愯 12 椤癸紝7 椤归€氳繃锛? 椤瑰洜鏈彁渚涙甯告暣鍖呮牱鏈垨 MSVC 鐜鑰岃烦杩囷紝0 椤瑰け璐ワ紱鐪熷疄婧愭祴璇?
瑕嗙洊寮€鍏冲垎绂汇€乨epth/瑙掑害閲囨牱銆佸疄浣撲笌绾挎閿ヤ綋銆佹洸绾垮啓鍥炪€佹簮绮剧‘寰€杩斿強淇濆瓨閲嶅紑銆備竷涓繍琛屾枃浠跺凡閫愪竴鍝堝笇鍚屾鍒?
`E:\vscode\EIEM_Blender`銆傚畨瑁呭寘涓?`bin/EIEM_Blender-0.17.0-angle-limit-cones.zip`锛屽ぇ灏?90137 瀛楄妭锛?
SHA256锛歚55EE280211E044B403B1F5874D80BA277AF1A529B363BBC528CB54F7C5CE63C5`锛涜В鍘嬪悗鐨勫寘閫氳繃鎻掍欢鍙戠幇鍜屼笁杞噸杞姐€?

### 20.35 Blender 0.18.0锛氱墿鐞嗕綔鑰呬唬鐮佹暣鐞?

鏈疆鍙暣鐞?Blender 浣滆€呭伐鍏峰拰娴嬭瘯锛屾病鏈変慨鏀?DLL銆丳hysics 璧勬簮鍚堝悓鎴栨父鎴忕洰褰曘€傚鏌ヨ寖鍥翠负
`eiem_blender_addon.py` 涓庡洓涓?Physics 妯″潡锛屽叡鐢?6452 琛岄檷鑷?6364 琛岋紱琛屾暟涓嶆槸鐩爣锛屽噺灏戠殑 88 琛屽潎瀵瑰簲
宸茬粡纭鐨勯噸澶嶅疄鐜版垨鏃犺皟鐢ㄥ叆鍙ｃ€?

- 鍒犻櫎浜嗕粠鏈璋冪敤鐨勬棫 `collider_paths()`锛屾妸涓夌纰版挒浣撶殑绾挎璺緞缁熶竴鍒板疄闄呬娇鐢ㄧ殑
  `collider_wire_paths()`锛涘疄浣?绾挎棰勮缁х画瑕嗙洊鐞冦€佽兌鍥婂拰鏃犻檺骞抽潰銆?
- Curve 涓?Mesh 杈呭姪瀵硅薄鍘熷厛鍚勮嚜閲嶅闆嗗悎閾炬帴銆佺埗绾с€佷笉鍙覆鏌?涓嶅彲閫夋嫨銆佺┛閫忓拰鏍囪璁剧疆锛岀幇鍦ㄧ粺涓€缁忚繃
  `finish_visual()`銆傛柊澧?v1 涓庡師鐢?v2 鐨勮妭鐐?杩炵嚎涔熷叡鐢ㄥ悓涓€缁勬牱寮忓垎娲惧嚱鏁帮紝鍙繚鐣欏崐寰勫拰鍧愭爣鍩哄簳宸紓銆?
- 鍘熺敓缁勫眰绾ц繛绾夸笌瑙掑害閿ュ尮閰嶅師鍏堝悇鑷绠?Transform 涓栫晫鐭╅樀鍜岀粍浠跺眬閮ㄥ師鐐癸紝鐜板湪缁熶竴鐢?
  `component_local_graph()` 缁欏嚭锛岄伩鍏嶅悓涓€婧愬浘鍑虹幇涓ょ灞€閮ㄧ┖闂磋В閲娿€?
- 瀵硅薄灞炴€т腑閫愬叧閿抚鐨勨€滄洸绾垮弬鏁扳€濊〃涓?Blender Graph Editor 缂栬緫鍚屼竴浠芥姇褰憋紝灞炰簬閲嶅鎿嶄綔鐣岄潰锛屽凡缁忓垹闄ゃ€?
  鏃ュ父鏇茬嚎鍏ュ彛鍙繚鐣欌€滄煡鐪?/ 缂栬緫 9 鏉℃洸绾库€濓紱瀵煎嚭浠嶄細妫€娴?Action 绛惧悕骞惰嚜鍔ㄥ啓鍥炴簮 RNA 瀛楁锛岄珮绾ф簮瀛楁浠嶅彲
  绛涢€夊拰鍒嗛〉鏌ョ湅瀹屾暣 TypeTree 鏁版嵁銆傛病鏈夊垹闄ゆ洸绾跨紪瑙ｇ爜銆佸垏绾?鏉冮噸寰€杩旀垨婧愬瓧娈典繚鐣欍€?
- F-Curve 鍙樺寲鍘熷厛鍚屾椂缁忚繃鍘熺敓 depsgraph 澶勭悊鍣ㄥ拰 0.2 绉掔鍚嶈疆璇€傜洿鎺ヤ慨鏀?F-Curve 鍦?Blender 涓苟涓嶄繚璇?
  浜х敓绋冲畾鐨?Action depsgraph 鏇存柊锛屽洜姝や繚鐣欏彧鍦ㄢ€滃惎鐢ㄨ搴﹂檺鍒朵笖宸叉湁 EIEM Action鈥濈殑缁勫瓨鍦ㄦ椂杩愯鐨勮疆璇紝
  鍒犻櫎鍘熺敓 depsgraph 澶勭悊鍣ㄣ€倂1 楠ㄦ灦闈欐濮挎€佷粛鐢变綔鑰呮ā鍧楀敮涓€鐨?depsgraph 澶勭悊鍣ㄥ悎骞跺埛鏂般€?
- 鈥滃埛鏂板綋鍓嶉摼鏄剧ず鈥濅笌鍒涘缓銆佸姞鍏ャ€佺Щ闄ゅ強 Armature 鑷姩閲嶅缓閲嶅锛屾寜閽拰鎿嶄綔鍒嗘敮宸插垹闄ゃ€傜鎾炰綋鐨勨€滃埛鏂伴楠?
  缁戝畾鈥濅繚鐣欙紝鍥犱负瀹冧細閲嶇畻 Child Of 閫嗙煩闃碉紱鈥滄暣鐞嗗綋鍓?Rig鈥濅繚鐣欑敤浜庢棫 `.blend` 闆嗗悎杩佺Щ銆?

浠ヤ笅鑳藉姏缁忓鏌ュ悗鏄庣‘淇濈暀锛歚Groups / Colliders / Visuals` 鍒嗗埆鎵胯浇缁勬暟鎹€佸彲鍏变韩纰版挒缁勪欢鍜屽彲闅忔椂閲嶅缓鐨勮鍥撅紱
v1 鏂板浣滆€呮牸寮忎笌 v2 鍘熼厤缃線杩斿叿鏈変笉鍚屽悎鍚岋紱鍘熺敓缁勫畬鏁存ā鏉垮鍒朵笌鏄犲皠鍒?v1 鐨勪簲鏍囬噺澶嶅埗鐩爣涓嶅悓锛涘疄浣撲笌
璇婃柇绾挎鏄悓涓€鏁版嵁鐨勪袱绉嶈鍥俱€傝繖浜涗笉鏄噸澶嶅姛鑳姐€?

Blender 5.0.1 鐨?12 椤瑰涓诲洖褰掍负 7 椤归€氳繃銆? 椤瑰洜鏈彁渚涙甯告暣鍖呮牱鏈垨 MSVC 鑰岃烦杩囥€? 椤瑰け璐ャ€傜湡瀹?
Typhoea 娴嬭瘯瑕嗙洊 11 缁勩€?7 纰版挒浣撱€?4 涓搴﹂敟锛屾柊澧炰簡鈥滅洿鎺ユ敼 F-Curve 鍚庝粎鐢辫疆璇㈡爣璁板苟閲嶅缓褰撳墠缁勨€濈殑
楠岃瘉锛涙敞鍐屾祴璇曠‘璁や綔鑰呮ā鍧楁湁 1 涓?depsgraph 澶勭悊鍣ㄣ€佸師鐢熸ā鍧椾负 0锛屽苟閫氳繃涓夎疆娉ㄥ唽/鍗歌浇銆備竷涓繍琛屾枃浠跺凡閫愪竴
鍝堝笇鍚屾鍒?`E:\vscode\EIEM_Blender`銆傚畨瑁呭寘涓?`bin/EIEM_Blender-0.18.0-code-cleanup.zip`锛屽ぇ灏?90082 瀛楄妭锛?
SHA256锛歚67090877CFD55662236EBE522B03872BF5AEB498835ABA912D983F1E11F08B2C`锛涜В鍘嬪悗鐨勫寘閫氳繃鎻掍欢鍙戠幇鍜屼笁杞噸杞姐€?
褰撳墠浜や簰 Blender 鐨?MCP 鐩戝惉绔彛浠嶅瓨鍦紝浣嗕袱娆¤皟鐢ㄥ潎鏈湪瓒呮椂鍐呰繑鍥烇紝鎵€浠ユ湰杞笉鎶婅浼氳瘽璁颁负鐑噸杞藉畬鎴愶紝
涔熸病鏈変繚瀛樼敤鎴?`.blend`銆?

### 20.36 鑳搁儴楠ㄩ椹卞姩鏉ユ簮锛氱绾胯竟鐣屼笌 v72 杩愯鏃跺眬閮ㄥ彉鎹㈤噰鏍?

鏈疆鍏堟部 Typhoea 妯″瀷 Prefab 鐨?Animator 寮曠敤缁х画杩借釜銆傛柊澧炵殑鐙珛绂荤嚎鍛戒护
`EndfieldVfsProbe --inspect-prefab-animation` 浼氫粠鍚屼竴 VFS 鎸囩汗鍜屼緷璧栭棴鍖呬腑瑙ｆ瀽 Animator銆?
RuntimeAnimatorController銆丄nimationClip 涓?Transform GenericBinding銆傚
`chr_0034_typhoea_postmodel.prefab` 鐨勫疄闄呯粨鏋滀负锛? 涓?Animator锛屼絾
`m_Controller=null`銆? 涓彲瑙ｆ瀽 Clip銆? 鏉?Transform binding銆傚洜姝わ紝妯″瀷鍖呮湰韬笉鑳藉洖绛?
`breast_base_L/R_a_01/02_jnt` 鏄惁鏈夊姩鐢绘洸绾匡紱瑙掕壊绯荤粺鍦ㄨ繍琛屾椂瑁呴厤鎺у埗鍣ㄣ€傛満鍣ㄥ彲璇荤粨鏋滀綅浜?
`bin/diagnostics/typhoea-animation-bindings-20260909.json`銆傝繖椤圭粨鏋滃彧纭畾绂荤嚎杈圭晫锛屼笉绛変簬
鈥滆兏楠ㄦ病鏈夊姩鐢烩€濄€?

涓哄尯鍒嗏€滆兏楠ㄥ彧缁ф壙 Spine2 鐨勪笘鐣岃繍鍔ㄢ€濅笌鈥滆兏楠ㄥ眬閮?TRS 姣忓抚琚煇涓繍琛屾椂绯荤粺鍐欏叆鈥濓紝v72 鍦?
`eiem_native_physics_diagnostic.h` 涓姞鍏ョ嫭绔嬬殑鑳搁儴杩愬姩瑙傚療銆傚畠涓嶈繘鍏?Dump 鎴栦富鍔熻兘 UI锛?
闅忕幇鏈夎嚜鍔ㄨ瘖鏂惎鍔紝骞跺湪宸叉湁 `SolverManager.LateUpdate` 鍘熻皟鐢ㄨ繑鍥炲悗姣?50 ms 閲囨牱
`Bip001_Spine2`銆佸乏鍙?`breast_base_*_a_01/02_jnt` 鐨勫眬閮?涓栫晫浣嶇疆鍜屾棆杞€傛瘡涓ā鍨嬫渶澶氳褰?
3600 娆★紱閫€鍑烘椂鍦ㄥ悓涓€ TSV 灏鹃儴鍐欏叆鍚勯楠肩浉瀵归甯х殑鏈€澶у眬閮?涓栫晫浣嶇疆宸拰鍥涘厓鏁拌宸€傝緭鍑鸿矾寰勪负
`plugin/physics_diagnostics/chest_motion_<pid>_<tick>.tsv`銆傝嫢鑳搁灞€閮ㄥ彉鍖栨帴杩戦浂銆佷笘鐣屽彉鍖栨槑鏄撅紝
璇佹嵁鏀寔缁ф壙韬共锛涜嫢灞€閮ㄦ棆杞垨浣嶇疆鏄庢樉鍙樺寲锛屽垯鍙‘璁ゅ瓨鍦ㄥ眬閮ㄥ啓鍏ワ紝浣嗕粎鍑繖涓噰鏍风偣杩樹笉鑳芥妸鍐欏叆鑰?
杩涗竴姝ユ柇瑷€涓?Animator銆両K 鎴栧叾浠栬繍琛屾椂浣滀笟銆?

`test_native_physics_trace.py`銆丒ndfieldVfsProbe .NET 9 Release 鏋勫缓鍜屽畬鏁?`build.bat` 鍧囬€氳繃銆?
娓告垙鏈繍琛屾椂宸插浠藉師瀹夎 DLL/鏃ュ織鍒?
`E:\EIEM_Workspace\plugin-releases\before-v72-chest-motion-20260909-131214`锛屽苟閮ㄧ讲 6266880 瀛楄妭
璇婃柇 DLL锛涙瀯寤哄拰瀹夎 SHA256 鍧囦负
`4F1679649F535C51329DF4469645347622A544BC83CCDB148B3A3E9A4AB199DC`銆傚皻鏈惎鍔ㄦ父鎴忔垨鍙栧緱鑳搁閲囨牱锛?
鎵€浠ユ湰鑺傛殏涓嶅啓杩愬姩鏉ユ簮缁撹銆?
#### v72 瀹炴満缁撴灉

PID 33520 姝ｅ父閫€鍑哄苟鐢熸垚 8703 琛屻€?822865 瀛楄妭鐨勮兏閮ㄨ繍鍔ㄨ褰曘€傛棩蹇楁妸鍏釜瑙傚療瀵硅薄鏄庣‘瀵瑰簲涓猴細
瑙掕壊妯″瀷銆佽鑹?UI銆佷袱涓?NPCAvatar.StartNPC 鎵€鏈夎€呫€佽兘鍔涘疄浣撳拰瀵硅瘽鏃堕棿绾垮璞°€傚垽鏂楠兼槸鍚﹁鐙珛椹卞姩
蹇呴』鐪嬪眬閮ㄦ棆杞紝璺ㄥ満鏅鑷寸殑 520.47 涓栫晫浣嶇疆宸笉鑳藉綋鎴愯兏閮ㄥ舰鍙樸€?

瑙掕壊妯″瀷杩炵画閲囨牱 626 娆°€傜浉瀵归甯э紝Bip001_Spine2 鐨勬渶澶у眬閮ㄨ宸负 35.65掳锛涘乏鑳哥涓€绾?绗簩绾у垎鍒负
43.38掳/83.80掳锛屽彸鑳哥涓€绾?绗簩绾у垎鍒负 40.03掳/69.35掳銆傝鑹?UI 杩炵画閲囨牱 104 娆★紝瀵瑰簲鍊间负
Bip001_Spine2 23.41掳銆佸乏鑳?8.56掳/18.79掳銆佸彸鑳?6.48掳/18.15掳銆傝兏閮ㄧ浜岀骇鑺傜偣鐨勫眬閮ㄤ綅缃繎浼间笉鍙橈紝
浣嗗眬閮ㄦ棆杞槑鏄炬敼鍙橈紝鎵€浠ヨ繖浜涜妭鐐瑰瓨鍦ㄦ棆杞€氶亾锛汢ip001_Spine2 鐨勪笘鐣屽彉鎹㈢户鎵挎棤娉曞崟鐙В閲婅繖涓粨鏋溿€?

涓や釜 NPC 閮界敱绮剧‘ owner 鏃ュ織纭銆侾_actor_typhoea_01 鍦?211 娆￠噰鏍锋湡闂村畬鍏ㄩ潤姝紱鍙︿竴涓?
chr_0034_typhoea_postmodel 鐨?Bip001_Spine2 灞€閮ㄨ宸负 3.02掳锛屽乏鍙宠兏绗竴绾у眬閮ㄨ宸害
0.11掳锛岀浜岀骇鍦ㄨ鏃堕棿绐楀彛淇濇寔灞€閮ㄤ笉鍙樸€傝繖璇存槑 NPC 鏄惁鍑虹幇鑳搁儴灞€閮ㄥ彉鍖栧彇鍐充簬褰撴椂鏄惁瀹為檯鎾斁浜?
鐩稿叧鍔ㄤ綔銆佹槸鍚﹁鏇存柊鍙婅鍔ㄤ綔鏄惁鍚浉搴旈€氶亾锛涗笉鑳芥嵁涓€涓潤姝?NPC 鎺ㄦ柇 NPC 楠ㄦ灦娌℃湁鑳搁儴椹卞姩銆?

鍘熺敓閰嶇疆鍏崇郴淇濇寔涓嶅彉锛歍yphoea 鐨?11 涓?BeyondBoneCloth 涓病鏈変换浣?rootBones 鎸囧悜鑳搁儴鑺傜偣銆?
鑳搁儴鐩稿叧鐨勪笁涓粍浠跺潎涓鸿兌鍥婄鎾炰綋锛歋pine2_Breast銆佸乏鑳镐竴绾у拰鍙宠兏涓€绾э紱涓夎€呭彧琚?
MBC_Typhoea_Hair_Front_Side_Long 鐨?collider list 寮曠敤銆傚洜姝わ紝Blender 涓笉搴旇櫄鏋勪竴涓?
鈥滆兏閮?BoneCloth 鐗╃悊缁勨€濄€傝兏閮ㄩ楠艰嚜韬細琚繍琛屾椂鍔ㄧ敾閾捐矾鍐欏叆锛岃€屼緷闄勫叾涓婄殑纰版挒浣撻殢楠ㄩ绉诲姩骞朵緵
渚ч暱鍙戠粍纰版挒銆傚綋鍓嶈瘉鎹珮搴︽敮鎸佽繍琛屾椂 Animator/鍔ㄧ敾鎺у埗閾捐矾锛屼絾閲囨牱鐐瑰彧鑳借瘉鏄庡抚鏈眬閮ㄥ啓鍏ワ紝
灏氫笉鑳藉尯鍒?AnimationClip Transform 鏇茬嚎涓庡彟涓€涓?Animator job锛屼篃涓嶈兘澹扮О姣忎釜 Clip 閮界粰鑳搁鎵撲簡
鍏抽敭甯с€?

瀹屾暣璇佹嵁褰掓。浜?bin/diagnostics/v72-chest-motion-runtime-33520/锛?
analysis.json銆佸師濮?TSV銆佸畬鏁?EIEM 鏃ュ織銆佸師鐢?Physics 杩愯蹇収鍙婄绾?Animator 鎶ュ憡銆?
鍘熷 TSV SHA256 涓?
99A8B14251133B08E76D429BB57CA9C4E333D9A9196C1FF25ED595E9FF97D369銆?

### 20.37 Blender 0.19.0锛氱墿鐞嗛摼鑺傜偣纰版挒鍗婂緞鍙鍖?

鏈疆鍙慨鏀?Blender 鍘熺敓鐗╃悊閾鹃瑙堛€佹祴璇曞拰鏂囨。锛屾病鏈変慨鏀?DLL銆佹父鎴忕洰褰曟垨鐙珛 Collider 缂栬緫銆傛鍓嶅師鐢熺粍鐨?
FIXED/MOVE Selection 鐐逛娇鐢ㄥ浐瀹?0.006 绫崇悆锛屽彧鑳借〃杈捐妭鐐硅鑹诧紝涓嶈兘琛ㄨ揪 `serializeData.radius`銆?.19.0 鏀逛负
鎸夋瘡涓?Selection 鐐圭殑鍘熺敓 depth 璁＄畻 `radius.value 脳 radius.curve(depth)`锛沗radius.useCurve=false` 鏃跺€嶇巼涓?1銆?
depth 缁х画浣跨敤 20.34 鑺傚凡缁忎粠 `VirtualMesh.CreateVertexRootAndDepth` 鏂规硶浣撶‘璁ょ殑缁勫唴鏈€闀跨疮璁￠娈甸暱搴﹁鍒欍€?
FIXED 涓?MOVE 鍒嗗埆淇濈暀姗欒壊鍜岃摑鑹诧紝瀹炰綋妯″紡鐢熸垚鐪熷疄灏哄浣庨潰鏁扮悆锛岀嚎妗嗘ā寮忕敓鎴愪笁涓浜ゅ渾鐜紱IGNORE 涓嶆槸
妯℃嫙鐐癸紝缁х画浣跨敤鍥哄畾鐏拌壊鎷撴墤鏍囪锛屼笉铏氭瀯纰版挒鍗婂緞銆?

瑙掑害涓庡崐寰勫叡鐢ㄥ悓涓€涓€滃熀纭€鍊?脳 鍙€夋洸绾库€濇眰鍊煎嚱鏁般€傚師鏉ョ殑瑙掑害涓撶敤鍒锋柊闃熷垪鏀逛负缁勯瑙堝埛鏂伴槦鍒楋紱淇敼鍩虹
鍗婂緞銆乣radius.useCurve`銆佸崐寰勫叧閿抚/鍒囩嚎锛屾垨鐩存帴鍦?Graph Editor 璋冩暣鍗婂緞 F-Curve锛岄兘浼氬悎骞堕噸寤哄綋鍓嶇粍鐨勮妭鐐?
鍗婂緞涓庤搴﹂敟銆傛櫘閫氶樆灏笺€侀噸鍔涚瓑涓嶅奖鍝嶅嚑浣曠殑瀛楁浠嶄笉閲嶅缓棰勮銆侳-Curve 杞鐜板湪璺熻釜鎵€鏈夊凡寤虹珛 EIEM Action
鐨勫師鐢熺粍锛屽洜姝ゅ嵆浣胯缁勬病鏈夊紑鍚搴﹂檺鍒讹紝鍗婂緞鏇茬嚎涔熶細瀹炴椂鏇存柊銆?

Blender 5.0.1 鐪熷疄 Typhoea `components.json` 娴嬭瘯楠岃瘉 11 涓粍銆?64 涓?Selection 鐐圭殑鍗婂緞鏍锋湰涓庡師鐢熸洸绾挎眰鍊?
涓€鑷达紱瀹炰綋鐞冮椤剁偣鍒扮悆蹇冪殑璺濈绛変簬鏍锋湰鍗婂緞锛岀嚎妗嗘瘡鐐规伆鏈変笁涓渾鐜€傛祴璇曡繕鐩存帴淇敼涓€涓惎鐢ㄦ洸绾跨殑缁勬湯绔?
鍗婂緞鍊嶇巼锛岀‘璁ゆ棤闇€鎵嬪姩鍒锋柊鍗冲彲閲嶅缓鑺傜偣鐞冨苟鍦ㄦ仮澶嶅悗淇濇寔婧愭暟鎹簿纭線杩斻€傛櫘閫?package 鐨?Mesh/Rig/Physics
鑱斿悎瀵煎叆涓庢彃浠朵笁杞敞鍐?鍗歌浇缁х画閫氳繃銆傛湰缁撴灉楠岃瘉浣滆€呰鍥惧拰搴忓垪鍖栧悎鍚岋紝涓嶇瓑鍚屼簬 DLL 杩愯鏃剁鎾炲搷搴旈獙璇併€?

涓冧釜杩愯鏂囦欢宸查€愪竴鍝堝笇鍚屾鍒?`E:\vscode\EIEM_Blender`銆傚畨瑁呭寘涓?
`bin/EIEM_Blender-0.19.0-node-radius-preview.zip`锛屽ぇ灏?89988 瀛楄妭锛孲HA256锛?
`36C39A46ADA4D9C86E334019F4A16BEE480574F31B1A7EE834D11392660FECA3`锛沍IP 鍐呭瀹屾暣鎬у強浠庡紑鍙戠洰褰曡繘琛岀殑涓夎疆
Blender 鎻掍欢娉ㄥ唽/鍗歌浇鍧囧凡楠岃瘉銆?

### 20.38 Blender 0.20.0 / 浣滆€?v3锛氭柊澧為摼鑺傜偣鍗婂緞鏇茬嚎涓?DLL 閰嶇疆鏄犲皠

鏈疆淇浜嗘柊澧炰綔鑰呴摼涓庡師鐢熷鍏ラ摼涔嬮棿鐨勪笉涓€鑷淬€俙BeyondBoneCloth.serializeData.radius` 鏄ā鎷熺偣鑷韩鐨?
纰版挒鍘氬害閰嶇疆锛涘畠涓庣粍寮曠敤鐨勭嫭绔嬬悆銆佽兌鍥娿€佸钩闈?Collider 鏄袱绫绘暟鎹€?.19.0 鍙粰鍘熺敓瀵煎叆缁勬樉绀?
`radius.value 脳 curve(depth)`锛岃€屾柊澧炰綔鑰呯粍浠嶄娇鐢ㄥ浐瀹氳楗扮偣锛屼綔鑰呮枃浠朵篃鍙繚瀛樹簲涓爣閲忋€?.20.0 灏嗚妭鐐瑰崐寰?
鍔犲叆鏂板閾剧殑鍞竴浣滆€呮暟鎹簮锛岀嫭绔?Collider 鏈疆淇濇寔鍘熺姸銆?

浜岃繘鍒舵柊澧炰綔鑰呮牸寮忎娇鐢ㄧ増鏈?3锛屽洜涓虹増鏈?2 宸茶 `native-authoring` 婧愬浘鍗犵敤銆倂3 鍦ㄦ瘡涓粍鐨勪簲涓熀纭€鏍囬噺鍚庝繚瀛橈細

- 鍩虹鍗婂緞 `value` 涓?`useCurve`锛?
- 2锝?4 涓叧閿抚锛屾瘡甯у惈褰掍竴鍖栨椂闂淬€佸€嶇巼銆佽繘鍏?绂诲紑鍒囩嚎銆乣weightedMode`銆佽繘鍏?绂诲紑鏉冮噸锛?
- 婧愭洸绾跨殑 pre/post infinity 涓?rotation order 鍏冩暟鎹€?

Python 涓?C++ 鍧囦弗鏍兼牎楠屽叧閿抚浣嶄簬 0锝?銆佹椂闂撮€掑銆佹湁闄愭暟鍊煎拰鏉冮噸鑼冨洿锛涗笉寮哄埗棣栨湯甯ф伆濂戒负 0/1锛屽洜涓?
Typhoea 鐏缁勭殑鐪熷疄鍗婂緞鏇茬嚎鏈抚涓?`0.9974365234375`锛岄暱椹熬缁勭殑棣栨湯甯т负
`0.001953125` / `0.9961351752281189`銆傛棫 v1 鏂囦欢浠嶅彲璇诲彇鍜屽師鏍烽噸缂栫爜锛?
杞藉叆 Blender 鍚庝細鑾峰緱 0.006 绫炽€佸€嶇巼鎭掍负 1 鐨勯粯璁ゅ崐寰勬洸绾匡紝鍐嶆瀵煎嚭浣跨敤褰撳墠 v3銆傛病鏈夋妸鍘熺敓婧愬浘 v2 寮哄埗
闄嶇骇涓轰綔鑰?v3銆?

鏂板缁?Empty 鐨勫璞″睘鎬х幇鍦ㄧ洿鎺ユ樉绀衡€滆妭鐐瑰熀纭€鍗婂緞鈥濆拰鈥滀娇鐢ㄥ崐寰勬洸绾库€濄€傗€滃湪鏇茬嚎缂栬緫鍣ㄦ煡鐪嬧€濇墦寮€灞炰簬璇?
Empty 鐨?Blender Action/F-Curve锛涙í杞?0锝?00 瀵瑰簲缁勫唴浠庡浐瀹氭牴娌?MOVE 鐖堕摼绱鐨勯娈甸暱搴︼紝鍐嶉櫎浠ユ湰缁勬渶杩?
鏈闀垮害锛岀旱杞翠负鍗婂緞鍊嶇巼銆傜敤鎴峰彲鍦?0锝?00 鍐呭鍒犲叧閿抚骞剁紪杈?Bezier 鎵嬫焺銆傛洸绾裤€侀潤闊崇姸鎬佹垨鍩虹
鍗婂緞鍙樺寲鐢?0.2 绉掔鍚嶈疆璇㈠悎骞跺埛鏂帮紝鏃犻渶鎵嬪姩閲嶅缓銆侳IXED/MOVE 鐐逛娇鐢ㄧ湡瀹炲昂瀵稿疄浣撶悆鎴栦笁涓嚎妗嗗渾鐜紝IGNORE
浠嶅彧鏄嫇鎵戞爣璁般€傚鍒朵綔鑰呯粍浼氬鍒跺崐寰勬洸绾匡紱浠庡師鐢熸ā鏉挎槧灏勬椂浼氬鍒朵簲涓熀纭€鍙傛暟鍙婂師鐢熻妭鐐瑰崐寰勫畬鏁村叧閿抚銆?

DLL 閰嶇疆鏄犲皠渚濇嵁褰撳墠娓告垙 `global-metadata.dat` 鐨勫彲鏍稿鍚堝悓瀹炵幇锛歚CurveSerializeData` 鏄庣‘鍚?
`value:System.Single`銆乣useCurve:System.Boolean`銆乣curve:UnityEngine.AnimationCurve`锛屽苟鍏锋湁
`SetValue(System.Single, UnityEngine.AnimationCurve)`锛沗AnimationCurve` 鍏锋湁
`.ctor(UnityEngine.Keyframe[])` 涓?`get_keys()`锛沗Keyframe` 鐨勪竷涓瓧娈甸『搴忓搴?time/value/in/out tangent銆?
weighted mode銆乮n/out weight锛屽€肩被鍨嬪ぇ灏忎负 28 瀛楄妭銆傝繍琛屾椂鎸夌▼搴忛泦銆佸懡鍚嶇┖闂淬€佺被鍚嶃€佹柟娉曠鍚嶅拰瀛楁绫诲瀷瑙ｆ瀽锛?
鍚屾椂鏍稿 `il2cpp_class_value_size==28`锛屾病鏈夊姞鍏ユ父鎴忕増鏈瓧娈靛亸绉汇€倂3 閰嶇疆鍒涘缓 `Keyframe[]` 鍜?
`AnimationCurve`锛岃皟鐢?`CurveSerializeData.SetValue`锛屽啓鍏?`useCurve`锛岄殢鍚庝粠鏂板缓 `ClothSerializeData`
閫愰」鍥炶鍩虹鍊笺€佸紑鍏炽€佹洸绾垮紩鐢ㄥ強鍏抽敭甯у瓧鑺傦紱浠讳綍涓嶄竴鑷撮兘涓嶄細鍙戝竷璇ユ壒鑽夌銆傛洸绾垮彧鍦?0锝? depth 鍐呮眰鍊硷紝
鏈疆鏈妸搴忓垪鍖?infinity/rotation 鍏冩暟鎹寽鍐欐垚 Unity 鐨勫叕寮€ WrapMode銆?

鏈€缁堥拡瀵规€ч獙璇佷负 36 椤归€氳繃銆? 璺宠繃銆? 澶辫触锛岃鐩?Python/C++ v1/v3 绾挎牸寮忋€佸潖鏂囦欢銆佽祫婧愪緷璧栥€侀厤缃簨鍔°€?
Blender 瀹炰綋鍗婂緞銆丗-Curve銆佸鍒躲€佷繚瀛橀噸寮€銆佹棫 v1 鍗囩骇銆佺粍鍚?Mod 鍜?Typhoea 鏂伴 fixture銆傚畬鏁?`build.bat`
鎴愬姛鐢熸垚 `bin/eiem.dll` 鍙婁袱涓唬鐞?DLL銆傚叏浠?219 椤逛腑 215 椤归€氳繃銆? 椤瑰洜缂哄皯鐪熷疄 Physics 鍖呯幆澧冨彉閲忚€岃烦杩囷紱
`test_material_baseline_lifecycle` 涓?`test_partner_controls` 涓や釜鏃㈡湁瀹夸富澶瑰叿鍥犲叾鐙珛缂哄け澹版槑鑰岀紪璇戝け璐ワ紝鏈秹鍙?
鏈疆淇敼鏂囦欢銆傚紑鍙戠洰褰曚竷涓繍琛屾枃浠堕€愪竴 SHA256 鐩稿悓锛屽苟閫氳繃 BlenderMCP 鐑噸杞藉埌 0.20.0锛涗細璇濅腑鐨?11 涓?
鍘熺敓缁勪繚鐣欍€傚畨瑁呭寘 `bin/EIEM_Blender-0.20.0-author-radius.zip` 涓?94493 瀛楄妭锛孲HA256锛?
`138C57B8BEA41C16D8D41DB108B2647C26B4B4E3B26F00E4314C39E53A465F36`锛孼IP 瀹屾暣鎬у強瑙ｅ帇鍚庣殑涓夎疆鎻掍欢娉ㄥ唽/鍗歌浇閫氳繃銆?

杩欎簺缁撴灉楠岃瘉浣滆€呬氦浜掋€佸簭鍒楀寲銆佸厓鏁版嵁瑙ｆ瀽鍜屾湭鎸傛帴閰嶇疆鑽夌銆傚畠浠繕娌℃湁璇佹槑 v3 鍗婂緞浼氬湪娓告垙涓骇鐢熼鏈熺鎾?
鍝嶅簲锛涜缁撹闇€瑕侀儴缃插悗瀵瑰悓涓€閾句娇鐢ㄦ槑鏄句笉鍚屽崐寰?鏇茬嚎鍋?A/B锛屽苟瑙傚療纰版挒璺濈銆傜嫭绔嬬悆銆佽兌鍥娿€佸钩闈?Collider
浠嶆湭鎺ュ叆鏂板缁勮繍琛屾椂銆?

### 20.39 Blender 0.21.0锛氫綔鑰呯粍鏀寔鍘熺敓寮忓 FIXED 鏍瑰垎鏀?

`maid.002` 鐨勫叚鏉′笁楠ㄨ鎽嗗垎鏀毚闇蹭簡浣滆€?v3 鐨勫崟鏍归檺鍒躲€傞娆′慨姝ｆ浘鎶婂叡鍚岄鏋剁埗绾?`Bip001_Spine1`
鑷姩鍔犲叆涓哄敮涓€ FIXED锛屽苟鎶?18 鏍硅楠ㄥ叏閮ㄦ爣涓?MOVE锛涚湡瀹炴簮鏁版嵁鏍稿璇佹槑杩欑缁撴瀯涓嶅繝瀹炪€傚師鐢?
`MBC_Typhoea_Cloth_Skirt` 鐨?`rootBones` 鏄竷鏍?`skirt_base_*_01_jnt`锛屾瘡鏍瑰湪 SelectionData 涓潎涓?FIXED锛?
瀹冧滑鐨勯鏋剁埗绾ф槸 `Bip001_Spine1`锛屼絾 Spine1 涓嶅睘浜庤鐗╃悊缁勩€傚悗缁?`02/03/04` 鑺傜偣涓?MOVE銆?

0.21.0 鍥犳鐩存帴鎵╁睍浣滆€呯粍鐨勬嫇鎵戣涔夎€屼笉鏀瑰彉 v3 浜岃繘鍒跺竷灞€锛氫竴涓粍鍙寘鍚竴涓垨澶氫釜鏍癸紝姣忎釜鏍瑰繀椤讳负
FIXED锛岄潪鏍逛笉寰楁爣涓?FIXED锛岀粍鍐呰嚦灏戞湁涓€涓?MOVE銆侭lender 閫夋嫨澶氭潯鍒嗘敮鏃朵繚鐣欏悇鍒嗘敮鏍癸紝涓嶅啀琛ュ叆鍏卞悓鐖堕锛?
鎵€鏈夊垎鏀叡浜粍鍙傛暟銆佽妭鐐瑰崐寰勬洸绾垮拰纰版挒浣撳紩鐢ㄣ€侰++ 璇诲彇鍣ㄩ噰鐢ㄧ浉鍚屾牎楠岋紱DLL 閰嶇疆鑽夌鏀堕泦鍏ㄩ儴鏍瑰苟閫愰」鍐欏叆
鏋勯€犲櫒鑷甫鐨?`rootBones` 鍒楄〃锛屽啀鏍稿鏁伴噺銆侀『搴忓拰瀵硅薄寮曠敤銆備袱鏍瑰涓婚厤缃€佷袱鍒嗘敮 Blender 鍒涘缓銆佽拷鍔犮€佷繚瀛?
閲嶅紑鍙?Python/C++ 寰€杩斿潎鏈夊洖褰掕鐩栥€?

褰撳墠 Blender 浼氳瘽涓殑 `maid.002 Skirt Physics` 宸叉敼涓?18 涓妭鐐癸細鍏牴 `maid_skirt_*_a_jnt` 涓?FIXED锛?
鍗佷簩鏍?`_b/_c_jnt` 涓?MOVE锛宍Bip001_Spine1` 鍙繚鐣欎负鍏牴楠ㄩ鐨勭埗绾с€傜嫭绔嬬鎾炰綋涓?v3 鍗婂緞鐨勬父鎴忓唴 A/B
杈圭晫涓嶅洜鏈妭鍙樺寲銆傛渶缁堜綔鑰呮牸寮忋€佽祫婧愩€丏LL 閰嶇疆銆丅lender 鍒涘缓/娉ㄥ唽鍜屾柊楠ㄥす鍏峰叡 38 椤归€氳繃锛屽畬鏁?
`build.bat` 閫氳繃銆傚畨瑁呭寘 `bin/EIEM_Blender-0.21.0-multi-root-groups.zip` 涓?94643 瀛楄妭锛孲HA256锛?
`C3E2A3B5D760156768C7AF27B1F9925BC54399060E5E1EE403251DD6CB49D220`锛沍IP 瀹屾暣鎬у強瑙ｅ帇鍚庣殑涓夎疆鎻掍欢娉ㄥ唽/鍗歌浇閫氳繃銆?

### 20.40 Blender 0.22.0锛氭樉寮忓弬鏁板壀璐存澘涓庡垏鎹㈡寜閿綍鍒?

鐗╃悊渚ф爮鍘熸湁涓€涓€滃鍒垛€濊彍鍗曪紝鍐呴儴鍚屾椂鏀剧疆澶嶅埗鏁翠釜缁勩€佽鍘熺敓妯℃澘銆佸簲鐢ㄥ師鐢熸ā鏉垮拰浠庢ā鏉挎槧灏勪綔鑰呭弬鏁帮紝
鐢ㄦ埛蹇呴』璁颁綇鎿嶄綔椤哄簭銆?.22.0 灏嗛灞傛敼涓轰袱涓‘瀹氬姩浣滐細**澶嶅埗鍙傛暟**涓?*绮樿创鍙傛暟**銆傚壀璐存澘淇濆瓨澶嶅埗鏃剁殑
鍊煎揩鐓э紝涓嶆槸鎸囧悜婧?Empty 鐨勫疄鏃跺紩鐢ㄣ€傚鍒跺師鐢熺粍鏃讹紝鍏堝皢鍏?Blender Graph Editor 涓殑涔濈被 F-Curve 鍐欏洖
RNA 婧愬瓧娈碉紝鍐嶆敹闆嗗叏閮ㄥ彲缂栬緫鏁板€煎拰鏇茬嚎瀛楁锛汿yphoea 鐪熷疄缁勫疄娴嬩负 249 椤癸紝闃诲凹鏇茬嚎绛夐潪鍩虹瀛楁涔熶細鍦?
鍘熺敓缁勪箣闂村鍒躲€傛牴楠ㄣ€丼electionData銆佺粍浠惰韩浠姐€佺鎾炰綋寮曠敤鍜岄摼鑺傜偣涓嶅睘浜庡弬鏁帮紝淇濇寔鐩爣缁勮嚜宸辩殑缁撴瀯銆?

鏂板浣滆€?v3 缁勫綋鍓嶅彲琛ㄨ揪浜斾釜鍩虹鍙傛暟鍜岃妭鐐圭鎾炲崐寰勬洸绾裤€傜矘璐村埌浣滆€呯粍浼氫竴娆″啓鍏ュ叏閮ㄨ繖浜涘瓧娈碉紱鍗婂緞鏇茬嚎
鍖呮嫭 value/useCurve銆佸叧閿抚鏃堕棿鍜屽€笺€佽繘鍑哄垏绾裤€亀eightedMode銆佽繘鍑烘潈閲嶃€乸re/post infinity 涓?rotation order銆?
鍏朵綑鍘熺敓瀛楁浣滀负瀹屾暣蹇収淇濆瓨鍦ㄤ綔鑰呯粍 Empty 涓紝骞朵細闅忎笅涓€娆″鍒剁户缁紶閫掞紱瀵硅薄鍙傛暟闈㈡澘鏄剧ず蹇収瀛楁鏁般€?
璇ュ揩鐓у綋鍓嶄笉浼氬啓鍏ヤ綔鑰?v3 浜岃繘鍒讹紝鍥犺€屼篃涓嶄細鐢?DLL 搴旂敤銆傛病鏈夋妸瀹冩弿杩版垚宸插彲瀵煎嚭鏁版嵁锛屼篃娌℃湁淇敼 DLL
閰嶇疆鍚堝悓鎴栨父鎴忕洰褰曘€?

缃戞牸鍒囨崲闈㈡澘鍘熷厛瑕佹眰鎵嬪～ `eiem_key` 瀛楃涓层€?.22.0 鏂板 Blender 妯℃€?*褰曞埗鎸夐敭**锛氱偣鍑诲悗鎸変竴涓敭鎴?
Ctrl/Shift/Alt 缁勫悎閿嵆鍙紝Esc 鍙栨秷锛涘綍鍒跺€肩粡杩囦笌 C++ INI 瑙ｆ瀽鍣ㄤ竴鑷寸殑鏈夐檺閿悕瑙勮寖鍖栵紝骞跺湪鍐欏叆鍓嶆嫆缁?
缁勯棿閲嶅銆傚垱寤虹粍浠嶈嚜鍔ㄥ垎閰嶅彲鐢?F 閿紝闈㈡澘鍚屾椂鍒楀嚭鍏ㄩ儴鈥滄父鎴忔寜閿?鈫?鍒囨崲缁勨€濆叧绯汇€傚悗鍙?Blender 鍥炲綊瑕嗙洊
瀛楁瘝銆佹暟瀛椼€佺炕椤甸敭銆佷慨楗伴敭鎺掑簭銆侀噸澶嶉敭涓嶆敼鍐欍€佷綔鑰呭畬鏁村崐寰勬洸绾垮鍒躲€佺湡瀹炴簮 249 瀛楁澶嶅埗鍜屼笁杞敞鍐屽嵏杞姐€?

### 20.41 Blender 0.23.0 / 浣滆€?v4锛氬畬鏁村弬鏁版ā鏉胯繘鍏ヨ祫婧愬拰 DLL 閰嶇疆鑽夌

0.22.0 鐨勫弬鏁板壀璐存澘铏界劧淇濈暀浜嗗師鐢熺粍鐨勫叏閮ㄥ彲缂栬緫瀛楁锛屼絾浣滆€?v3 浜岃繘鍒跺彧鎼哄甫浜斾釜鍩虹鏍囬噺涓庤妭鐐瑰崐寰勬洸绾匡紝
鍥犳鈥滃鍒?249 椤光€濆苟涓嶇瓑浜?DLL 鑳芥敹鍒拌繖浜涘€笺€?.23.0 灏嗘柊澧炰綔鑰呮牸寮忓崌绾т负 v4锛氭瘡缁勫鍔?
`nativeParameters`锛岄€愰」璁板綍 `serializeData.*` 瀛楁璺緞銆佹诞鐐?鏁存暟绫诲埆鍜屽€笺€傜湡瀹?Typhoea
`ClothSerializeData` 鏍锋湰鍦ㄦ帓闄ゆ嫇鎵戝拰杩愯鏃跺瓧娈靛悗寰楀埌 249 椤癸紱浜旈」甯哥敤鍙傛暟鍜屽崐寰勬洸绾垮湪瀵煎嚭鍓嶄互 Empty
褰撳墠鍊艰鐩栨ā鏉匡紝鍥犺€岀敤鎴峰鍒跺弬鏁板悗浠嶅彲缁х画鐩存帴璋冩暣杩欎簺甯哥敤椤广€?

璧勬簮鏍￠獙鍙帴鍙?`serializeData` 涓嬬殑瀹夊叏瀛楁璺緞銆佹湁闄?float32 鍜?int32銆俙sourceRenderers`銆乣paintMaps`銆?
`rootBones`銆乣ignoreFromRootBones`銆乣colliderList`銆乣verificationResult`銆丳athID銆佹暟缁勫師濮嬪瓧鑺傜瓑寮曠敤銆佹嫇鎵戝拰
杩愯鏃舵暟鎹槑纭姝㈣繘鍏?v4 鍙傛暟琛ㄣ€傝繖鏍峰鍒剁殑鏄柊澧為摼鍙鐢ㄧ殑姹傝В鍙傛暟妯℃澘锛屼笉浼氭妸婧愮粍浠惰韩浠姐€佸師鐢熷璞″紩鐢?
鎴?Selection 椤哄簭浼鎴愬彲绉绘鍙傛暟銆?

DLL 閰嶇疆鑽夌涓嶄娇鐢ㄧ‖缂栫爜瀛楁鍋忕Щ锛岃€屾槸娌垮綋鍓嶆父鎴?IL2CPP 鍏冩暟鎹殑瀛楁鍚嶅拰澹版槑绫诲瀷閫愮骇瑙ｆ瀽寮曠敤瀵硅薄銆?
褰撳墠鏀寔 `System.Single`銆乣System.Boolean`銆?2 浣嶆暣鏁般€? 瀛楄妭鏋氫妇鍜屽畬鏁翠笁鍒嗛噺
`UnityEngine.Vector3 gravityDirection`銆傞樆灏笺€佽妭鐐瑰崐寰勩€佽窛绂诲垰搴︺€佽搴︽仮澶嶅垰搴︺€佽搴﹂檺鍒躲€佹渶澶ц窛绂汇€?
鍥炴尅璺濈銆佺鎾為檺鍒惰窛绂诲拰鑷鎾炶〃闈㈠帤搴︿節绫?`CurveSerializeData` 浼氬垱寤?`Keyframe[]` 涓?
`AnimationCurve`锛屽啓鍏ュ熀纭€鍊笺€乽seCurve銆佹椂闂淬€佸€笺€佸垏绾裤€亀eightedMode 鍜屾潈閲嶏紝鍐嶉€愰」鍥炶銆?
璧勬簮浠嶅畬鏁翠繚鐣欐洸绾跨殑 pre/post infinity 涓?rotation order锛涘綋鍓嶈繍琛屾椂鍐欏叆娌℃湁鎶婅繖涓夐」鐚滄槧灏勪负 Unity
鍏紑 WrapMode锛屽洜姝ゅ畠浠皻涓嶅奖鍝嶉厤缃崏绋裤€備换涓€瀵硅薄銆佸瓧娈点€佺被鍨嬫垨鍥炶鍊间笉鍖归厤鏃讹紝鏁存壒鏆傚瓨閰嶇疆涓嶅彂甯冦€?

鏈€缁?MSVC 瀹夸富鍥炲綊 88 椤归€氳繃锛岃鐩?v1/v3/v4 璺ㄨ瑷€璇诲彇銆佸潖鏂囦欢銆佹爣閲忋€佸竷灏斻€佹灇涓俱€侀噸鍔涙柟鍚戙€佸祵濂楀璞°€?
鏇茬嚎鍜屼簨鍔″け璐ャ€侭lender 5.0.1 浣滆€呭洖褰掍笌 Typhoea 鐪熷疄婧愬洖褰掑潎閫氳繃锛屽悗鑰呯‘璁?11 缁勩€?7 涓湁褰㈢鎾炰綋鍙?
249 椤规ā鏉夸粠鍘熺敓缁勫鍒跺埌鏂板缁勫苟杩涘叆 v4 鏂囨。銆俙build.bat` 宸插畬鏁寸敓鎴愭湰鍦?`bin/eiem.dll`銆?
`d3dcompiler_47.dll` 涓?`vulkan-1.dll`銆傝繖浜涚粨鏋滃皻涓嶅寘鍚父鎴忕洰褰曢儴缃叉垨 v4 鍙傛暟鐨勬父鎴忓唴鍝嶅簲楠岃瘉锛涚嫭绔嬬悆銆?
鑳跺泭鍜屽钩闈?Collider 杞崲浠嶆湭鎺ュ叆缁勫悎 Mod銆?

涓冧釜鎻掍欢杩愯鏂囦欢宸查€愪竴鍚屾鍒板紑鍙戠洰褰曞拰 Blender 5.0 鐨勫綋鍓嶅畨瑁呯洰褰曘€傚畨瑁呭寘
`bin/EIEM_Blender-0.23.0-full-physics-parameters.zip` 涓?99431 瀛楄妭锛孲HA256锛?
`87B6673F20051AEA5CCDAC1345EE86BD5C12A3A5CB244133D897CCB72F784E46`锛沍IP 瀹屾暣鎬у拰瑙ｅ帇鐩綍涓夎疆
娉ㄥ唽/鍗歌浇閫氳繃銆侭lenderMCP 宸插皢姝ｅ湪缂栬緫鐨勪細璇濈儹閲嶈浇鍒?0.23.0锛涢噸杞藉墠鍚庢枃浠朵粛涓?
`G:\zmd\typhoeus\18324_autosave.blend`锛?74 涓璞°€? 涓綔鑰呯墿鐞嗙粍鍜屾湭淇濆瓨鐘舵€佸潎淇濇寔涓嶅彉锛屾湭鑷姩淇濆瓨鍦烘櫙銆?

### 20.42 Blender 0.24.0锛氫綔鑰呯粍瀹屾暣鍙傛暟缂栬緫涓庣鎾為泦鍚?

0.23.0 宸叉妸 `nativeParameters` 鍐欏叆浣滆€?v4锛屼絾 Blender 浣滆€呯粍鍙繚鐣欎竴浠介殣钘?JSON 蹇収锛屽璞￠潰鏉夸粛鍙樉绀?
浜旈」甯哥敤鍙傛暟锛涘弬鏁板壀璐存澘 v1 杩樻槑纭帓闄や簡纰版挒寮曠敤锛屼笖鈥滃姞鍏ュ綋鍓嶇墿鐞嗙粍鈥濆彧鎺ュ彈浣滆€呯鎾炰綋銆傚洜鑰屼粠鍘熺敓瑁欐憜缁?
澶嶅埗鍒版柊澧炶鎽嗛摼鍚庯紝鏂囦欢鍐呴儴铏芥湁 249 椤瑰弬鏁帮紝鐢ㄦ埛鍗存棤娉曟煡鐪嬫垨淇敼澶ч儴鍒嗗瓧娈碉紝涔熷緱涓嶅埌鍘熺粍瀹為檯浣跨敤鐨勭鎾為泦鍚堛€?

0.24.0 灏嗗鍒舵潵鐨勬瘡涓師鐢熷瓧娈佃鍏ヤ綔鑰呯粍 Empty 鑷繁鐨?RNA 瀛楁闆嗗悎銆傚璞″睘鎬т腑鐨勨€滃畬鏁寸墿鐞嗗弬鏁扳€濅娇鐢ㄤ腑鏂囩煭鍚嶃€?
绛涢€夊拰姣忛〉 32 椤规樉绀猴紱淇敼鏅€氬瓧娈典細绔嬪嵆鍐欏洖浣滆€呭揩鐓э紝淇敼浜旈」甯哥敤瀛楁鎴栬妭鐐瑰崐寰?F-Curve 浼氬悓姝ュ埌鍚屼竴瀛楁琛紝
浣滆€?v4 瀵煎嚭缁х画浠庤繖浠藉綋鍓嶆暟鎹敓鎴?`nativeParameters`銆傛棫 `.blend` 鍙湁闅愯棌蹇収鏃讹紝鎻掍欢閲嶈浇浼氳縼绉讳负鍙紪杈戝瓧娈点€?
杩佺Щ娴嬭瘯鍚屾椂鍙戠幇閲嶅缓棰勮鏃朵慨鏀?`bpy.data.objects` 浼氫娇 Blender 鐨?C 杩唬鍣ㄥけ鏁堬紱鐜版敼涓哄厛鍥哄畾鏀堕泦浣滆€呯粍锛?
鍐嶉€愮粍杩佺Щ涓庨噸寤恒€?

鍙傛暟鍓创鏉垮崌绾т负 v2锛屽苟闄勫甫缁勪娇鐢ㄧ殑纰版挒浣撶ǔ瀹氳韩浠姐€傜矘璐村埌浣滆€呯粍浼氫簨鍔″紡鏇挎崲鍙傛暟鍜岀鎾為泦鍚堬紱鐩爣 Empty
鍙湪鈥滅鎾為泦鍚堚€濅腑閫夋嫨銆佺Щ闄わ紝鎴栦粠鍚屼竴 Rig 鐨勪綔鑰?娓告垙婧愮鎾炰綋涓嬫媺妗嗗姞鍏ャ€傚綋鍓嶇粍鏄鹃殣浼氳繛鍚岃繖浜涘紩鐢ㄦ洿鏂般€?
鍘熺敓缁勪箣闂寸矘璐翠粛鍙敼鍙傛暟锛屼笉鏀瑰師濮嬫簮鍥惧叧绯汇€傛父鎴忔簮 Collider 鐨勫叧鑱斾細闅?`.blend` 淇濆瓨锛屼絾浣滆€?v4 瀵煎嚭浠嶆槑纭?
鎷掔粷瀹冿紝鍥犱负婧愮悆/鑳跺泭/骞抽潰鍒?DLL 閰嶇疆鐨勮浆鎹㈠皻鏈疄鐜帮紱鏈妭娌℃湁鎶?Blender 鍏宠仈瀹ｇО涓烘父鎴忕鎾炲凡鎺ュ叆銆?

Blender 5.0.1 鐨勪綔鑰呮祴璇曘€佺湡瀹?Typhoea 婧愬浘娴嬭瘯鍜屾甯?package 娴嬭瘯鍧囬€氳繃锛氬悗涓よ€呯户缁緱鍒?62 涓?Mesh銆?
1 涓叡浜?Rig銆?1 涓師鐢熺粍銆?7 涓鎾炰綋涓?192 涓墍闇€ Transform锛涘師鐢熺粍鐨?249 椤瑰瓧娈靛拰纰版挒闆嗗悎澶嶅埗鍒版柊澧炵粍锛?
鏃у揩鐓ц縼绉汇€佸瓧娈典慨鏀硅繘鍏?v4銆佺Щ闄?閲嶆柊鍔犲叆婧愮鎾炰綋銆佸甫婧愮鎾炰綋瀵煎嚭鎷掔粷鍧囨湁瑕嗙洊銆侰++ 浣滆€?reader 涔熻鍙?
Blender 鐨?v4 杈撳嚭閫氳繃锛屾彃浠舵簮鐮佺洰褰曞拰瑙ｅ帇瀹夎鍖呭悇瀹屾垚涓夎疆娉ㄥ唽/鍗歌浇銆?

涓冧釜杩愯鏂囦欢宸查€愪竴鍝堝笇鍚屾鍒?`E:\vscode\EIEM_Blender` 鍜?Blender 5.0 瀹夎鐩綍銆傚畨瑁呭寘
`bin/EIEM_Blender-0.24.0-editable-physics-groups.zip` 涓?102745 瀛楄妭锛孲HA256锛?
`DC24C7D55503104006D805316389217BAC5A64C3A23121691240A18F1019E11C`銆侭lenderMCP 宸叉妸褰撳墠浼氳瘽鐑噸杞藉埌
0.24.0锛涙枃浠朵粛涓?`C:\Users\25487\AppData\Local\Temp\18324_autosave_27152_autosave_31644_autosave.blend`锛?
174 涓璞″拰褰撳墠 `maid.002 Skirt Physics` 淇濇寔锛屾棫蹇収鐓ц縼绉讳负 249 涓彲缂栬緫瀛楁銆傛棫鍓创鏉挎病鏈夌鎾炶韩浠斤紝
鍥犳鎸夊叾涓敮涓€婧愬悕 `MBC_Typhoea_Cloth_Skirt` 涓哄綋鍓嶄綔鑰呯粍琛ュ洖浜嗗悓 Rig 鐨勫乏鍙冲ぇ鑵裤€侀鐩嗗拰宸﹀彸灏忓墠鑷傚叡 5 涓?
鍘熺敓鑳跺泭寮曠敤锛涙病鏈夎皟鐢ㄤ繚瀛樸€?

鏈妭鏈慨鏀规垨閮ㄧ讲 DLL锛屾湭鍐欏叆娓告垙鐩綍锛屼篃鏈獙璇佽繖浜?Collider 鐨勬父鎴忓唴瀹炰緥鍖栨垨鍝嶅簲銆?

### 20.43 Blender 0.25.0锛氫綔鑰呯粍涓庡師鐢熺粍缁熶竴鍙傛暟鐣岄潰

0.24.0 铏藉凡鎶婂師鐢熸ā鏉跨殑 249 椤瑰瓧娈靛鍒惰繘浣滆€呯粍锛屼絾浣滆€呯粍浠嶄娇鐢ㄧ嫭绔嬬殑绠€鍖栧弬鏁伴潰鏉垮拰鍗曠嫭鐨勮妭鐐瑰崐寰?
Action锛涘師鐢熺粍鍒欐樉绀哄父鐢ㄥ弬鏁般€佽搴﹂檺鍒躲€佷節鏉℃洸绾垮拰楂樼骇瀛楁銆備袱鑰呮暟鎹兘澶熷線杩旓紝缂栬緫鍏ュ彛鍜?Empty 鑷畾涔?
灞炴€у嵈骞朵笉涓€鑷淬€傝妭鐐瑰崐寰勫瓧娈电殑浣滆€呭洖璋冭繕浼氶噸寤烘棫鐨勫崟鏇茬嚎鎶曞奖锛屾棤娉曠淮鎸佸畬鏁存ā鏉跨殑涔濇洸绾胯〃绀恒€?

0.25.0 浠モ€滄槸鍚﹀叿鏈夊畬鏁翠節鏇茬嚎瀛楁闆嗏€濆尯鍒嗗畬鏁存ā鏉垮拰鏃х殑鏈€灏忎綔鑰呮暟鎹€傚畬鏁存ā鏉夸綔鑰呯粍涓庡師鐢熺粍鍏辩敤鐩稿悓鐨?
甯哥敤鍙傛暟銆佽搴﹂檺鍒跺拰楂樼骇瀛楁缁樺埗鍑芥暟锛涗節绫?`CurveSerializeData` 鎶曞奖鍒板悓涓€濂椾節涓腑鏂?Empty ID 灞炴€у強
Action/F-Curve銆傚鍒躲€佺矘璐淬€佷綔鑰?v4 瀵煎叆鍜屾棫 `.blend` 杩佺Щ閮戒細寤虹珛杩欎唤鎶曞奖銆傚瓧娈佃〃缂栬緫浼氶噸寤虹浉搴旀姇褰憋紝
Graph Editor 缂栬緫浠嶅啓鍥炲瓧娈佃〃涓庝綔鑰呭揩鐓э紱浣滆€呴瑙堝悓鏃朵娇鐢ㄨ妭鐐瑰崐寰勬洸绾垮苟鏄剧ず瑙掑害闄愬埗閿ャ€傛病鏈夊畬鏁存ā鏉跨殑
鏃т綔鑰呯粍缁х画浣跨敤浜旈」鏈€灏忓弬鏁板拰鍗曠嫭鑺傜偣鍗婂緞鏇茬嚎锛岄伩鍏嶆妸缂哄け鐨勫師鐢熷瓧娈电寽鎴愰粯璁ゅ€笺€?

Blender 5.0.1 鐨勬渶灏忎綔鑰呮祦绋嬨€佺湡瀹?Typhoea 婧愬浘鍜屾甯?package 鍥炲綊鍧囬€氳繃銆傜湡瀹炴簮鍥剧‘璁ゅ鍒跺悗鐨勬柊澧炵粍鏈?
249 涓瓧娈点€? 鏉?F-Curve銆? 涓腑鏂囪嚜瀹氫箟鏇茬嚎灞炴€у拰鍘熺粍纰版挒闆嗗悎锛涢珮绾ф洸绾垮瓧娈典慨鏀硅兘澶熻繘鍏?Action銆傛甯?
package 浠嶅鍏?62 涓?Mesh銆? 涓叡浜?Rig銆?1 涓師鐢熺粍銆?7 涓鎾炰綋鍜?192 涓墿鐞嗙浉鍏?Transform銆傛彃浠跺彂鐜般€?
浣滆€呯姸鎬佹祴璇曞拰 ZIP 鍐呬笁杞敞鍐?鍗歌浇涔熼€氳繃锛涙枃妗?Python 娴嬭瘯 15 椤归€氳繃锛? 椤瑰洜褰撳墠鍛戒护琛屾湭閰嶇疆 MSVC 鑰岃烦杩囥€?

涓冧釜杩愯鏂囦欢宸插悓姝ュ埌 `E:\vscode\EIEM_Blender` 鍜?Blender 5.0 鎻掍欢鐩綍銆傚畨瑁呭寘
`bin/EIEM_Blender-0.25.0-unified-physics-ui.zip` 涓?104214 瀛楄妭锛孲HA256锛?
`95D119309DDA9B5C1F665CF255E5B3B424A883A8812630DDBD95D6DD8921CC93`銆侭lenderMCP 宸茬儹閲嶈浇褰撳墠浼氳瘽锛涙椿鍔?
瀵硅薄浠嶄负 `maid.002 Skirt Physics`锛屽叾 249 椤瑰瓧娈点€? 涓鎾炰綋寮曠敤銆? 鏉℃洸绾垮拰 9 涓嚜瀹氫箟鏇茬嚎灞炴€у潎淇濈暀锛?
瑙掑害闄愬埗棰勮鐢熸垚 12 涓敟浣撱€傛枃浠朵粛涓?
`C:\Users\25487\AppData\Local\Temp\18324_autosave_27152_autosave_31644_autosave.blend`锛岄噸杞藉墠鍚庣殑鏈繚瀛樼姸鎬?
鍧囦负 true锛屾湰杞病鏈夎嚜鍔ㄤ繚瀛樸€?

鏈妭娌℃湁淇敼 DLL銆佸啓鍏ユ父鎴忕洰褰曘€侀儴缃?DLL 鎴栬繘琛屾父鎴忓唴 Physics 楠屾敹锛涙簮纰版挒浣撹浆鎹粛灞炰簬鍚庣画 DLL 闃舵銆?

### 20.44 Blender 0.26.0锛氬崟涓€鍙傛暟鍏ュ彛涓庡唴宓屾诞鐐规洸绾?

0.25.0 铏界粺涓€浜嗕綔鑰呯粍涓庡師鐢熺粍鐨勬暟鎹ā鍨嬶紝鍗村悓鏃舵毚闇蹭簲椤瑰父鐢ㄥ弬鏁般€佽搴︿笓鐢ㄥ尯銆佷節涓?Empty 鑷畾涔夊睘鎬с€?
Graph Editor 鍜?249 椤瑰彲缂栬緫婧愬瓧娈点€傛簮缁勫瓧娈垫部 TypeTree 閬嶅巻椤哄簭鍔犲叆锛屼綔鑰呭揩鐓у張鎸夎矾寰勬帓搴忥紝閫犳垚鍚屼竴鍙傛暟妯℃澘
鍦ㄤ笉鍚屽璞′笂鎺掑垪涓嶄竴鑷达紱瀹屾暣婧愬瓧娈佃繕浼氳鐢ㄦ埛璇互涓烘柊澧為摼蹇呴』鎵嬪伐濉啓鏁扮櫨椤广€?

0.26.0 灏嗗璞″睘鎬ф敹鏁涗负涓€涓棩甯哥紪杈戝叆鍙ｃ€傚熀纭€鍙傛暟鍥哄畾鎸夌墿鐞嗘贩鍚堟潈閲嶃€侀噸鍔涖€侀噸鍔涜“鍑忋€佸姩鐢诲Э鎬佹瘮渚嬪拰
閲嶇疆鍚庣ǔ瀹氭椂闂存帓鍒椼€備節绫绘部閾惧弬鏁伴€氳繃涓嬫媺妗嗕竴娆￠€夋嫨涓€椤癸紝褰撳墠椤瑰彧鏄剧ず鍩虹鍊笺€乣useCurve`銆佷笌瀹冪洿鎺ョ浉鍏崇殑绾︽潫
寮€鍏?闄勫姞鍊硷紝浠ュ強涓€涓?Blender `Float Curve`锛涙í杞?0锝? 鏄牴鍒版湯绔紝绾佃酱鏄€嶇巼銆傝搴﹂檺鍒舵€诲紑鍏冲拰鍒氬害鍙湪
閫夋嫨瑙掑害鏇茬嚎鏃跺悓鍖哄嚭鐜般€傛棫鐨?Graph Editor 鎸夐挳銆佷節涓彲缂栬緫 ID 灞炴€у拰涓撶敤瑙掑害鍙傛暟鍖轰笉鍐嶆樉绀恒€?

瀹屾暣妯℃澘浠嶄繚鐣欏湪 Empty 鐨?RNA 瀛楁闆嗗悎涓庝綔鑰?v4 `nativeParameters` 涓紝鐢ㄤ簬澶嶅埗銆佺矘璐村拰瀵煎嚭锛涙姌鍙犲尯鏀瑰悕涓?
鈥滄簮鏁版嵁妫€鏌ワ紙鍙锛夆€濓紝鎸夊惈鏁板瓧绱㈠紩鐨勭ǔ瀹氳矾寰勬帓搴忥紝鏀寔绛涢€夊拰鍒嗛〉锛屼絾涓嶅啀鎻愪緵绗簩濂椾慨鏀瑰叆鍙ｃ€傛湭缂栬緫鐨勫師鐢?
鏇茬嚎缁х画鐢卞師瀛楁淇濆瓨鍘熷叧閿抚銆佸垏绾裤€佹潈閲嶅拰 Infinity/RotationOrder銆傜敤鎴峰疄闄呯紪杈戝唴宓?Float Curve 鍚庯紝鎵嶆寜褰撳墠
鎺у埗鐐圭敓鎴愭柊鐨勫叧閿抚鍜屾棤鏉冮噸鍒囩嚎锛涘厑璁?2锝?4 涓帶鍒剁偣銆備綔鑰呮渶灏忕粍鐨勮妭鐐瑰崐寰勪篃鏀圭敤鍚屼竴鍐呭祵鎺т欢锛屾棫 `.blend`
涓殑 EIEM Action 浼氬湪閲嶈浇鏃跺厛鍐欏洖鍐嶈縼绉伙紝闅忓悗鍒犻櫎鏃?Action 鍜屽璞℃洸绾垮睘鎬с€?

Blender 5.0.1 鍥炲綊閫氳繃鎻掍欢鍙戠幇/涓夎疆娉ㄥ唽銆佹渶灏忎綔鑰呯粍淇濆瓨閲嶅紑銆佺湡瀹?Typhoea 婧愬浘鍜屾甯?package 瀵煎叆銆傜湡瀹炴簮鍥?
浠嶅緱鍒?11 涓粍銆?7 涓湁褰㈢鎾炰綋锛涙甯?package 浠嶅緱鍒?62 涓?Mesh銆? 涓叡浜?Rig 鍜?192 涓墍闇€ Transform銆傚師鐢?
绮剧‘鏈敼瀵煎嚭銆丗loat Curve 鏀瑰姩鍐欏洖銆佽妭鐐瑰崐寰?瑙掑害棰勮鍒锋柊銆?49 椤瑰弬鏁颁笌纰版挒闆嗗悎澶嶅埗鍒版柊澧炵粍鍧囨湁瑕嗙洊銆?
瑙ｅ帇瀹夎鍖呬笁杞敞鍐岄€氳繃銆?

涓冧釜杩愯鏂囦欢宸插悓姝ュ埌 `E:\vscode\EIEM_Blender` 鍜?Blender 5.0 鎻掍欢鐩綍銆傚畨瑁呭寘
`bin/EIEM_Blender-0.26.0-compact-physics-ui.zip` 涓?105959 瀛楄妭锛孲HA256锛?
`E5FBD4652B457FCC80F80AD48749FA1BD41801114D0E4FCCE848864EC3A2611E`銆侭lenderMCP 鐑噸杞藉悗鐨勬椿鍔ㄥ璞′粛涓?
`maid.002 Skirt Physics`锛?49 椤规ā鏉裤€? 涓鎾炰綋寮曠敤鍜?9 涓鏈?Float Curve 鑺傜偣淇濈暀锛屾棫 Action 涓庝節涓洸绾?
鑷畾涔夊睘鎬у潎涓虹┖锛涙枃浠惰矾寰勫拰鏈繚瀛樼姸鎬佷繚鎸佷笉鍙橈紝鏈嚜鍔ㄤ繚瀛樸€?

鏈妭娌℃湁淇敼 DLL銆佸啓鍏ユ父鎴忕洰褰曘€侀儴缃?DLL 鎴栧鍔犳父鎴忓唴 Physics 缁撹锛涚嫭绔嬫簮纰版挒浣撹浆鎹㈢姸鎬佹湭鏀瑰彉銆?

### 20.45 Blender 0.26.1锛氳交閲忔洸绾垮垎甯冧笌閫変腑鑺傜偣瀹炲€?

0.26.0 鐨勫唴宓?Float Curve 浠嶈鏃ュ父鍙傛暟淇敼杩涘叆涓€濂楄繃閲嶇殑鏇茬嚎缂栬緫宸ヤ綔娴併€傚鏈満瀹夎鐨?Wilds Chain2
缂栬緫鍣ㄥ鏍稿悗锛岄噰鐢ㄥ叾鈥滅粍淇濆瓨鍏变韩鍔ㄥ姏瀛︺€佽妭鐐规樉绀哄眬閮ㄧ粨鏋溿€佹壒閲忕敓鎴愬垎甯冣€濈殑浜や簰鎬濊矾锛屼絾涓嶇収鎼?RE Engine
鐨勬暟鎹粨鏋勶細缁堟湯鍦?`BeyondBoneCloth` 鐨勬瘡绫诲弬鏁颁粛鏄竴浠界粍绾у熀纭€鍊笺€乣useCurve` 鍜?Unity
`AnimationCurve`锛屽悓缁勫鏍逛笌鍒嗘敮鍏卞悓閲囨牱瀹冿紝鑺傜偣鍊间笉鏄嫭绔嬬殑鍙鍑烘簮鏁版嵁銆?

0.26.1 鐨勫璞″睘鎬ч粯璁ゅ彧鏄剧ず褰撳墠鍙傛暟鐨勫熀纭€鍊笺€佹牴绔€嶇巼銆佹湯绔€嶇巼鍜岀嚎鎬?骞虫粦鍙樺寲銆傚ぇ鍨?Blender 鏇茬嚎鎺т欢
涓嶅啀鏄剧ず锛涘彧鏈夐渶瑕佸眬閮ㄥ彉鍖栨椂鎵嶅睍寮€鈥滈珮绾у叧閿偣鈥濓紝鐢ㄦ暟鍊煎瓧娈电紪杈?0锝? 鐨勯摼浣嶇疆涓庡€嶇巼銆傛牴绔拰鏈涓嶈兘
鍒犻櫎锛屼腑闂寸偣鍙鍒狅紱鎻掑叆鐐瑰厛鎸夊綋鍓嶆洸绾挎眰鍊硷紝鍥犳鎻掑叆鍔ㄤ綔鏈韩涓嶆敼鍙樺垎甯冦€傛湭淇敼鐨勫師鐢熸洸绾夸粛閫愬瓧娈典繚鐣?
鍘熷叧閿抚銆佸垏绾裤€佹潈閲嶅拰鍏冩暟鎹紱瀹為檯淇敼鍊嶇巼銆佹彃鍊兼垨鍏抽敭鐐瑰悗锛屾墠鎸夊綋鍓嶆姇褰辩敓鎴愭柊鐨勬棤鏉冮噸鍒囩嚎璁板綍銆?

鐗╃悊渚ф爮鐜板湪浼氬湪褰撳墠缁勭殑 Rig 楠ㄩ琚€変腑鏃舵樉绀鸿鑺傜偣鐨?FIXED/MOVE/IGNORE 瑙掕壊銆佹寜绱楠ㄦ闀垮害褰掍竴鍖栫殑
閾句綅缃€佸綋鍓嶅弬鏁板€嶇巼鍜?`鍩虹鍊?脳 鍊嶇巼` 瀹為檯鍊硷紝骞跺彲鐩存帴鍦ㄨ閾句綅缃鍔犲叧閿偣銆傚師鐢熺粍娌跨敤宸茬粡鐢卞嚑浣曚綅缃?
鍖归厤寤虹珛鐨?Transform鈫扴election 鏄犲皠缂撳瓨锛涗綔鑰呯粍浠庢樉寮忛楠艰韩浠戒笌鐖堕摼璁＄畻銆傝鏄剧ず涓嶆妸 SelectionData 鏁扮粍
搴忓彿澹版槑涓洪€氱敤楠ㄩ韬唤锛屼篃涓嶆妸鑺傜偣閲囨牱鍊煎鍒舵垚绗簩浠藉彲缂栬緫鐗╃悊鍙傛暟銆?

Blender 5.0.1 楠岃瘉閫氳繃鎻掍欢鍙戠幇涓庝笁杞敞鍐?鍗歌浇銆佹渶灏忎綔鑰呴摼淇濆瓨閲嶅紑銆佺湡瀹?Typhoea 婧愬浘寰€杩斿拰姝ｅ父 package
瀵煎叆銆傜湡瀹炴簮缁撴灉浠嶄负 11 涓墿鐞嗙粍銆?7 涓湁褰㈢鎾炰綋锛涙甯?package 浠嶄负 62 涓?Mesh銆? 涓叡浜?Rig銆?
11 涓墿鐞嗙粍銆?7 涓鎾炰綋鍜?192 涓墍闇€ Transform銆傛祴璇曞彟瑕嗙洊绾挎€?骞虫粦鍒囨崲銆佷腑闂寸偣鎻掑叆/鍒犻櫎銆佺鐐瑰垹闄ゆ嫆缁?
浠ュ強閫変腑楠ㄩ鐨勮鑹?閾句綅缃噰鏍枫€?

涓冧釜杩愯鏂囦欢宸查€愪竴鍚屾鍒?`E:\vscode\EIEM_Blender` 鍜?Blender 5.0 鎻掍欢鐩綍銆傚畨瑁呭寘
`bin/EIEM_Blender-0.26.1-lightweight-physics-curves.zip` 涓?110013 瀛楄妭锛孲HA256锛?
`C8E04F69AE61EF156F73F48F3FBD91E3941FD73F062789E164FE13087ABD1AE4`銆傛湰鑺傛病鏈変慨鏀规垨閮ㄧ讲 DLL锛?
娌℃湁鍐欏叆娓告垙鐩綍锛屼篃娌℃湁澧炲姞鍘熺敓纰版挒浣撳疄渚嬪寲鐨勬父鎴忓唴缁撹銆?

### 20.46 Blender 0.26.2锛氫慨姝ｆ柊澧炰綔鑰呯粍瑙掑害閿ョ殑閲嶅鍧愭爣杞崲

鐢ㄦ埛鍦ㄥ綋鍓?`maid.002 Skirt Physics` 瑙嗗浘涓寚鍑猴紝榛勮壊瑙掑害閿ラ泦涓湪瑙掕壊鑴氳竟鑰屼笉鏄叚鏉¤鎽嗛閾句笂銆傚璞＄‘瀹炴湁
12 涓敟浣撲笖鏈殣钘忥紝浣嗏€滃璞″瓨鍦ㄢ€濅笉鑳借瘉鏄庣┖闂翠綅缃纭€傚鏍哥敓鎴愯矾寰勫悗纭锛氫綔鑰呯粍鐨勮妭鐐瑰崐寰勭悆鍜岀豢鑹茶繛鎺ョ嚎
閮芥妸 `Bone.head_local` 浣滀负 Blender Rig 灞€閮ㄥ潗鏍囩洿鎺ュ缓妯★紱瑙掑害閿ュ嵈澶嶇敤浜嗗師鐢?v2 婧愬浘鍏ュ彛鐨勯粯璁ゅ弬鏁帮紝鍙堟墽琛?
涓€娆?Unity鈫払lender 鍩哄彉鎹€傞噸澶嶈浆鎹㈡妸瑁欐憜灞€閮ㄥ潗鏍囨棆杞埌浜嗛敊璇綅缃€?

`make_angle_visual` 鐜板湪鏄惧紡鎺ユ敹鍧愭爣鏉ユ簮锛涘鍏ュ師鐢?v2 缁х画淇濇寔涓€娆?Unity鈫払lender 杞崲锛屾柊澧炰綔鑰呯粍鍒欎紶鍏?
`native_coordinates=False`锛屼笌鑺傜偣鐞冨強杩炴帴绾垮叡鐢?Blender Rig 灞€閮ㄧ┖闂淬€傜儹閲嶈浇 0.26.2 鍚庯紝褰撳墠浣滆€呯粍绗竴涓敟灏?
涓庣涓€鏍?FIXED 楠ㄩ `maid_skirt_01_a_jnt` 鐨?`head_local` 鍧囦负
`(0.098348245, -0.003961120, 0.995286882)`锛岃窛绂昏宸负 0锛?2 涓?MOVE 楠ㄦ鐨勯敟浣撳凡鍥炲埌瑁欐憜浣嶇疆銆?
鑺傜偣纰版挒鍗婂緞鏇茬嚎鍜屼簲涓嫭绔嬪師鐢熻兌鍥婃病鏈夋敼鍙樸€?

鐪熷疄 Typhoea Blender 鍥炲綊鏂板鈥滄妸鍚敤瑙掑害闄愬埗鐨勫師鐢熸ā鏉跨矘璐村埌鏂板浣滆€呴摼鍚庯紝閿ュ皷蹇呴』涓?FIXED 鏍归鍧愭爣涓€鑷粹€?
妫€鏌ワ紝骞朵笌鏈€灏忎綔鑰呬繚瀛橀噸寮€銆佹彃浠朵笁杞敞鍐屻€?2 Mesh 姝ｅ父 package 瀵煎叆涓€骞堕€氳繃銆備竷涓繍琛屾枃浠跺凡鍚屾鍒?
`E:\vscode\EIEM_Blender` 鍜?Blender 5.0 鎻掍欢鐩綍锛涘綋鍓嶆湭淇濆瓨 `.blend` 娌℃湁鑷姩淇濆瓨銆傚畨瑁呭寘
`bin/EIEM_Blender-0.26.2-author-angle-preview.zip` 涓?110156 瀛楄妭锛孲HA256锛?
`5674FD8698140C5819AACB744A820CF0F16051290908AD7FBFC18A5C40F429F4`銆傛湰鑺傛病鏈変慨鏀规垨閮ㄧ讲 DLL锛?
涔熸病鏈夊啓鍏ユ父鎴忕洰褰曘€?

### 20.47 Blender 0.30.0 / 浣滆€?v5 / DLL v72锛氱悆浣撲笌鑳跺泭杩涘叆杩愯鏃堕厤缃?

鏈疆鎶婃鍓嶅彧淇濆瓨鍦?Blender 鍦烘櫙涓殑浣滆€呯鎾為泦鍚堟帴鍒扮粍鍚堝鍑哄拰 DLL 杩愯鏃躲€備綔鑰呮牸寮忓崌绾т负 v5锛屽湪鍘熸湁
`radius` 涓庝袱绔悆蹇冭窛绂?`span` 涔嬪澧炲姞 `endRadius` 鍜?`alignedOnCenter`锛泇1/v3/v4 鐨勫師瀛楄妭甯冨眬
缁х画鍏煎璇诲彇銆傜悆浣撹姹備袱绔崐寰勭浉绛変笖 `span=0`銆傝兌鍥婅皟鐢ㄥ師鐢?
`SetSize(startRadius, endRadius, span + startRadius + endRadius)`锛涘叧闂腑蹇冨榻愭椂棣栫鐞冨績浣嶄簬纰版挒鐗╀綋
鍘熺偣锛屽紑鍚椂娌垮師鐢熷眳涓叕寮忓垎閰嶄袱绔窛绂汇€傛棤娉曠敱璇ュ叕寮忔棤鎹熻〃杈剧殑鏋佺寮傚緞灞呬腑鑳跺泭鍦ㄥ鍑烘椂鎷掔粷銆?

浠?`components.json` 瀵煎叆鐨?`BeyondBoneSphereCollider` 鍜?`BeyondBoneCapsuleCollider` 鍙湪浣滆€呯粍
纰版挒闆嗗悎涓洿鎺ュ紩鐢ㄣ€傚鍑哄櫒璇诲彇 Empty 褰撳墠瀛楁锛屽皢鍘熺敓 `center`銆佹柟鍚戣酱銆佸弽鍚戞爣蹇椼€佷袱绔崐寰勩€佸閮ㄩ暱鍜?
涓績瀵归綈鏂瑰紡杞崲鎴愰楠煎眬閮?v5 璁板綍銆傚綋鍓?Typhoea 鍦烘櫙鐨?`maid.002 Skirt Physics` 寮曠敤宸﹀彸澶ц吙銆侀鐩嗗強
宸﹀彸鍓嶈噦鍏变簲涓兌鍥婏紱瀹冧滑鍧囧彲琛ㄨ揪涓?v5銆傚師鐢熸棤闄愬钩闈粛鏃犱綔鑰呰褰曪紝寮曠敤瀹冩椂缁勫悎瀵煎嚭鏄庣‘澶辫触銆?

DLL v72 閫氳繃鍏冩暟鎹悕绉拌В鏋?`ColliderComponent`銆乣BeyondBoneSphereCollider`銆?
`BeyondBoneCapsuleCollider`銆佷袱绉?`SetSize`銆佽兌鍥婃柟鍚?鍙嶅悜/寮傚緞/涓績瀵归綈瀛楁锛屼互鍙?
`ClothSerializeData.colliderCollisionConstraint.colliderList`銆傛瘡涓綔鑰呯鎾炰綋鍙垱寤轰竴涓粦瀹氬埌澹版槑楠ㄩ
鐨勫瓙 GameObject 鍜屽師鐢?Collider 缁勪欢锛涘悓涓€缁勪欢寮曠敤鎸変綔鑰?UUID 鍔犲叆鎵€鏈変娇鐢ㄥ畠鐨勭粍鍒楄〃銆傜鎾炰綋鍦?
`BuildAndRun` 鍓嶆縺娲伙紝骞朵笌鐗╃悊 host 涓€鍚岃繘鍏ュ疄渚嬮€€浼戝拰 Unity 鍘熺敓瀵硅薄姝讳骸鏀堕泦銆傚瓧娈点€佸垪琛ㄦ暟閲忓拰鎴愬憳寮曠敤
鍦ㄥ惎鍔ㄥ墠鍥炶鏍稿銆?

閽堝鎬ч獙璇佸叡 45 涓祴璇曟柟娉曢€氳繃锛氫綔鑰?v1/v3/v4/v5 Python/C++ 寰€杩?9 椤广€丏LL 閰嶇疆 13 椤广€佽繍琛屾椂婧愮爜濂戠害
10 椤广€丅lender 浣滆€?缁勫悎 Skeleton/鐪熷疄 Typhoea 婧愬浘 3 椤广€佸師鐢熸簮鍥?鑳跺泭鍑犱綍 9 椤广€佹彃浠朵笁杞敞鍐岄噸杞?
1 椤广€傚畬鏁?`build.bat` 宸茬敓鎴愭湰鍦?`bin/eiem.dll` 鍜屼袱涓唬鐞?DLL銆傛湰杞皻鏈妸 v72 鍐欏叆娓告垙鐩綍锛屼篃娌℃湁
娓告垙鏃ュ織璇佹槑缁勪欢瀹為檯鍒涘缓銆佽兌鍥婁綅缃?鏂瑰悜銆佺鎾炲搷搴旀垨鍗歌浇缁撴灉锛涜繖浜涙槸瀵煎嚭瀹為檯瑁欐憜鍖呭悗鐨勪笅涓€杞獙鏀堕」銆?

BlenderMCP 宸叉妸褰撳墠 `G:\zmd\typhoeus\18324_autosave.blend` 鐑噸杞藉埌 0.30.0銆傚鍑哄墠鍙戠幇 body Mesh
浠嶇暀鏈?v70 鍙楠ㄩ澶瑰叿鐨?`EIEM_PhysicsTip` 椤剁偣缁勶紝鑰屽搴旀祴璇曢楠煎凡琚垹闄わ紱鎭板ソ 202 涓師
`Bip001_L_Finger02` 鏉冮噸浠嶅湪璇ュ绔嬬粍銆傜幇宸查€愰」鎶婄浉鍚屾潈閲嶆仮澶嶅埌 `Bip001_L_Finger02` 骞跺垹闄ゅ绔嬬粍锛?
鏈慨鏀硅鎽嗘潈閲嶃€傞殢鍚庣敤褰撳墠閫夋嫨鐨?12 涓?Mesh銆佸叡浜?Rig 鍜?`maid.002 Skirt Physics` 鎴愬姛鍐欏嚭鏆傚瓨鍖?
`E:\EIEM_Workspace\export-validation\typhoea-collider-v72-preflight`锛?5 涓枃浠躲€?24988405 瀛楄妭锛?
12 涓?Render 寮曠敤鍚屼竴 v5 Physics锛涜鏂囨。鍚?1 缁勩€?8 鑺傜偣銆? 涓兌鍥婂拰 5 涓粍寮曠敤銆傜敓浜?C++ Mod/Physics
璇诲彇鍣ㄤ互 `validate` 妯″紡鎺ュ彈鏁翠唤 `mod.ini`銆傚綋鍓?`.blend` 淇濇寔鏈繚瀛樼姸鎬併€?

鐢ㄦ埛閫€鍑烘父鎴忓悗锛屽凡浜?2026-09-10 17:24 灏嗘湰鍦?v72 鍐欏叆
`D:\Hypergryph Launcher\games\Endfield Game\plugin\eiem.dll`锛涘畨瑁呮枃浠跺ぇ灏?6322176 瀛楄妭锛孲HA256 涓?
`37E401A7BBB0C15EAA32D581454736F6F9C09C76069B5B629D930BF0848BCB36`锛屽唴鍚?
`[PHYSICS-RUNTIME-v72]`銆傛棫 v70 澶囦唤浣嶄簬
`E:\EIEM_Workspace\plugin-releases\before-v72-colliders-20260910-172406\eiem.dll`銆傛鏃跺皻鏈噸鏂板惎鍔ㄦ父鎴忥紝
閮ㄧ讲浜嬪疄涓嶆瀯鎴愮鎾炰綋鍒涘缓鎴栧搷搴旈獙鏀躲€?

### 20.48 v72 棣栬疆浣滆€?v5 澶辫触銆佽韩浣撳垏绾垮畾浣嶄笌 v73 鏀舵暃璇婃柇

鐢ㄦ埛鍚姩 v72 涓庡疄闄呰鎽嗗寘鍚庤瀵熷埌杞姩瑙嗚鏃剁敾闈㈠儚骞荤伅鐗囪烦鍙樸€佷汉鐗╁厜鐓у紓甯革紝骞朵笖瑁欐憜鐗╃悊鍜岀鎾為兘娌℃湁
鏁堟灉銆俙eiem_log.txt` 缁欏嚭浜嗗悓涓€涓師鍥犻摼锛氬悓涓€妯″瀷鍜屽悓涓€
`PhysicsSkeletonchr_0034_typhoea_postmodel_0` 鍏辫繘琛?400 娆￠厤缃皾璇曪紝姣忔閮藉湪
`Native Physics gravityDirection readback mismatch` 澶勬嫆缁濄€傛棩蹇楁病鏈夎瀹炰緥鐨?`binding`銆?
`build-started` 鎴?`ready`锛屽洜姝よ繖娆℃病鏈夋墽琛?`BuildAndRun`锛屼篃娌℃湁寤虹珛鍙骇鐢熷搷搴旂殑纰版挒缁勩€傜敓浜ч€傞厤鍣ㄦ瘡
250ms 閲嶆柊鍗忚皟涓€娆★紝鑰屽け璐ュ疄渚嬫湭杩涘叆娲诲姩瀹炰緥琛紝閬傚弽澶嶅垎閰?Skeleton/config 鑽夌锛涜繖涓庤瑙掕浆鍔ㄦ椂鐨勫懆鏈熸€?
鍗￠】鏈夌洿鎺ユ椂搴忚瘉鎹紝涓嶈兘瑙ｉ噴鎴愮墿鐞嗗凡缁忚繍琛屼絾鍙傛暟澶急銆?

v73 瀵瑰悓涓€妯″瀷銆佸悓涓€ `EiemPhysicsAsset` 鍜屽悓涓€杩愯鏃?key 鐨勭‘瀹氭€уけ璐ュ彧璁板綍涓€娆★紱妯″瀷姝讳骸銆佽祫婧愬璞″洜 F10
閲嶈浇鏇存崲鎴栨剰鍥炬秷澶卞悗璁板綍浼氭竻闄ゃ€俁enderer/Animator 灏氭湭灏辩华浠嶆爣璁颁负 pending 骞跺厑璁搁噸璇曘€傞噸鍔涙柟鍚戞鏌ュ悓鏃?
鎷嗘垚瀛楁缂哄け/姝т箟銆佸０鏄庣被鍨嬪彉鍖栥€佸啓鍏ュけ璐ャ€佽鍙栧け璐ュ拰甯?expected/actual 涓夊垎閲忕殑鏁板€间笉鍖归厤锛涙暟鍊兼瘮杈冧笉鍐?
鎶婄瓑鍊兼诞鐐圭殑浣嶆ā寮忓樊寮傚綋鎴愬け璐ャ€傝鏀瑰姩娑堥櫎宸茶瀵熷埌鐨?400 娆℃棤鎰忎箟閲嶅缓锛屼絾鏄惁瓒婅繃褰撳墠閲嶅姏鏂瑰悜澶辫触骞跺惎鍔?
妯℃嫙锛屼粛蹇呴』鐢变笅涓€娆℃父鎴忔棩蹇楃‘璁ゃ€?

韬綋鍏夌収鍙﹁鎸変簩杩涘埗 Mesh 鏁版嵁妫€鏌ャ€傚綋鍓嶅畨瑁呭寘鐨?
`MeshS_actor_typhoea_body_01_lod0_0.mesh` 涓?14724 椤剁偣锛屾棫韬綋涓?14664 椤剁偣锛涙柊澧?60 涓槸瀵煎嚭鏃朵负闈㈣
UV/鍒囩嚎宸紓鐢熸垚鐨勬帴缂濆壇鏈€傛寜鍧愭爣瀵归綈鍚庯紝褰撳墠鏂囦欢鐨勬瘡鏉℃硶绾块兘鑳藉湪鏃ц韩浣撲腑鎵惧埌瀹屽叏涓€鑷寸殑婧愭硶绾匡紱澶ц吙闄勮繎
鍚屼綅缃笉鍚屾硶绾跨殑 19 缁勭‖鎺ョ紳鍦ㄦ棫鏂囦欢涓凡缁忓瓨鍦紝鏈彂鐜板鍑哄櫒閲嶇畻娉曠嚎鐨勮瘉鎹€傚紓甯稿嚭鐜板湪鍒囩嚎锛?0 涓《鐐圭殑
`abs(dot(normal,tangent)) > 0.1`锛屾渶澶т负 0.9934247锛屼笖涓昏闆嗕腑鍦ㄩ珮搴︾害 0.815锝?.817 鐨勫ぇ鑵垮唴渚у尯鍩燂紝鍒囩嚎
缁熶竴閫€鍖栦负 `(-1,0,0)`銆傝繖浼氱牬鍧忔硶绾胯创鍥句娇鐢ㄧ殑鍒囩嚎绌洪棿銆?

Blender 0.30.1 鍥犳鎶娾€滃垏绾块潪闆垛€濇墿灞曚负鈥滃垏绾挎湁闄愩€佺鍙锋湁鏁堜笖涓庢渶缁堝簭鍒楀寲娉曠嚎姝ｄ氦鈥濄€傝繛鎺ョ綉鏍笺€佹帴缂濇媶鍒嗘垨
鑷畾涔夋硶绾垮彉鍖栧悗浠嶉潪闆朵絾涓嶅尮閰嶇殑鏃у垏绾匡紝鍙鍙楀奖鍝嶈鐐逛粠 UV0 閲嶅缓锛屽苟閽堝鏈€缁堝鍑烘硶绾挎浜ゅ寲锛涘師娉曠嚎鍜屽叾浣?
鏈夋晥婧愬垏绾夸笉鍙樸€傛柊澧炲钩琛屽垏绾垮洖褰掑湪 Blender 5.0.1 閫氳繃锛屽師鏈夌己澶?娣峰悎/闀滃儚 UV/鑷畾涔夋硶绾?鏃?UV 鍥炲綊涔熼€氳繃銆?

褰撳墠 Blender 鍦烘櫙涓殑韬綋缃戞牸杩樻湁涓€涓笌姝ょ浉鍏崇殑璇垽锛氳褰曠殑娉曠嚎 CRC 涓?`462b7e5c`锛屽綋鍓?CRC 涓?
`283209df`锛屼絾閫愯鐐规柟鍚戞瘮杈冪殑鏈€灏忕偣绉负 `0.9999996349`锛屾渶澶уす瑙掍粎绾?`0.049` 搴︼紝鏈彂鐜扮偣绉綆浜?
`0.99999` 鐨勮鐐广€傝繖鏄?Blender 瀵硅嚜瀹氫箟鍒嗚娉曠嚎鐨勫井灏忛噸鏂扮紪鐮侊紝涓嶆槸鍙鐨勬硶绾夸慨鏀广€?.30.1 鐜板湪浠?CRC
浣滀负蹇€熻矾寰勶紝骞朵互鐪熷疄鏂瑰悜涓€鑷存€т綔涓哄洖閫€锛涜繖绉嶅井灏忕紪鐮佸彉鍖栦細缁х画搴忓垪鍖栫簿纭簮娉曠嚎锛屾槑鏄剧紪杈戞垨杩炴帴鍚庢柊澧炵殑闆?
鍗犱綅娉曠嚎浠嶄細瀵煎嚭褰撳墠娉曠嚎銆傜儹閲嶈浇鍚庣殑褰撳墠韬綋涓存椂瀵煎嚭涓?14724 椤剁偣锛屾墍鏈夐《鐐瑰潎婊¤冻
`abs(dot(normal,tangent)) <= 0.02`锛屽洜姝ゅぇ鑵垮唴渚х殑 50 涓€€鍖栧垏绾垮凡鍦ㄧ绾垮鍑轰腑娑堝け銆傝鍦烘櫙灏氭湭淇濆瓨锛屾寮?
Mod 涔熷皻鏈敤 0.30.1 閲嶆柊瀵煎嚭锛屾父鎴忓唴鍏夌収浠嶉渶浠ラ噸鏂板鍑虹殑鍖呴獙璇併€?

### 20.49 Blender 0.30.2 璐村浘鍚嶇О涓庡師鐢熶緷璧栨敹鏁?

瀹為檯 Typhoea 鍦烘櫙鏆撮湶鍑鸿创鍥炬鍚嶈璇敤涓烘枃浠跺悕鐨勯棶棰樸€傚閮?`body.png` 鍏堣鍐呴儴璧勬簮娈电粺涓€鍔犲墠缂€涓?
`TextureBody`锛屽啀娆″鍏ュ悗鍙堝彲鑳藉舰鎴?`TextureTextureBody`锛涙棫瀵煎嚭鍣ㄩ殢鍚庣敓鎴?
`textures/TextureTextureBody.png`銆?.30.2 灏?INI 璧勬簮娈佃韩浠戒笌纾佺洏鏂囦欢鍚嶅垎寮€锛氳祫婧愭鍙湪缂哄皯 `Texture`
鍓嶇紑鏃惰ˉ涓€娆★紝鍐欏叆 `textures/` 鐨勬枃浠跺缁堜繚鐣欑敤鎴锋墍閫夋枃浠剁殑鍘熷 stem锛屼緥濡?`body.png` 浠嶅鍑轰负
`body.png`銆備袱涓笉鍚屽唴瀹圭殑宸蹭慨鏀硅创鍥捐嫢浣跨敤鍚屼竴鏂囦欢鍚嶄細鏄庣‘鎶ラ敊锛屼笉鍐嶉潤榛樻敼鍚嶃€?

澶?package 瀵煎叆杩樹細涓哄悓涓€鍘熺敓璐村浘鐢熸垚浠呯敤浜?Blender 鍐呴儴鍘婚噸鐨勫熬鍙枫€備緥濡傚綋鍓嶅満鏅殑鍘熺敓
`T_actor_common_cloth_04_RS_4523718382697154697.png` 鏈変袱涓笉鍚岀鐩樿矾寰勫拰涓や釜娈靛悕锛屽叾涓浜屼釜涓?
`TextureT_actor_common_cloth_04_RS_45237183826971546972`锛涗袱浠芥枃浠跺ぇ灏忓潎涓?8225 瀛楄妭涓?SHA-256 鍚屼负
`B085075B3685826BF83FBD6343C152767F86F195540C160F4850594DC5260BB2`銆?.30.2 鍏堟寜鍘熸枃浠跺悕褰掍竴娈靛悕锛?
蹇呰鏃跺啀姣旇緝鏈紪杈戝浘鐗囩殑瀹為檯瀛楄妭锛涗笌鏉愯川瀵煎叆鍩虹嚎鍐呭涓€鑷寸殑璐村浘缁х画鐢?`source` 娓告垙鏉愯川缁ф壙锛屼笉鐢熸垚
Texture 娈垫垨鏂囦欢銆傝剰鍥剧墖鍜屽唴瀹逛笉鍚岀殑鏇挎崲鍥剧墖浠嶄綔涓烘樉寮忚鐩栧鍑恒€?

Blender 5.0.1 澧為噺瀵煎嚭鍥炲綊浣跨敤鐪熷疄 Typhoea 鍖呴獙璇侊細`body.png` 杈撳嚭鏂囦欢鍚嶄繚鎸佷笉鍙橈紝鏉愯川瑕嗙洊寮曠敤
`TextureBody`锛屼汉涓轰粠绗簩涓?package 澶嶅埗鐨勬湭淇敼鍘熺敓璐村浘娌℃湁杩涘叆杈撳嚭锛涚粺璁′负 1 Mesh銆? Material銆?
1 Texture銆?.30.2 鐑噸杞藉悗瀵瑰綋鍓嶅満鏅繘琛屽彧璇昏鍒掞細韬綋鍙緭鍑?`body.png/body_NM.png`锛涗袱濂楀コ浠嗘潗璐ㄥ垎鍒?
鍙緭鍑哄悇鑷殑 diffuse/normal/P 涓庡叡鐢?`white.png`锛沗RS`銆乣RD`銆佸師鐢?emission 鍜?vertex-noise 鍧囩户鎵挎簮鏉愯川銆?
鎻掍欢娉ㄥ唽涓庤祫婧愮绾垮彟澶?37 椤规祴璇曢€氳繃銆?

DLL 鐨勯厤缃笌杩愯鏃剁浉鍏虫祴璇?24 椤归€氳繃锛屽畬鏁?`build.bat` 鎴愬姛銆傛父鎴忛€€鍑哄悗宸查儴缃?v73 鍒?
`D:\Hypergryph Launcher\games\Endfield Game\plugin\eiem.dll`锛屽ぇ灏?6328832 瀛楄妭锛孲HA256 涓?
`2A016E7065FCE10F063A87A420B22FF602FB67E4067F3EF5EEF7E2FC5BD9D8BB`锛屽寘鍚?
`[PHYSICS-RUNTIME-v73]`锛涙棫 DLL 澶囦唤鐩綍涓?
`E:\EIEM_Workspace\plugin-releases\before-v73-gravity-tangent-20260910`銆傛簮鏂囦欢涓庡畨瑁呮枃浠跺搱甯屼竴鑷淬€?

### 20.50 Blender 0.30.3 閲嶅鍑轰笌 DLL v74 瀹炴満鍓嶄慨姝?

v73 棣栨鏂拌繘绋嬫棩蹇楁妸 `gravityDirection` 鐨勭湡瀹炲０鏄庣被鍨嬫姤鍛婁负
`Unity.Mathematics.float3`锛屽洜姝や綔鑰?v5 浠嶅湪閰嶇疆闃舵琚嫆缁濓紝娌℃湁鍑虹幇璇ュ疄渚嬬殑 `build-started` 鎴?`ready`銆?
v74 鎺ュ彈 `UnityEngine.Vector3` 涓庡疄闄?`Unity.Mathematics.float3` 涓ょ涓夋诞鐐瑰瓧娈靛悕锛屼絾鍦ㄥ啓鍏ュ墠浠嶉€氳繃鍏冩暟鎹?
妫€鏌ュ€肩被鍨嬪ぇ灏忓繀椤讳负 12 瀛楄妭銆佸榻愪负 4 瀛楄妭锛屽啓鍏ュ悗鍐嶉€愬垎閲忓洖璇汇€傛祴璇曞彟鐢?16 瀛楄妭鍋囧竷灞€纭涓嶄細瓒婄晫鐚滃啓銆?
杩欎竴淇敼鍙秷闄ゅ凡缁忕敱鏃ュ織璇佹槑鐨勭被鍨嬪悕涓嶅吋瀹癸紱瀹為檯鍒涘缓銆佽鎽嗚繍鍔ㄥ拰纰版挒鍝嶅簲浠嶇敱涓嬩竴娆℃父鎴忚繍琛岀‘璁ゃ€?

杞姩瑙嗚鏃朵汉鐗╃獊鍙樺彟鏈夌嫭绔嬬殑瑁佸壀鏉′欢銆傚綋鍓嶆浛鎹㈣韩浣撲负 14724 椤剁偣骞惰鐩栧畬鏁磋鑹查珮搴︼紝鑰屽懡涓殑鍘熻韩浣撳垎鐗?
鑼冨洿鏄庢樉鏇村皬锛涙棫杩愯鏃剁粰鏇挎崲鍚庣殑 `SkinnedMeshRenderer` 淇濈暀鍘?`localBounds`銆倂74 鍦ㄦ浛鎹㈠墠璇诲彇鍘?
`localBounds`锛屽湪 Mesh 鏋勫缓瀹屾垚鍚庤鍙栨柊 Mesh 鐨?`bounds`锛屽皢浜岃€呰仈鍚堝悗鍐欏洖锛涗紮浼?Renderer 浣跨敤鐩稿悓瑙勫垯銆?
杩欓伩鍏嶆柊澧炲嚑浣曡惤鍦ㄥ師鍖呭洿鐩掑锛屼絾鏄惁瀹屽叏娑堥櫎鐢ㄦ埛瑙傚療鍒扮殑鐢婚潰璺冲彉浠嶉渶瀹炴満杞姩瑙嗚楠屾敹銆傛鍓嶇敤浜庡畾浣嶈祫婧?
璺緞鍜屾畫鐣欏紩鐢ㄧ殑涓存椂 residue 鎺㈤拡宸蹭粠鐢熶骇 `il2cpp_trace.h` 璋冪敤閾剧Щ闄わ紝鏅€氳祫婧愯窡韪绠楀叧闂紝閬垮厤缁х画杈撳嚭
姣忓抚绾ц瘖鏂棩蹇椼€?

Blender 0.30.3 淇涓夐」鎴愬搧閾捐矾闂銆傜涓€锛孖NI 瀵煎叆鍣ㄥ拷鐣?`if/elif/else/endif` 鎺у埗琛岋紝鍙鍙?Mesh銆?
Material銆乀exture銆丼keleton 鍜?Physics 璧勬簮锛屽洜姝ゅ甫鎸夐敭鍒囨崲鐨勫鍑哄寘鍙互閲嶆柊瀵煎叆鑰屼笉鎭㈠鍒囨崲浣滆€呯姸鎬併€?
绗簩锛屽悓涓€娓告垙 Texture 鍦ㄥ涓?package 涓殑鐭簭鍙锋垨璧勬簮鍝堝笇鍚庣紑浼氬綊鍏ュ悓涓€鍘熺敓璐村浘瀹舵棌锛涙湭鏀圭殑 `RD/RS`銆?
鍘熺敓 emission銆乶oise 绛夌户缁敱婧愭潗璐ㄧ户鎵匡紝涓嶅啀琚敊璇鍒躲€傜涓夛紝瀹為檯瑕嗙洊鐨?`_BumpMap` 杈撳嚭
`linear=true`锛岄伩鍏嶈嚜瀹氫箟娉曠嚎 PNG 浠?sRGB 閲囨牱銆傚惈涓や釜 Skeleton 鐨勬垚鍝佸寘浼氭寜姣忎釜 Render 鐨?
`physics=` 涓?`skeleton=` 鍏崇郴涓?Physics 閫夋嫨 Rig锛屼笉鍐嶅洜鍖呭唴楠ㄦ灦鏁伴噺澶т簬涓€鑰屾嫆缁濆鍏ャ€?

褰撳墠 Blender 鍦烘櫙宸茬儹閲嶈浇鍒?0.30.3锛屽鍏ュ拰瀵煎嚭鑿滃崟鍚勪繚鐣欎竴涓洖璋冦€傚綋鍓嶉€夋嫨鎸夋樉寮?
`maid.002 Skirt Physics` 缁勯噸鏂板鍑哄埌
`D:\Hypergryph Launcher\games\Endfield Game\plugin\mods\typhoeus`锛岀粺璁′负 12 Mesh銆? Skeleton銆?
1 Physics銆? Material銆?0 Texture銆俆exture 鐩綍鍙寘鍚?`body/body_NM`銆佷袱濂?maid 鐨?diffuse/normal/P銆?
`siwa` 鍜屽叡鐢?`white`锛涙病鏈夊師鐢?`T_actor_*` 鎴?`T_pipe_*` 鏂囦欢锛屼笁寮犺嚜瀹氫箟娉曠嚎鍧囦负 `linear=true`銆?
鍚庡彴 Blender 浠?`include_physics=True` 鍙嶅悜瀵煎叆鎴愬搧锛屽緱鍒?12 Mesh 鍜?1 涓粦瀹?
`Skeletonchr_0034_typhoea_postmodel_0` 鐨?`maid.002 Skirt Physics` 缁勩€傛寮忚韩浣?Mesh 鐨勬硶绾?鍒囩嚎鏈€澶?
`abs(dot)` 涓?`2.00855806e-07`锛?4724 涓《鐐逛腑娌℃湁瓒呰繃 0.02 鐨勫€笺€?

97 椤?MSVC 閽堝鎬ф祴璇曢€氳繃锛屽畬鏁?`build.bat` 鎴愬姛銆倂74 宸插啓鍏?
`D:\Hypergryph Launcher\games\Endfield Game\plugin\eiem.dll`锛屾湰鍦颁笌瀹夎鏂囦欢 SHA256 鍧囦负
`A511F2ECB9171C13530D47608C7525376D3687040E9DA73405E7DD66134B20B5`锛涙棫 v73 澶囦唤浣嶄簬
`E:\EIEM_Workspace\plugin-releases\before-v74-float3-bounds-20260910\eiem.dll`銆傛湰鑺傚皻鏈惎鍔ㄦ父鎴忥紝
涓嶈兘鎹璁ゅ畾 v74 宸插垱寤虹墿鐞嗙粍浠躲€佸畬鎴?`BuildAndRun`銆佷骇鐢熺鎾炲搷搴旀垨瑙ｅ喅鐢婚潰璺冲彉銆?

### 20.51 v74 瀹炴満缁撴灉涓?DLL v75 鍖呭洿鐩掋€丩OD銆佸彲瑙佸啓鍥炶瘖鏂?

鐢ㄦ埛杩愯 v74 鍚庣‘璁よ浆鍔ㄨ瑙掓椂浜虹墿浠嶄細绐佺劧鍒囨崲锛岃鎽嗙墿鐞嗕篃娌℃湁鍙鏁堟灉銆傛柊杩涚▼鏃ュ織纭
`resource-runtime-v74-float3-bounds` 宸插姞杞斤紱`Unity.Mathematics.float3` 淇鐢熸晥锛屼綔鑰呯墿鐞嗛€氳繃閰嶇疆锛屽垱寤?
5 涓鎾炵粍浠跺苟鎵ц `BuildAndRun`锛岄殢鍚庤繘鍏?`running` 鐨?team 37銆傝瀹炰緥鐩村埌娓告垙閫€鍑烘椂鎵嶈繘鍏?retire銆?
鍥犳 v74 宸查獙璇佺粍浠跺垱寤恒€佺鎾炲垪琛ㄨ閰嶃€乀eam 鍚姩鍜?Animator 韬唤鎺ョ撼锛屼絾娌℃湁楠岃瘉 MOVE 鑺傜偣杩愬姩銆?
鍙瑁欏瓙钂欑毊鍝嶅簲鎴栧疄闄呯鎾炲搷搴斻€?

鍚屼竴鏃ュ織涔熻瘉鏄?v74 鐨勫寘鍥寸洅淇鏍规湰娌℃湁鎵ц锛歚SkinnedMeshRenderer` 鍒濆鍖栬涓殑
`localBounds=0/0`銆俙get_localBounds/set_localBounds` 澹版槑鍦ㄧ埗绫?`UnityEngine.Renderer`锛屾棫浠ｇ爜鍗村彧鏌?
`SkinnedMeshRenderer` 鑷韩銆備紮浼?LOD 鎺ュ彛涔熶负 `0/0`锛沀nity 鏆撮湶鐨勬槸鏂规硶 `GetLODs/SetLODs`锛屾棫浠ｇ爜鏌ユ壘
浜嗕笉瀛樺湪鐨勫睘鎬ц闂櫒 `get_lods/set_lods`銆傝繖涓ゅ閮芥槸鍚庢潵鏂板浼欎即 Renderer 璺緞鐨勭‘瀹氶敊璇紝浼氫娇鏇挎崲
鍖呭洿鐩掍繚鎸侀敊璇紝骞朵娇浼欎即鑴辩婧?Renderer 鐨?LOD 灞傜骇锛涗笉鍐嶆妸鐢婚潰绐佸彉褰掑洜浜庨珮妯￠潰鏁般€?

v75 灏?`localBounds` 鏀逛负娌跨被缁ф壙灞傛煡鎵撅紝骞朵娇鐢ㄧ湡瀹炵殑 `LODGroup.GetLODs/SetLODs`銆傜墿鐞嗕晶鏂板涓€娆℃€с€?
浣庨鑷姩瑙傛祴锛歍eam ready 鍚庣粺璁℃墍鏈夊叡浜悓涓€ Skeleton 瀹炰緥鐨勪紮浼?Renderer 楠ㄩ鏁扮粍瀵?18 涓墿鐞嗚妭鐐圭殑
鍛戒腑鏁帮紝骞舵瘡 500 ms 閲囨牱 12 涓?MOVE 鑺傜偣鐨勫眬閮ㄤ綅缃笌鏃嬭浆锛屾渶澶?16 娆★紝鍙湪绗?4 娆″拰绗?16 娆″啓姹囨€汇€?
杩欎細鎶娾€滃師鐢熸ā鎷熸病鏈夊啓 MOVE Transform鈥濅笌鈥淭ransform 宸插姩浣嗗彲瑙?Mesh 娌＄粦瀹氣€濆垎寮€銆傝瑙傛祴涓嶆帴鍏?Dump
椤甸潰锛屼笉鍒涘缓绗簩濂楃粍浠讹紝涔熶笉鍙備笌璧勬簮閰嶇疆銆?

闈欐€佸绾?60 椤归€氳繃锛涘畬鏁?MSVC 鐜杩愯 234 椤规祴璇曪紝222 椤归€氳繃銆?2 椤规寜鐜璺宠繃銆? 澶辫触锛涘畬鏁?
`build.bat` 鎴愬姛銆傛父鎴忛€€鍑哄悗宸查儴缃?v75 鍒?
`D:\Hypergryph Launcher\games\Endfield Game\plugin\eiem.dll`锛屾湰鍦颁笌瀹夎鏂囦欢 SHA256 鍧囦负
`97A68C4B442FBE6AB885926A0E79844E8E60DD56B7BCAB65E2D757D7CAEEDA92`銆傛棫 v74 澶囦唤浣嶄簬
`E:\EIEM_Workspace\plugin-releases\before-v75-bounds-lod-motion-20260911\eiem.dll`銆傞儴缃叉湰韬笉璇佹槑
鐢婚潰绐佸彉宸茬粡娑堝け锛屼篃涓嶈瘉鏄庤鎽嗘垨纰版挒宸叉湁鍙鍝嶅簲锛涢渶瑕佷笅涓€娆℃父鎴忚繘绋嬫鏌?v75 鐨勯潪绌?bounds/LOD
鎺ュ彛銆佷紮浼?LOD 鍔犲叆璁板綍銆乣visible-binding` 鍜?`motion` 姹囨€汇€?

### 20.52 v75 瀹炴満鍐欏洖缁撴灉涓?DLL v76 浼欎即钂欑毊銆丩OD 淇

鐢ㄦ埛杩愯 v75 鍚庣‘璁や袱涓幇璞″潎鏈秷澶憋細杞姩瑙嗚鏃朵汉鐗╀粛浼氬儚骞荤伅鐗囦竴鏍风獊鍙橈紝瑁欐憜鐗╃悊浠嶆棤鍙鏁堟灉銆?
璇ラ棶棰樹笉鍐嶆寜楂樻ā娓叉煋璐熻浇鍒嗘瀽銆傛柊杩涚▼鏃ュ織鏄剧ず `localBounds` 璇诲啓鎺ュ彛宸茬粡闈炵┖锛岃鏄?v75 鐨勭户鎵垮眰鏌ユ壘淇鐢熸晥锛?
浣?`LODGroup GetLODs/SetLODs` 浠嶄负 `0/闈為浂`锛屾墍浠ヤ紮浼?LOD 缁存姢娌℃湁鎵ц銆?

鍚屼竴鏃ュ織鎶婄墿鐞嗛棶棰樿繘涓€姝ユ敹鏁涖€傜洰鏍囧疄渚嬭繘鍏?running Team锛? 涓叡浜悓涓€ Skeleton 鐨勪紮浼?Renderer 鍏卞惈 998 涓?
楠ㄩ琛ㄩ」锛屽浣滆€呭０鏄庣殑 18 涓墿鐞嗚妭鐐硅揪鍒?18 涓敮涓€鍛戒腑銆傝繛缁?16 娆￠噰鏍蜂腑锛?2 涓?MOVE 鑺傜偣鍧囧彂鐢熷眬閮ㄤ綅绉诲拰鏃嬭浆锛?
鏈€澶у眬閮ㄤ綅缃樊骞虫柟绾?`0.00115446211`锛屾渶澶ф棆杞樊骞虫柟绾?`0.225109577`銆傚洜姝ゅ師鐢熸ā鎷熷凡缁忛┍鍔?Transform锛?
涓斿彲瑙佷紮浼村紩鐢ㄨ繖浜?Transform锛涒€滄病鏈夊彲瑙佽鎽嗗搷搴斺€濅綅浜庡悗缁紮浼磋挋鐨煩闃垫彁浜ゆ垨娓告垙娓叉煋娉ㄥ唽閾捐矾锛屼笉鑳藉啀瑙ｉ噴涓虹墿鐞嗘病鏈夎繍琛屻€?

绂荤嚎璇诲彇褰撳墠鍚敤鐨?`MeshS_actor_typhoea_cloth_01_lod0_2_5.mesh` 杩樼‘璁わ細7295 涓《鐐瑰叏閮ㄨ嚦灏戞湁涓€涓柊澧?
瑁欐憜鐗╃悊楠ㄩ鐨勬鏉冮噸锛?8 鏍?`maid_skirt_01..06_{a,b,c}_jnt` 鍧囪瀹為檯浣跨敤锛涘綋鍓?`state.ini` 涓瀛愬紑鍏充负 0锛?
瀵瑰簲 Part4 澶勪簬鍚敤鍒嗘敮銆傝繖鎺掗櫎浜嗏€滄椿鍔ㄨ鐗囨病鏈夋潈閲嶁€濊繖涓€瑙ｉ噴銆?

闅忓悗鐩存帴璇诲彇鏈父鎴?IL2CPP v29 鍏冩暟鎹紝纭 `UnityEngine.LODGroup.GetLODs` 瀹為檯澹版槑涓?
`GetLODs(bool getPlatformLODs)`锛岃€?`SetLODs` 涓轰竴鍙傛暟銆倂75 鎸?Unity 甯歌鐨勯浂鍙傛暟鍏紑 API 鏌ユ壘锛屽洜姝ゅ緱鍒扮┖鎸囬拡锛?
杩欎笉鏄帴鍙ｆ暣浣撹瑁佸壀銆倂76 鏀逛负鎸変竴鍙傛暟瑙ｆ瀽骞朵紶鍏?`false` 璇诲彇浣滆€?LOD 鏁扮粍銆備紮浼?SkinnedMeshRenderer 鍙︿粠婧?
Renderer 澶嶅埗 `skinningRoot`銆乣quality`銆乣updateWhenOffscreen`銆乣forceMatrixRecalculationPerRender` 鍜?
`skinnedMotionVectors`锛屽苟杈撳嚭涓€娆℃簮/浼欎即鎴愬鍊硷紝鏍稿鏂板缁勪欢鏄惁鍙栧緱鐩稿悓鐨勮挋鐨┖闂翠笌鏇存柊璋冨害銆?

閽堝鎬ч潤鎬佸绾?60 椤归€氳繃锛涘畬鏁?MSVC 鐜杩愯 234 椤规祴璇曪紝222 椤归€氳繃銆?2 椤规寜鐜璺宠繃銆? 澶辫触锛涘畬鏁?
`build.bat` 鎴愬姛銆傚綋鍓嶅寘鍙湁 LOD0 璧勬簮鍜岃鍒欙紝鎵€浠ュ嵆浣夸紮浼存纭繘鍏?LOD0锛屽垏鎹㈠埌娓告垙鍘熺敓 LOD1/2 鏃朵粛鍙兘鍑虹幇
妯″瀷褰㈡€佸垏鎹紱杩欏睘浜?LOD 瑕嗙洊鑼冨洿闂锛屼笌妯″瀷闈㈡暟瀵艰嚧鐨勫抚鐜囦笅闄嶆棤鍏炽€倂76 鐨勯娆″疄鏈洪渶瑕佸垎鍒牳瀵?
`LODGroup get/set lods`銆乣Added partner Renderer`銆乣MOD-PARTNER-SKIN-v76`锛屽啀瑙傚療杩戣窛绂昏浆鍔ㄨ瑙掑拰瑁欐憜杩愬姩銆?

纭娓告垙鐩綍涓嬫病鏈夎繍琛屼腑鐨勮繘绋嬪悗锛屽凡灏?v76 鍐欏叆
`D:\Hypergryph Launcher\games\Endfield Game\plugin\eiem.dll`銆傛湰鍦颁笌瀹夎鏂囦欢澶у皬鍧囦负 6288896 瀛楄妭锛?
SHA256 鍧囦负 `C811ED6E2E6E5DEDD19ECFC453D38B8644C3D260D79B38BA2DD1C3AE416C4DC2`锛泇75 澶囦唤浣嶄簬
`E:\EIEM_Workspace\plugin-releases\before-v76-partner-skin-lod-20260911-005225\eiem.dll`銆傞儴缃插彧涓轰笅涓€杞娴嬫彁渚涙瀯寤猴紝
灏氭湭璇佹槑鐢婚潰绐佸彉鎴栬鎽嗗彲瑙佸啓鍥炲凡缁忎慨澶嶃€?

### 20.53 v76 瀹炴満缁撴灉銆佹彁浜ょ増宸紓涓庡師 Renderer A/B

鐢ㄦ埛杩愯 v76 鍚庡啀娆＄‘璁わ細Mesh 鑳芥樉绀轰笖鍩虹钂欑毊姝ｅ父锛屼絾杞姩瑙嗚鏃朵粛鏈夊够鐏墖寮忕獊鍙橈紝瑁欐憜鐗╃悊娌℃湁鍙鏁堟灉銆?
鏈疆鏃ュ織涓?`LODGroup.GetLODs/SetLODs` 鍧囧凡瑙ｆ瀽锛屼粛娌℃湁浠讳綍 `Added partner Renderer` 璁板綍锛涜繖璇存槑鐩爣瑙掕壊娌℃湁娌?
鏍囧噯 Unity `LODGroup` 绠＄悊璇ユ簮 Renderer锛寁76 鐨勬爣鍑?LOD 鎻掑叆娌℃湁鏀瑰彉浼欎即褰掑睘銆? 涓紮浼寸殑
`quality/updateWhenOffscreen/forceMatrixRecalculationPerRender/skinnedMotionVectors/skinningRoot` 鍧囦笌婧?Renderer
涓€鑷淬€傜墿鐞?Team 鍚屾椂淇濇寔 running锛?2 涓?MOVE 鑺傜偣鍏ㄩ儴鍙樺寲锛屾渶澶у眬閮ㄤ綅缃樊骞虫柟涓?`0.00895480532`銆佹渶澶ф棆杞樊
骞虫柟涓?`0.183481708`銆傚洜姝ゆ湰杞粛鎶婇棶棰樺畾浣嶅湪浼欎即 Renderer 鐨勬父鎴忎笓鐢ㄦ敞鍐?钂欑毊鎻愪氦锛岃€屼笉鏄珮妯¤礋杞姐€侀潤鎬佽挋鐨?
缁戝畾鎴栧師鐢熸眰瑙ｅ櫒娌℃湁杩愯銆?

褰撳墠宸ヤ綔鏍戜笌鏈€鍚庢彁浜?`4558509`锛圖LL v70锛夌殑 Renderer 浠ｇ爜宸紓涓昏鏄?replacement bounds 鍚堝苟銆佷紮浼?
SkinnedMeshRenderer 鐘舵€佸鍒跺拰鏍囧噯 LOD API 淇锛涘叾涓悗涓ら」鐨勫疄鏈鸿鍥炲凡缁忚瘉鏄庢病鏈夎В鍐崇棁鐘躲€傛洿鍏抽敭鐨勬暟鎹樊寮傛槸锛?
v70 鐨勫彲瑙佸疄娴嬫妸鍗曚釜 Mesh 鍘熶綅鍐欏洖娓告垙宸叉湁 SkinnedMeshRenderer锛岃€屽綋鍓?Mod 瀵?cloth 01/02 浣跨敤
`handling=skip`锛屽啀鍒涘缓澶氫釜 partner Renderer銆傛棫 v70 澶囦唤涓殑 cloth 瑙勫垯娌℃湁 partner锛涘够鐏墖寮忕獊鍙樼涓€娆¤褰曞湪
閲囩敤鎷嗗垎琛ｇ墿鍖呯殑 v72 瀹炴祴銆傚洜鑰屸€滃師浣嶆浛鎹⑩€濆拰鈥滄柊澧炰紮浼粹€濇槸褰撳墠鏈€鏈夊尯鍒嗗害鐨勫彉閲忋€?

娓告垙杩涚▼閫€鍑哄悗锛屽凡澶囦唤褰撳墠 `mod.ini/state.ini` 鍒?
`E:\EIEM_Workspace\plugin-releases\before-v77-inplace-render-ab-20260911-011336`锛屽苟鍙敼鍐欏畨瑁?Mod 鐨勪袱涓?cloth
鍏ュ彛锛歝loth 01 鐩存帴鎶婂綋鍓嶈鐗?`MeshS_actor_typhoea_cloth_01_lod0_2_5` 鍐欏叆鍘?Renderer锛宑loth 02 鐩存帴鍐欏叆
`MeshS_actor_typhoea_cloth_02_lod0_3`锛涗笉鍐嶄粠杩欎袱涓叆鍙ｅ垱寤?partner銆侱LL銆丮esh銆丼keleton銆丳hysics 鍜屾潗璐ㄦ枃浠跺潎鏈?
鏀瑰姩銆傝鐗堟湰浼氭殏鏃跺彧鏄剧ず杩欎袱涓。鐗╃墖娈碉紝鍒囨崲閿篃涓嶆帶鍒跺叾浣欒。鐗囷紱鐢ㄩ€旀槸涓€娆℃€ф牳瀵瑰師 Renderer 璺緞鑳藉惁鍚屾椂娑堥櫎
瑙嗚绐佸彉骞舵樉绀烘柊澧炶鎽嗛鐨勭墿鐞嗗啓鍥炪€傚湪瀹炴満缁撴灉鍑烘潵鍓嶏紝杩欎粛鏄?A/B 璇婃柇锛屼笉鑳借浣滃凡纭鏍瑰洜鎴栨寮忎慨澶嶃€?

鐢ㄦ埛瀹屾垚涓婅堪鍘?Renderer A/B 鍚庯紝鐢婚潰绐佸彉浠嶅瓨鍦紱瑁欐憜鍥犵┛妯℃棤娉曞彲闈犲垽鏂槸鍚︿骇鐢熺墿鐞嗗搷搴斻€傚搴旀棩蹇楃‘璁ゆ病鏈夊垱寤?
partner锛宑loth 01 宸插湪娓告垙鍘?SkinnedMeshRenderer 涓婄粦瀹?138 妲介楠艰〃锛屽師浣?Mesh 鍐欏叆鎴愬姛銆傚師鐢?Team 浠嶄负
running锛?2 涓?MOVE 鑺傜偣鍏ㄩ儴鍙樺寲锛屾渶澶у眬閮ㄤ綅缃樊骞虫柟涓?`0.00915865973`锛屾渶澶ф棆杞樊骞虫柟涓?`0.195221469`銆?
鍥犳鈥減artner 鏄獊鍙樼殑鍞竴鍘熷洜鈥濆凡琚惁瀹氥€傝杞?`visible-binding` 涓洪浂鍙洜璇婃柇鍑芥暟浠呯粺璁?partner锛屼笉鑳芥嵁姝ゆ柇瑷€鍘熶綅
cloth 娌℃湁寮曠敤鐗╃悊鑺傜偣锛涘悗缁瘖鏂繀椤诲悓鏃惰鐩栧彈鎺у師 Renderer銆?

涓嬩竴杞繚鎸佸悓涓€涓夊紶 Mesh銆丼keleton銆佹潗璐ㄥ拰鍘熶綅 Renderer 閰嶇疆锛屽彧绉婚櫎鎵€鏈?Render 鐨?`physics=`锛屼互闅旂 Physics
杩愯鏃舵槸鍚﹀奖鍝嶇敾闈€備慨鏀瑰墠鐨勫師浣嶉厤缃凡澶囦唤鍒?
`E:\EIEM_Workspace\plugin-releases\before-v77-physics-off-ab-20260911-011953`銆傚畨瑁?`mod.ini` 鐨?SHA256 涓?
`72228E21BDE778CD1E52BD7FC8D4709136C5642E9B57B29C03F2EFD0A96669F6`銆傝 A/B 灏氭湭杩愯锛屼笉鑳芥彁鍓嶅啓鎴?
Physics 宸叉帓闄ゆ垨宸茬粡纭鏄牴鍥犮€?

鐢ㄦ埛瀹屾垚 Physics-off A/B 鍚庯紝杞姩瑙嗚鏃朵粛鐒跺彂鐢熺浉鍚岀獊鍙樸€傛棩蹇楃‘璁よ杩涚▼娌℃湁鍑虹幇浠讳綍
`PHYSICS-RUNTIME` 鎴?`PHYSICS-PLAN`锛屼篃娌℃湁鍒涘缓 partner Renderer锛沚ody銆乧loth 01 鍜?cloth 02 浠嶉€氳繃娓告垙鍘?
SkinnedMeshRenderer 鍘熶綅鏇挎崲銆傚洜姝?Physics 杩愯鏃跺拰 partner Renderer 鍧囧凡琚帓闄や负鐢婚潰绐佸彉鐨勫繀瑕佹潯浠躲€傝繖涓€杞?
鍒绘剰鍏抽棴浜?Physics锛屼笉鑳界敤浜庡垽鏂鎽嗚繍鍔ㄦ垨纰版挒鏄惁鍙銆?

涓嬩竴杞户缁繚鎸佷笂杩?`mod.ini`銆丮esh銆丼keleton銆佹潗璐ㄥ拰鍘熶綅 Renderer 涓嶅彉锛屽彧灏嗗畨瑁?DLL 浠?v76 鍒囨崲鍒版棫鐨?
`resource-runtime-v70-physics-npc-owner`锛岀敤浜庣洿鎺ュ姣旀渶鍚庢彁浜ょ増涓庡綋鍓?Renderer 瀹炵幇銆傚綋鍓?v76 宸插浠藉埌
`E:\EIEM_Workspace\plugin-releases\before-v70-render-ab-20260911-012715\eiem-v76.dll`锛涘畨瑁呯殑 v70 鏉ヨ嚜
`E:\EIEM_Workspace\plugin-releases\before-v72-colliders-20260910-172406\eiem.dll`锛屽ぇ灏?6266880 瀛楄妭锛孲HA256 涓?
`4F1679649F535C51329DF4469645347622A544BC83CCDB148B3A3E9A4AB199DC`銆俙mod.ini` SHA256 浠嶄负
`72228E21BDE778CD1E52BD7FC8D4709136C5642E9B57B29C03F2EFD0A96669F6`锛屾椿鍔?`physics=` 鏁伴噺涓?0銆傝嫢 v70 浠?
绐佸彉锛岃寖鍥村皢鏀舵暃鍒板綋鍓嶅鍑虹殑 Mesh锛廠keleton 鎴栨父鎴忚嚜瀹氫箟 LOD锛涜嫢 v70 鎭㈠骞虫粦锛屽啀閫愰」妫€鏌?v70 鍒?v76 鐨?
Renderer 鍙樻洿銆傛湰鑺傚彧璁板綍宸查儴缃茬殑璇婃柇鍙橀噺锛屽皻鏃犺杞疄鏈虹粨璁恒€?

v70 棣栨杩愯娌℃湁褰㈡垚棰勬湡鐨勨€滄棫 Renderer + 褰撳墠 Mod鈥濆鐓с€傚惎鍔ㄦ棩蹇楀湪瑙ｆ瀽
`plugin\mods\typhoeus\mod.ini:16` 鏃舵姤鍛?`Invalid/duplicate key chord (file skipped)`锛歷70 灏氫笉鏀寔褰撳墠瀵煎嚭鍣ㄤ娇鐢ㄧ殑
`NUMPAD6` 绛夊皬閿洏鍚嶇О锛屽綋鍓?`shape_speed.*` 涔熷睘浜?v70 涔嬪悗鐨勮娉曘€俆yphoeus 鏁翠唤鏂囦欢鍥犳琚烦杩囷紝鏃ュ織鍙姞杞戒簡
鍏朵粬閰嶇疆涓殑 1 鏉?Render 瑙勫垯銆傜敤鎴锋敞閲婁笌鎭㈠ Typhoeus 閰嶇疆鍚庣湅鍒扮殑閮戒粛鏄父鎴忓師妯″瀷锛孎10 涔熸棤娉曢噸鏂板簲鐢ㄤ竴浠?
瑙ｆ瀽澶辫触鐨?Mod锛涜繖涓嶆槸褰撳墠 Mesh 宸茬敱 v70 鎴愬姛鍔犺浇鍚庣殑缁撴灉銆?

鍊煎緱鍗曠嫭璁板綍鐨勬槸锛氬湪 Typhoeus 閰嶇疆琚畬鏁磋烦杩囥€佸睆骞曟樉绀烘父鎴忓師妯″瀷鏃讹紝鐢ㄦ埛浠嶈瀵熷埌鐩稿悓鐨勭敾闈㈢獊鍙樸€傛鏃跺綋鍓?
Mesh銆丼keleton銆丳hysics 涓庝紮浼?Renderer 鍧囨湭鍙備笌锛屼絾 v70 DLL 鐨勫叏灞€ Hook 鍜?`disable_camera_fade=1` 浠嶅湪杩愯銆?
涓哄尯鍒?EIEM 鍏ㄥ眬杩愯鏃朵笌娓告垙鑷韩琛ㄧ幇锛屾父鎴忛€€鍑哄悗宸叉妸
`D:\Hypergryph Launcher\games\Endfield Game\plugin\eiem.dll` 鏀瑰悕涓?
`eiem.dll.disabled-v70-ab-20260911-013327`锛涙枃浠?SHA256 浠嶄负
`4F1679649F535C51329DF4469645347622A544BC83CCDB148B3A3E9A4AB199DC`锛屽綋鍓嶇洰褰曚笉瀛樺湪鍙姞杞界殑 `eiem.dll`銆備笅涓€杞?
鍙牳瀵瑰畬鍏ㄤ笉鍔犺浇 EIEM 鏃惰浆鍔ㄨ瑙掓槸鍚︿粛绐佸彉銆傝嫢鎭㈠骞虫粦锛屾牴鍥犲湪 EIEM 鐨勫叏灞€ Hook锛忛厤缃紱鑻ヤ粛绐佸彉锛屽綋鍓?Mod
閾捐矾鍗冲彲鎺掗櫎锛屽簲杞煡娓告垙鍘熺敓鑷畾涔?LOD銆佸姩鐢绘垨鏄剧ず璁剧疆銆傛湰娈靛皻鏈褰曟棤 DLL 瀹炴満缁撴灉銆?

鏃?EIEM DLL 瀵圭収涓紝鐢ㄦ埛纭娓告垙鍘熸ā鍨嬭浆鍔ㄨ瑙掓甯革紝娌℃湁绐佸彉銆傚洜鑰岀棁鐘舵潵鑷?EIEM DLL 鐨勫叏灞€杩愯璺緞锛涘綋鍓?
瀵煎嚭 Mesh銆丼keleton銆丳hysics銆佹潗璐ㄥ拰 Mod Render 瑙勫垯閮戒笉鏄鐥囩姸鍑虹幇鐨勫繀瑕佹潯浠躲€備笅涓€椤瑰崟鍙橀噺涓哄叏灞€
`plugin\eiem.ini` 鐨?`disable_camera_fade`銆倂70 鏃ュ織姝ゅ墠鏄剧ず瀹冧负 1锛涘搴?Hook 浼氬湪姣忔
`CameraMono._ProcessDitherByPitch` 杩斿洖鍚庤皟鐢?`ForceClearDither`锛屽嵆浣垮叏閮?Mod 鍧囪В鏋愬け璐ヤ篃浠嶆寔缁繍琛屻€?

娓告垙閫€鍑哄悗宸叉仮澶嶅悓涓€涓?v70 DLL锛屽悓鏃跺彧鎶?`disable_camera_fade=true` 鏀逛负 `false`銆俆yphoeus INI 淇濇寔鍘熸牱锛屼粛浼氬洜
v70 涓嶆敮鎸佸皬閿洏鍚嶇О鑰岃鏁翠唤璺宠繃锛屾墍浠ヨ繖涓€杞睆骞曚粛搴旀樉绀烘父鎴忓師妯″瀷銆傞儴缃?DLL SHA256 涓?
`4F1679649F535C51329DF4469645347622A544BC83CCDB148B3A3E9A4AB199DC`锛涗慨鏀瑰墠鐨勫叏灞€閰嶇疆涓?v70 DLL 澶囦唤浣嶄簬
`E:\EIEM_Workspace\plugin-releases\before-camera-fade-off-ab-20260911-013651`銆傝嫢鏈疆鎭㈠骞虫粦锛屽嵆鍙妸绐佸彉褰掑洜浜?
鐩告満鍙嶈櫄鍖栧姛鑳斤紱鑻ヤ粛绐佸彉锛屽啀缁х画鍏抽棴鍏朵粬鍏ㄥ眬 Hook銆傛湰娈靛皻鏈褰曡杞疄鏈虹粨鏋溿€?

鍏抽棴鐩告満鍙嶈櫄鍖栧悗鐢ㄦ埛浠嶈瀵熷埌绐佸彉锛屽洜姝よ Hook 涔熶笉鏄敮涓€鏍瑰洜銆傛鏃舵棤 Mod 瑙勫垯浠嶄細瀹夎涓ょ被鍏ㄥ眬鍏ュ彛锛氱涓€绫绘槸
Mod 鎵€闇€鐨勮祫婧愩€丷enderer銆丳refab/UI/NPC 鐢熷懡鍛ㄦ湡鍏ュ彛锛涚浜岀被鏄棭鏈?MMD锛忓姩浣滃疄楠岀暀涓嬬殑 HumanPose
`GetInternalAvatarPose`銆佸洓涓叏灞€ Transform 鍐欏叆 Hook銆乣MovementComponent.Tick`銆佷笁涓?FinalIK 鍏ュ彛銆?
`PlayerController.SetMainCharacter`銆佷笁涓?SkeletalMorph锛廙orph Job 鍏ュ彛鍜屽姩鐢诲伐浣滅嚎绋嬨€傚挨鍏?
`Hooked_MorphToBoneJob` 鍦ㄧ涓€娆＄‘璁ゅ璞℃椂浼氱洿鎺ユ妸 EyeLookAtIK 鍐欎负 false锛岃鏄庣浜岀被骞堕潪绾瀵熶唬鐮侊紝涓嶈兘缁х画鎶?
绌?Mod 绛夊悓浜?DLL 瀵规父鎴忕姸鎬佹棤褰卞搷銆?

鍚庣画鎸変簩鍒嗘硶楠岃瘉锛岃€岄潪閫?Hook 閲嶅惎銆傜涓€杞粠鎻愪氦 `4558509` 寤虹珛鐙珛 v70 璇婃柇宸ヤ綔鏍?
`E:\EIEM_Workspace\diagnostics\hook-bisect-v70-20260911-014332`锛氫繚鐣欏叏閮ㄨ祫婧愶紡Renderer 鍏ュ彛锛屾暣缁勭姝笂杩版棫
HumanPose銆乀ransform銆丮ovement銆丗inalIK銆丳layerController銆丼keletalMorph 鍏ュ彛鍜屽姩鐢荤嚎绋嬨€傝宸紓鍙敱缂栬瘧瀹?
`EIEM_BISECT_DISABLE_LEGACY_ANIMATION_HOOKS=1` 鍚敤锛屾瀯寤烘爣璇嗕负
`resource-runtime-v70-bisect-resource-renderer-only`锛涘畬鏁?DLL 鏋勫缓鎴愬姛銆俆yphoeus 浠嶄繚鎸?v70 瑙ｆ瀽澶辫触鐨勯浂瑙勫垯鐘舵€侊紝
`disable_camera_fade=false`銆?

璇婃柇 DLL 宸查儴缃插埌娓告垙鐩綍锛屽ぇ灏?6248448 瀛楄妭锛孲HA256 涓?
`817D77C57CB6EFCD3CEA32D16DD96AA7E124887AF29FFF59DFD02C1AB00EBE8E`锛涢儴缃插墠鐨勫畬鏁?v70 DLL銆佸叏灞€ INI 涓?Mod INI
澶囦唤鍦?`E:\EIEM_Workspace\plugin-releases\before-v70-resource-renderer-bisect-20260911-014605`銆傝嫢鏈疆鐢婚潰骞虫粦锛?
鏍瑰洜鍦ㄦ棫鍔ㄤ綔缁勶紝涓嬩竴杞皢璇ョ粍瀵瑰崐锛涜嫢浠嶇獊鍙橈紝鏍瑰洜鍦ㄨ祫婧愶紡Renderer 缁勶紝涓嬩竴杞璇ョ粍浜屽垎銆傛湰娈靛皻鏈褰曞疄鏈虹粨鏋溿€?
绗竴杞簩鍒嗙殑瀹炴満缁撴灉涓虹敾闈㈠钩婊戙€佹病鏈夌獊鍙樸€傛棩蹇楃‘璁ゅ姞杞界殑鏄?
`resource-runtime-v70-bisect-resource-renderer-only`锛孴yphoeus 涓?0 鏉¤鍒欙紝鐩告満鍙嶈櫄鍖栧叧闂紝鏃у姩浣滅粍瀹夎鐐瑰潎琚?
璺宠繃銆傚洜姝よ祫婧愶紡Renderer銆丳refab/UI/NPC 鐢熷懡鍛ㄦ湡 Hook 鍦ㄨ鏉′欢涓嬪彲浠ユ帓闄わ紝鏍瑰洜浣嶄簬鏃у姩浣滅粍銆?

绗簩杞妸鏃у姩浣滅粍鍒嗘垚涓ゅ崐锛欰 缁勪负 HumanPose銆乣PlayerController.SetMainCharacter`銆丼keletalMorph锛廙orph Job 鍜?
鍔ㄧ敾绾跨▼锛汢 缁勪负鍥涗釜鍏ㄥ眬 Transform 鍐欏叆銆乣MovementComponent.Tick` 鍜屼笁涓?FinalIK 鍏ュ彛銆傚綋鍓嶉儴缃插彧鍚敤 A 缁勶紝
缁х画鍏抽棴 B 缁勶紝鏋勫缓鏍囪瘑涓?`resource-runtime-v70-bisect-pose-morph-only`銆侱LL 澶у皬 6291968 瀛楄妭锛孲HA256 涓?
`6CB917CF9F9DBCBE8415468CBE98EDF0EC5E19771DB5F7B2D903328B502861A0`锛涗笂涓€杞钩婊戠増鏈強閰嶇疆澶囦唤浣嶄簬
`E:\EIEM_Workspace\plugin-releases\before-v70-pose-morph-bisect-20260911-015038`銆傝嫢鏈疆绐佸彉锛屽悗缁彧浜屽垎 A 缁勶紱
鑻ヤ粛骞虫粦锛屽悗缁彧浜屽垎 B 缁勩€傛湰娈靛皻鏈褰曠浜岃疆瀹炴満缁撴灉銆?

绗簩杞疄鏈哄悓鏍峰钩婊戯紝鍥犳 A 缁勫彲浠ユ帓闄わ紝鏍瑰洜浣嶄簬 B 缁勩€傜涓夎疆鎶?B 缁勫啀鍒嗕负锛欱1 涓哄洓涓叏灞€ Transform 鍐欏叆
Hook 涓?`MovementComponent.Tick`锛汢2 涓?`SolverManager.LateUpdate`銆乣BipedIK.UpdateSolver` 鍜?
`IKSolverTrigonometric.OnUpdate` 涓変釜 FinalIK 鍏ュ彛銆傚綋鍓嶈瘖鏂?DLL 鍙惎鐢?B1锛孭ose锛廙orph 涓?B2 鍧囧叧闂紝鏋勫缓鏍囪瘑
涓?`resource-runtime-v70-bisect-transform-movement-only`銆侱LL 澶у皬 6248960 瀛楄妭锛孲HA256 涓?
`C6FCDC81BBB1C08FE716E394C4EC894EA83FAAD99177D650FACC70A983D72D96`锛涗笂涓€杞?A 缁勭増鏈拰閰嶇疆澶囦唤浣嶄簬
`E:\EIEM_Workspace\plugin-releases\before-v70-transform-movement-bisect-20260911-015601`銆俆yphoeus 浠嶄负闆惰鍒欙紝
鐩告満鍙嶈櫄鍖栦粛鍏抽棴銆傛湰娈靛彧璁板綍宸查儴缃茬殑绗笁杞彉閲忥紝灏氭棤瀹炴満缁撴灉銆?

绗笁杞疄鏈轰粛鐒跺钩婊戯紝鍥犳 B1 涔熷彲鎺掗櫎锛涙寜浜屽垎鑼冨洿锛屽墿浣?B2 涓轰笁涓?FinalIK 鍏ュ彛銆備负楠岃瘉璇ョ粨璁哄苟鎺掗櫎鍙湁缁勫悎鏃?
鎵嶅彂鐢熺殑浜や簰锛岀鍥涜疆鍙惎鐢?`SolverManager.LateUpdate`銆乣BipedIK.UpdateSolver` 鍜?
`IKSolverTrigonometric.OnUpdate`锛屽叧闂?Pose锛廙orph銆乀ransform锛廙ovement锛岃祫婧愶紡Renderer 缁勭户缁繚鐣欍€傛瀯寤烘爣璇嗕负
`resource-runtime-v70-bisect-finalik-only`锛孌LL 澶у皬 6256640 瀛楄妭锛孲HA256 涓?
`1EECF0040D3DC1F17BED73FF7B5364772C9305907DC4BA3F959E8291ADD5B0E3`锛涚涓夎疆鐗堟湰涓庨厤缃浠戒綅浜?
`E:\EIEM_Workspace\plugin-releases\before-v70-finalik-bisect-20260911-020023`銆俆yphoeus 浠嶄负闆惰鍒欙紝鐩告満鍙嶈櫄鍖栧叧闂紱
鏈灏氭棤绗洓杞疄鏈虹粨鏋溿€?

绗洓杞?FinalIK-only 瀹炴満澶嶇幇鐢婚潰绐佸彉锛岀‘璁ょ棁鐘舵潵鑷繖涓変釜鍏ュ彛涓殑鑷冲皯涓€涓€傛棩蹇楄瘉鏄庝笁鑰呭潎閫氳繃 IL2CPP 鍏冩暟鎹姩鎬?
瀹夎锛屾病鏈夎蛋纭紪鐮?RVA 鍥為€€锛沗SolverManager.LateUpdate` 鍚屾椂鍚姩浜嗚嚜鍔ㄨ兏楠ㄨ繍鍔ㄩ噰鏍峰苟鍦?161 甯у悗鍐欏嚭 TSV銆?
绗簲杞户缁簩鍒嗭細C1 浠呬繚鐣?`SolverManager.LateUpdate`锛孋2 鐨?`BipedIK.UpdateSolver` 涓?
`IKSolverTrigonometric.OnUpdate` 涓€骞跺叧闂紱鍏朵粬鏃у姩浣滅粍浠嶅叧闂€傛瀯寤烘爣璇嗕负
`resource-runtime-v70-bisect-solver-manager-only`锛孌LL 澶у皬 6255616 瀛楄妭锛孲HA256 涓?
`DE6799EC73617B44E77D804C1FDEB3BDCB038D53C1EF9436B6ED6C37822A0DED`锛汧inalIK-only 鐗堟湰涓庨厤缃浠戒綅浜?
`E:\EIEM_Workspace\plugin-releases\before-v70-solver-manager-bisect-20260911-020511`銆俆yphoeus 浠嶄负闆惰鍒欙紝鐩告満
鍙嶈櫄鍖栧叧闂紱鏈灏氭棤绗簲杞疄鏈虹粨鏋溿€?

绗簲杞疄鏈哄彧鍚敤 `SolverManager.LateUpdate` 鏃跺啀娆″鐜扮敾闈㈢獊鍙橈紝鍥犺€屽皢闂鏀舵暃鍒拌鍏ュ彛锛涙鍓嶇殑
`BipedIK.UpdateSolver` 涓?`IKSolverTrigonometric.OnUpdate` 鍦ㄥ悓缁勬祴璇曚腑灏氭湭琚崟鐙瘉鏄庢湁闂銆傜敓浜ф簮鐮佷腑鐨?
`SolverManager.LateUpdate` Hook 鍘熸湰鍙槸涓轰簡鍦ㄦ瘡甯х粨鏉熷悗璋冪敤鑳搁杩愬姩閲囨牱锛屽苟涓嶆槸鐗╃悊杩愯鏃剁殑蹇呰鍏ュ彛銆傝閲囨牱
浼氶€掑綊鏌ユ壘 Transform銆佽鍙栧Э鎬佸苟鍚屾鍐?TSV锛屼笉鑳戒綔涓烘父鎴忓姛鑳戒繚鐣欙紱瀹冧細鏀瑰彉 FinalIK 鐨勬瘡甯ц皟鐢ㄨ矾寰勶紝涓斿凡琚疄鏈轰簩鍒?
璇佹槑涓庣敾闈㈢獊鍙樺悓鏃跺嚭鐜般€?

宸蹭粠鐢熶骇 `src/init.h`銆乣src/trojan.h` 鍜?`src/eiem_native_physics_diagnostic.h` 鍒犻櫎璇?Hook銆佽兏楠ㄩ噰鏍峰嚱鏁板強鍏?
鑷姩鍚姩/缁撴潫鍏ュ彛銆傛甯?Physics 璧勬簮瑙ｆ瀽銆佸垱寤哄拰鐢熷懡鍛ㄦ湡浠ｇ爜淇濈暀锛涜瘖鏂笉鍐嶆寕鍦?`SolverManager.LateUpdate` 鎴?
Dump UI 涓娿€傚洖褰掓祴璇曞悓鏃惰姹傜敓浜ф簮鐮佷笉瀛樺湪 `Hooked_SolverManager_LateUpdate`銆乣SolverManager.LateUpdate`銆?
`EiemSampleChestMotionAfterLateUpdate` 鍜?`CHEST-MOTION`銆?

淇鍚庣殑姝ｅ紡鏋勫缓鏍囪瘑涓?`resource-runtime-v77-remove-lateupdate-probe`銆侻SVC 瀹屾暣 `build.bat` 鏋勫缓鎴愬姛锛汥LL
澶у皬涓?6278656 瀛楄妭锛孲HA256 涓?
`339EB1823763A82132C4DE5CE5248800DB2A89A11ADB4EDA8787BEE03F0D1462`銆傞儴缃插墠鐨勮瘖鏂?DLL銆佷复鏃?Physics-off Mod銆?
鍏ㄥ眬閰嶇疆鍜岃繍琛岀姸鎬佸浠戒綅浜?`E:\EIEM_Workspace\plugin-releases\before-v77-final-deploy-20260911-025856`銆?
娓告垙鐩綍宸叉仮澶嶅畬鏁?Mod 閰嶇疆锛?2 鏉?`physics=`锛孲HA256 涓?
`B13CBBECD109653902CCEFB8F8B551901454296B93E46554D97C7C5637B10A86`锛夛紝骞舵仮澶?
`plugin\eiem.ini` 鐨?`disable_camera_fade=true`锛沗state.ini` 鍘熸牱淇濈暀銆傞儴缃插悗鐨勬父鎴忓疄鏈虹粨鏋滀粛闇€涓嬩竴娆″惎鍔ㄥ悗鐢辩敤鎴?
纭锛屽綋鍓嶈瘉鎹彧璇佹槑闂鍏ュ彛宸插垹闄ゃ€佹寮?DLL 宸叉瀯寤哄苟瀹屾垚鏂囦欢鏍￠獙銆?

### 20.54 v77 瀹炴満缁撹銆佸綋鍓嶈瀛愬鍑烘牳楠屼笌 v78 寤惰繜婵€娲讳慨澶?

鐢ㄦ埛鍚姩 v77 鍚庣‘璁ょ敾闈笉鍐嶇獊鍙橈紝鍥犳 `SolverManager.LateUpdate` 璇婃柇 Hook 涓庤鐥囩姸鐨勫洜鏋滃叧绯诲凡瀹屾垚瀹炴満闂幆锛?
杩欓」缁撹鍙鐩栫敾闈㈢獊鍙橈紝涓嶇瓑鍚屼簬鏂板瑁欏瓙鐗╃悊涓庣鎾炲凡缁忛€氳繃瀹炴満楠屾敹銆倂77 鏈€鍚庝竴娆¤繍琛屾棩蹇椾腑鐨勬棫鍖呭凡缁忓埌杈?
Physics `ready`锛?2 涓?MOVE 鑺傜偣鍏ㄩ儴浜х敓灞€閮ㄤ綅缃垨鏃嬭浆鍙樺寲锛涗絾褰撳墠 Mod 鍦?03:15 閲嶆柊瀵煎嚭锛屾櫄浜庤娆℃父鎴忚繍琛岋紝
鎵€浠ユ棫鏃ュ織涓嶈兘鐢ㄦ潵瀹ｇО鏂板寘宸茬粡鍦ㄦ父鎴忎腑鎵ц銆?

瀵?03:15 褰撳墠鍖呮寜鏂囦欢鍐呭閲嶆柊鎵弿锛岃€屼笉鏄緷璧?Blender 瀵硅薄椤哄簭鎴栧浐瀹?Part 缂栧彿銆傚敮涓€鍖呭惈鏂板瑁欓鐨勮祫婧愭槸
`MeshS_actor_typhoea_cloth_01_lod0_2_4.mesh`锛?295 涓鍑洪《鐐瑰潎鍏锋湁鏈夋晥钂欑毊锛?38 妲介楠艰〃涓柊澧炵殑
18 涓?`maid_skirt_01..06_{a,b,c}_jnt` 鍏ㄩ儴琚疄闄呮潈閲嶅紩鐢紱娌℃湁闆舵潈閲嶉《鐐规垨瓒婄晫楠ㄧ储寮曘€傜墿鐞嗘枃浠朵负浣滆€?v5锛?
鍖呭惈涓€涓?`maid.002 Skirt Physics` 缁勩€?8 涓妭鐐广€? 涓?FIXED 鏍瑰拰 12 涓?MOVE 鑺傜偣锛屼繚鐣?249 椤瑰師鐢熷弬鏁板苟寮曠敤
宸﹀彸澶ц吙銆侀鐩嗗拰宸﹀彸鍓嶈噦鍏?5 涓兌鍥娿€傚悇 Render Part 鍧囧０鏄庡悓涓€ Skeleton 涓?Physics銆傚綋鍓?
`state.ini` 涓瀛愬彉閲?`$switch_3247ace4b0c74d49=1`锛岃€岃 `_2_4.mesh` 浠呭湪鍊间负 0 鏃跺姞鍏ワ紱瀵瑰簲鍒囨崲閿负
`NUMPAD5`銆傚洜姝よ嫢涓嶅厛鎶婅缁勫垏鍥?0锛岀敾闈腑娌℃湁鍙敤浜庡垽鏂柊澧炶楠ㄨ繍鍔ㄧ殑缃戞牸銆?

`MeshS_actor_typhoea_cloth_02_lod0_clothes` 鐨勪笂涓€杞€滆汉鍦扳€濅綔涓虹嫭绔嬮棶棰樺鐞嗐€傚叾瀵煎嚭鏂囦欢
`MeshS_actor_typhoea_cloth_02_lod0_3_3.mesh` 鏈?3775/3775 鏉¤挋鐨褰曘€?26 妲介楠艰〃銆佹棤闆舵潈閲嶅拰瓒婄晫绱㈠紩锛?
鍏跺師濮嬮《鐐规部 Z 杞翠綅浜?0.841..1.203锛岃€?Y 杞翠粎涓?-0.108..0.177銆傝濮挎€佷笌娓告垙閲岃瀵熷埌鐨勮创鍦扮幇璞′竴鑷翠簬
鈥滄柊澧?SkinnedMeshRenderer 娌℃湁杩涘叆钂欑毊鏇存柊鈥濓紝鑰屼笉鏄枃浠剁己灏戞潈閲嶃€傛棫鍒涘缓娴佺▼浼氬湪娲诲姩 GameObject 涓婂厛
AddComponent锛屼娇 Renderer 鍦?Mesh銆乥ones銆乥indposes銆乺ootBone 涓?skinningRoot 灏氭湭璧嬪€兼椂杩涘叆鍚敤璺緞銆?

v78 灏嗘墍鏈?partner Renderer 鐨勫垱寤烘敼涓洪€氱敤鐨勫欢杩熸縺娲绘祦绋嬶細璇诲彇婧?GameObject 鐘舵€侊紝鍏堣鏂板璞′繚鎸?inactive锛?
瀹屾垚 Mesh銆侀楠艰〃銆乺ootBone銆乻kinningRoot銆丷enderer 鐘舵€併€佽竟鐣屻€佹潗璐ㄤ笌 LOD 褰掑睘鍚庯紝鏈€鍚庢仮澶嶆簮婵€娲荤姸鎬併€?
瀹炵幇涓嶅垽鏂鑹层€丮esh 鍚嶃€丩OD 缂栧彿鎴栭楠煎悕锛沗cloth_02` 鍙槸鏆撮湶閫氱敤鍒濆鍖栭『搴忛棶棰樼殑鏍锋湰銆傛柊澧為『搴忓绾︽祴璇曞悗锛?
鐩稿叧 MSVC/瀹夸富娴嬭瘯 111 椤瑰叏閮ㄩ€氳繃锛宍git diff --check` 鏃犺ˉ涓侀敊璇紝瀹屾暣 `build.bat` 鏋勫缓鎴愬姛銆傞儴缃茬増鏈爣璇嗕负
`resource-runtime-v78-deferred-partner-activation`锛屾湰鍦颁笌娓告垙鐩綍 DLL 鐨?SHA256 鍧囦负
`2AA52AA0FB340B27FF91DD04935255ED29BF4FF1389D023DEF0F351895F2089D`锛泇77 澶囦唤浣嶄簬
`C:\Users\25487\AppData\Local\Temp\EIEM-deploy-backups\eiem-v77-before-v78-20260911-031730.dll`銆?
閮ㄧ讲鍙浛鎹?`plugin\eiem.dll`锛屾牎楠岀‘璁ゅ綋鍓?`typhoeus\mod.ini` 鏈彉鍖栥€倂78 灏氭湭鍚姩娓告垙锛沗cloth_02` 绔欑珛銆?
褰撳墠 `_2_4.mesh` 鐨勭墿鐞嗚繍鍔ㄥ強浜斾釜鑳跺泭鐨勫彲瑙佺鎾炴晥鏋滀粛闇€涓嬩竴娆″疄鏈哄垎鍒‘璁ゃ€?

### 20.55 v78 鐗╃悊鍙傛暟璇佹嵁涓?v79 F10 浠ｉ檯婊氬姩

鐢ㄦ埛鍦?v78 瀹炴満涓凡缁忚瀵熷埌鐗╃悊纰版挒銆傚搴旇繍琛屾棩蹇楃‘璁ゅ綋鍓嶄綔鑰呯墿鐞嗕互 `groups=1 colliders=5` 杩涘叆鏋勫缓锛?
鍘熺敓 Team 鍒拌揪 `ready`锛?2 涓?MOVE 鑺傜偣鍦?16 娆￠噰鏍蜂腑鍏ㄩ儴鍙戠敓鍙樺寲锛屾渶澶у眬閮ㄤ綅缃樊骞虫柟涓?
`0.00720635988`銆佹渶澶у眬閮ㄦ棆杞樊骞虫柟涓?`0.512426734`銆傞厤缃矾寰勫苟闈炲彧璇诲彇鏂囦欢锛氭瀯寤哄墠浼氭妸缁勭骇
`blendWeight/gravity/gravityFalloff/animationPoseRatio/stablizationTimeAfterReset`銆佸崐寰勬洸绾垮拰 249 椤?
`nativeParameters` 鍐欏叆鏂板缓鐨?`ClothSerializeData`锛屾瘡椤瑰潎绔嬪嵆浠庡師鐢熷璞″洖璇诲苟绮剧‘鏍稿锛涗换涓€鍐欏叆銆佹洸绾块敭銆?
绫诲瀷鎴栧洖璇讳笉涓€鑷撮兘浼氭嫆缁濊娆℃瀯寤恒€傛湰杞棩蹇楁病鏈夎繖浜涙嫆缁濄€傚洜姝ゅ彲浠ョ‘璁ゅ綋鍓嶅寘鐨勫弬鏁板凡杩涘叆鍘熺敓姹傝В鍣紝鑰屼笉鏄?
浠呰 Blender 鎴?DLL 瑙ｆ瀽鍚庝涪寮冿紱浣嗗皻鏈敤鍗曞弬鏁?A/B 瀹氶噺纭姣忎竴涓弬鏁板鐢婚潰鐨勭嫭绔嬪奖鍝嶏紝涓嶈兘鎹澹扮О鎵€鏈夊瓧娈电殑
瑙嗚璇箟鍧囧凡閫愰」楠岃瘉銆?

鍚屼竴娆¤繍琛屾毚闇蹭簡鐙珛鐨?F10 鐑噸杞芥晠闅溿€傛瘡娆″叏灞€閲嶈浇鍏堟仮澶嶅苟閿€姣佹棫 partner Renderer锛岄殢鍚庡彉鏇村悗鐨?Skeleton
鏂囦欢闇€瑕侀噸寤猴紱鏃?Physics Team 姝ゆ椂浠嶆寔鏈夋棫 Skeleton锛屾棫瀹炵幇杩斿洖 `Skeleton changed while in use`銆傚叏閮ㄦ柊 partner
鍥犳鍒涘缓澶辫触锛岃€屾簮 Renderer 鍙堝洜 `handling=skip` 淇濇寔闅愯棌锛岃〃鐜颁负妯″瀷鍜屾潗璐ㄥ湪绗竴娆?F10 鍚庢秷澶便€傛寜鍒囨崲閿細鍐嶆
瑙﹀彂 Reconcile锛涙鏃舵棫 Physics 宸查€€浼戙€佹棫 Skeleton 鑺傜偣宸查噴鏀撅紝鎵€浠ユā鍨嬫墠閲嶆柊鍑虹幇銆傛棩蹇楅『搴忔槑纭褰曚簡 partner
澶辫触鍙戠敓鍦?`PHYSICS-RUNTIME retire` 涔嬪墠锛屾帓闄や簡璐村浘缂哄け鍜屽垏鎹㈢姸鎬佹湭淇濆瓨浣滀负杩欐棣栬疆娑堝け鐨勬牴鍥犮€?

v79 鎶?Skeleton 缂撳瓨鏀逛负閫氱敤鐨勪唬闄呮粴鍔細鏂囦欢鏃堕棿鎴冲彉鍖栨椂锛屾棫浠ｉ檯鍋滄鎺ユ敹鏂版秷璐硅€咃紝浣嗙户缁敱鏃㈡湁 Renderer 鎴?
鍘熺敓 Physics 鎸佹湁锛涙柊娑堣垂鑰呯珛鍗充粠鍙樺寲鍚庣殑鏂囦欢鍒涘缓鐙珛鏂颁唬闄呫€傛棫 Team 閫€浼戝悗锛屽叾鏃х鏈夎妭鐐规墠鎸夋棦鏈夊紩鐢ㄨ鏁板洖鏀躲€?
璇ュ疄鐜颁笉鍒ゆ柇瑙掕壊鍚嶃€丮esh 鍚嶃€侀楠煎悕銆丩OD 鎴栧浐瀹氳妭鐐规暟閲忋€備负澶勭悊閲嶅 F10 鍚庨€愭笎澧炲鐨勮创鍦板璞★紝partner 閫€浼戦『搴?
鍚屾椂鏀逛负鍏堝仠鐢ㄦ暣涓?GameObject锛屽啀闅愯棌 Renderer銆佺Щ鍑?LOD銆佽В缁?Transform 骞惰姹?Unity 閿€姣侊紝閬垮厤寤惰繜閿€姣佺獥鍙ｄ腑
宸茶В缁戝璞¤鎺у埗鍣ㄩ噸鏂版樉绀哄湪鍦烘櫙鏍归儴銆?

鏂板鐨勫涓诲洖褰掑垎鍒獙璇佲€滄棫楠ㄦ灦浠嶈娑堣垂鏃跺彲浠ュ垱寤烘柊浠ｉ檯鈥濆拰鈥減artner 蹇呴』鍏堝仠鐢ㄥ啀瑙ｇ粦閿€姣佲€濓紱杩炲悓鐑噸杞姐€佽挋鐨€?
鏉愯川鎭㈠銆佺墿鐞嗛厤缃?鐢熷懡鍛ㄦ湡鍙婃寜閿姸鎬佸叡 126 椤规祴璇曞叏閮ㄩ€氳繃锛屽畬鏁?`build.bat` 鏋勫缓鎴愬姛銆傞儴缃茬増鏈爣璇嗕负
`resource-runtime-v79-hot-reload-skeleton-rollover`锛屾湰鍦颁笌娓告垙鐩綍 DLL 鐨?SHA256 鍧囦负
`BC26F69BC0D1D3B15658D340DCBF7A1F09F2A19BA632297DD03241A1835D0B6F`銆倂78 澶囦唤浣嶄簬
`C:\Users\25487\AppData\Local\Temp\EIEM-deploy-backups\eiem-v78-before-v79-20260911-0341.dll`銆?
閮ㄧ讲浠呮浛鎹?`plugin\eiem.dll`锛屾病鏈変慨鏀?Mod INI銆佺姸鎬併€丮esh銆佹潗璐ㄣ€佽创鍥俱€丼keleton 鎴?Physics 鏂囦欢銆倂79 鐨?
鈥滀竴娆?F10 鍗虫仮澶嶅叏閮ㄦā鍨嬧€濅互鍙娾€滆繛缁?F10 涓嶅啀绱Н璐村湴瀵硅薄鈥濅粛闇€涓嬩竴杞疄鏈洪獙璇侊紝褰撳墠涓嶅緱璁板綍涓哄凡缁忛獙鏀躲€?

### 20.56 v79 瀹炴満澶嶆牳銆?3:46 Physics 涓㈠け鏍瑰洜涓?v80 澧為噺鍒囨崲

v79 杩愯鏃ュ織璇佹槑 03:46 瑕嗙洊瀵煎嚭涔嬪墠鐨勪綔鑰?Physics 纭疄杩涘叆娓告垙鍘熺敓姹傝В锛歡eneration 1 鐨勫彲瑙佺粦瀹氬湪
7 涓?partner銆?76 涓楠兼Ы涓€変腑 18 涓敮涓€鐗╃悊鑺傜偣锛孴eam 鍒拌揪 `ready`锛?6 娆￠噰鏍蜂腑 12/12 涓?MOVE 鑺傜偣
鍏ㄩ儴鍙樺寲锛屾渶澶у眬閮ㄤ綅缃樊骞虫柟 `0.00917975325`銆佹渶澶у眬閮ㄦ棆杞樊骞虫柟 `0.196268007`銆傞噸澶?F10 寤虹珛鐨?
generation 4 鍚屾牱鍒拌揪 `ready`锛屽寘鍚?5 涓鎾炰綋锛涘洜姝も€滃弬鏁板拰纰版挒浠庢湭杩涘叆鍘熺敓瀹炰緥鈥濅笌鏃ュ織涓嶇銆?
杩欎簺璇佹嵁璇佹槑璇ユ棫鍖呯殑鍘熺敓瀹炰緥杩愯锛屼笉绛夊悓浜庢瘡涓弬鏁扮殑瑙嗚璇箟閮藉凡閫愰」 A/B銆?

03:46 鐨勫綋鍓嶅鍑烘櫄浜庝笂杩拌褰曘€傝鍖呯洰褰曚腑宸叉病鏈?`.physics` 鏂囦欢锛宍mod.ini` 涔熸病鏈?`[Physics...]` 鎴?
`physics=`銆傞殢鍚?F10 鐨勭 2201 琛屾槑纭褰?`[PHYSICS-PLAN] previous=1 current=0`锛屽苟閫€浼?generation 4銆?
5 涓鎾炰綋鍜?18 涓?Skeleton 鑷湁鑺傜偣銆傚洜姝ゅ綋鍓嶇敾闈㈢殑杞诲井鎽嗗姩涓嶆槸杩欎唤鏂板 Physics 鐨勬晥鏋滐紝涔熶笉鑳藉厛褰掑拵浜?
鏉冮噸銆傚鍑哄櫒姝ゅ墠鍙敹闆嗘樉寮忛€変腑鐨勭墿鐞嗙粍 Empty锛屽張浼氭竻鐞嗘棫鐢熸垚鐩綍锛涘彧閫?Mesh 瑕嗙洊瀵煎嚭渚挎妸鏈夋晥鐗╃悊渚濊禆鍒犻櫎銆?

瀵?03:46 褰撳墠 Mesh 鍐呭閲嶆柊鎸夐楠艰矾寰勬壂鎻忥紝鐗╃悊瑁欑墖鐜板湪鏄?
`MeshS_actor_typhoea_cloth_01_lod0_2_5.mesh`锛堜綅缃笉鑳藉啓鎴愯繍琛屾椂纭紪鐮侊級銆?295 涓《鐐瑰叏閮ㄥ叿鏈夋湁鏁堣挋鐨紝
鏃犻浂鏉冮噸銆佽秺鐣岀储寮曟垨璺?Mesh bind pose 鍐茬獊銆傛瘡椤剁偣鏂板瑁欓鎬绘潈閲嶆渶灏忕害 0.018銆佷腑浣嶆暟绾?0.173銆?
90 鍒嗕綅绾?0.418銆佹渶澶х害 0.755銆佸钩鍧囩害 0.210锛涙柊澧炶楠ㄦ潈閲嶄腑绾?90.9% 钀藉湪 12 涓?MOVE 鑺傜偣锛屾湭閿欒闆嗕腑浜?
6 涓?FIXED 鏍广€傝繖绗﹀悎姝ゅ墠鈥滃潎鍖€涓旇交鈥濈殑浣滆€呰姹傦紝璇存槑鏉冮噸鍚堟硶浣嗚瑙夊搷搴旇涓诲姩鍘嬩綆锛涘彧鏈夋仮澶?Physics 鍚庝粛鍋忓急锛?
鎵嶅簲浠ュ崟鍙橀噺鏂瑰紡鎻愰珮 b/c MOVE 楠ㄦ潈閲嶏紝鑰屼笉鏄湪 Physics 缂哄腑鏃剁洸鐩噸鍒枫€?

Blender 0.30.3 鐜颁粠鎵€閫夊彲瑙?Mesh 鐨勫疄闄呮鏉冮噸鍙嶆帹鍚?Rig 浣滆€呯墿鐞嗙粍锛歁esh 浣跨敤璇ョ粍浠讳竴鑺傜偣楠ㄩ鏃讹紝
鍗充究鏈墜鍔ㄩ€夋嫨 Group Empty锛屼篃鎶?Physics 涓庡叡浜?Skeleton 绾冲叆瀵煎嚭闂寘锛涘畬鍏ㄤ笉浣跨敤浣滆€呯墿鐞嗛楠肩殑 Mesh
浠嶄繚鎸佺函 Mesh 瀵煎嚭銆傚悗鍙?Blender 鍥炲綊鍏堝湪鏃у疄鐜板緱鍒?`physics=0`锛屼慨澶嶅悗寰楀埌 `physics=1` 骞剁敓鎴?Render 缁戝畾銆?
婧愮爜宸插悓姝ュ埌 `E:\vscode\EIEM_Blender`锛屼絾褰撳墠娓告垙鍖呬笉浼氳浠ｇ爜淇敼鑷姩琛ュ洖锛屼粛闇€鎻掍欢閲嶈浇鍚庨噸鏂板鍑恒€?

鎸夐敭璐村湴鐨勮繍琛屾椂鏃ュ織杩樻樉绀猴細浠讳竴鏅€?`mod control` 閮戒細閿€姣佸悓 Mod 鐨?5锝? 涓?partner銆佹仮澶嶅叏閮ㄦ簮 Renderer锛?
鍐嶉噸寤哄綋鍓嶅叏閮ㄥ彲瑙侀」锛涗竴涓樉闅愰敭鍥犳浼氳鏈彉鍖栫殑琛ｆ湇鍙嶅缁忚繃钂欑毊娉ㄥ唽銆倂80 涓虹函 partner 鍒楄〃鍙樺寲澧炲姞宸泦璺緞锛?
姣忎釜 partner 璁板綍鏉ユ簮 Render锛屼粛琚柊瑙勫垯寮曠敤鐨勫疄渚嬩繚鎸佸師瀵硅薄銆侀楠艰〃涓?Skeleton lease锛屽彧閫€浼戝凡绉婚櫎椤瑰苟鍒涘缓
鏂板椤广€傝瀹炵幇鎸?Mod/Render 鍏崇郴宸ヤ綔锛屼笉鍖呭惈瑙掕壊銆丮esh銆丩OD銆侀楠兼垨鎸夐敭纭紪鐮併€傚搴旂殑鍒嗙被涓庡紩鐢ㄥ叧绯诲涓绘祴璇曘€?
妯″瀷閲嶈浇娴嬭瘯銆乸artner 鐢熷懡鍛ㄦ湡娴嬭瘯鍙婂悗鍙?Skeleton/Physics 渚濊禆娴嬭瘯閫氳繃锛涘畬鏁?`build.bat` 鏋勫缓鎴愬姛銆?

閮ㄧ讲鏋勫缓鏍囪瘑涓?`resource-runtime-v80-selective-partner-controls`锛屾湰鍦颁笌娓告垙鐩綍 DLL SHA256 鍧囦负
`FD6A048FE15C96E964AE807E99C6A17E966F462C3426D40B1B037770DF23D6D0`锛泇79 澶囦唤浣嶄簬
`C:\Users\25487\AppData\Local\Temp\EIEM-deploy-backups\eiem-v79-before-v80-20260911.dll`銆傞儴缃插彧鏇挎崲 DLL锛?
娌℃湁淇敼褰撳墠 Mod銆佺姸鎬佹垨璧勬簮銆倂80 灏氭湭鍚姩娓告垙锛涙櫘閫氭寜閿悗鏄惁浠嶈创鍦般€侀噸鏂板鍑哄悗 Physics 鐨勮瑙夊己搴︿互鍙?
杩炵画 F10 鐨勫畬鏁撮噸寤哄潎鏈疄鏈洪獙鏀躲€傚叏閲忔祴璇曚腑鐨勬潗璐ㄨ创鍥炬暟閲忎笌瑙掔偣鍒囩嚎鍚勬湁涓€椤瑰け璐ワ紱鏈疆娌℃湁淇敼杩欎袱鏉′唬鐮佽矾寰勶紝
浣嗕篃鏈敤鏈疆寮€濮嬪墠鐨勫熀绾胯繍琛岃瘉鏄庡叾涓烘棦瀛樺け璐ャ€傛湰杞浉鍏崇洰鏍囨祴璇曞拰瀹屾暣 DLL 缂栬瘧閫氳繃锛屼笉鎶婂叏閲忔祴璇曡褰曟垚鍏ㄧ豢銆?

### 20.57 UI銆丯PC 涓庡ぇ涓栫晫鐨?Physics 鍒涘缓杈圭晫锛堣皟鏌ョ粨璁猴級

涓夌被鍦烘櫙搴斿叡浜悓涓€涓?Physics 杩愯鏃堕€傞厤鍣紝浣嗕笉搴斿叡浜竴涓叏灞€ Physics 瀹炰緥銆傛纭殑鍒涘缓杈圭晫鏄?
鈥滄ā鍨嬪疄渚嬪凡缁忓畬鎴愩€佸尮閰?Renderer 宸叉敹闆嗐€佷笖瀛樺湪鍞竴鏈€杩?Animator鈥濊繖涓€妯″瀷灞傦紱`RendererInfo._Init`
鍙礋璐?Mesh/鏉愯川鍛戒腑涓庢敹闆?Physics intent锛屼笉鑳界洿鎺ュ垱寤?Physics銆傚ぇ涓栫晫鐢?`PrefabInstantiateProxy`
鎴?`BaseModelViewPart` 瀹屾垚杈圭晫鎻愪緵妯″瀷锛孨PC 鐢?`NPCAvatar.StartNPC` 鎻愪緵锛岃鑹?UI 鐢?
`CharUIModelMono.OnAwake/SetVisible` 鎻愪緵锛涘畠浠悇鑷寔鏈?owner锛岄噴鏀捐竟鐣屽垎鍒繘鍏ュ悓涓€涓畨鍏ㄩ€€浼戞祦绋嬨€?

褰撳墠瀹炵幇宸茬粡鏈夊叡浜叆鍙ｏ細`EiemStoreModelPhysicsIntents` 灏?Renderer 鍛戒腑鐨?intent 浜ょ粰
`EiemReconcileModelPhysics`銆備絾 `src/eiem_native_physics_runtime.h` 鐨勮鍑芥暟鐩墠鏄惧紡蹇界暐 `active`锛?
鍥犳 UI 闅愯棌銆丯PC 鏆傚仠涓庡ぇ涓栫晫鍙鐘舵€佽繕娌℃湁褰㈡垚缁熶竴鐨勬縺娲昏涔夛紝杩欏簲鍦?DLL 鐨勬ā鍨嬬紪鎺掑眰淇銆?

鏈 v80 鏃ュ織涓紝Typhoea 鐨勪竴涓?UI 妯″瀷璁板綍浜?`applied=1`锛岃鏄?UI Hook 鍜?Mesh 瑙勫垯纭疄鍛戒腑锛涘叾瀹?
`applied=0` 鐨?UI 妯″瀷瀵瑰簲鍏跺畠瑙掕壊銆俆yphoea 鐨勬湇瑁?partner 鍒欏洜瀵煎嚭 Skeleton 寮曠敤浜嗗綋鍓?live rig
涓笉瀛樺湪鎴栭噸澶嶇殑纰版挒浣撹矾寰勮€屽垱寤哄け璐ャ€傛洿鍏抽敭鐨勬槸褰撳墠娓告垙鐩綍 `typhoeus` 娌℃湁 `.physics` 鏂囦欢銆?
`mod.ini` 娌℃湁 `[Physics]` 鎴?`physics=`锛屾棩蹇椾篃娌℃湁 `PHYSICS-RUNTIME`锛屾墍浠ユ湰娆¤繍琛屼笉鑳借瘉鏄庝换浣?
UI/NPC Physics 宸插缓绔嬫垨杩愯銆?

涓嬩竴姝ュ簲鍏堝湪鍏变韩妯″瀷缂栨帓灞傝ˉ榻?owner active/ready 鐘舵€侊紝鍐嶄慨姝?Skeleton 瀵?live rig 鐨勭粦瀹氾紝鎭㈠甯?
Physics 鐨勫鍑哄寘鍚庡垎鍒獙璇佸ぇ涓栫晫銆丯PC銆乁I 涓変釜 owner锛涗笉鑳芥妸 UI 鐨?Mesh 鍛戒腑褰撲綔 Physics 宸茬敓鏁堛€?

### 20.58 13:49 閲嶆柊瀵煎嚭鍚庣殑 Physics 鍖呮牳瀵?

20.57 涓叧浜庘€滃綋鍓嶆父鎴忕洰褰曟病鏈?Physics鈥濈殑缁撹鍙搴?03:46 鐨勬棫 Mesh-only 瀵煎嚭鍖咃紝涓嶈兘鐢ㄤ簬鍒ゆ柇鍚庣画瀵煎嚭銆?
鐢ㄦ埛鍦?13:49 閲嶆柊瀵煎嚭鍚庯紝褰撳墠 `typhoeus/mod.ini` 宸插寘鍚?
`[Skeletonchr_0034_typhoea_postmodel_0]`銆?
`[PhysicsSkeletonchr_0034_typhoea_postmodel_0]`锛屽苟鍦ㄥ涓?`Render` action 涓啓鍏?
`physics=PhysicsSkeletonchr_0034_typhoea_postmodel_0`銆傚搴旂殑
`physics/PhysicsSkeletonchr_0034_typhoea_postmodel_0/PhysicsSkeletonchr_0034_typhoea_postmodel_0.physics`
鏂囦欢涔熷瓨鍦紝鏂囦欢澶翠负 `EIEPHYS` v5锛屽苟鍖呭惈鍘熺敓鑳跺泭纰版挒浣撹褰曘€?

鍥犳褰撳墠璇佹嵁鏀寔鈥淏lender 瀵煎嚭鍣ㄥ凡缁忔妸鏈鐗╃悊璧勬簮鍐欏叆娓告垙鍖呪€濓紝涓嶆敮鎸佲€滄父鎴忓唴 Physics 宸茬粡寤虹珛骞舵纭ā鎷熲€濄€?
鍚庤€呬粛闇€浠ユ湰娆″寘鍚姩鍚庣殑 `PHYSICS-RUNTIME` 鏃ュ織銆乷wner 鍛戒腑銆乀eam ready 鍜岃妭鐐硅繍鍔ㄩ噰鏍锋牳瀹炪€傝嫢鍐嶆鍙鍑?
Mesh锛屾棫鐢熸垚鐩綍浼氳娓呯悊锛宍mod.ini` 鍙兘鍐嶆鍥炲埌娌℃湁 `physics=` 鐨勭姸鎬侊紱妫€鏌ユ椂蹇呴』鍚屾椂璁板綍鍖呯殑瀵煎嚭鏃堕棿銆?
`mod.ini` 鍜?`.physics` 鏂囦欢锛岄伩鍏嶆妸涓嶅悓瀵煎嚭鎵规娣蜂负涓€娆＄粨鏋溿€?

### 20.59 UI銆丯PC 纰版挒浣撴潵婧愪笌褰撳墠鏈畬鎴愰」

鈥淧hysics 纰版挒楠ㄩ涓嶅彲鐢ㄢ€濇寚杩愯鏃剁粦瀹氶樁娈电殑澶辫触锛屼笉鏄?`.physics` 鏂囦欢娌℃湁瀵煎嚭纰版挒浣撱€傚綋鍓嶉€傞厤鍣ㄥ厛鎶婂鍑?
Skeleton 鐨勮妭鐐硅矾寰勮В鏋愬埌鏌愪釜妯″瀷瀹炰緥鐨?live Transform锛屽啀鐢ㄥ悓涓€浠借矾寰勮〃鏌ユ壘姣忎釜 collider 鐨?`source.bone`锛?
婧愮爜鍦?`EiemPhysicsRuntimeCreateColliders` 涓槑纭墽琛岃繖涓€姝ャ€傚彧瑕?UI 鎴?NPC 鐨勫疄闄呭眰绾х己灏戣鐩稿璺緞锛屾垨璺緞鍑虹幇
閲嶅姝т箟锛岃妯″瀷瀹炰緥鐨?Physics 鏋勫缓灏变細琚嫆缁濄€傚鍑烘枃浠舵湰韬彲浠ュ畬鏁村瓨鍦紝浠嶇劧浼氬嚭鐜拌繖涓繍琛屾椂閿欒銆?

澶т笘鐣?PFB 鏄墿鐞嗕綔鑰呮暟鎹殑鏉ユ簮锛屼絾涓嶆槸 UI銆丯PC 鐨勮繍琛屾椂瀵硅薄韬唤銆俇I 浣跨敤瑙掕壊灞曠ず prefab锛孨PC 鐢?
`NPCAvatar` 缁勮妯″瀷锛涘畠浠彲鑳藉鐢ㄥ悓涓€瑙掕壊楠ㄦ灦鍛藉悕锛屼篃鍙兘浣跨敤瑁佸壀銆佸彉浣撴垨涓嶅悓鏍硅妭鐐圭殑灞傜骇銆傝嫢鐩稿楠ㄩ璺緞
鍜?Transform 鎷撴墤瀹屽叏涓€鑷达紝鍚屼竴浠?Physics 璧勬簮鍙互澶嶇敤锛岀鎾炰綋鐨勫舰鐘躲€佸崐寰勩€佸弬鏁颁笉闇€瑕佹寜鍦烘櫙澶嶅埗涓変唤锛涜繍琛屾椂
鍙簲鎶婂畠浠垎鍒寕鍒板悇鑷疄渚嬬殑瀵瑰簲楠ㄩ涓娿€傝嫢鎷撴墤涓嶄竴鑷达紝灏卞繀椤讳负璇ュ疄渚嬪鍑哄尮閰嶇殑 Skeleton/Physics 鏉ユ簮锛屼笉鑳芥妸
澶т笘鐣?PFB 鐨?Transform 鎸囬拡鎴栫粷瀵硅矾寰勭‖濂楀埌 UI/NPC銆?

鍥犳褰撳墠鐘舵€佸垎鎴愪笁灞傦細Mesh 鍛戒腑鍜?UI/NPC owner 鍏ュ彛宸叉湁瀹炴満璇佹嵁锛涘綋鍓?13:49 鍖呯殑 Physics 璧勬簮宸茬粡鍐欏叆锛?
浣?UI/NPC 瀵瑰綋鍓嶄簲涓兌鍥婂強鍏ㄩ儴瀵煎嚭楠ㄩ鐨勯€愬疄渚嬬粦瀹氬皻鏈噸鏂伴獙鏀躲€俙EiemReconcileModelPhysics` 浠嶆樉寮忓拷鐣?
`active`锛岃€岀鎾為楠肩己澶辨椂浼氭嫆缁濇暣娆℃瀯寤恒€備笅涓€姝ュ簲鍦ㄤ笉纭紪鐮佽鑹插悕銆丳FB 鎴栧満鏅殑鍓嶆彁涓嬶紝涓烘瘡涓ā鍨嬪疄渚嬭褰?
鈥滃疄闄?Skeleton 鏍广€佹垚鍔熻В鏋愮殑楠ㄩ鏁般€佺鎾炰綋缁戝畾鏁扳€濓紝鍒嗗埆楠岃瘉澶т笘鐣屻€丯PC銆乁I锛涗笉鑳戒粎鍑?Physics 鏂囦欢鏉ヨ嚜澶т笘鐣?
PFB 灏卞绉颁笁绫诲疄渚嬮兘宸茬敓鏁堛€?

### 20.60 v80 鍖呰繍琛岀粨鏋滐細澶т笘鐣屾垚鍔燂紝瑙掕壊 UI 鏆撮湶涓ょ被鐙珛缁戝畾闂

鐢ㄦ埛閫€鍑哄悗鐨勬渶鏂?`eiem_log.txt`锛圥ID 17328锛夌粰鍑轰簡鏈鍖呯殑瀹為檯鍒嗗弶銆傚ぇ涓栫晫瀹炰緥
`model=0000000FD6BE2A20` 鍦?`PrefabInstantiateProxy.OnCompleted` 寤虹珛 1 涓?Physics group銆? 涓?
collider锛岄殢鍚庤褰?`ready teams=17`锛?6 娆￠噰鏍蜂腑 12/12 涓?MOVE 鑺傜偣鍙樺寲銆傝繖璇佹槑褰撳墠 13:49 瀵煎嚭鐨?Physics
璧勬簮鍦ㄨ嚦灏戜竴涓ぇ涓栫晫妯″瀷瀹炰緥涓婂畬鎴愪簡鍒涘缓銆佺鎾炲垪琛ㄦ敞鍐屽拰鍘熺敓杩愬姩銆?

璇ヨ繍琛屾棩蹇椾娇鐢ㄧ殑杩愯鏃舵爣绛句粛鏄?`[PHYSICS-RUNTIME-v76]`锛涘畠鏄簮鐮佷腑鐨勮瘖鏂爣绛撅紝涓嶈兘鍗曠嫭鐢ㄦ潵鎺ㄦ柇閮ㄧ讲鏋勫缓
缂栧彿锛屽綋鍓嶅畨瑁?DLL 鐨?SHA256 浠嶉渶浠ユ枃浠跺搱甯屼负鍑嗐€?

瑙掕壊 UI 瀹炰緥 `model=0000001097CA0E60` 鐨?Physics 璁″垝纭疄杩涘叆浜?`CharUIModelMono.OnAwake`锛屼絾绗竴娆℃瀯寤哄洜
`Ambiguous live Skeleton path` 鎸囧悜 `EIEM Partner Render...` 澶辫触銆傚悗缁噸璇曞張鎶ュ憡
`Source bone is missing (not a new bone): .../Magica Capsule Collider (Bip001_L_Forearm_Large)`锛岄殢鍚庡悓涓€缂哄け璺緞
浣垮涓?UI partner 鍒涘缓澶辫触銆傝繖閲岀殑鈥滅己澶扁€濅笉鏄?UI 娌℃湁 `Bip001_L_Forearm` 涓婚楠硷紝鑰屾槸 UI 灞傜骇娌℃湁澶т笘鐣?PFB
涓綔涓烘簮鑺傜偣瀵煎叆鐨?`Magica Capsule Collider (...)` 瀛?Transform銆傚綋鍓嶅鍑虹殑 Physics Skeleton 鍏?415 涓妭鐐癸紝
鍏朵腑 397 涓爣涓烘簮鑺傜偣銆?8 涓负鏂板鑺傜偣锛涚鎾炰綋 owner 鑺傜偣灞炰簬鍓嶈€呫€?

鏈杩樻病鏈?`NpcAvatar` 鐨?Physics owner/ready 璁板綍锛屽彧鏈?`PrefabProxy`銆乣BaseModelPart` 鍜岃鑹?UI 璁板綍锛屽洜鑰?
涓嶈兘鎶?NPC 鍐欐垚鎴愬姛鎴栧け璐ャ€備笅涓€姝ョ殑瀹炵幇搴旀媶鎴愪袱涓槑纭慨澶嶏細Skeleton 瑙ｆ瀽鍙粠 Renderer 楠ㄩ palette 鍙婂叾绁栧厛
寤虹珛婧愯矾寰勮〃锛屼笉鑳介亶鍘嗗寘鍚?EIEM Partner 鐨勬暣妫垫ā鍨嬫爲锛汸hysics 瀹炰緥閬囧埌缂哄け鐨勭鎾炰綋 owner 鏃讹紝搴斿湪鍏舵渶杩?
瀛樺湪鐨勭埗楠ㄩ涓嬪垱寤?EIEM 鑷湁鐨勯敋鐐?Transform锛屽苟浣跨敤瀵煎嚭 Skeleton 涓鑺傜偣鐨勫眬閮?TRS锛屽啀鎸傝浇 Collider锛?
鑰屼笉鏄妸澶т笘鐣?PFB 鐨?Transform 鎸囬拡甯﹀埌 UI/NPC銆傝繖涓瓥鐣ヤ粛鎸夎祫婧愯矾寰勫拰瀹為檯鎷撴墤宸ヤ綔锛屼笉鎸夎鑹插悕鎴栧満鏅‖缂栫爜銆?

### 20.61 v81 UI/NPC 缁戝畾淇锛堜唬鐮佷笌绂荤嚎楠岃瘉锛?

閽堝 20.60 鏃ュ織涓殑涓や釜鏄庣‘澶辫触鐐癸紝v81 鍋氫簡涓夐」鏀舵暃锛?

1. `EiemSkeletonSourceNodes` 涓嶅啀閬嶅巻閿氱偣涓嬬殑瀹屾暣 Transform 瀛愭爲銆傚畠鍙寜 Skeleton 璧勬簮澹版槑鐨勭浉瀵硅矾寰勯€愭鏌ユ壘锛屽洜姝?EIEM 鑷繁鐢熸垚鐨?`Partner` 灞傜骇涓嶄細鍒堕€犱吉閫犵殑閲嶅璺緞锛涚湡瀹炲悓鍚嶆簮鑺傜偣浠嶄細鏄庣‘鎶?ambiguous銆?
2. 鏄庣‘缁戝畾 Physics 鐨?Render/Skeleton 璧勬簮浼氭妸鍏?Skeleton 鏂囨。涓殑婧愯妭鐐硅矾寰勬敹闆嗕负鏄惧紡 `virtualSkeletonPaths`銆傚綋 UI/NPC 缂哄皯澶т笘鐣?PFB 瀵煎嚭鐨勭鎾炰綋鎴栫墿鐞嗛摼鑺傜偣鏃讹紝Skeleton 閫傞厤鍣ㄥ厑璁歌璺緞缂哄け锛屽苟鍦ㄥ凡瑙ｆ瀽鐨勭埗楠ㄩ涓嬪垱寤哄甫鏈夊鍑哄眬閮?TRS 鐨?EIEM 绉佹湁閿氱偣锛涙櫘閫?Mesh-only Skeleton 璧勬簮浠嶄繚鎸佲€滅己澶辨簮楠ㄩ鍗冲け璐モ€濄€侰ollider 浠嶉€氳繃杩欎釜 Transform 缁戝畾锛屼笉澶嶅埗澶т笘鐣?PFB 鐨勫璞℃寚閽堛€?
3. `EiemReconcileModelPhysics` 涓嶅啀蹇界暐 `active`銆傛ā鍨?owner 涓嶅彲瑙佹椂锛屾墍鏈夊睘浜庤妯″瀷鐨?host 鍜岀鎾炰綋杩涘叆鏃㈡湁鐨?`BeginRetire`/native-death 绛夊緟娴佺▼锛涢噸鏂板彲瑙佸悗鎵嶅厑璁告柊涓€杞瀯寤恒€傛病鏈変娇鐢?`DisposeInternal`銆佽鏁板櫒鎴?trace 浣滀负瀹屾垚鏍呮爮銆?

鍥炲綊瑕嗙洊浜嗛噸澶?Partner 鍚嶇О銆佺己澶辨簮鑺傜偣鐨勫師鏈夋嫆缁濊涔夈€丳hysics 铏氭嫙 owner 濂戠害鍜?inactive retire 濂戠害銆侻SVC 涓嬪叏濂?`unittest` 涓?240 椤归€氳繃銆?2 椤瑰洜鐜缂哄皯 Blender/澶栭儴宸ュ叿璺宠繃锛沗build.bat` 瀹屾暣鏋勫缓閫氳繃銆倂81 DLL 宸查儴缃插埌 `D:\Hypergryph Launcher\games\Endfield Game\plugin\eiem.dll`锛孲HA256 涓?`EB404FC412842D2661CB180E43BF040AD971A208BC34A2C9D5F7CE0992DFCF10`锛屾棫鏂囦欢澶囦唤鍦?`C:\Users\25487\AppData\Local\Temp\EIEM-deploy-backups\`銆傝繖浠嶄笉绛変簬 UI/NPC 瀹炴満鎴愬姛锛涗笅涓€娆℃父鎴忚繍琛岃妫€鏌?`PHYSICS-RUNTIME-v81` 鐨?UI銆丯PC 涓庡ぇ涓栫晫瀹炰緥鏃ュ織锛屼互鍙?collider 鏁伴噺鍜?`ready`/`motion` 璁板綍銆?

### 20.62 v81 瀹炴満缁撴灉涓?v82 淇

閲嶅惎鍚庣殑 v81 鏃ュ織纭澶т笘鐣屽疄渚嬩粛寤虹珛 1 涓?Physics group銆? 涓?collider 骞惰繘鍏?`ready`锛?2 涓?MOVE 鑺傜偣鎸佺画鍙樺寲锛涗絾瑙掕壊 UI 鍦?Mesh Partner 鍒涘缓鍜?Physics 鏋勫缓鍓嶉兘鍥犲悓涓€鏉?`Magica Capsule Collider (Bip001_L_Forearm_Large)` 婧愯妭鐐圭己澶辫€屽け璐ャ€傝В鏋愬綋鍓?`.physics/.skeleton` 鍚庡彂鐜?Skeleton 鏈?415 涓妭鐐癸紝瀹為檯 Physics collider 渚濊禆涓?5 涓紝PFB 涓叾浣欑鎾炶妭鐐逛粛浠?source 鑺傜偣淇濆瓨鍦?Skeleton 涓紝鍥犳浠呭厑璁?5 涓?`collider.bone` 涓嶈冻浠ヨ UI 澶嶇敤璇?Skeleton銆?

v82 宸插皢铏氭嫙鍖栬寖鍥存敼涓衡€滃甫 `physics=` 鐨?Render 鎵€澹版槑 Skeleton 鏂囨。涓殑鍏ㄩ儴 source 鑺傜偣鈥濓紝缂哄け鑺傜偣鎸夊鍑?TRS 鍦ㄦ渶杩戝凡瑙ｆ瀽鐖惰妭鐐逛笅鍒涘缓锛涙病鏈?Physics 鐨?Mesh-only Skeleton 浠嶄繚鎸佷弗鏍肩己澶辨鏌ャ€倂82 鐨勯鏋?濂戠害娴嬭瘯鍜屽畬鏁?MSVC 鏋勫缓宸查€氳繃锛屽苟宸查儴缃插埌娓告垙鐩綍锛涙湰娆￠儴缃?DLL SHA256 涓?`D9A2E030C659CD115F0CD71679E47F5D408F3CA162D4449E2E40C14B4762A802`锛屾棫鏂囦欢澶囦唤鍦?`C:\Users\25487\AppData\Local\Temp\EIEM-deploy-backups\`銆傞噸鏂板惎鍔ㄥ悗浠嶉渶妫€鏌?UI銆丯PC 鍜屽ぇ涓栫晫涓夌被 owner 鐨?Physics 鏃ュ織銆?

### 20.63 v83 rootBone late-binding and switch investigation

The ground-position symptom changes after restart and after outfit-key changes. The current package does not contain `siwa` or `neiku` resources; those labels map to switch-controlled Render partners in `mod.ini`. The switch path therefore remains resource-driven and is not a valid reason to add a mesh-name exception.

The relevant lifecycle gap was in Partner creation: `EiemCreatePartnerRenderer` copied the source SkinnedMeshRenderer `rootBone` only once. The game can call `SetSMRRootBone` after the source Renderer has already been observed and after one or more Partners have been created. The source then has a valid root bone while an early Partner retains null or stale rootBone, which can place only the parts created in that timing window at the scene origin. Recreating a different switch branch changes which part falls into the window, matching the reported restart-dependent symptom.

v83 synchronizes rootBone by the source Renderer relationship, without hardcoded mesh, character, LOD, or switch names. It runs after `AssignSkinPost`, after `SetSMRRootBone`, and at the end of every Partner replay. Synchronization only writes a live source rootBone to a live Partner and verifies the setter readback; it does not use counters, disposal returns, or dump output as completion fences. Creation logs now include `rootBone=source/partner`, and late repairs are logged as `[MOD-PARTNER-ROOT]`.

Offline Partner, skeleton, and resource-contract tests pass; `build.bat` passes. v83 was deployed to the game directory with the previous DLL backed up on `E:` at `E:\EIEM_Workspace\plugin-releases\before-v83-rootbone-sync-20260911-164206`. Gameplay confirmation is still pending: after launching, switch the affected groups and check whether the log records root synchronization for the newly created Partners and whether the affected parts remain attached to the live rig.
### 20.64 v83 瀹炴満鏃ュ織锛氬ぇ涓栫晫/UI 宸茶繍琛岋紝NPC 涓庣鎾炲Э鎬佷粛闇€鍗曠嫭楠屾敹

鏈 v83 杩愯鏃ュ織纭浜嗕袱涓嫭绔嬫ā鍨嬪疄渚嬭繘鍏ュ悓涓€浠?Physics 璧勬簮锛?

- 澶т笘鐣?`PrefabInstantiateProxy.OnCompleted`锛歚groups=1`銆乣colliders=5`锛岄殢鍚?`ready`锛?6 娆￠噰鏍蜂腑 `12/12` 涓?MOVE 鑺傜偣鎸佺画鍙樺寲銆?
- 瑙掕壊 UI `CharUIModelMono.OnAwake`锛氬悓鏍峰缓绔?`groups=1`銆乣colliders=5`锛岄殢鍚?`ready`锛?6 娆￠噰鏍蜂腑 `12/12` 涓?MOVE 鑺傜偣鎸佺画鍙樺寲銆?

鍥犳褰撳墠璇佹嵁鍙互纭 Physics 缁勩€佺墿鐞嗛摼鍜屼簲涓鎾炰綋宸茬粡杩涘叆鍘熺敓杩愯鏃讹紱鈥滆瀛愯鎸ゅ帇鈥濅篃涓嶆槸闈欐€佽挋鐨€犳垚鐨勩€傛棩蹇椾粛鍙褰曠鎾炰綋鏁伴噺锛屾病鏈夐€愪釜璁板綍 `source.bone` 瀵瑰簲鐨勫疄闄?Transform銆佸眬閮ㄤ綅缃?鏃嬭浆鍜岃兌鍥婂崐寰勮鍥烇紝鎵€浠ヤ笉鑳戒粎鍑?`colliders=5` 璇佹槑 UI 鐨勭鎾炰綋绌洪棿浣嶇疆姝ｇ‘銆傛尋鍘嬫柟鍚戞垨寮哄害寮傚父浠嶅彲鑳芥潵鑷鎾炰綋缁戝畾灞傜骇銆佸眬閮ㄥЭ鎬併€佸崐寰?闀垮害锛屾垨瑁欏瓙鏉冮噸銆?

鏈杩愯娌℃湁鍑虹幇 `NPCAvatar.StartNPC` 鐨?Physics owner/ready 璁板綍锛屽彧鏈?PrefabProxy銆丅aseModelPart 鍜?CharUIModel 璁板綍锛涘洜姝?NPC Physics 浠嶆湭琚繖娆¤繍琛岄獙鏀讹紝涓嶈兘鎶?UI 鐨勬垚鍔熺粨鏋滄帹骞垮埌 NPC銆備笅涓€姝ュ簲鍦ㄦ櫘閫?NPC 瀹炰緥鍑虹幇鍚庡啀妫€鏌ュ叾鐙珛鐨?`binding/build-started/ready/motion` 璁板綍锛屽苟琛ュ厖姣忎釜 collider 鐨勫疄闄呯粦瀹?Transform 涓庡嚑浣曡鍥炴棩蹇楋紝鍐嶅垽鏂?UI 鎸ゅ帇鏄惁闇€瑕佽皟鏁寸鎾炰綋鎴栨潈閲嶃€?
### 20.65 v84 纰版挒浣撻€愰」杩愯杩借釜

v84 鍙鍔犳櫘閫氳繍琛屾棩蹇楋紝涓嶆敼鍙?Physics 鍒涘缓銆佹寜閿垏鎹㈡垨纰版挒鍙傛暟銆傛瘡涓疄渚嬪垱寤虹鎾炰綋鍚庝細璁板綍 `source.bone`銆佸疄闄呯粦瀹?Transform銆佺埗鑺傜偣鏄惁鍖归厤銆佸眬閮ㄤ綅缃?鏃嬭浆銆佸舰鐘躲€佸崐寰勩€佹湯绔崐寰勩€侀暱搴︿互鍙婂嚑浣曡鍥炲€笺€傝鏃ュ織鐢ㄤ簬鍖哄垎鈥滅鎾炰綋宸叉敞鍐屼絾濮挎€侀敊璇€濆拰鈥滆瀛愭潈閲?鐗╃悊閾惧搷搴斾笉姝ｇ‘鈥濄€?

鏈閮ㄧ讲鍚屾椂淇浜?DLL 鐩爣鏂囦欢鍚嶏細娓告垙鐩綍瀛樺湪涓€涓櫘閫?`eiem.dll` 鍜屼竴涓甫涓嶅彲瑙佸瓧绗︾殑鍚屽悕鍙樹綋锛寁84 宸查儴缃插埌鏅€氭枃浠讹紝鍙樹綋鏈垹闄ゃ€傛棫鏅€?DLL 澶囦唤鍦?`E:\EIEM_Workspace\plugin-releases\before-v84-collider-trace-20260911-170329`銆備笅涓€娆¤繍琛屼粛闇€鍒嗗埆鍙栧緱澶т笘鐣屻€乁I 鍜?NPC 鐨勯€愰」璁板綍銆?

### 20.66 2026-09-13 lifecycle probe cleanup and registration trace

The current build removes two isolated research modules: `src/eiem_native_physics_factory_probe.h` (including its periodic candidate poll) and `src/eiem_residue_probe.h`. Neither was included by production code. The NPC `_BuildBeyondCloth` observer hook was also removed because it did not own a model or feed the Render/Physics executor.

Production ownership remains on the game lifecycle: world `PrefabInstantiateProxy.OnCompleted`, character `BaseModelViewPart`/`CharUIModelMono`, and NPC `NPCAvatar.StartNPC` with `ReleaseAvatar`/`OnRelease`. These boundaries now emit deduplicated `[INSTANCE-REG-v1]` records for model registration, owner state, Partner bone binding, LOD membership, array boundaries, and release. The records contain object addresses, generation, stage, and readback values; they do not create, destroy, or poll Unity objects.

This cleanup has only been validated by the local test suite and `build.bat`. No DLL deployment or gameplay result is claimed by this section.

The same cleanup removes the unused `SubMeshInfo`/`NPCAvatarLodMeshAssets` observation hooks and the
`HGMeshRenderer`/`HGRendererStateController` flow hooks. They only forwarded to the original method and
their budgeted logs were disabled; the archived evidence recorded zero calls for the HG flow. Current LOD
evidence is emitted at the real Render matcher boundary as `[INSTANCE-REG-v1] event=eligibility`, including
`active`, `enabled`, `forceRenderingOff`, and the accept/reject reason. A `force-off` or `disabled-unowned`
record now proves that a game-suppressed LOD sibling was excluded before it could acquire a Render rule;
an `enabled-or-unread` record proves the opposite. These events are deduplicated by Renderer, draw Renderer,
stage, and Mod generation, so they are suitable for the next in-game capture without periodic polling.

The legacy per-frame MovementComponent and FinalIK detours are now absent from the production source
initialization path.
They belonged to the old MMD foot-IK experiment and could write solver targets and weights every frame;
the game's own IK remains in control. The unified trace also records `event=input` when a hotkey reaches
the generation queue and `event=reconcile` at the Unity-thread transaction begin/end. These records make
it possible to separate a stale key generation, an incomplete F10 replay, and a renderer rejected by the
game's LOD state without reintroducing a polling hook.

### 20.67 2026-09-13 v87 targeted LOD and skin binding evidence

The v87 diagnostic build does not change Render, Physics, visibility, or retirement behavior. It adds two event-boundary records needed for the unresolved cold-start symptoms:

- `event=lod` now records the discovered `LODGroup`, the source/Partner level bitmasks, `lodCount`, `enabled`, and `forceRenderingOff`. The de-duplication key includes the observed state, so a later game-owned LOD array rebuild is visible without a timer or per-frame poll.
- `event=lod-group-set` observes `LODGroup.SetLODs` after the original setter and identifies whether the call came from the game or the EIEM membership update.
- `event=partner` now records source/Partner Mesh pointers, Skeleton anchor, total and privately-created Skeleton nodes, and a process-local bone palette fingerprint alongside the existing root/parent/count checks.

The targeted contracts pass (63 tests); the complete local suite passes 119 tests with 46 environment skips; `build.bat` completes successfully. The resulting `bin/eiem.dll` and installed `D:\Hypergryph Launcher\games\Endfield Game\plugin\eiem.dll` have SHA-256 `C9FA4C1B614691534891DFC42D1BCC25C7C05B84AA6DC94C320040D5B2C2947F`. The previous DLL is backed up under `E:\EIEM_Workspace\plugin-releases\before-v87-lod-skin-trace-20260913-051902`. No proxy DLL, INI, Mesh, Material, Texture, Skeleton, or Physics resource was changed.

The next cold-start capture must contain `lod-group-set` and nonzero/zero `lod` detail for the affected source, plus the corresponding `partner` record. Those records will distinguish a custom game LOD path from a missing standard `LODGroup`, and a private-node/bone-palette mismatch from a lifecycle generation race, before any production decision is made.

### 20.68 2026-09-13 v87 cold-start capture result

The post-restart log confirms that v87 was loaded (`resource-runtime-v87-lod-skin-trace`, DLL build time `05:20:59`). The game called the standard `LODGroup.SetLODs` hook 18 times: most calls supplied four LOD levels and two supplied one level. This proves that the game does use standard `LODGroup` objects somewhere in the model pipeline.

The Typhoea source renderers still produced 44 `event=lod operation` records with `group=0`, `lodCount=0`, `sourceLevels=0`, and `partnerLevels=0`. Therefore `EiemFindSourceLodGroup` did not find a usable standard group for these source renderers. The current sibling-mesh problem cannot be fixed by assuming that the existing `LODGroup` membership update is connected; the target path is either a different LOD registry/hierarchy or is assembled outside the source renderer's parent chain. No production visibility behavior was changed in this capture.

The same run observed native Physics creation for `PhysicsSkeletonchr_0034_typhoea_postmodel_0`: one group, five colliders, and 47 native teams reached `ready` for one NPC generation. Collider records had `poseRead=1` and `parentMatch=1`, so resource parsing and the bone-parent lookup were observed. A second NPC generation reached `build-started` and was retired at `NPCCrowdEntityComponent.OnRelease` before a `ready` record. This is evidence of a real build/retire race boundary, but it does not by itself prove that the race caused a ground pose.

Partner skin records show source/Partner bone palette differences while the root bone matches. For the cloth01 sections, the source palette has 120 entries while Partners have 120, 122, or 138 entries; the first differing entry is consistently 24 in the 122/138 cases. Cloth02 sections have matching 126-entry palettes. This makes the cloth01 skin palette/bindpose path a stronger T-pose candidate than the root-bone assignment alone, but it remains an observation until correlated with the specific renderer that lies on the ground.

The next diagnostic change should enumerate the renderer membership of each game-owned `LODGroup.SetLODs` call and correlate Mesh pointers to the known Typhoea source/Partner records. It should also capture one-time bindpose/skin-array identity for the affected cloth01 renderer. No timer-based Physics polling or production LOD/lifecycle patch is justified by this capture.

### 20.69 2026-09-13 v88 LOD membership and bindpose trace

The v88 build adds only diagnostic enumeration after the original `LODGroup.SetLODs` call. For each LOD entry it records the Renderer pointer, resolved Mesh pointer (including the MeshFilter sibling for MeshRenderer), Renderer type, object name, active/enabled/`forceRenderingOff` state, and whether the object is a known EIEM source or Partner. Partner records also include source/Partner `Mesh.bindposes` counts and byte fingerprints. These records are deduplicated and capped per setter call; no LOD array, Renderer, Mesh, or Physics state is modified by the new code.

The targeted contract tests pass (49 tests), and `build.bat` completes successfully. The rebuilt `bin/eiem.dll` was deployed to `D:\Hypergryph Launcher\games\Endfield Game\plugin\eiem.dll` while no game process was using the file. Source and installed SHA-256 are `1F4471849E107BE2E60C91982DFB8ADE0BAB354EB49DA6222B27BC29CA035F76`; the previous DLL is backed up under `E:\EIEM_Workspace\plugin-releases\before-v88-lod-membership-bindpose-trace-20260913-053829`. The proxy DLLs and all Mod resources were left unchanged.

The next restart is expected to produce `event=lod-group-member` records. Those records are required before changing the LOD matcher, because the previous capture only proved that the Typhoea source's parent-chain lookup returned no group and did not show the actual Renderer arrays owned by the game.

### 20.70 2026-09-13 v88 restart capture: LOD断点已确认

The v88 restart produced 490 `event=lod-group-member` records. The Typhoea cloth01 and cloth02 source Renderers were found in actual game-owned LODGroups. For example, `S_actor_typhoea_cloth_01_lod0` (`renderer=0000000FD0A042C0`) and `S_actor_typhoea_cloth_02_lod0` (`renderer=0000000FD0A042A0`) were both members of `LODGroup=0000000FD0A6EB80`, `lod=0`, while both source Renderers had `enabled=0`. No known Partner appeared in any game LOD array (`source=1` occurred six times; `partner=1` occurred zero times).

The corresponding `event=lod operation` records for those same source pointers still reported `group=0`, `changed=0`, and zero level masks. This proves the parent-chain `EiemFindSourceLodGroup` lookup is the broken link: the game has the group and the source is a member, but EIEM does not discover that group and therefore never inserts the Partner. When the source is disabled, the Partner remains outside the game's LOD selection boundary and can continue rendering. The user's LOD symptom is therefore explained without changing Mesh matching or adding a second renderer rule.

The same capture did not show a ground-pose failure, but its bindpose records are conclusive about the next diagnostic target. Every cloth01 Part0/1/6/7 Partner used 120 source entries versus 122 Partner entries; Part4 used 120 versus 138. The bindpose counts and fingerprints differ in the same records. Cloth02 remained 126/126 with matching fingerprints. These are recorded as a separate skinning candidate and are not conflated with the confirmed LOD cause.

### 20.71 2026-09-13 v89 LOD membership reconcile and pose trace

The v89 build uses the Renderer membership delivered by the game-owned `SetLODs` call when Partner creation happened earlier than LODGroup assembly. After the original setter returns, existing Partners whose source Renderer is present in that concrete array are inserted into the same LOD levels; the recursive EIEM setter call is excluded from reconciliation. This is a lifecycle-boundary update, not a timer or per-frame poll. It also records one-time source/Partner world and local positions and root-bone positions at partner creation, skin assignment, root-bone assignment, and release-related boundaries.

Targeted contracts pass (49 tests), `build.bat` completes successfully, and the DLL was deployed while no game process held the file. The installed `eiem.dll` SHA-256 is `21D1C7D9521BFA00DC7807C74D6B98272E1C214F7AEB7CEBEBDFF46BBFD7C61C`. The previous DLL is backed up under `E:\EIEM_Workspace\plugin-releases\before-v89-lod-membership-reconcile-pose-trace-20260913-054807`.

The next restart should show the original game LOD member record followed by an EIEM-origin `lod-group-set` and `partner=1` membership for the same group. If a mesh still lies down, the new `event=partner-pose` records will show whether its Transform/root positions diverge before any skinning or lifecycle change is attempted.

### 20.72 2026-09-13 v90 redeploy: LOD boundary reconciliation and teardown pose evidence

The v90 DLL is the rebuilt v89 implementation plus the corrected pose-trace deduplication key. The key now includes the boundary name, so `partner-created`, `assign-skin`, `set-root-bone`, and `retire` observations cannot suppress one another merely because they refer to the same Renderer pointers. The `retire` observation is emitted immediately before the Partner is made inactive and detached, which records the last live Transform/root positions without treating disposal as a completion fence.

The build completed with all 119 repository tests passing except 46 environment-gated skips. The installed file is `D:\Hypergryph Launcher\games\Endfield Game\plugin\eiem.dll`, size 6,352,896 bytes, SHA-256 `4EEA96618411D87EA00AD28CA60FD3DB434B702FAFAB63E197DBB4A6369215A7`. The previous installed DLL is backed up at `E:\EIEM_Workspace\plugin-releases\before-v90-lod-membership-reconcile-pose-trace-20260913-055302\eiem.dll`. No game process held the file during deployment.

This deployment is not a claim that in-game LOD or pose behavior is fixed. One fresh launch is required. The acceptance evidence is: the source Renderer and its Partner appear in the same concrete `LODGroup.SetLODs` member records; the Partner is present at the source's LOD index; and, if a model lies down, the first divergent `partner-pose` boundary identifies whether the error exists at creation/skin/root assignment or only at retirement.

\n

### v91 刷新边界取证（2026-09-13）

v90 的运行日志确认 Partner 已被放回游戏实际使用的 `LODGroup.SetLODs` 数组；本轮报告的“躺地”仍不能归因于 LOD。冷启动记录中，Partner 与源 Renderer 的世界位置、局部位置和 `rootBone` 位置在创建、皮肤绑定、根骨骼设置三个边界均一致，因此简单的父 Transform 偏移不是已证实原因。

v91 只增加一次性诊断，不改变运行策略：F10 刷新某个已存在的 Partner 时记录 `partner-refresh-before/after`，记录刷新前后 generation、Mesh、骨骼数组首个差异及该槽位骨骼名称和位置；刷新事务成功则记录 `result=success`。没有新增周期轮询，也没有改变按键切换的显隐逻辑。DLL 已构建并部署到 `D:\Hypergryph Launcher\games\Endfield Game\plugin\eiem.dll`，SHA-256 为 `CDA867775A613392B6A2E796E90D5D48267E3687BDD22449E9FF35666...`（以部署文件实际哈希为准），旧文件备份在 `E:\EIEM_Workspace\plugin-releases\before-v91-refresh-boundary-bone-diff-20260913-061258`。

下一次取证只需要一条流程：冷启动确认当前状态，覆盖一次 Mesh/INI 后按 F10，观察是否躺地，然后退出。若日志出现 `partner-refresh-before` 但没有 `after`，问题在刷新事务或资源构建；若前后首个差异骨骼的位置/名称发生变化，问题在骨骼数组或新节点绑定；若前后绑定一致仍躺地，才继续检查 Animator/Physics 写回边界。当前没有把 v91 说成已修复，也没有把 `DisposeInternal`、计数器或日志返回值当作完成栅栏。

### v102 Partner 提交边界取证（2026-09-13）

v102 只增加 `[PARTNER-COMMIT-v102]` 一次性记录，并保留上一轮“按键只改显隐、F10 复用 Partner”的运行策略。记录发生在 Partner 创建或 F10 刷新之后，包含源/Partner 的骨骼数量、首个差异、Mesh、`rootBone`、启用状态，以及 `gameArray`：该字段只有在具体的游戏蒙皮 Renderer 数组中观察到 Partner 后才为 1。它不把公开 `SkinnedMeshRenderer.bones` 的读回值当成 Animator 接受，也不从该记录触发修复。

本地相关合同测试 28 项通过，`build.bat` 完整构建通过。v102 已部署到 `D:\Hypergryph Launcher\games\Endfield Game\plugin\eiem.dll`，SHA-256 为 `FA83372274F93C10AFEAE41863F27B43E940781CCE60680ADE0FC476E7D704A1`；旧文件备份在 `E:\EIEM_Workspace\plugin-releases\before-v102-partner-commit-probe-20260913-174849`。本次没有改 Mesh、Material、Texture、Skeleton、Physics 或代理 DLL。

当前仍不能说“躺地原因已经唯一定位”。已有日志排除了磁盘 Mesh 损坏、Partner `set_bones` 被游戏改写、以及创建/刷新边界的源与 Partner 根骨骼或 Transform 明显偏移；最强未决分支是直接 `RendererInfo._Init` 创建的 Partner 没有进入游戏内部蒙皮装配，而 PostModel 路径会进入。下一次出现躺地时，需用 `gameArray=0/1` 与同一 Partner 的刷新记录关联：`gameArray=0` 将把问题收敛到直接路径的游戏装配缺口，`gameArray=1` 则继续检查该数组之后的 Animator/Physics 写回。该结论仍以实际运行日志为准。

### v105 2026-09-13 私有骨骼槽位取证（部署，待下一次运行）

针对冷启动时不同 Partner 网格轮流出现躺地/T-pose 的报告，v105 只增加 `partner-skin-private-detail` 只读记录，不改变 Mesh、Skeleton、Partner、LOD、按键或生命周期行为。记录会把第一个 `EIEM_Bone_*` 槽位同时写出 Mesh skin index、Skeleton node index、Skeleton 节点的 `source` 标志，以及源/Partner 骨骼名称。

已有 v104 日志已经证明：`cloth_01` 的多个 Partner 都进入同一实例的 `AssignSkinGo` 数组，源 Renderer 与 Partner 数组命中完整；因此“neiyi 这次躺地是因为 Partner 没注册”不能成立。相同日志还显示该类 Mesh 第 24 个源槽位是 `skirt_base_L_c_03_jnt`，而导出的 Mesh/Skeleton 路径是 `...skirt_base_L_c_01_jnt/skirt_base_L_c_02_jnt/skirt_base_R_c_03_jnt`，Partner 对应槽位因此是私有 `EIEM_Bone_*`。这是一个确定的路径/身份不一致缺陷，但它本身应稳定影响使用该槽位的网格，不能单独解释“每次冷启动躺地网格不同”。变化部分仍需用 v105 把实例、可见 Partner 和具体槽位关联起来，重点检查创建时序、LOD/按键状态以及刷新后的绑定是否不同。

v105 已由 `build.bat` 完整构建并部署到 `D:\Hypergryph Launcher\games\Endfield Game\plugin\eiem.dll`，源/目标 SHA-256 为 `98D3F79EAA912168796F7D4D98E94CE9C6BF82406DEF0F5A7C87D7135AC3F31F`。本地相关 29 项合同测试通过（4 项环境跳过）。下一次启动后只需观察一次 `neiyi` 或其他躺地网格；在没有新的运行证据前，不把 v105 说成修复。

### v106 2026-09-13 Renderer 到 Model/PFB 关联取证（部署，待下一次运行）

v106 只增加 `[INSTANCE-REG-v3] event=renderer-owner` 记录，不改变 Render、Partner、Skeleton、LOD、按键切换或生命周期行为。每个已记录的源 Renderer/Partner 会把它的 `ownerPrefabInstance` 与统一 Model registry 对照，并记录 owner 类型、owner 指针、PFB/模型路径、instanceUid、active 状态和当前 generation。若 Partner 在 `RendererInfo._Init` 阶段先于 Model owner 发布，日志会明确写出 `<unregistered>` 或 `<model-without-owner>`，不把这类记录猜成大世界、NPC 或 UI。

这条关联用于区分三个分支：不同 PFB/owner 产生不同 live hierarchy；同一 Model 在 F10/按键路径被重复注册；或同一 owner 下骨架装配阶段发生变化。当前没有用它修复映射，也没有增加轮询或销毁操作。构建后的 `bin/eiem.dll` 与已部署文件 SHA-256 均为 `976A9C0B1034DC6A34B268C3F2F71E0A903188BA27A4B9463F564DAA2CF88B5B`，旧 DLL 备份在 `E:\EIEM_Workspace\plugin-releases\before-v106-render-owner-correlation-20260913-194420\eiem.dll`。相关合同测试 35 项通过（4 项环境跳过），`build.bat` 完整构建通过。

下一轮只需启动一次大世界当前角色，观察一个躺地网格；若条件允许，再进入一次角色 UI 或 NPC。退出后读取 `eiem_log.txt` 中同一 `renderer` 的 `renderer-owner`、`partner-skin-map`、`partner-pose` 和 `lod-group-member` 记录。只有拿到这组运行证据后，才能决定是否需要改绑定或生命周期。

### v106 运行结果：NUMPAD1 显示 Part8 后出现躺地（2026-09-13）

本轮 v106 日志确认大世界角色的 Model 是同一个 `chr_0034_typhoea_postmodel.prefab` 实例（`model=000000024CA9A860`），并不是把该 Renderer 误关联到 NPC 或角色 UI。`RenderS_actor_typhoea_cloth_01_lod0_2Part8` 在冷启动阶段已经存在：其 Partner 骨骼为 `120/120`、`firstDiff=-1`，`rootBone` 和创建时的源/Partner Transform 位置一致；按键阶段没有再次出现 `partner-created`。

按下 `NUMPAD1` 后，日志只把同一个 Partner `0000000FCFF51F00` 从隐藏状态改为 `partnerEnabled=1`（`partner-visibility`）。同一笔 `control state change` 事务还首次把该世界 Model 的 Physics 计划从 `previous=0` 变成 `current=1`，随后创建 1 个组、5 个原生碰撞体并进入 `ready`。当前世界 Part8 的 `PARTNER-COMMIT` 记录仍是 `gameArray=0`；本轮 `AssignSkinGo` 数组索引 25 的记录使用的是另一组 source/Partner 指针，不能拿来证明世界 Part8 已被该实例的游戏蒙皮装配接受。因此，世界 Part8 在创建后是否进入当前实例的游戏蒙皮数组仍未证实，这与“按键没有重新创建 Partner”是两件事。

因此本轮最强结论是：躺地发生在“已有 Partner 被显示”与“该 Model 首次启动 Physics/Animator 写回”同时发生的边界，不能归因于按键重新创建 Partner、错误 PFB 或冷启动 Transform 偏移。日志仍没有给出 Physics/Animator 启动后 Part8 骨骼的逐帧姿态，因此还不能把责任唯一归到 Physics 写回；剩余两个可验证分支是 Physics 首次启动改变了共享骨骼，或 Partner 虽在 `SkinnedMeshRenderer[]` 中但其运行时姿态没有被正确接受。当前不修改生产逻辑，下一步应只增加该按键边界的骨骼姿态/Animator 写回取证，或先用一次不启动 Physics 的对照运行隔离这两个分支。

### v107 按键显隐与 Physics 重放解耦（2026-09-13）

基于 v106 的运行证据，`partnerLinksOnly` 事务现在只执行 Partner 显隐和必要的结构检查，跳过全量 Model 资源重放以及 `EiemStoreModelPhysicsIntents`。这修正了按键路径把同一 Model 的 Physics 从未创建状态推进到首次 `BuildAndRun` 的问题；F10 和真实模型生命周期边界仍保留完整资源/Physics 重放。该修改不销毁或重建 Partner，也不改变 LOD 数组或骨骼映射。

v107 已由 `build.bat` 完整构建并部署到 `D:\Hypergryph Launcher\games\Endfield Game\plugin\eiem.dll`。源文件与部署文件 SHA-256 均为 `BBAE12728D43F7FC6C1815588591A8624098207799DD423267FBDB0B0BD969CA`；旧 DLL 备份在 `E:\EIEM_Workspace\plugin-releases\before-v107-key-visibility-no-physics-replay-20260913-200848`。相关合同测试 35 项通过（4 项环境跳过）。

验收重点是：按 `NUMPAD1` 只出现 `partner-visibility` 和“skipped model resource/Physics replay”，不再出现同一按键事务的 `PHYSICS-PLAN previous=0 current=1`；若 Part8 仍躺地，原因就落在已存在 Partner 的 Animator/蒙皮姿态链路，而不是按键触发的 Physics 首次创建。部署本身不代表游戏内问题已经修复，仍需一次运行日志确认。

### v107 冷启动结果与 v108 BaseModel 蒙皮缓存取证（2026-09-13）

v107 的下一次运行没有 `event=input`、F10 或大世界 Physics 创建，但用户观察到 `MeshS_actor_typhoea_cloth_01_lod0_2.neiku` 躺地。Blender 当前文件与导出资源的顶点数对应确认该对象是 `MeshS_actor_typhoea_cloth_01_lod0_2_3`，即 `RenderS_actor_typhoea_cloth_01_lod0_2Part2`。大世界 Part2 在创建和后续根骨骼设置边界均为 `bones=120/120`、`firstDiff=-1`，源与 Partner 的 `rootBone`、bindpose 指纹及 Transform 位置一致。因而这次冷启动现象不能由按键事务启动 Physics、磁盘 Mesh 选错或公开骨骼数组/根骨骼不一致解释。

元数据显示 `Beyond.Gameplay.View.BaseModelViewPart` 自己保存 `m_renderers`、`m_renderersInitState`、`m_meshes`、`m_meshesInitState` 和 `m_lodGroups`。现有 NPC 路径会在游戏执行 `AssignSkinGo` 前把 Partner 加入它的 Renderer 数组；大世界路径只登记 `m_model`，尚未证明 Partner 是否进入 BaseModel 的这些缓存。v108 因此只在原始 `BaseModelViewPart.OnLoadFinish` 返回后、EIEM Model 登记前后输出 `[BASEMODEL-SKIN-v108]`：它记录已知源/Partner 在两个缓存中的索引及相应初始启用值，不创建、不扩展也不改写任何游戏数组。

v108 已完整构建并部署到 `D:\Hypergryph Launcher\games\Endfield Game\plugin\eiem.dll`，源文件与部署文件 SHA-256 均为 `65BC0E694C62945EFC98995F9CAC7B4D8333133EBEDF35818D996571A6D73828`；旧 DLL 备份在 `E:\EIEM_Workspace\plugin-releases\before-v108-basemodel-skin-array-evidence-20260913-203648`。本轮运行的 58 项本地测试全部通过，其中 4 项宿主环境测试跳过。v108 是取证版，不代表躺地问题已经修复；下一次冷启动后观察任意一个躺地网格并退出，即可用同一 Part 的 `meshIndex` 和 `meshInit` 判断是否漏过大世界的蒙皮初始化缓存。

### v109 PostDealLoadedModel 晚期边界取证（2026-09-13）

用户本轮确认是冷启动且没有按键，因此不能把现象归因于按键切换事务。v108 已显示 `neiyi` 的 Partner 位于大世界 `BaseModelViewPart` 的 `m_renderers`/`m_meshes` 缓存中，Partner 对应的初始化标志为 1，且源/Partner 的公开骨骼数组与根骨骼一致；这排除了“未登记”或简单父变换偏移，但没有覆盖 `OnLoadFinish` 之后的最后装配边界。

v109 只新增 `BaseModelViewPart.PostDealLoadedModel` 与 `ComplexModelViewPart.PostDealLoadedModel` 的原始方法返回后观测。它再次读取同一组 `m_renderers`、`m_renderersInitState`、`m_meshes`、`m_meshesInitState` 和 `m_lodGroups`，不写数组、不改可见性、不创建/销毁 Partner，也不启动 Physics。若同一 Partner 在 `OnLoadFinish-after-register` 与 `PostDealLoadedModel-after` 间索引或初始化值改变，问题落在游戏晚期缓存重写；若保持不变，则继续检查导出蒙皮顶点坐标/绑定基准或 Animator 写回，而不再修改生命周期猜测。

v109 已完整构建并部署到 `D:\Hypergryph Launcher\games\Endfield Game\plugin\eiem.dll`，源文件与部署文件 SHA-256 均为 `80B85498AF1F929E4CF16D502554670A621A30132554E5A7B2FE5719A72FB6B9`；旧文件备份在 `E:\EIEM_Workspace\plugin-releases\before-v109-postdeal-20260913-210134`。相关资源管线 44 项、Partner 12 项（2 项跳过）及皮肤/重载合同检查均通过。该版本仍是取证版，不代表已修复；下一次只需冷启动进入大世界，观察任意躺地网格后退出，日志应包含对应 Part 的 `PostDealLoadedModel-after` 记录。

### v110 final AssignSkin membership probe (2026-09-13)

v110 adds only a bounded startup marker [DIAG-LOOP-v110] and read-only source/Partner membership records at the final AssignSkin observation boundary. Each event=array-pair includes the Partner owner model pointer, section, sourceHit/partnerHit, array indices, visibility and generation; the model registry correlation records owner kind and path. No game array is extended or modified, and no Render, LOD, key, Physics, Animator, or lifecycle behavior is changed.

The full local unittest suite passed: 125 tests, 46 skipped by environment. build.bat completed successfully. v110 is deployed to D:\Hypergryph Launcher\games\Endfield Game\plugin\eiem.dll; the deployed SHA-256 is AF11122DCBA64D67EE7CFD405C82A24BA0CD33AA3812A0A945B8B571902D686A. The previous DLL is backed up at E:\EIEM_Workspace\plugin-releases\before-v110-negative-fix-20260913-233342. Use one fresh cold start, observe any grounded mesh, then exit; analyze only records carrying that startup marker. Do not treat v110 as a fix until that run is inspected.

### v110 多实例结果与 v111 装配边界 A/B（2026-09-14）

v110 冷启动中用户观察到 `shoes` 与 `neiku` 躺地。同一场景同时存在大世界角色和 NPC，但日志没有发现跨 PFB 复用可变蒙皮实例：大世界源 Renderer 为 `0000001009FCEF00`，NPC 源 Renderer 为 `0000000FD8162A00`；双方的 Partner Renderer 与骨骼 Transform 数组均不同，只共享生成后的 Mesh 资源。NPC 的最终蒙皮数组记录为 `sourceHits=2/2`、`partnerHits=12/12`。因此，“两个 PFB 直接共用了同一个 Partner 或同一组骨骼对象”不符合本轮证据。

日志同时确认，Partner 仍在单个 `RendererInfo._Init` 回调内创建。大世界 Partner 创建时 `ownerModel=0`，之后才在 BaseModel 生命周期登记时补充归属；NPC 也依赖 `_Init` 先创建，再由 `CreateSMSInfoForPostModel` 输出数组追加。由于 `_Init` 本身位于游戏逐个 Renderer 初始化期间，在其中递归增加 GameObject/Renderer 会让层级增长与游戏扫描交错；多实例会改变回调顺序，可能放大这种装配时序缺陷，但这仍是待 A/B 验证的假设。

v111 只调整 Partner 创建边界，不改 Mesh/Skeleton/Physics 文件格式、按键状态或资源匹配：`RendererInfo._Init` 和公开 Mesh setter 仍可对已有 Renderer 执行原位资源动作，但不再创建 Partner；大世界在 `BaseModelViewPart.PostDealLoadedModel` 原函数扫描层级前按具体 Model 创建 Partner；NPC 在 `CreateSMSGO`/`CreateSMSInfoForPostModel` 返回该实例的 Renderer 数组后创建 Partner，并在调用者继续 `AssignSkin` 前将其追加到同一 Renderer/RootBoneInfo 数组。Partner 的键仍包含具体源 Renderer，所以同 Mesh、不同 PFB 各自拥有独立实例。

完整本地测试为 127 项通过，其中 46 项因宿主环境跳过；`build.bat` 完整链接成功。v111 已部署到 `D:\Hypergryph Launcher\games\Endfield Game\plugin\eiem.dll`，源/目标 SHA-256 均为 `B5AA537C2CD2F3D631F906211214F135D5DFDA8FACAC259E7CCFB0418D9B3426`，旧 DLL 备份在 `E:\EIEM_Workspace\plugin-releases\before-v111-partner-assembly-20260914-015515\eiem.dll`。这只是装配时序 A/B 版；需要冷启动分别观察大世界和同屏 NPC，并在按键显示隐藏部件后再观察。若仍随机躺地，则排除 `_Init` 递归创建分支，下一步比较游戏原生 SkinnedMeshRenderer 工厂与当前 `GameObject + AddComponent` 创建路径，不能把本地测试当作游戏内修复证明。

### v113 EntityRenderHelper 公共装配边界（2026-09-14，已部署待实机验证）

v112 运行后，用户确认大世界角色与 NPC 都存在随机 Partner 网格躺地/T-pose。日志中 Partner 的公开 `bones`、`rootBone` 和 Transform 读回正常，但世界 Partner 只进入 `BaseModelViewPart` 缓存，NPC Partner 只进入 `CreateSMSInfoForPostModel`/`AssignSkin` 的公开数组；两条路径都没有证据表明新增 Renderer 进入游戏的 `EntityRenderHelper` 内部表。相反，`RendererInfo._Init` 只为原生 source Renderer 出现，Partner 创建后没有游戏发起的 `set_bones`。因此当前最强共同原因是 Partner 创建晚于游戏内部 Renderer/Animator 装配，而不是 NPC PFB、世界 PFB、单个 Mesh 坐标或按键值。

v113 在游戏共同的 `EntityRenderHelper._InitRenderAndMaterial` 原函数执行前应用 Mesh 身份规则并创建所有 Partner。原函数随后按自己的层级扫描建立 `m_allChildrenRenderers`、`RendererInfo`、材质及可见性控制器，使 Partner 与 source 通过同一个游戏装配入口。该入口只在模型初始化时运行，不增加帧轮询；按键仍只修改 `Renderer.enabled`，F10 仍复用现有 Partner。原有 NPC 数组与 BaseModel 缓存逻辑暂时保留，待本轮实机日志确认公共入口命中后再删除重复装配路径。

本地直接运行的相关测试共 74 项通过（其中 2 项宿主环境跳过），`build.bat` 完整链接成功。v113 已部署到 `D:\Hypergryph Launcher\games\Endfield Game\plugin\eiem.dll`，源/目标 SHA-256 均为 `034F9210696ECFF770C5737357A8D34D988A0E904B005B430F2AEEB5F5D0E348`；旧 DLL 备份在 `E:\vscode\EIEM\backups\deploy_20260914_133600\eiem.dll.previous`。启动日志应包含 `[DIAG-LOOP-v113]` 和 `[MOD-ASSEMBLY-v113]`。部署与离线测试不代表躺地问题已经修复；需要一次冷启动同时观察大世界与 NPC，并读取这两个标记以及 Partner 创建顺序。
