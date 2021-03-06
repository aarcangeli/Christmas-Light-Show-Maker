#include "configLoader.h"
#include "map"
#include <imgui_internal.h>

namespace sm {
namespace loader {

std::map<ImGuiContext *, Application *> configs;

Application *getApplication(ImGuiContext *ctx) {
    auto it = configs.find(ctx);
    if (it == configs.end()) return nullptr;
    return it->second;
}

void saveFn(ImGuiContext *ctx, ImGuiSettingsHandler *handler, ImGuiTextBuffer *out_buf) {
    Application *app = getApplication(ctx);
    if (!app) return;

    out_buf->appendf("[%s][main]\n", handler->TypeName);
    out_buf->appendf("WindowWidth=%i\n", app->windowWidth);
    out_buf->appendf("WindowHeight=%i\n", app->windowHeight);
    out_buf->appendf("Maximized=%i\n", app->showMaximized);
    out_buf->appendf("LastDirectory=%s\n", app->lastDirectory.c_str());
    out_buf->appendf("LastDocument=%s\n", app->filename.c_str());
    out_buf->appendf("\n");
}

void *openFn(ImGuiContext *ctx, ImGuiSettingsHandler *handler, const char *name) {
    return getApplication(ctx);
}

void loadFn(ImGuiContext *ctx, ImGuiSettingsHandler *handler, void *entry, const char *line) {
    Application *app = (Application *) entry;
    int i;
    char c;
    if (sscanf(line, "WindowWidth=%i", &i) == 1) app->windowWidth = i;
    if (sscanf(line, "WindowHeight=%i", &i) == 1) app->windowHeight = i;
    if (sscanf(line, "Maximized=%i", &i) == 1) app->showMaximized = i != 0;
    if (sscanf(line, "LastDirectory=%c", &c) == 1) {
        app->lastDirectory = &line[14];
    }
}

void installConfigLoader(ImGuiContext *ctx, Application *app) {
    ImGuiSettingsHandler handler;
    handler.TypeName = "lsw";
    handler.TypeHash = ImHash(handler.TypeName, 0, 0);
    handler.WriteAllFn = &saveFn;
    handler.ReadOpenFn = &openFn;
    handler.ReadLineFn = &loadFn;
    ctx->SettingsHandlers.push_back(handler);
    configs[ctx] = app;
}

}
}
