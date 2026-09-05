#pragma once
#include "eiem_lua_ui.h"
#include <wrl/client.h>

// Dedicated native overlay, not a game swap-chain hook and not the INSERT
// panel. All methods execute on GuiThread; only variable events cross threads.
class EiemUiHost {
  template<class T> using Com = Microsoft::WRL::ComPtr<T>;
  HWND hwnd = nullptr;
  ImGuiContext *context = nullptr;
  Com<ID3D11Device> device;
  Com<ID3D11DeviceContext> immediate;
  Com<IDXGISwapChain1> swap;
  Com<ID3D11RenderTargetView> target;
  Com<IDCompositionDevice> composition;
  Com<IDCompositionTarget> compositionTarget;
  Com<IDCompositionVisual> visual;
  LONG generation = -1;
  bool failed = false, shown = false;
  int width = 0, height = 0;
  struct Entry { EiemUiSnapshot snapshot; std::unique_ptr<EiemLuaUi> script; bool reported = false; };
  std::vector<Entry> entries;
  struct ContextScope {
    ImGuiContext *previous;
    explicit ContextScope(ImGuiContext *next) : previous(ImGui::GetCurrentContext()) { ImGui::SetCurrentContext(next); }
    ~ContextScope() { ImGui::SetCurrentContext(previous); }
  };
  static LRESULT CALLBACK WndProc(HWND window, UINT message, WPARAM wp, LPARAM lp) {
    auto *self = reinterpret_cast<EiemUiHost *>(GetWindowLongPtrW(window,GWLP_USERDATA));
    if (message == WM_NCCREATE) {
      self = static_cast<EiemUiHost *>(reinterpret_cast<CREATESTRUCTW *>(lp)->lpCreateParams);
      SetWindowLongPtrW(window,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(self));
    }
    if (self && self->context) {
      ContextScope scope(self->context);
      if (ImGui_ImplWin32_WndProcHandler(window,message,wp,lp)) return 1;
    }
    if (message == WM_CLOSE && self) {
      for (auto &entry : self->entries) entry.script->open = false;
      return 0;
    }
    if (message == WM_ERASEBKGND) return 1;
    return DefWindowProcW(window,message,wp,lp);
  }
  void DeleteContext() {
    if (!context) return;
    ContextScope scope(context);
    ImGui_ImplDX11_Shutdown(); ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext(context); context = nullptr;
  }
  bool NewContext() {
    context = ImGui::CreateContext();
    ImGui::SetCurrentContext(context);
    auto &io = ImGui::GetIO(); io.IniFilename = nullptr;
    ImGui::StyleColorsDark();
    // System font path is discovered, not tied to a user or drive.
    wchar_t windows[MAX_PATH]; GetWindowsDirectoryW(windows,MAX_PATH);
    auto fontPath = std::filesystem::path(windows) / L"Fonts" / L"msyh.ttc";
    if (std::filesystem::is_regular_file(fontPath))
      io.Fonts->AddFontFromFileTTF(fontPath.u8string().c_str(),18,nullptr,io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
    else io.Fonts->AddFontDefault();
    bool win32 = ImGui_ImplWin32_Init(hwnd);
    bool dx11 = win32 && ImGui_ImplDX11_Init(device.Get(),immediate.Get());
    if (!dx11) {
      if (win32) ImGui_ImplWin32_Shutdown();
      ImGui::DestroyContext(context); context = nullptr;
    }
    return dx11;
  }
  bool Create() {
    WNDCLASSW wc = {}; wc.lpfnWndProc = WndProc; wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"EIEM_ModLuaUI"; wc.hCursor = LoadCursor(nullptr,IDC_ARROW);
    if (!RegisterClassW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return false;
    hwnd = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOREDIRECTIONBITMAP | WS_EX_TRANSPARENT,
      wc.lpszClassName,L"EIEM Mod UI",WS_POPUP,0,0,1,1,nullptr,nullptr,wc.hInstance,this);
    if (!hwnd) return false;
    g_modUiHwnd = hwnd;
    D3D_FEATURE_LEVEL level;
    if (FAILED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        nullptr,0,D3D11_SDK_VERSION,&device,&level,&immediate))) return false;
    Com<IDXGIDevice> dxgi; Com<IDXGIAdapter> adapter; Com<IDXGIFactory2> factory;
    if (FAILED(device.As(&dxgi)) || FAILED(dxgi->GetAdapter(&adapter)) ||
        FAILED(adapter->GetParent(IID_PPV_ARGS(&factory)))) return false;
    DXGI_SWAP_CHAIN_DESC1 desc = {}; desc.Width = desc.Height = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; desc.SampleDesc.Count = 1;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; desc.BufferCount = 2;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; desc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;
    if (FAILED(factory->CreateSwapChainForComposition(device.Get(),&desc,nullptr,&swap)) ||
        FAILED(DCompositionCreateDevice(dxgi.Get(),IID_PPV_ARGS(&composition))) ||
        FAILED(composition->CreateTargetForHwnd(hwnd,TRUE,&compositionTarget)) ||
        FAILED(composition->CreateVisual(&visual)) || FAILED(visual->SetContent(swap.Get())) ||
        FAILED(compositionTarget->SetRoot(visual.Get())) || FAILED(composition->Commit())) return false;
    return NewContext();
  }
  bool Resize(int w, int h) {
    if (width == w && height == h && target) return true;
    immediate->OMSetRenderTargets(0,nullptr,nullptr); target.Reset();
    if (FAILED(swap->ResizeBuffers(0,w,h,DXGI_FORMAT_UNKNOWN,0))) return false;
    Com<ID3D11Texture2D> buffer;
    if (FAILED(swap->GetBuffer(0,IID_PPV_ARGS(&buffer))) ||
        FAILED(device->CreateRenderTargetView(buffer.Get(),nullptr,&target))) return false;
    width = w; height = h; return true;
  }
  void Visible(bool value) {
    if (shown == value) return;
    shown = value; g_modUiVisible = value;
    ShowWindow(hwnd,value ? SW_SHOWNOACTIVATE : SW_HIDE);
    if (value) ReleaseCursorToGui();
    else {
      if (!g_guiVisible) ReturnCursorToGame();
      if (GetForegroundWindow() == hwnd && g_gameHwnd) SetForegroundWindow(g_gameHwnd);
    }
  }
public:
  ~EiemUiHost() { Shutdown(); }
  bool InitializeHidden() {
    ContextScope restore(ImGui::GetCurrentContext());
    if (hwnd) return context != nullptr;
    if (Create()) return true;
    Shutdown(); return false;
  }
  void Shutdown() {
    entries.clear(); Visible(false); DeleteContext();
    target.Reset(); visual.Reset(); compositionTarget.Reset(); composition.Reset();
    swap.Reset(); immediate.Reset(); device.Reset();
    if (hwnd) DestroyWindow(hwnd);
    hwnd = nullptr; g_modUiHwnd = nullptr;
    UnregisterClassW(L"EIEM_ModLuaUI",GetModuleHandleW(nullptr));
  }
  void Tick() {
    ContextScope restore(ImGui::GetCurrentContext());
    LONG nextGeneration; auto snapshots = EiemGetModUis(&nextGeneration);
    if (nextGeneration != generation) {
      std::unordered_set<std::string> opened;
      for (const auto &entry : entries) if (entry.script->open)
        opened.insert(EiemModIdentifier(entry.snapshot.modPath.c_str(),entry.snapshot.ui.section.c_str()));
      if (failed) Shutdown(); // a failed device is rebuilt, never reused as a fallback
      entries.clear(); generation = nextGeneration; failed = false;
      // Full context replacement removes active widgets, drag state and backend
      // input belonging to the previous generation, even when titles match.
      if (context) { DeleteContext(); if (!NewContext()) failed = true; }
      for (const auto &snapshot : snapshots) {
        std::string id = EiemModIdentifier(snapshot.modPath.c_str(),snapshot.ui.section.c_str());
        auto script = std::make_unique<EiemLuaUi>(id); script->open = opened.count(id) != 0;
        bool loaded = script->LoadFile(snapshot.modPath,snapshot.ui.path);
        if (!loaded)
          Log("[UI] %s/%s: %s",snapshot.modPath.c_str(),snapshot.ui.section.c_str(),script->error.c_str());
        entries.push_back({snapshot,std::move(script),!loaded});
      }
    } else {
      for (size_t i = 0; i < entries.size() && i < snapshots.size(); ++i)
        entries[i].snapshot.variables = std::move(snapshots[i].variables);
    }
    const auto global = EiemGetGlobalConfig();
    auto keys = EiemTakeUiKeys();
    bool any = false;
    for (auto &entry : entries) {
      auto &s = entry.snapshot;
      bool eligible = !s.ui.condition || s.ui.condition->Evaluate(s.variables) != 0;
      if (!eligible) entry.script->open = false;
      for (const auto &key : keys)
        if (eligible && key.second == generation && key.first == s.ui.chord &&
            !(key.first == global.gui) && !(key.first == global.reload)) entry.script->open = !entry.script->open;
      any |= entry.script->open;
    }
    HWND foreground = GetForegroundWindow();
    const bool owned = foreground == g_gameHwnd || foreground == g_guiHwnd || foreground == hwnd;
    if (!any || !g_pluginActive || !owned || IsIconic(g_gameHwnd)) { Visible(false); return; }
    if (failed) return;
    if (!hwnd && !InitializeHidden()) {
      Log("[UI] Native host creation failed err=%lu; retry with Reload",GetLastError());
      Shutdown(); failed = true; return;
    }
    if (!context) { failed = true; return; }
    ImGui::SetCurrentContext(context);
    RECT rect; GetClientRect(g_gameHwnd,&rect); POINT origin = {}; ClientToScreen(g_gameHwnd,&origin);
    if (rect.right <= 0 || rect.bottom <= 0) { Visible(false); return; }
    if (!Resize(rect.right,rect.bottom)) { Log("[UI] Resize failed; retry with Reload"); Visible(false); failed = true; return; }
    SetWindowPos(hwnd,HWND_TOPMOST,origin.x,origin.y,rect.right,rect.bottom,SWP_NOACTIVATE);
    Visible(true);
    ImGui_ImplDX11_NewFrame(); ImGui_ImplWin32_NewFrame(); ImGui::NewFrame();
    for (auto &entry : entries) {
      EiemVariables values;
      if (entry.script->Draw(entry.snapshot.variables,values) && !values.empty()) {
        // Validation at submission gives script authors immediate diagnostics;
        // publication validates again against the actual generation/state.
        std::string error;
        EiemVariables proposed;
        bool valid = false;
        AcquireSRWLockShared(&s_eiemModLock);
        for (size_t i = 0; i < s_eiemModProgram.states.size(); ++i)
          if (s_eiemModProgram.states[i].path == entry.snapshot.modPath)
            valid = EiemValidateUiValues(s_eiemModProgram,i,values,proposed,error);
        ReleaseSRWLockShared(&s_eiemModLock);
        if (valid) EiemQueueModInput({{},generation,entry.snapshot.modPath,entry.snapshot.ui.section,std::move(values)});
        else entry.script->error = error.empty() ? "UI Mod is no longer available" : error;
      }
      entry.script->DrawError();
      if (!entry.script->error.empty() && !entry.reported) {
        Log("[UI] %s/%s: %s",entry.snapshot.modPath.c_str(),entry.snapshot.ui.section.c_str(),entry.script->error.c_str());
        entry.reported = true;
      }
    }
    ImGui::Render();
    POINT mouse; GetCursorPos(&mouse); ScreenToClient(hwnd,&mouse);
    bool hit = context->ActiveId != 0;
    for (auto *window : context->Windows)
      if (window->Active && !window->Hidden && !(window->Flags & ImGuiWindowFlags_NoMouseInputs) &&
          window->Rect().Contains(ImVec2((float)mouse.x,(float)mouse.y))) hit = true;
    auto ex = GetWindowLongPtrW(hwnd,GWL_EXSTYLE);
    auto desired = hit ? ex & ~WS_EX_TRANSPARENT : ex | WS_EX_TRANSPARENT;
    if (desired != ex) SetWindowLongPtrW(hwnd,GWL_EXSTYLE,desired);
    ID3D11RenderTargetView *rt = target.Get(); float clear[4] = {};
    immediate->OMSetRenderTargets(1,&rt,nullptr); immediate->ClearRenderTargetView(rt,clear);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    if (FAILED(swap->Present(0,0))) { Log("[UI] Present failed; retry with Reload"); Visible(false); failed = true; }
  }
};
