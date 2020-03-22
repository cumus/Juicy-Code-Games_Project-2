#ifndef __HIERARCHY_WINDOW_H__
#define __HIERARCHY_WINDOW_H__

#include "EditorWindow.h"

class Gameobject;

class HierarchyWindow : public EditorWindow
{
public:

	HierarchyWindow(const RectF rect);
	~HierarchyWindow();

	bool Init() override;
	void RecieveEvent(const Event& e) override;

private:

	void _Update() override;

private:

	Gameobject* root = nullptr;

	std::vector<std::pair<float,Gameobject*>> gos;
};
#endif // __HIERARCHY_WINDOW_H__