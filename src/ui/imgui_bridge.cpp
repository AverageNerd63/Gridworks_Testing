#include <windows.h>
#include "imgui.h"
#include "backends/imgui_impl_vulkan.h"
#include "backends/imgui_impl_win32.h"

extern "C" {
#include "imgui_bridge.h"
#include "../renderer/vulkan/vk_ctx.h"
#include "../renderer/vulkan/vk_loader.h"
#include "../renderer/vulkan/vk_renderpass.h"
#include "../renderer/vulkan/vk_swapchain.h"
#include "../core/logger.h"
}

static bool s_initialized = false;

static PFN_vkVoidFunction vk_func_loader(const char *name, void *) {
    PFN_vkVoidFunction f = nullptr;
    /* instance first — avoids validation warnings for instance-level functions */
    if (s_ctx.instance) f = vk.vkGetInstanceProcAddr(s_ctx.instance, name);
    if (!f && s_ctx.device) f = vk.vkGetDeviceProcAddr(s_ctx.device, name);
    return f;
}

extern "C" bool imgui_init(void *hwnd) {
    if (s_initialized) return true;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();
    ImGui::GetStyle().WindowRounding = 4.0f;

    if (!ImGui_ImplVulkan_LoadFunctions(VK_API_VERSION_1_3, vk_func_loader, nullptr)) {
        LOG_ERROR("[imgui] failed to load Vulkan functions");
        return false;
    }

    ImGui_ImplWin32_Init(hwnd);

    /* Strip multi-viewport flags — Platform_CreateVkSurface not wired up yet. */
    io.ConfigFlags  &= ~ImGuiConfigFlags_ViewportsEnable;
    io.BackendFlags &= ~ImGuiBackendFlags_PlatformHasViewports;
    io.BackendFlags &= ~ImGuiBackendFlags_HasMouseHoveredViewport;

    ImGui_ImplVulkan_InitInfo vi = {};
    vi.ApiVersion      = VK_API_VERSION_1_3;
    vi.Instance        = s_ctx.instance;
    vi.PhysicalDevice  = s_ctx.gpu;
    vi.Device          = s_ctx.device;
    vi.QueueFamily     = s_ctx.graphics_family;
    vi.Queue           = s_ctx.graphics_queue;
    vi.DescriptorPool  = s_ctx.imgui_pool;
    vi.MinImageCount   = 2;
    vi.ImageCount      = s_swapchain.image_count;
    vi.UseDynamicRendering = false;

    vi.PipelineInfoMain.RenderPass   = s_render_pass;
    vi.PipelineInfoMain.MSAASamples  = VK_SAMPLE_COUNT_1_BIT;
    vi.PipelineInfoMain.Subpass      = 0;

    if (!ImGui_ImplVulkan_Init(&vi)) {
        LOG_ERROR("[imgui] ImGui_ImplVulkan_Init failed");
        return false;
    }

    LOG_INFO("[imgui] initialized");
    s_initialized = true;
    return true;
}

extern "C" void imgui_shutdown(void) {
    if (!s_initialized) return;
    s_initialized = false;
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    LOG_INFO("[imgui] shut down");
}

extern "C" void imgui_new_frame(void) {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

extern "C" void imgui_render(void *cmd_buf) {
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(),
                                    (VkCommandBuffer)cmd_buf);
}

static ImGuiKey vk_to_imgui_key(uintptr_t vk) {
    if (vk >= 'A' && vk <= 'Z') return (ImGuiKey)(ImGuiKey_A + (vk - 'A'));
    if (vk >= '0' && vk <= '9') return (ImGuiKey)(ImGuiKey_0 + (vk - '0'));
    switch (vk) {
        case VK_TAB:       return ImGuiKey_Tab;
        case VK_LEFT:      return ImGuiKey_LeftArrow;
        case VK_RIGHT:     return ImGuiKey_RightArrow;
        case VK_UP:        return ImGuiKey_UpArrow;
        case VK_DOWN:      return ImGuiKey_DownArrow;
        case VK_PRIOR:     return ImGuiKey_PageUp;
        case VK_NEXT:      return ImGuiKey_PageDown;
        case VK_HOME:      return ImGuiKey_Home;
        case VK_END:       return ImGuiKey_End;
        case VK_INSERT:    return ImGuiKey_Insert;
        case VK_DELETE:    return ImGuiKey_Delete;
        case VK_BACK:      return ImGuiKey_Backspace;
        case VK_SPACE:     return ImGuiKey_Space;
        case VK_RETURN:    return ImGuiKey_Enter;
        case VK_ESCAPE:    return ImGuiKey_Escape;
        case VK_LCONTROL:  return ImGuiKey_LeftCtrl;
        case VK_RCONTROL:  return ImGuiKey_RightCtrl;
        case VK_LSHIFT:    return ImGuiKey_LeftShift;
        case VK_RSHIFT:    return ImGuiKey_RightShift;
        case VK_LMENU:     return ImGuiKey_LeftAlt;
        case VK_RMENU:     return ImGuiKey_RightAlt;
        case VK_LWIN:      return ImGuiKey_LeftSuper;
        case VK_RWIN:      return ImGuiKey_RightSuper;
        case VK_F1:        return ImGuiKey_F1;
        case VK_F2:        return ImGuiKey_F2;
        case VK_F3:        return ImGuiKey_F3;
        case VK_F4:        return ImGuiKey_F4;
        case VK_F5:        return ImGuiKey_F5;
        case VK_F6:        return ImGuiKey_F6;
        case VK_F7:        return ImGuiKey_F7;
        case VK_F8:        return ImGuiKey_F8;
        case VK_F9:        return ImGuiKey_F9;
        case VK_F10:       return ImGuiKey_F10;
        case VK_F11:       return ImGuiKey_F11;
        case VK_F12:       return ImGuiKey_F12;
        case VK_CONTROL:   return ImGuiKey_LeftCtrl;
        case VK_SHIFT:     return ImGuiKey_LeftShift;
        case VK_MENU:      return ImGuiKey_LeftAlt;
        default:           return ImGuiKey_None;
    }
}

extern "C" void imgui_process_msg(unsigned msg, uintptr_t wparam, intptr_t lparam) {
    (void)lparam;
    if (!ImGui::GetCurrentContext()) return;
    ImGuiIO &io = ImGui::GetIO();
    switch (msg) {
        case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK: io.AddMouseButtonEvent(0, true);  break;
        case WM_LBUTTONUP:                           io.AddMouseButtonEvent(0, false); break;
        case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK: io.AddMouseButtonEvent(1, true);  break;
        case WM_RBUTTONUP:                           io.AddMouseButtonEvent(1, false); break;
        case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK: io.AddMouseButtonEvent(2, true);  break;
        case WM_MBUTTONUP:                           io.AddMouseButtonEvent(2, false); break;
        case WM_MOUSEWHEEL:
            io.AddMouseWheelEvent(0.0f, (float)GET_WHEEL_DELTA_WPARAM(wparam) / (float)WHEEL_DELTA);
            break;
        case WM_MOUSEHWHEEL:
            io.AddMouseWheelEvent((float)GET_WHEEL_DELTA_WPARAM(wparam) / (float)WHEEL_DELTA, 0.0f);
            break;
        case WM_CHAR:
            if (wparam > 0 && wparam < 0x10000)
                io.AddInputCharacterUTF16((unsigned short)wparam);
            break;
    }
}