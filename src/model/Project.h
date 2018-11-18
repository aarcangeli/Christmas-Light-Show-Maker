#ifndef PROJECT_H
#define PROJECT_H

#include "vector"
#include "string"
#include "Canvas.h"
#include "Settings.h"

namespace sm {
namespace project {

/**
 * Main class, contains the entire project structure
 */
class Project {
public:
    Project();

    std::string name;
    Canvas canvas;
    Settings settings;

};

}
}

#endif //PROJECT_H
