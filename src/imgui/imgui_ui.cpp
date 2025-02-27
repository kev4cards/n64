#include "imgui_ui.h"
#include <rdp/parallel_rdp_wrapper.h>
#include <volk.h>
#include <imgui.h>
#include <implot.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_vulkan.h>
#include <cstdio>
#include <cstdlib>
#include <nfd.hpp>
#include <map>

#include <frontend/render_internal.h>
#include <metrics.h>
#include <mem/pif.h>
#include <mem/mem_util.h>
#include <cpu/dynarec/dynarec.h>
#include <frontend/audio.h>
#include <frontend/render.h>
#include <disassemble.h>

static bool show_metrics_window = false;
static bool show_imgui_demo_window = false;
static bool show_settings_window = false;
static bool show_dynarec_block_browser = false;

static bool is_fullscreen = false;

#define METRICS_HISTORY_SECONDS 5

#define METRICS_HISTORY_ITEMS ((METRICS_HISTORY_SECONDS) * 60)

template<typename T>
struct RingBuffer {
    int offset;
    T data[METRICS_HISTORY_ITEMS];
    RingBuffer() {
        offset = 0;
        memset(data, 0, sizeof(data));
    }

    T max() {
        T max_ = 0;
        for (int i = 0; i < METRICS_HISTORY_ITEMS; i++) {
            if (data[i] > max_) {
                max_ = data[i];
            }
        }
        return max_;
    }

    void add_point(T point) {
        data[offset++] = point;
        offset %= METRICS_HISTORY_ITEMS;
    }
};

RingBuffer<double> frame_times;
RingBuffer<ImU64> block_compilations;
RingBuffer<ImU64> block_sysconfig_misses;
RingBuffer<ImU64> rsp_steps;
RingBuffer<ImU64> codecache_bytes_used;
RingBuffer<ImU64> audiostream_bytes_available;
RingBuffer<ImU64> si_interrupts;
RingBuffer<ImU64> pi_interrupts;
RingBuffer<ImU64> ai_interrupts;
RingBuffer<ImU64> dp_interrupts;
RingBuffer<ImU64> sp_interrupts;


void render_menubar() {
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Load ROM")) {
                nfdchar_t* rom_path;
                nfdfilteritem_t filters[] = {{"N64 ROMs", "n64,v64,z64,N64,V64,Z64"}};
                if(NFD_OpenDialog(&rom_path, filters, 1, nullptr) == NFD_OKAY) {
                    reset_n64system();
                    n64_load_rom(rom_path);
                    NFD_FreePath(rom_path);
                    pif_rom_execute();
                }

            }

            if (ImGui::MenuItem("Save RDRAM dump (big endian)")) {
#ifdef N64_BIG_ENDIAN
                save_rdram_dump(false);
#else
                save_rdram_dump(true);
#endif
            }

            if (ImGui::MenuItem("Save RDRAM dump (little endian)")) {
#ifdef N64_BIG_ENDIAN
                save_rdram_dump(true);
#else
                save_rdram_dump(false);
#endif
            }

            if (ImGui::MenuItem("Quit")) {
                n64_request_quit();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Emulation")) {
            if(ImGui::MenuItem("Reset")) {
                if(strcmp(n64sys.rom_path, "") != 0) {
                    reset_n64system();
                    n64_load_rom(n64sys.rom_path);
                    pif_rom_execute();
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Window"))
        {
            if (ImGui::MenuItem("Metrics", nullptr, show_metrics_window)) { show_metrics_window = !show_metrics_window; }
            if (ImGui::MenuItem("Settings", nullptr, show_settings_window)) { show_settings_window = !show_settings_window; }
            if (ImGui::MenuItem("Dynarec Block Browser", nullptr, show_dynarec_block_browser)) { show_dynarec_block_browser = !show_dynarec_block_browser; }
            if (ImGui::MenuItem("ImGui Demo Window", nullptr, show_imgui_demo_window)) { show_imgui_demo_window = !show_imgui_demo_window; }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItem("Fullscreen", nullptr, is_fullscreen)) {
                is_fullscreen = !is_fullscreen;
                if (is_fullscreen) {
                    SDL_SetWindowFullscreen(get_window_handle(), SDL_WINDOW_FULLSCREEN_DESKTOP); // Fake fullscreen
                } else {
                    SDL_SetWindowFullscreen(get_window_handle(), 0); // Back to windowed
                }
            }

            if (ImGui::MenuItem("Unlock Framerate", nullptr, is_framerate_unlocked())) {
                set_framerate_unlocked(!is_framerate_unlocked());
            }

            ImGui::EndMenu();
        }

        ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);

        ImGui::EndMainMenuBar();
    }
}

void render_metrics_window() {
    block_compilations.add_point(get_metric(METRIC_BLOCK_COMPILATION));
    block_sysconfig_misses.add_point(get_metric(METRIC_BLOCK_SYSCONFIG_MISS));
    rsp_steps.add_point(get_metric(METRIC_RSP_STEPS));
    double frametime = 1000.0f / ImGui::GetIO().Framerate;
    frame_times.add_point(frametime);
    codecache_bytes_used.add_point(n64dynarec.codecache_used);
    audiostream_bytes_available.add_point(get_metric(METRIC_AUDIOSTREAM_AVAILABLE));

    si_interrupts.add_point(get_metric(METRIC_SI_INTERRUPT));
    pi_interrupts.add_point(get_metric(METRIC_PI_INTERRUPT));
    ai_interrupts.add_point(get_metric(METRIC_AI_INTERRUPT));
    dp_interrupts.add_point(get_metric(METRIC_DP_INTERRUPT));
    sp_interrupts.add_point(get_metric(METRIC_SP_INTERRUPT));

    const auto flags = ImDrawListFlags_AntiAliasedLines;

    ImGui::Begin("Performance Metrics", &show_metrics_window);
    ImGui::Text("Average %.3f ms/frame (%.1f FPS)", frametime, ImGui::GetIO().Framerate);

    //ImPlot::GetStyle().AntiAliasedLines = true;

    ImPlot::SetNextAxisLimits(ImAxis_Y1, 0, frame_times.max(), ImGuiCond_Always);
    ImPlot::SetNextAxisLimits(ImAxis_X1, 0, METRICS_HISTORY_ITEMS, ImGuiCond_Always);
    if (ImPlot::BeginPlot("Frame Times")) {
        ImPlot::PlotLine("Frame Time (ms)", frame_times.data, METRICS_HISTORY_ITEMS, 1, 0, flags, frame_times.offset);
        ImPlot::EndPlot();
    }

    ImPlot::SetNextAxisLimits(ImAxis_Y1, 0, rsp_steps.max(), ImGuiCond_Always);
    ImPlot::SetNextAxisLimits(ImAxis_X1, 0, METRICS_HISTORY_ITEMS, ImGuiCond_Always);
    if (ImPlot::BeginPlot("RSP Steps Per Frame")) {
        ImPlot::PlotLine("RSP Steps", rsp_steps.data, METRICS_HISTORY_ITEMS, 1, 0, flags, rsp_steps.offset);
        ImPlot::EndPlot();
    }

    ImGui::Text("Block compilations this frame: %" PRId64, get_metric(METRIC_BLOCK_COMPILATION));
    ImPlot::SetNextAxisLimits(ImAxis_Y1, 0, block_compilations.max(), ImGuiCond_Always);
    ImPlot::SetNextAxisLimits(ImAxis_X1, 0, METRICS_HISTORY_ITEMS, ImGuiCond_Always);
    if (ImPlot::BeginPlot("Block Compilations Per Frame")) {
        ImPlot::PlotBars("Block compilations", block_compilations.data, METRICS_HISTORY_ITEMS, 1, 0, flags, block_compilations.offset);
        ImPlot::EndPlot();
    }

    ImGui::Text("Block sysconfig misses this frame: %" PRId64, get_metric(METRIC_BLOCK_SYSCONFIG_MISS));
    ImPlot::SetNextAxisLimits(ImAxis_Y1, 0, block_sysconfig_misses.max(), ImGuiCond_Always);
    ImPlot::SetNextAxisLimits(ImAxis_X1, 0, METRICS_HISTORY_ITEMS, ImGuiCond_Always);
    if (ImPlot::BeginPlot("Block Sysconfig Misses Per Frame")) {
        ImPlot::PlotBars("Block sysconfig misses", block_sysconfig_misses.data, METRICS_HISTORY_ITEMS, 1, 0, flags, block_sysconfig_misses.offset);
        ImPlot::EndPlot();
    }

    ImPlot::SetNextAxisLimits(ImAxis_Y1, 0, n64dynarec.codecache_size, ImGuiCond_Always);
    ImPlot::SetNextAxisLimits(ImAxis_X1, 0, METRICS_HISTORY_ITEMS, ImGuiCond_Always);
    if (ImPlot::BeginPlot("Codecache bytes used")) {
        ImPlot::PlotBars("Codecache bytes used", codecache_bytes_used.data, METRICS_HISTORY_ITEMS, 1, 0, flags, codecache_bytes_used.offset);
        ImPlot::EndPlot();
    }

    ImGui::Text("Audio stream bytes available: %" PRId64, get_metric(METRIC_AUDIOSTREAM_AVAILABLE));
    ImPlot::SetNextAxisLimits(ImAxis_Y1, 0, audiostream_bytes_available.max(), ImGuiCond_Always);
    ImPlot::SetNextAxisLimits(ImAxis_X1, 0, METRICS_HISTORY_ITEMS, ImGuiCond_Always);
    if (ImPlot::BeginPlot("Audio Stream Bytes Available")) {
        ImPlot::PlotLine("Audio Stream Bytes Available", audiostream_bytes_available.data, METRICS_HISTORY_ITEMS, 1, 0, flags, audiostream_bytes_available.offset);
        ImPlot::EndPlot();
    }

#define MAX(a, b) ((a) > (b) ? (a) : (b))

    int interruptsMax = MAX(MAX(MAX(MAX(si_interrupts.max(), pi_interrupts.max()), ai_interrupts.max()), dp_interrupts.max()), sp_interrupts.max());
    ImPlot::SetNextAxisLimits(ImAxis_Y1, 0, interruptsMax, ImGuiCond_Always);
    ImPlot::SetNextAxisLimits(ImAxis_X1, 0, METRICS_HISTORY_ITEMS, ImGuiCond_Always);
    if (ImPlot::BeginPlot("Interrupts Per Frame")) {
        ImPlot::PlotLine("SI Interrupts", si_interrupts.data, METRICS_HISTORY_ITEMS, 1, 0, flags, si_interrupts.offset);
        ImPlot::PlotLine("PI Interrupts", pi_interrupts.data, METRICS_HISTORY_ITEMS, 1, 0, flags, pi_interrupts.offset);
        ImPlot::PlotLine("AI Interrupts", ai_interrupts.data, METRICS_HISTORY_ITEMS, 1, 0, flags, ai_interrupts.offset);
        ImPlot::PlotLine("DP Interrupts", dp_interrupts.data, METRICS_HISTORY_ITEMS, 1, 0, flags, dp_interrupts.offset);
        ImPlot::PlotLine("SP Interrupts", sp_interrupts.data, METRICS_HISTORY_ITEMS, 1, 0, flags, sp_interrupts.offset);
        ImPlot::EndPlot();
    }

    ImGui::End();
}

void render_settings_window() {
    ImGui::Begin("Settings", &show_settings_window);
    ImGui::End();
}
struct block {
    block(u32 address, int outer_index, int inner_index) : address(address), outer_index(outer_index), inner_index(inner_index) {}
    block() : block(0, 0, 0) {}
    block(u32 address) : block(address, BLOCKCACHE_OUTER_INDEX(address), BLOCKCACHE_INNER_INDEX(address)) {}
    u32 address;
    int outer_index;
    int inner_index;
};
std::vector<block> blocks;
std::map<u32, std::string> mips_block;
std::map<u32, std::string> host_block;
block selected_block;
char block_filter[9] = { 0 };
void render_dynarec_block_browser() {
    ImGui::Begin("Block Browser", &show_dynarec_block_browser);
    if (ImGui::Button("Refresh")) {
        block old_selected_block = selected_block;
        bool old_selected_block_still_valid = false;
        blocks.clear();
        mips_block.clear();
        host_block.clear();
        for (int outer_index = 0; outer_index < BLOCKCACHE_OUTER_SIZE; outer_index++) {
            if (n64dynarec.blockcache[outer_index]) {
                n64_dynarec_block_t* block_list = n64dynarec.blockcache[outer_index];
                for (int inner_index = 0; inner_index < BLOCKCACHE_INNER_SIZE; inner_index++) {
                    if (block_list[inner_index].run != nullptr) {
                        u32 addr = INDICES_TO_ADDRESS(outer_index, inner_index);
                        if (addr == old_selected_block.address) {
                            old_selected_block_still_valid = true;
                        }
                        blocks.emplace_back(addr, outer_index, inner_index);
                    }
                }
            }
        }

        if (old_selected_block_still_valid) {
            selected_block = old_selected_block;
        } else if (blocks.empty()) {
            selected_block = block(0);
        } else {
            selected_block = blocks[0].address;
        }
    }
    ImGui::SameLine();
    ImGui::PushItemWidth(100);
    ImGui::InputText("Filter blocks", block_filter, 8);
    ImGui::PopItemWidth();
    for (int i = 0; i < 9 && block_filter[i] != 0; i++) {
        block_filter[i] = toupper(block_filter[i]);
    }

    ImGui::BeginGroup();
    ImGui::Text("Blocks");
    if (ImGui::BeginListBox("##Blocks", ImVec2(150, -1))) {
        if (blocks.empty()) {
            ImGui::Selectable("No blocks loaded", false);
        }
        for (const block b : blocks) {
            char str_block_addr[9];
            snprintf(str_block_addr, 9, "%08X", b.address);
            if (strlen(block_filter) == 0 || strstr(str_block_addr, block_filter) != nullptr) {
                if (ImGui::Selectable((std::string(str_block_addr)).c_str(), selected_block.address == b.address)) {
                    selected_block = b.address;
                }
            }
        }
        ImGui::EndListBox();
    }
    ImGui::EndGroup();
    ImGui::SameLine();

    if (host_block.count(selected_block.address) == 0 || mips_block.count(selected_block.address) == 0) {
        n64_dynarec_block_t* block_list = n64dynarec.blockcache[selected_block.outer_index];
        if (block_list && block_list[selected_block.inner_index].host_size > 0) {
            n64_dynarec_block_t* b = &block_list[selected_block.inner_index];
            host_block[selected_block.address] = disassemble_multi(DisassemblyArch::HOST,  (uintptr_t)b->run, (u8*)b->run, b->host_size);
            bool valid_guest_addr = false;
            u8* guest_block_address;

            switch (selected_block.address) {
                case REGION_RDRAM:
                    guest_block_address = &n64sys.mem.rdram[selected_block.address];
                    valid_guest_addr = true;
                    break;
                case REGION_SP_MEM:
                    /*
                    if (selected_block.address & 0x1000) {
                        guest_block_address = &N64RSP.sp_imem[selected_block.address & 0xFFF];
                        valid_guest_addr = true;
                    } else {
                        guest_block_address = &N64RSP.sp_dmem[selected_block.address & 0xFFF];
                        valid_guest_addr = true;
                    }
                    break;
                     */
                default:
                    valid_guest_addr = false;
            }

            if (valid_guest_addr) {
                mips_block[selected_block.address] = disassemble_multi(DisassemblyArch::GUEST, selected_block.address, (u8*)guest_block_address, b->guest_size);
            } else {
                mips_block[selected_block.address] = "Guest block not in valid region, not disassembling";
            }
        } else {
            host_block[selected_block.address] = "Invalid";
            mips_block[selected_block.address] = "Invalid";
        }
    }

    ImGui::BeginGroup();
    std::string& mips_disasm = mips_block[selected_block.address];
    ImGui::Text("Mips Disassembly");
    ImGui::InputTextMultiline(
            "##MipsDisAsm",
            mips_disasm.data(),
            mips_disasm.size(),
            ImVec2(400, -1),
            ImGuiInputTextFlags_ReadOnly);
    ImGui::EndGroup();
    ImGui::SameLine();
    ImGui::BeginGroup();
    std::string& host_disasm = host_block[selected_block.address];
    ImGui::Text("Host Disassembly");
    ImGui::InputTextMultiline(
            "##HostDisAsm",
            host_disasm.data(),
            host_disasm.size(),
            ImVec2(-1, -1),
            ImGuiInputTextFlags_ReadOnly);
    ImGui::EndGroup();
    ImGui::End();
}

void render_ui() {
    if (SDL_GetMouseFocus() || n64sys.mem.rom.rom == nullptr) {
        render_menubar();
    }
    if (show_metrics_window) { render_metrics_window(); }
    if (show_imgui_demo_window) { ImGui::ShowDemoWindow(&show_imgui_demo_window); }
    if (show_settings_window) { render_settings_window(); }
    if (show_dynarec_block_browser) { render_dynarec_block_browser(); }
}

static VkAllocationCallbacks*   g_Allocator = NULL;
static VkInstance               g_Instance = VK_NULL_HANDLE;
static VkPhysicalDevice         g_PhysicalDevice = VK_NULL_HANDLE;
static VkDevice                 g_Device = VK_NULL_HANDLE;
static uint32_t                 g_QueueFamily = (uint32_t)-1;
static VkQueue                  g_Queue = VK_NULL_HANDLE;
static VkPipelineCache          g_PipelineCache = VK_NULL_HANDLE;
static VkDescriptorPool         g_DescriptorPool = VK_NULL_HANDLE;

static ImGui_ImplVulkanH_Window g_MainWindowData;
static uint32_t                 g_MinImageCount = 2;

static void check_vk_result(VkResult err)
{
    if (err == 0)
        return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
        abort();
}

void load_imgui_ui() {
    VkResult err;

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    g_Instance = get_vk_instance();
    g_PhysicalDevice = get_vk_physical_device();
    g_Device = get_vk_device();
    g_QueueFamily = get_vk_graphics_queue_family();
    g_Queue = get_graphics_queue();
    g_PipelineCache = nullptr;
    g_DescriptorPool = nullptr;
    g_Allocator = nullptr;
    g_MinImageCount = 2;


    // Create Descriptor Pool
    {
        VkDescriptorPoolSize pool_sizes[] =
                {
                        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
                        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
                        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
                        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
                        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
                        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
                        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
                        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
                        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
                        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
                        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
                };
        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
        pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;
        err = vkCreateDescriptorPool(g_Device, &pool_info, g_Allocator, &g_DescriptorPool);
        check_vk_result(err);
    }

    // Create the Render Pass
    VkRenderPass renderPass;
    {
        VkAttachmentDescription attachment = {};
        attachment.format = get_vk_format();
        attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        VkAttachmentReference color_attachment = {};
        color_attachment.attachment = 0;
        color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color_attachment;
        VkSubpassDependency dependency = {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        VkRenderPassCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        info.attachmentCount = 1;
        info.pAttachments = &attachment;
        info.subpassCount = 1;
        info.pSubpasses = &subpass;
        info.dependencyCount = 1;
        info.pDependencies = &dependency;
        err = vkCreateRenderPass(g_Device, &info, g_Allocator, &renderPass);
        check_vk_result(err);
    }

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForVulkan(get_window_handle());
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = g_Instance;
    init_info.PhysicalDevice = g_PhysicalDevice;
    init_info.Device = g_Device;
    init_info.QueueFamily = g_QueueFamily;
    init_info.Queue = g_Queue;
    init_info.PipelineCache = g_PipelineCache;
    init_info.DescriptorPool = g_DescriptorPool;
    init_info.Allocator = g_Allocator;
    init_info.MinImageCount = g_MinImageCount;
    init_info.ImageCount = 2;
    init_info.CheckVkResultFn = check_vk_result;

    ImGui_ImplVulkan_Init(&init_info, renderPass);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    // Upload Fonts
    {
        VkCommandBuffer command_buffer = get_vk_command_buffer();
        ImGui_ImplVulkan_CreateFontsTexture(command_buffer);
        submit_requested_vk_command_buffer();
    }

    NFD_Init();
}

ImDrawData* imgui_frame() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL2_NewFrame(get_window_handle());
    ImGui::NewFrame();

    render_ui();

    ImGui::Render();
    return ImGui::GetDrawData();
}

bool imgui_wants_mouse() {
    return ImGui::GetIO().WantCaptureMouse;
}

bool imgui_wants_keyboard() {
    return ImGui::GetIO().WantCaptureKeyboard;
}

bool imgui_handle_event(SDL_Event* event) {
    bool captured = false;
    switch (event->type) {
        case SDL_KEYDOWN:
        case SDL_KEYUP:
        case SDL_TEXTEDITING:
        case SDL_TEXTINPUT:
        case SDL_KEYMAPCHANGED:
            captured = imgui_wants_keyboard();
            break;
        case SDL_MOUSEMOTION:
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
        case SDL_MOUSEWHEEL:
            captured = imgui_wants_mouse();
            break;
    }
    ImGui_ImplSDL2_ProcessEvent(event);

    return captured;
}