#pragma once
// Native assembly boundary probes for EntityRenderHelper.
//
// These probes are OBSERVATION ONLY. They never mutate a managed array, never
// create or destroy an object, and never change the game's call order. They
// exist to answer one concrete question that the existing traces cannot:
//
//   Does the game's own EntityRenderHelper actually receive our Partner
//   Renderers when it builds `m_allChildrenRenderers` and its material
//   controller's RendererInfo list?
//
// The runtime evidence before this probe was that 36 Partner Renderers were
// created in one session and NONE of them reached RendererInfo._Init, while all
// 6 source Renderers did. `_ValidRenderers(List<Renderer>, bool)` is the native
// filter that feeds `m_allChildrenRenderers`, so it is the exact boundary where
// a Partner is either admitted or silently dropped.
//
// Tag: [ASSEMBLY-PROBE-v114]

static constexpr const char *EiemAssemblyProbeTag = "[ASSEMBLY-PROBE-v114]";

typedef void (__fastcall *EiemValidRenderersFn)(void *self, void *renderers,
                                                bool flag, void *methodInfo);
static void *s_eiemOrigValidRenderers = nullptr;

typedef void (__fastcall *EiemInitAllFn)(void *self, void *methodInfo);
static void *s_eiemOrigInitAll = nullptr;

// Bounded, de-duplicated label set so a scene-wide sweep cannot turn this into
// a per-frame logger. Keyed by the boundary name only.
static bool EiemAssemblyProbeFirst(const char *key) {
  static SRWLOCK lock = SRWLOCK_INIT;
  static char seen[64][48] = {};
  static size_t count = 0;
  if (!key) return false;
  AcquireSRWLockExclusive(&lock);
  for (size_t i = 0; i < count; ++i) {
    if (_stricmp(seen[i], key) == 0) {
      ReleaseSRWLockExclusive(&lock);
      return false;
    }
  }
  bool fresh = false;
  if (count < _countof(seen)) {
    strncpy_s(seen[count], sizeof(seen[count]), key, _TRUNCATE);
    ++count;
    fresh = true;
  }
  ReleaseSRWLockExclusive(&lock);
  return fresh;
}

// Describe one Renderer slot as the native filter sees it.
static void EiemAssemblyProbeDescribeList(const char *boundary, void *self,
                                          void *renderers, bool flag,
                                          const char *when) {
  const size_t count = EiemManagedArrayLength(renderers);
  if (!renderers || count > 4096) {
    Log("%s event=list-unreadable boundary=%s self=%p array=%p when=%s flag=%d",
        EiemAssemblyProbeTag, boundary ? boundary : "?", self, renderers,
        when ? when : "?", flag ? 1 : 0);
    return;
  }

  size_t sourceHits = 0, partnerHits = 0, visiblePartners = 0;
  void *firstPartner = nullptr;
  const char *firstPartnerSection = "<none>";
  bool firstPartnerVisible = false;

  void **items = (void **)((char *)renderers + IL2CPP_ARRAY_DATA);
  __try {
    for (size_t index = 0; index < count; ++index) {
      void *renderer = items[index];
      if (!renderer) continue;
      if (EiemIsKnownSourceRenderer(renderer)) ++sourceHits;
      if (!EiemIsPartnerRenderer(renderer)) continue;
      ++partnerHits;
      bool enabled = false;
      if (EiemReadRendererEnabled(renderer, &enabled) && enabled)
        ++visiblePartners;
      if (firstPartner) continue;
      firstPartner = renderer;
      EiemReadRendererEnabled(renderer, &firstPartnerVisible);
      AcquireSRWLockShared(&s_eiemPartnerLock);
      for (const auto &state : s_eiemPartners) {
        if (state.partnerRenderer == renderer) {
          firstPartnerSection = state.section;
          break;
        }
      }
      ReleaseSRWLockShared(&s_eiemPartnerLock);
    }
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    Log("%s event=list-failed boundary=%s self=%p array=%p when=%s "
        "exception=0x%08lX",
        EiemAssemblyProbeTag, boundary ? boundary : "?", self, renderers,
        when ? when : "?", GetExceptionCode());
    return;
  }

  Log("%s event=valid-renderers boundary=%s self=%p when=%s flag=%d "
      "count=%zu sourceHits=%zu partnerHits=%zu visiblePartners=%zu "
      "firstPartner=%p firstPartnerSection=%s firstPartnerEnabled=%d",
      EiemAssemblyProbeTag, boundary ? boundary : "?", self,
      when ? when : "?", flag ? 1 : 0, count, sourceHits, partnerHits,
      visiblePartners, firstPartner, firstPartnerSection,
      firstPartnerVisible ? 1 : 0);
}

static void TraceValidRenderers(void *self, void *renderers, bool flag,
                                void *methodInfo) {
  auto original = (EiemValidRenderersFn)s_eiemOrigValidRenderers;
  // The input list is built by the caller. Record it before the native filter
  // can drop anything, so an absent Partner is attributable to the caller while
  // a present-but-dropped Partner is attributable to this filter.
  if (EiemAssemblyProbeFirst("valid-renderers-before"))
    EiemAssemblyProbeDescribeList("EntityRenderHelper._ValidRenderers", self,
                                  renderers, flag, "before");
  if (original) original(self, renderers, flag, methodInfo);
}

static void TraceInitAll(void *self, void *methodInfo) {
  auto original = (EiemInitAllFn)s_eiemOrigInitAll;
  if (self) {
    // m_allChildrenRenderers lives at 0x138 on EntityRenderHelper. Reading it
    // before the original runs shows whether a previous init already froze the
    // renderer list, which is the difference between "Partner arrived too late"
    // and "Partner was never discovered".
    void *before = nullptr;
    __try {
      before = *(void **)((char *)self + 0x138);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
      before = nullptr;
    }
    const size_t beforeCount = EiemManagedArrayLength(before);
    if (beforeCount)
      Log("%s event=init-all-enter self=%p existingAllChildrenRenderers=%p "
          "count=%zu",
          EiemAssemblyProbeTag, self, before, beforeCount);
  }
  if (original) original(self, methodInfo);
}

// Installed from InitIl2CppResourceTrace right after the EntityRenderHelper is
// resolved and before the _InitRenderAndMaterial boundary is hooked.
static void EiemInstallAssemblyProbes(void *entityRenderHelperClass) {
  if (!entityRenderHelperClass) return;
  {
    static const char *const validTypes[] = {
        "System.Collections.Generic.List<UnityEngine.Renderer>",
        "System.Boolean"};
    void *target = FindMethodWithParamTypes(
        entityRenderHelperClass, "_ValidRenderers", validTypes,
        _countof(validTypes));
    if (target &&
        Hook(target, "EntityRenderHelper._ValidRenderers",
             (void *)TraceValidRenderers, &s_eiemOrigValidRenderers))
      Log("%s installed boundary=EntityRenderHelper._ValidRenderers",
          EiemAssemblyProbeTag);
    else
      Log("%s unavailable boundary=EntityRenderHelper._ValidRenderers",
          EiemAssemblyProbeTag);
  }
  {
    void *target = FindMethodWithReturnType(entityRenderHelperClass, "InitAll",
                                            "System.Void", 0);
    if (target &&
        Hook(target, "EntityRenderHelper.InitAll", (void *)TraceInitAll,
             &s_eiemOrigInitAll))
      Log("%s installed boundary=EntityRenderHelper.InitAll",
          EiemAssemblyProbeTag);
    else
      Log("%s unavailable boundary=EntityRenderHelper.InitAll",
          EiemAssemblyProbeTag);
  }
}
