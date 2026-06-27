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