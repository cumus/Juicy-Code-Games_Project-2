#ifndef __BEHAVIOUR_H__
#define __BEHAVIOUR_H__

#include "Component.h"
#include "Point.h"
#include <vector>
#include <map>

enum UnitType
{
	EDGE,
	GATHERER,
	UNIT_MELEE,
	UNIT_RANGED,
	ENEMY_MELEE,
	ENEMY_RANGED,
	BASE_CENTER,
	TOWER,

	/*USER_GATHERER,
	USER_RANGED,
	USER_SUPER,
	USER_SPECIAL,
	IA_MELEE,
	IA_RANGED,
	IA_SUPER,
	IA_SPECIAL,

	//////Structures//////
	TOWN_HALL,
	LAB,
	BARRACKS,
	RANGED_TURRET,
	NEAR_TURRET,
	RESOURCE,
	IA_SPAWN*/
};

enum UnitState
{
	// Unit
	IDLE = 0,
	MOVING_N,
	MOVING_S,
	MOVING_W,
	MOVING_E,
	MOVING_NE,
	MOVING_NW,
	MOVING_SE,
	MOVING_SW,
	ATTACKING_N,
	ATTACKING_S,
	ATTACKING_W,
	ATTACKING_E,
	ATTACKING_NE,
	ATTACKING_NW,
	ATTACKING_SE,
	ATTACKING_SW,
	DEAD,

	// Building
	BUILDING,
	FULL_LIFE,
	HALF_LIFE,
	DESTROYED
};


class Sprite;
class AudioSource;

class Behaviour : public Component 
{
public:
	Behaviour(Gameobject* go, UnitType type, UnitState starting_state, ComponentType comp_type = BEHAVIOUR);
	virtual ~Behaviour();

	void RecieveEvent(const Event& e) override;

	virtual void Selected();
	virtual void UnSelected();
	virtual void OnRightClick(float x, float y) {}
	virtual void OnDamage(int damage);
	virtual void OnKill();
	virtual void DoAttack(vec pos) {}
	virtual void OnDestroy(){}

	UnitType GetType() const { return type; }
	UnitState* GetStatePtr() { return &current_state; }

	//void QuickSort();
	unsigned int GetBehavioursInRange(vec pos, float dist, std::map<float, Behaviour*>& res) const;

protected:

	static std::map<double, Behaviour*> b_map;

	UnitType type;
	UnitState current_state;

	// Stats
	int max_life, current_life, damage;
	float attack_range, vision_range;

	// Complementary components
	AudioSource* audio;
	Sprite* selection_highlight;
};

class B_Unit : public Behaviour
{
public:

	B_Unit(Gameobject* go, UnitType type, UnitState starting_state, ComponentType comp_type = B_UNIT);

	void Update() override;
	void OnRightClick(float x, float y) override;
	void DoAttack(vec pos) override;
	void OnDestroy() override;
	
protected:

	float speed;
	float aux_speed;
	float attackRange;
	int damage;
	std::vector<iPoint>* path;
	iPoint nextTile;
	bool next;
	bool move;
	bool positiveX;
	bool positiveY;
	int dirX;
	int dirY;
	bool cornerNW;
	bool cornerNE;
	bool cornerSW;
	bool cornerSE;
	double objectiveID;
};

#endif // __BEHAVIOUR_H_