#ifndef TIMELINEEDITOR_H
#define TIMELINEEDITOR_H

#define IMGUI_DEFINE_MATH_OPERATORS

#include "Canvas.h"
#include <imgui.h>
#include <AudioLoader.h>

namespace sm {
class Application;
namespace editor {

class TimelineEditor {
    const char *POPUP_ADD_LAYER = "POPUP_ADD_LAYER";
    const char *MODAL_ADD_AUDIO = "Audio Track";
    const char *MODAL_ERROR = "Error!";

    const int POPUP_SETTINGS = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings;

    const int scrollBarHeight = 14;
    // multiplier of zoom and fontsize
    const float TIME_WIDTH = 1;

public:
    TimelineEditor(Application *app);
    void reset();
    void editorOf(project::Canvas canvas);

private:
    float zoom;
    float zoomTarget;
    media::AudioLoader loader;
    sm::Application *app;

    std::string lastError;

    ImU32 setAlpha(ImU32 color, double alpha);
    void addAudioModal();

    void saveLastDirectory(const char *outPath) const;

    void errorBox();
};

}
}

#endif //TIMELINEEDITOR_H