#pragma once

// dxgi_hook.hpp — IDXGISwapChain::Present hook (vtable[8]).
// - PE32+ owner only, 0x20B, reject 0x10B.
// - capture vtable[8] at init, call through captured, restore exactly captured.
// - ImGui DX11+Win32 init on first valid Present.
// - DXGI_ERROR_DEVICE_REMOVED: release RTV and attempt recreate next frame.

#include <windows.h>
#include <winternl.h>
#include <psapi.h>
#include <intrin.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <dxgi.h>

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include <atomic>
#include <cstdint>
#include <cstring>

#pragma comment(lib, "Psapi.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

void rb_renderOverlay(); // main.cpp

namespace rb::hook {
namespace detail {

inline std::atomic<bool> g_hookActive{false};
inline std::atomic<bool> g_renderEnabled{false};
inline IDXGISwapChain* g_swapchain = nullptr;
inline ID3D11Device* g_device = nullptr;
inline ID3D11DeviceContext* g_ctx = nullptr;
inline ID3D11RenderTargetView* g_rtv = nullptr;
inline HWND g_hwnd = nullptr;
inline void* g_capturedPresentAtInit = nullptr;
inline void** g_patchedSlot = nullptr;
inline bool g_imguiInit = false;
inline WNDPROC g_origWndProc = nullptr;
inline std::atomic<bool> g_wndProcHooked{false};
inline std::atomic<bool> g_resizePending{false};
inline UINT g_pendingWidth = 0;
inline UINT g_pendingHeight = 0;

using PresentFn = HRESULT(WINAPI*)(IDXGISwapChain*, UINT, UINT);

inline void Patch(void* addr, const void* data, size_t n) {
  if (!addr || !data || !n) return;
  DWORD old = 0;
  if (!VirtualProtect(addr, n, PAGE_EXECUTE_READWRITE, &old)) return;
  std::memcpy(addr, data, n);
  DWORD tmp = 0;
  VirtualProtect(addr, n, old, &tmp);
  FlushInstructionCache(GetCurrentProcess(), addr, n);
}

inline HMODULE moduleByNamePebLdr(const wchar_t* wanted) {
  if (!wanted) return nullptr;
#ifdef _M_X64
  auto* peb = reinterpret_cast<PTEB>(__readgsqword(reinterpret_cast<DWORD_PTR>(&static_cast<NT_TIB*>(nullptr)->Self)))->ProcessEnvironmentBlock;
#else
  auto* peb = reinterpret_cast<PTEB>(__readfsdword(reinterpret_cast<DWORD_PTR>(&static_cast<NT_TIB*>(nullptr)->Self)))->ProcessEnvironmentBlock;
#endif
  if (!peb || !peb->Ldr) return nullptr;
  auto* head = &peb->Ldr->InMemoryOrderModuleList;
  for (auto* entry = head->Flink; entry && entry != head; entry = entry->Flink) {
    auto* mod = CONTAINING_RECORD(entry, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
    if (!mod->BaseDllName.Buffer) continue;
    if (_wcsicmp(mod->BaseDllName.Buffer, wanted) == 0) return reinterpret_cast<HMODULE>(mod->DllBase);
  }
  return nullptr;
}

inline HMODULE moduleByNameEnumProcessModules(const wchar_t* wanted) {
  if (!wanted) return nullptr;
  HMODULE modules[1024]{};
  DWORD needed = 0;
  if (!EnumProcessModulesEx(GetCurrentProcess(), modules, sizeof(modules), &needed, LIST_MODULES_ALL)) return nullptr;
  const size_t count = (needed / sizeof(HMODULE) < 1024) ? (needed / sizeof(HMODULE)) : 1024;
  for (size_t i = 0; i < count; ++i) {
    wchar_t name[MAX_PATH]{};
    if (!GetModuleBaseNameW(GetCurrentProcess(), modules[i], name, MAX_PATH)) continue;
    if (_wcsicmp(name, wanted) == 0) return modules[i];
  }
  return nullptr;
}

inline HMODULE moduleByName(const wchar_t* wanted) {
  if (HMODULE m = moduleByNamePebLdr(wanted)) return m;
  return moduleByNameEnumProcessModules(wanted);
}

inline uintptr_t moduleContaining(void* addr) {
  if (!addr) return 0;
  HMODULE modules[1024]{};
  DWORD needed = 0;
  if (!EnumProcessModulesEx(GetCurrentProcess(), modules, sizeof(modules), &needed, LIST_MODULES_ALL)) return 0;
  const size_t count = (needed / sizeof(HMODULE) < 1024) ? (needed / sizeof(HMODULE)) : 1024;
  const uintptr_t a = reinterpret_cast<uintptr_t>(addr);
  for (size_t i = 0; i < count; ++i) {
    MODULEINFO mi{};
    if (!GetModuleInformation(GetCurrentProcess(), modules[i], &mi, sizeof(mi))) continue;
    const uintptr_t base = reinterpret_cast<uintptr_t>(mi.lpBaseOfDll);
    const uintptr_t end = base + static_cast<uintptr_t>(mi.SizeOfImage);
    if (a >= base && a < end) return base;
  }
  return 0;
}

inline bool moduleIsPE32Plus(uintptr_t base) {
  if (!base) return false;
  auto* dos = reinterpret_cast<PIMAGE_DOS_HEADER>(base);
  if (dos->e_magic != IMAGE_DOS_SIGNATURE) return false;
  auto* nt = reinterpret_cast<PIMAGE_NT_HEADERS64>(base + dos->e_lfanew);
  if (nt->Signature != IMAGE_NT_SIGNATURE) return false;
  if (nt->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) return false; // reject 0x10B
  return nt->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC; // 0x20B
}

inline void CleanupRenderTarget() {
  if (g_rtv) { g_rtv->Release(); g_rtv = nullptr; }
}

inline bool CreateRenderTarget() {
  CleanupRenderTarget();
  if (!g_swapchain || !g_device) return false;
  ID3D11Texture2D* backBuffer = nullptr;
  if (FAILED(g_swapchain->GetBuffer(0, IID_PPV_ARGS(&backBuffer))) || !backBuffer) return false;
  const HRESULT hr = g_device->CreateRenderTargetView(backBuffer, nullptr, &g_rtv);
  backBuffer->Release();
  return SUCCEEDED(hr) && g_rtv != nullptr;
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

inline LRESULT CALLBACK HkWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  if (g_imguiInit) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
    ImGuiIO& io = ImGui::GetIO();
    const bool mouseMsg = (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST) || msg == WM_MOUSEWHEEL || msg == WM_MOUSEHWHEEL;
    const bool keyMsg = (msg >= WM_KEYFIRST && msg <= WM_KEYLAST) || msg == WM_CHAR || msg == WM_SYSCHAR;
    if ((mouseMsg && io.WantCaptureMouse) || (keyMsg && io.WantCaptureKeyboard)) return true;
  }
  if (msg == WM_SIZE && wParam != SIZE_MINIMIZED) {
    g_pendingWidth = static_cast<UINT>(LOWORD(lParam));
    g_pendingHeight = static_cast<UINT>(HIWORD(lParam));
    g_resizePending.store(true, std::memory_order_release);
  }
  if (g_origWndProc) return CallWindowProcW(g_origWndProc, hWnd, msg, wParam, lParam);
  return DefWindowProcW(hWnd, msg, wParam, lParam);
}

inline void InstallWndProcHook(HWND hwnd) {
  if (!hwnd || g_wndProcHooked.load(std::memory_order_acquire)) return;
  g_hwnd = hwnd;
  g_origWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(HkWndProc)));
  if (g_origWndProc) g_wndProcHooked.store(true, std::memory_order_release);
}

inline void RestoreWndProcHook() {
  if (g_wndProcHooked.load(std::memory_order_acquire) && g_hwnd && g_origWndProc) {
    SetWindowLongPtrW(g_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(g_origWndProc));
  }
  g_origWndProc = nullptr;
  g_wndProcHooked.store(false, std::memory_order_release);
}

inline void CleanupImGui() {
  RestoreWndProcHook();
  if (g_imguiInit) {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    if (ImGui::GetCurrentContext()) ImGui::DestroyContext();
    g_imguiInit = false;
  }
  CleanupRenderTarget();
  if (g_ctx) { g_ctx->Release(); g_ctx = nullptr; }
  if (g_device) { g_device->Release(); g_device = nullptr; }
  g_swapchain = nullptr;
  g_hwnd = nullptr;
}

inline bool InitImGui(IDXGISwapChain* sc) {
  if (g_imguiInit) return true;
  if (!sc) return false;
  g_swapchain = sc;
  if (FAILED(sc->GetDevice(IID_PPV_ARGS(&g_device))) || !g_device) return false;
  g_device->GetImmediateContext(&g_ctx);
  if (!g_ctx) return false;

  DXGI_SWAP_CHAIN_DESC sd{};
  if (FAILED(sc->GetDesc(&sd))) return false;
  g_hwnd = sd.OutputWindow;
  if (!g_hwnd) return false;
  if (!CreateRenderTarget()) return false;

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  ImGui::StyleColorsDark();
  InstallWndProcHook(g_hwnd);
  if (!ImGui_ImplWin32_Init(g_hwnd)) {
    ImGui::DestroyContext();
    CleanupRenderTarget();
    return false;
  }
  if (!ImGui_ImplDX11_Init(g_device, g_ctx)) {
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupRenderTarget();
    return false;
  }
  g_imguiInit = true;
  return true;
}

inline void HandleResize(IDXGISwapChain* sc) {
  if (!g_resizePending.exchange(false, std::memory_order_acq_rel)) return;
  if (!sc || !g_imguiInit) return;
  CleanupRenderTarget();
  const UINT w = g_pendingWidth;
  const UINT h = g_pendingHeight;
  if (w == 0 || h == 0) { CreateRenderTarget(); return; }
  DXGI_SWAP_CHAIN_DESC sd{};
  if (FAILED(sc->GetDesc(&sd))) { CreateRenderTarget(); return; }
  // Release ImGui DX11 device objects referencing backbuffer before ResizeBuffers.
  ImGui_ImplDX11_InvalidateDeviceObjects();
  const HRESULT rhr = sc->ResizeBuffers(sd.BufferCount, w, h, sd.BufferDesc.Format, sd.Flags);
  if (FAILED(rhr)) {
    OutputDebugStringA("[rb] ResizeBuffers failed; rebuilding ImGui\n");
    CleanupImGui();
    return;
  }
  if (!CreateRenderTarget()) { CleanupImGui(); return; }
  ImGui_ImplDX11_CreateDeviceObjects();
}

__declspec(noinline) HRESULT WINAPI PresentStub(IDXGISwapChain* sc, UINT sync, UINT flags) {
  auto real = reinterpret_cast<PresentFn>(g_capturedPresentAtInit);
  if (!real) return DXGI_ERROR_INVALID_CALL;

  if (g_renderEnabled.load(std::memory_order_acquire)) {
    if (g_imguiInit && g_swapchain != sc) {
      CleanupImGui(); // swapchain changed
    }
    if (!g_imguiInit) InitImGui(sc);
    if (g_imguiInit) HandleResize(sc);
    if (g_imguiInit && g_rtv && g_ctx) {
      ImGui_ImplDX11_NewFrame();
      ImGui_ImplWin32_NewFrame();
      ImGui::NewFrame();
      ::rb_renderOverlay();
      ImGui::Render();
      g_ctx->OMSetRenderTargets(1, &g_rtv, nullptr);
      ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }
  }

  const HRESULT hr = real(sc, sync, flags);
  if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
    OutputDebugStringA("[rb] DXGI device removed/reset; rebuilding\n");
    CleanupImGui(); // recreate on next frame
  }
  return hr;
}

inline void InstallPresentHook() {
  if (g_hookActive.load(std::memory_order_acquire)) return;
  if (!g_swapchain) return;
  void** vtbl = *reinterpret_cast<void***>(g_swapchain);
  if (!vtbl) return;

  uintptr_t owner = moduleContaining(g_swapchain);
  if (!owner) owner = moduleContaining(vtbl[8]);
  if (!owner || !moduleIsPE32Plus(owner)) return;

  void* current = vtbl[8];
  if (!current) return;
  g_capturedPresentAtInit = current;
  g_patchedSlot = &vtbl[8];
  void* stub = reinterpret_cast<void*>(&PresentStub);
  Patch(g_patchedSlot, &stub, sizeof(void*));
  g_hookActive.store(true, std::memory_order_release);
  OutputDebugStringA("[rb] Present hook installed\n");
}

} // namespace detail

inline void Attach(IDXGISwapChain* sc) {
  detail::g_swapchain = sc;
  detail::InstallPresentHook();
}

inline void SetRender(bool b) { detail::g_renderEnabled.store(b, std::memory_order_release); }

inline void UnhookPresent() {
  detail::g_renderEnabled.store(false, std::memory_order_release);
  // Restore vtable slot to exactly captured pointer before tearing down ImGui.
  if (detail::g_hookActive.load(std::memory_order_acquire)) {
    if (detail::g_patchedSlot && detail::g_capturedPresentAtInit) {
      void* captured = detail::g_capturedPresentAtInit;
      detail::Patch(detail::g_patchedSlot, &captured, sizeof(void*));
    }
    detail::g_patchedSlot = nullptr;
    detail::g_capturedPresentAtInit = nullptr;
    detail::g_hookActive.store(false, std::memory_order_release);
  }
  detail::CleanupImGui();
}

inline IDXGISwapChain* FindSwapchain() {
  // Dummy-swapchain probe: create temp window + D3D11 swapchain, hook vtable[8] (Present).
  // Vtable is typically shared in dxgi.dll; patch affects game swapchain sharing it.
  WNDCLASSEXW wc{};
  wc.cbSize = sizeof(wc);
  wc.style = CS_CLASSDC;
  wc.lpfnWndProc = DefWindowProcW;
  wc.hInstance = GetModuleHandleW(nullptr);
  wc.lpszClassName = L"rb_dummy_dx11";
  RegisterClassExW(&wc);
  HWND tmpWnd = CreateWindowW(wc.lpszClassName, L"", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, nullptr, nullptr, wc.hInstance, nullptr);
  if (!tmpWnd) { UnregisterClassW(wc.lpszClassName, wc.hInstance); return nullptr; }

  DXGI_SWAP_CHAIN_DESC sd{};
  sd.BufferCount = 1;
  sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  sd.OutputWindow = tmpWnd;
  sd.SampleDesc.Count = 1;
  sd.Windowed = TRUE;
  sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

  ID3D11Device* dev = nullptr;
  ID3D11DeviceContext* ctx = nullptr;
  IDXGISwapChain* sc = nullptr;
  D3D_FEATURE_LEVEL level = D3D_FEATURE_LEVEL_11_0;
  const HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, &level, 1, D3D11_SDK_VERSION, &sd, &sc, &dev, nullptr, &ctx);
  if (ctx) ctx->Release();
  if (dev) dev->Release();
  if (FAILED(hr) || !sc) {
    if (sc) sc->Release();
    DestroyWindow(tmpWnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return nullptr;
  }

  // Attach hooks the vtable slot. We keep sc alive? No - release after attaching; the slot is patched in-place.
  detail::g_swapchain = sc;
  detail::InstallPresentHook();
  sc->Release(); // vtable already patched; real swapchain captured in PresentStub
  detail::g_swapchain = nullptr;

  DestroyWindow(tmpWnd);
  UnregisterClassW(wc.lpszClassName, wc.hInstance);
  return nullptr; // hook installed via dummy; real swapchain arrives in PresentStub
}

} // namespace rb::hook
