#pragma once
#include "../scene/Scene.h"

class Editor {
public:
    void draw(Scene& scene);

private:
    int selectedObjectIndex = -1;
    void drawSceneHierarchy(Scene& scene);
    void drawInspector(Scene& scene);
};
