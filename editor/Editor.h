#pragma once
#include "../scene/Scene.h"

enum class GizmoAxis { None, X, Y, Z };

class Editor {
public:
    void draw(Scene& scene);
    void handleMouseClick(Scene& scene, int mouseX, int mouseY, int screenW, int screenH, Object* cameraObj);
	void drawGizmo(Scene& scene, int screenW, int screenH, Object* cameraObj);
    GizmoAxis pickGizmoAxis(Scene& scene, int mouseX, int mouseY, int screenW, int screenH, Object* cameraObj);
    void updateGizmoDrag(Scene& scene, int mouseX, int mouseY, int screenW, int screenH, Object* cameraObj);
	GizmoAxis activeGizmoAxis = GizmoAxis::None;
	ImVec2    gizmoLastMousePos;
private:
    int selectedObjectIndex = -1;
    void drawSceneHierarchy(Scene& scene);
    void drawInspector(Scene& scene);
};
