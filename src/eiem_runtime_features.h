#pragma once

// The authoring formats remain readable while their native runtime adapters
// are validated independently.  Keep these switches compile-time and
// explicit so a production build cannot accidentally start an experimental
// Physics owner or install high-volume observation hooks.
static constexpr bool kEiemEnableExperimentalPhysicsRuntime = false;
static constexpr bool kEiemEnableNativePhysicsObservation = false;
