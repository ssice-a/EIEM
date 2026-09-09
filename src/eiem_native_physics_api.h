#pragma once
#include <string>
#include <array>

// Exact metadata access shared by native-physics diagnostics and parameter staging.
// No game-version field offsets, character names or native RVAs.
static constexpr const char *EiemPhysicsAnimatorImage="UnityEngine.AnimationModule.dll";

static std::string EiemPhysicsTypeName(void *type) {
  if (!type || !il2cpp_type_get_name || !il2cpp_free) return {};
  const char *name = il2cpp_type_get_name(type);
  std::string result = name ? name : "";
  if (name) il2cpp_free((void *)name);
  return result;
}

static void *EiemPhysicsClass(void **assemblies, size_t count,
                               const char *imageName, const char *ns,
                               const char *name) {
  void *found = nullptr;
  for (size_t i = 0; i < count; ++i) {
    void *image = il2cpp_assembly_get_image(assemblies[i]);
    const char *imageLabel = image ? il2cpp_image_get_name(image) : nullptr;
    if (!imageLabel || strcmp(imageLabel, imageName)) continue;
    void *klass = il2cpp_class_from_name(image, ns, name);
    if (klass) {
      if (found) return nullptr; // ambiguous assembly, not first-match wins
      found = klass;
    }
  }
  return found;
}

template<class... Parameters>
static void *EiemPhysicsMethod(void *klass, const char *name, const char *ret,
                                bool isStatic, Parameters... parameters) {
  if (!klass) return nullptr;
  const std::array<const char *, sizeof...(parameters)> params{parameters...};
  void *iterator = nullptr, *method = nullptr, *found = nullptr;
  while ((method = il2cpp_class_get_methods(klass, &iterator))) {
    const char *label = il2cpp_method_get_name(method);
    if (!label || strcmp(label, name) ||
        il2cpp_method_get_param_count(method) != params.size()) continue;
    constexpr uint32_t MethodStatic = 0x10;
    uint32_t impl = 0;
    if (bool(il2cpp_method_get_flags(method, &impl) & MethodStatic) != isStatic ||
        EiemPhysicsTypeName(il2cpp_method_get_return_type(method)) != ret) continue;
    bool matches = true;
    for (uint32_t i = 0; i < params.size(); ++i)
      if (!params[i] || EiemPhysicsTypeName(il2cpp_method_get_param(method, i)) != params[i]) {
        matches = false; break;
      }
    if (!matches) continue;
    if (found) return nullptr;
    found = method;
  }
  return found;
}

static void *EiemPhysicsField(void *klass, const char *name, const char *type) {
  if (!klass) return nullptr;
  void *iterator = nullptr, *field = nullptr, *found = nullptr;
  while ((field = il2cpp_class_get_fields(klass, &iterator))) {
    const char *label = il2cpp_field_get_name(field);
    constexpr int FieldStatic = 0x10;
    if (!label || strcmp(label, name) ||
        (il2cpp_field_get_flags(field) & FieldStatic) ||
        EiemPhysicsTypeName(il2cpp_field_get_type(field)) != type) continue;
    if (found) return nullptr;
    found = field;
  }
  return found;
}

static void *EiemPhysicsField(void *klass, const char *name) {
  if (!klass) return nullptr;
  void *iterator = nullptr, *field = nullptr, *found = nullptr;
  while ((field = il2cpp_class_get_fields(klass, &iterator))) {
    const char *label = il2cpp_field_get_name(field);
    constexpr int FieldStatic = 0x10;
    if (!label || strcmp(label, name) || (il2cpp_field_get_flags(field) & FieldStatic)) continue;
    if (found) return nullptr;
    found = field;
  }
  return found;
}

template<class T>
static bool EiemPhysicsReadField(void *object, void *field, T &value) {
  if (!EiemOnUnityThread() || !object || !field || !il2cpp_field_get_value) return false;
  __try {
    il2cpp_field_get_value(object, field, &value);
    return true;
  } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}

template<class T>
static bool EiemPhysicsUnbox(void *boxed,T &value) {
  if (!EiemOnUnityThread() || !boxed || !il2cpp_object_unbox) return false;
  __try {
    void *data = il2cpp_object_unbox(boxed);
    if (!data) return false;
    memcpy(&value, data, sizeof(T));
    return true;
  } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}

template<class T>
static bool EiemPhysicsReadValue(void *object, void *method, T &value) {
  if (!EiemOnUnityThread() || !object || !method) return false;
  void *boxed = nullptr;
  return InvokeChecked(method, object, nullptr, &boxed) && EiemPhysicsUnbox(boxed,value);
}
