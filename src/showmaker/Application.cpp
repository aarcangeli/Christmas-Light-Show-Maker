#include <utility>

#include <utility>
#include <memory>
#include <unistd.h>
#include <string>
#include <use_glad.h>
#include "Application.h"
#include "configLoader.h"
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include "dpi.h"
#include "res_fonts.h"
#include "IconsFontAwesome4.h"
#include "json/json.h"
#include <algorithm>
#include <fstream>
#include <streambuf>
#include <nfd.h>
#include <set>

using namespace sm;
using namespace sm::editor;

namespace sm {
Application *gApp;
float dpi = 1;
}

Application::Application() {
    gApp = this;
}

bool Application::init() {

    // Initialize imgui
    IMGUI_CHECKVERSION();
    ctx = ImGui::CreateContext();
    loader::installConfigLoader(ctx, this);
    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = iniPath.c_str();
    ImGui::LoadIniSettingsFromDisk(io.IniFilename);
    if (windowWidth <= 100) windowWidth = 1280;
    if (windowHeight <= 100) windowHeight = 720;

    // Initialize glfw
    if (!glfwInit()) {
        printf("ERROR: Could not initialize glfw\n");
        return false;
    }

    // Make main window
    glfwWindowHint(GLFW_MAXIMIZED, showMaximized);
    mainWindow = glfwCreateWindow(windowWidth, windowHeight, APPL_NAME, nullptr, nullptr);
    if (!mainWindow) {
        printf("ERROR: Cannot create window\n");
        return false;
    }

    glfwMakeContextCurrent(mainWindow);
    glfwSwapInterval(1);

    // load opengl 3.0
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        printf("Cannot load OpenGL context\n");
        return false;
    }

    printf("Loaded OpenGL %i.%i\n", GLVersion.major, GLVersion.minor);
    if (GLVersion.major < 3) {
        printf("ERROR: OpenGL 3.0 required\n");
        return false;
    }

    ImGui_ImplGlfw_InitForOpenGL(mainWindow, true);
    ImGui_ImplOpenGL3_Init(nullptr);
    applyTheme();

    // some gl parameters
    glClearColor(0.3, 0.3, 0.3, 1);
    glClearDepth(1);

    // restore previous project
    load(autoSavePath);
    if (!proj) {
        open(std::make_shared<project::Project>());
        proj->canvas.makeGroup();
    }

    return true;
}

void Application::applyTheme() {
    ImGuiStyle &style = ImGui::GetStyle();
    style = ImGuiStyle();
    ImGui::StyleColorsDark();
    style.Colors[ImGuiCol_Text] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    style.FramePadding = {6, 4};
    style.WindowRounding = 0;
    style.WindowBorderSize = 0;
    style.ScrollbarRounding = 3;
    ImGuiIO &io = ImGui::GetIO();
    style.ScaleAllSizes(dpi);
    io.Fonts->Clear();
    // FONT_NORMAL
    io.FontDefault = loadFont(fonts::Roboto_Medium, fonts::Roboto_Medium_end, FONT_BASE * dpi, false);
    loadFont(fonts::fontawesome_webfont, fonts::fontawesome_webfont_end, FONT_BASE * dpi, true);
    // FONT_BIG
    loadFont(fonts::Roboto_Medium, fonts::Roboto_Medium_end, FONT_BASE * dpi * 2, false);
    loadFont(fonts::fontawesome_webfont, fonts::fontawesome_webfont_end, FONT_BASE * dpi * 2, true);
    dirtyStyle = false;
    ImGui_ImplOpenGL3_DestroyFontsTexture();
}

int Application::runLoop() {

    while (!exit && !glfwWindowShouldClose(mainWindow)) {
        glfwSetWindowTitle(mainWindow, (std::string(APPL_NAME) + (filename.empty() ? "" : " - " + filename)).c_str());

        // manage dpi change
        float currentDpi = getDpiForWindow(mainWindow);
        if (dpi != currentDpi) {
            sm::dpi = currentDpi;
            dpi = currentDpi;
            dirtyStyle = true;
        }
        if (dirtyStyle) applyTheme();

        // process events
        glfwPollEvents();
        glfwMakeContextCurrent(mainWindow);

        // Start the ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        int viewportWidth, viewportHeight;
        glfwGetFramebufferSize(mainWindow, &viewportWidth, &viewportHeight);

        // show frame
        projectWindow.resize(viewportWidth, viewportHeight);
        projectWindow.showFrame();

        // gui palette
        //ImGui::ShowDemoWindow();

        // Rendering
        ImGui::Render();

        glViewport(0, 0, viewportWidth, viewportHeight);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        showMaximized = glfwGetWindowAttrib(mainWindow, GLFW_MAXIMIZED) != 0;
        if (!showMaximized) {
            windowWidth = viewportWidth;
            windowHeight = viewportHeight;
        }

        glfwSwapBuffers(mainWindow);
        //usleep(500 * 1000);

        // run commands
        for (auto &cmd : commands) {
            cmd.fn();
        }
        commands.resize(0);

        hotKey.update();
        player.update(proj->canvas);
    }

    save(autoSavePath);

    return 0;
}

void Application::cleanUp() {
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();

    if (ctx) ImGui::DestroyContext(ctx);
    ctx = nullptr;

    if (mainWindow) glfwDestroyWindow(mainWindow);
    mainWindow = nullptr;

    glfwTerminate();
}

void Application::open(std::shared_ptr<project::Project> _proj) {
    close();
    proj = std::move(_proj);
    projectWindow.open(proj);
}

void Application::close() {
    if (proj) {
        // some close behavior, for example save confirm
        proj.reset();
    }
    projectWindow.close();
    filename = "";
}

ImFont *Application::loadFont(const char *start, const char *end, float size, bool fontAwesome) const {
    ImFontConfig config;
    config.FontDataOwnedByAtlas = false;
    const ImWchar *ranges = nullptr;
    if (fontAwesome) {
        config.MergeMode = true;
        config.GlyphMinAdvanceX = size;
        static const ImWchar icon_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
        ranges = icon_ranges;
    }
    return ImGui::GetIO().Fonts->AddFontFromMemoryTTF(
            (void *) start,
            (int) (end - start),
            size, &config, ranges);
}

void Application::beginCommand(const std::string &name, bool mergeable) {
}

void Application::endCommand() {
}

void Application::stopMerging() {
}

void Application::asyncCommand(const std::string &name, bool mergeable, const std::function<void()> &fn) {
    commands.push_back(AppCommand{name, mergeable, fn});
}

void Application::setAppHome(std::string path) {
    removeExtension(path);

    home = std::move(path);
    iniPath = home + ".ini";
    autoSavePath = home + "-autosave.clsproj";
}

bool Application::save(std::string filename) {
    if (proj) {
        std::string data = serializeObject(proj);
        std::ofstream file(filename);
        file << data;
        exportIno(filename);
        return true;
    }
    return false;
}

bool Application::load(std::string filename) {
    std::ifstream file(filename);
    if (!file.good()) return false;
    std::string data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    std::shared_ptr<project::Project> proj = deserializeObject<project::Project>(data);
    open(proj);
    this->filename = filename;
    return true;
}

std::string Application::getPath(const std::string &pathes, bool isSave) {
    std::string path;
    nfdchar_t *outPath = nullptr;
    nfdresult_t result;
    if (isSave) {
        result = NFD_SaveDialog(pathes.c_str(), gApp->lastDirectory.c_str(), &outPath);
    } else {
        result = NFD_OpenDialog(pathes.c_str(), gApp->lastDirectory.c_str(), &outPath);
    }
    if (result == NFD_OKAY) {
        path = outPath;
        free(outPath);
    }
    return path;
}

void Application::saveLastDirectory(std::string path) {
    // remove last /
    int lastSlash = -1;
    for (int i = 0; path[i]; i++) if (path[i] == '/' || path[i] == '\\') lastSlash = i;
    if (lastSlash >= 0) {
        path[lastSlash] = '\0';
        lastDirectory = path;
    }
}

void Application::quit() {
    glfwSetWindowShouldClose(mainWindow, true);
}

void Application::exportIno(std::string filename) {
    std::set<project::arduino_number> tabooList;
    for(auto &layer : proj->canvas.groups) {
        auto it = tabooList.find(layer->number);
        if (it == tabooList.end()) {
            tabooList.insert(layer->number);
            exportChunk(filename, layer->number);
        }
    }
}

void Application::exportChunk(std::string filename, project::arduino_number number) const {
    removeExtension(filename);
    filename += "_" + std::to_string(number) + ".h";
    std::ofstream outputFile(filename);

    outputFile << std::string("// Auto-generated with ") + APPL_NAME + "\n";
    outputFile << "\n";
    outputFile << "#ifndef __CLSM_H\n";
    outputFile << "#define __CLSM_H\n\n";

    int count = 0;
    for(auto &layer : proj->canvas.groups) {
        if (layer->number == number) {
            count++;
        }
    }

    outputFile << "const unsigned long LIGHT_COUNT = " << count << ";\n\n";

    size_t maxLetters = 0;
    for (auto &layer : proj->canvas.groups) {
        if (layer->number != number) continue;
        maxLetters = std::max(maxLetters, layer->identifier.size());
    }
    size_t i = 0;
    for (auto &layer : proj->canvas.groups) {
        if (layer->number != number) continue;
        if (!layer->identifier.empty()) {
            size_t letterCount = maxLetters - layer->identifier.size();
            outputFile << "#define LAYER_" << layer->identifier << " ";
            for (size_t j = 0; j < letterCount; j++) {
                outputFile << " ";
            }
            outputFile << std::to_string(i) << "\n";
        }
        i++;
    }

    outputFile << R"(
namespace CLS {

// 32 bit unsigned
typedef unsigned long time_unit;

enum FADE_TYPE {
    LINEAR,
    EXPONENTIAL,
    SIN,
    SIN_DOUBLE,
};

struct FadeParams {
    FADE_TYPE type;
    float exponent;
};

struct Key {
    // key
    time_unit start;
    time_unit end;
    time_unit duration;

    // fade start
    FadeParams fade1;
    time_unit fade1_end;
    float fade1_duration;

    // fade end
    FadeParams fade2;
    time_unit fade2_start;
    float fade2_duration;
};

struct LightGroup {
    unsigned int keysCount;
    const Key *keys;
};

)";

    i = 0;
    for (auto &g : proj->canvas.groups) {
        if (g->number != number) continue;
        outputFile << "// " << g->name << "\n";
        outputFile << "const PROGMEM Key __keys__" << i << "[" << g->keys.size() << "] = {\n";
        for (auto &k : g->keys) {
            outputFile << "    {\n";
            outputFile << "        " << k->start << ",\n";
            outputFile << "        " << k->start + k->duration << ",\n";
            outputFile << "        " << k->duration << ",\n";
            outputFile << "        {(FADE_TYPE) " << k->fadeStart.type << ", " << k->fadeStart.exponent << "},\n";
            outputFile << "        " << k->start + k->fadeStart.duration << ",\n";
            outputFile << "        " << k->fadeStart.duration << ",\n";
            outputFile << "        {(FADE_TYPE) " << k->fadeEnd.type << ", " << k->fadeEnd.exponent << "},\n";
            outputFile << "        " << k->start + k->duration - k->fadeEnd.duration << ",\n";
            outputFile << "        " << k->fadeEnd.duration << ",\n";
            outputFile << "    },\n";
        }
        outputFile << "};\n\n";
        i++;
    }

    i = 0;
    outputFile << "const LightGroup groups[LIGHT_COUNT] = {\n";
    for (auto &g : proj->canvas.groups) {
        if (g->number != number) continue;
        outputFile << "    {" << g->keys.size() << ", __keys__" << i << "}, // " << g->name << "\n";
        i++;
    }
    outputFile << "};";

    outputFile << R"(

float computeEasing(FadeParams &params, float alpha) {
    switch (params.type) {
        case LINEAR:
            return alpha;
        case EXPONENTIAL:
            return pow(alpha, params.exponent);
        case SIN:
            return (float) sin(alpha * M_PI_2);
        case SIN_DOUBLE:
            return (float) ((1 + sin((alpha - 0.5) * M_PI)) / 2);
        default:
            return 1;
    }
}

float computeEasing(Key &key, time_unit pos) {
    if (pos < key.start || pos > key.end) return 0;
    time_unit diff1 = pos - key.fade1_end;
    if (pos < key.fade1_end) {
        float alpha = (pos - key.start) / key.fade1_duration;
        return computeEasing(key.fade1, alpha);
    }
    if (pos > key.fade2_start) {
        float alpha = (pos - key.fade2_start) / key.fade2_duration;
        return computeEasing(key.fade2, 1 - alpha);
    }
    return 1;
}

struct LightContext {
    Key current;
    int index = -1;
    long cycles;
    float alpha;
    bool on;
};

LightContext ctxs[LIGHT_COUNT];

void updateAlpha(time_unit position) {
    for (int i = 0; i < LIGHT_COUNT; i++) {
        const LightGroup &group = groups[i];
        LightContext &ctx = ctxs[i];
        ctx.alpha = 0;
        int &index = ctx.index;
        while (index < 0 || (index < group.keysCount && position > ctx.current.end)) {
            index++;
            const char *source = (char*) &group.keys[index];
            char *dest = (char*) &ctx.current;
            for (int j = 0; j < sizeof(Key); j++) {
                dest[j] = pgm_read_byte_near(&source[j]);
            }
        }
        if (index >= group.keysCount) continue;
        Key &key = ctx.current;
        ctx.alpha = computeEasing(key, position);
    }
}

)";

    outputFile << "} // namespace CLS \n\n";

    outputFile << "#endif //__CLSM_H\n";
}
