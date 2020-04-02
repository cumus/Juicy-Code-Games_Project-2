#ifndef __EDGE_H__
#define __EDGE_H__

#include "Behaviour.h"
#include "Gameobject.h"
#include "Component.h"

//class Gameobject;

class Edge : public B_Building
{
public:
	Edge(Gameobject* go, ComponentType type = B_BUILDING) : B_Building(go, type) {}
	~Edge();
	void SetTexture() override;
	void CheckSprite() override;
	void BuildingAction() override;
};

#endif