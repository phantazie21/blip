#pragma once
#include "../scene/Scene.h"

enum class GizmoAxis { None, X, Y, Z };
enum class GizmoMode { Translate, Rotate, Scale };

class Editor {
public:
    void draw(Scene& scene);
    void handleMouseClick(Scene& scene, int mouseX, int mouseY, int screenW, int screenH, Object* cameraObj);
	void drawGizmo(Scene& scene, int screenW, int screenH, Object* cameraObj);
    void updateGizmoHover(Scene& scene, int mouseX, int mouseY, int screenW, int screenH, Object* cameraObj);
    GizmoAxis pickRotateGizmo(Scene& scene, int mouseX, int mouseY, int screenW, int screenH, Object* cameraObj);
    GizmoAxis pickGizmoAxis(Scene& scene, int mouseX, int mouseY, int screenW, int screenH, Object* cameraObj);
    void updateGizmoTranslateDrag(Scene& scene, int mouseX, int mouseY, int screenW, int screenH, Object* cameraObj);
    void updateGizmoRotateDrag(Scene& scene, int mouseX, int mouseY, int screenW, int screenH, Object* cameraObj);
    void updateGizmoScaleDrag(Scene& scene, int mouseX, int mouseY, int screenW, int screenH, Object* cameraObj);
	GizmoMode gizmoMode = GizmoMode::Translate;
	GizmoAxis activeGizmoAxis = GizmoAxis::None;
	GizmoAxis hoveredGizmoAxis = GizmoAxis::None;
	ImVec2    gizmoLastMousePos;
private:
    int selectedObjectIndex = -1;
    void drawSceneHierarchy(Scene& scene);
    void drawInspector(Scene& scene);
};
