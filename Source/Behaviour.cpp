#include "Behaviour.h"
#include "Application.h"
#include "TimeManager.h"
#include "PathfindingManager.h"
#include "Gameobject.h"
#include "Transform.h"
#include "Sprite.h"
#include "AudioSource.h"
#include "Log.h"
#include "Vector3.h"
#include "Canvas.h"
#include "Scene.h"
#include "Audio.h"
#include "BaseCenter.h"
#include "FogOfWarManager.h"
#include "JuicyMath.h"
#include "Input.h"
#include "ParticleSystem.h"

#include <vector>

std::map<double, Behaviour*> Behaviour::b_map;

Behaviour::Behaviour(Gameobject* go, UnitType t, UnitState starting_state, ComponentType comp_type) :
	Component(comp_type, go),
	type(t),
	current_state(starting_state),
	spriteState(starting_state)
{
	current_life = max_life = damage = 10;
	dieDelay = 2.0f;
	deathFX = B_DESTROYED; 
	rayCastTimer = 0;
	shoot = false;
	selectionPanel = nullptr;
	drawRanges = false;
	objective = nullptr;
	providesVisibility = true;
	visible = true;
	baseCollOffset = { 0,0 };
	visionCollOffset = { 0,0 };
	attackCollOffset = { 0,0 };
	selectionRect = {0,0,64,64};
	game_object->SetStatic(true);
	active = false;
	buildProgress = 0;

	audio = new AudioSource(game_object);
	characteR = new AnimatedSprite(this);

	selection_highlight = new Sprite(go, App->tex.Load("textures/selectionMark.png"), { 0, 0, 64, 64 }, BACK_SCENE, { 0, -50, 1.f, 1.f });
	selection_highlight->SetInactive();

	mini_life_bar.Create(go);
	mini_life_bar.Hide();

	b_map.insert({ GetID(), this });
}

Behaviour::~Behaviour()
{
	b_map.erase(GetID());
}

bool Behaviour::IsDestroyed()
{
	if (current_state == DESTROYED) return true;
	else return false;
}

void Behaviour::SetColliders()
{
	//Colliders
	pos = game_object->GetTransform()->GetGlobalPosition();
	vec s = game_object->GetTransform()->GetLocalScale();
	switch(type)
	{
		case GATHERER:
		case UNIT_MELEE:
		case UNIT_RANGED:
		case UNIT_SUPER:
		{
			bodyColl = new Collider(game_object, { pos.x,pos.y,game_object->GetTransform()->GetLocalScaleX(),game_object->GetTransform()->GetLocalScaleY() }, NON_TRIGGER, PLAYER_TAG, { 0,Map::GetBaseOffset(),0,0 }, BODY_COLL_LAYER);
			attackColl = new Collider(game_object, { pos.x,pos.y,attack_range,attack_range }, TRIGGER, PLAYER_ATTACK_TAG, { 0,Map::GetBaseOffset(),0,0 }, ATTACK_COLL_LAYER);
			std::pair<float, float> world = Map::F_MapToWorld(pos.x, pos.y);
			selectionOffset = { 10,-30 };
			selectionRect = { world.first + selectionOffset.first,world.second + selectionOffset.second,40,70 };
			break;
		}
		case ENEMY_MELEE:
		case ENEMY_RANGED:
		case ENEMY_SUPER:
		{
			bodyColl = new Collider(game_object, { pos.x,pos.y,game_object->GetTransform()->GetLocalScaleX(),game_object->GetTransform()->GetLocalScaleY() }, NON_TRIGGER, ENEMY_TAG, { 0,Map::GetBaseOffset(),0,0 }, BODY_COLL_LAYER);
			visionColl = new Collider(game_object, { pos.x,pos.y,vision_range,vision_range }, TRIGGER, ENEMY_VISION_TAG, { 0,Map::GetBaseOffset(),0,0 }, VISION_COLL_LAYER);
			attackColl = new Collider(game_object, { pos.x,pos.y,attack_range,attack_range }, TRIGGER, ENEMY_ATTACK_TAG, { 0,Map::GetBaseOffset(),0,0 }, ATTACK_COLL_LAYER);
			std::pair<float, float> world = Map::F_MapToWorld(pos.x, pos.y);
			selectionOffset = { 10,-30 };
			selectionRect = { world.first + selectionOffset.first,world.second + selectionOffset.second,40,70 };
			break;
		}
		case BASE_CENTER:
		{
			bodyColl = new Collider(game_object, { pos.x,pos.y,game_object->GetTransform()->GetLocalScaleX(),game_object->GetTransform()->GetLocalScaleY() }, TRIGGER, PLAYER_TAG, { 90,Map::GetBaseOffset() + 65,0,0 }, BODY_COLL_LAYER);
			std::pair<float, float> world = Map::F_MapToWorld(pos.x, pos.y);
			selectionOffset = { 30,-100};
			selectionRect = { world.first + selectionOffset.first,world.second + selectionOffset.second,185,250 };
			break;
		}
		case TOWER:
		{
			bodyColl = new Collider(game_object, { pos.x,pos.y,game_object->GetTransform()->GetLocalScaleX() * 1.5f,game_object->GetTransform()->GetLocalScaleY() * 1.5f }, TRIGGER, PLAYER_TAG, { 0,Map::GetBaseOffset()+10,0,0 }, BODY_COLL_LAYER);
			attackColl = new Collider(game_object, { pos.x,pos.y,attack_range,attack_range }, TRIGGER, PLAYER_ATTACK_TAG, { 0,Map::GetBaseOffset(),0,0 }, ATTACK_COLL_LAYER);
			std::pair<float, float> world = Map::F_MapToWorld(pos.x, pos.y);
			selectionOffset = { 5,-105 };
			selectionRect = { world.first + selectionOffset.first,world.second + selectionOffset.second,55,160 };
			selection_highlight->offset = { 0, -30, 1.f, 1.f };
			break;
		}
		case BARRACKS:
		{
			bodyColl = new Collider(game_object, { pos.x,pos.y,game_object->GetTransform()->GetLocalScaleX(),game_object->GetTransform()->GetLocalScaleY() }, TRIGGER, PLAYER_TAG, { 150,Map::GetBaseOffset()+95,0,0 }, BODY_COLL_LAYER);
			std::pair<float, float> world = Map::F_MapToWorld(pos.x, pos.y);
			selectionOffset = { 30,-50 };
			selectionRect = { world.first + selectionOffset.first,world.second + selectionOffset.second,300,250 };
			selection_highlight->offset = { 0, -50, 1.f, 1.f };
			break;
		}
		case LAB:
		{
			bodyColl = new Collider(game_object, { pos.x,pos.y,game_object->GetTransform()->GetLocalScaleX(),game_object->GetTransform()->GetLocalScaleY() }, TRIGGER, PLAYER_TAG, { 90,Map::GetBaseOffset() + 65,0,0 }, BODY_COLL_LAYER);
			std::pair<float, float> world = Map::F_MapToWorld(pos.x, pos.y);
			selectionOffset = { 25,-100 };
			selectionRect = { world.first + selectionOffset.first,world.second + selectionOffset.second,215,235 };
			break;
		}
		case EDGE:
		{
			bodyColl = new Collider(game_object, { pos.x,pos.y,game_object->GetTransform()->GetLocalScaleX() * 2.0f,game_object->GetTransform()->GetLocalScaleY() * 2.0f }, TRIGGER, ENEMY_TAG, { 0,Map::GetBaseOffset(),0,0 }, BODY_COLL_LAYER);
			std::pair<float, float> world = Map::F_MapToWorld(pos.x, pos.y);
			selectionOffset = { 0,20 };
			selectionRect = { world.first + selectionOffset.first,world.second + selectionOffset.second,65,35 };
			break;
		}
		case CAPSULE:
		{
			bodyColl = new Collider(game_object, { pos.x,pos.y,game_object->GetTransform()->GetLocalScaleX(),game_object->GetTransform()->GetLocalScaleY() }, TRIGGER, ENEMY_TAG, { 0,Map::GetBaseOffset(),0,0 }, BODY_COLL_LAYER);
			std::pair<float, float> world = Map::F_MapToWorld(pos.x, pos.y);
			selectionOffset = { 0,-130 };
			selectionRect = { world.first + selectionOffset.first,world.second + selectionOffset.second,55,175 };
			break;
		}
		case SPAWNER:
		{			
			bodyColl = new Collider(game_object, { pos.x,pos.y,game_object->GetTransform()->GetLocalScaleX()*3,game_object->GetTransform()->GetLocalScaleY()*3 }, TRIGGER, ENEMY_TAG, { 0,Map::GetBaseOffset(),0,0 }, BODY_COLL_LAYER);
			std::pair<float, float> world = Map::F_MapToWorld(pos.x, pos.y);
			selectionOffset = { -70,-70.0f };
			selectionRect = { world.first + selectionOffset.first,world.second + selectionOffset.second,200,150 };
			break;
		}
	}
}

vec Behaviour::GetPos() { return pos; }

RectF Behaviour::GetSelectionRect() { return selectionRect; }

Collider* Behaviour::GetSelectionCollider()
{
	return selColl;
}

void Behaviour::RecieveEvent(const Event& e)
{
	switch (e.type)
	{
	case ON_PLAY: break;
	case ON_PAUSE: break;
	case ON_STOP: break;
	case ON_SELECT: Selected(); break;
	case ON_UNSELECT: UnSelected(); break;
	case ON_DESTROY: OnDestroy(); break;
	case ON_RIGHT_CLICK: OnRightClick(e.data1.AsVec(), e.data2.AsVec()); break;
	case DAMAGE: OnDamage(e.data1.AsInt(), UnitType(e.data2.AsInt())); break;
	case BUILD_GATHERER:
	{
		AddUnitToQueue(GATHERER, e.data1.AsVec(), e.data2.AsFloat());
		break;
	}
	case BUILD_MELEE:
	{
		AddUnitToQueue(UNIT_MELEE, e.data1.AsVec(), e.data2.AsFloat());
		break;
	}
	case BUILD_RANGED:
	{
		AddUnitToQueue(UNIT_RANGED, e.data1.AsVec(), e.data2.AsFloat());
		break;
	}
	case BUILD_SUPER:
	{
		AddUnitToQueue(UNIT_SUPER, e.data1.AsVec(), e.data2.AsFloat());
		break;
	}
	case BUILD_CAPSULE:
	{
		App->scene->SpawnBehaviour(CAPSULE, { 153, 136});
		break;
	}
	case DO_UPGRADE: Upgrade(); break;
	case UPDATE_PATH: UpdatePath(e.data1.AsInt(), e.data2.AsInt()); break;
	case DRAW_RANGE: drawRanges = !drawRanges; break;
	case SHOW_SPRITE: ActivateSprites(); break;
	case HIDE_SPRITE: DesactivateSprites(); break;
	case CHECK_FOW: CheckFoWMap(e.data1.AsBool()); break;
	case ON_COLLISION:
		OnCollision(*Component::ComponentsList[e.data1.AsDouble()]->AsCollider(), *Component::ComponentsList[e.data2.AsDouble()]->AsCollider());
		break;
	case REPATH: Repath(); break;
	}
}

void Behaviour::PreUpdate()
{
	pos = game_object->GetTransform()->GetGlobalPosition();
	if(providesVisibility) GetTilesInsideRadius(); 
	else CheckFoWMap();
}

void Behaviour::ActivateSprites()
{
	characteR->SetActive();
}

void Behaviour::DesactivateSprites()
{
	characteR->SetInactive();
}

Collider* Behaviour::GetBodyCollider() 
{ 
	return bodyColl;
}

void Behaviour::CheckFoWMap(bool debug)
{
	visible = FogOfWarManager::fogMap[int(pos.x)][int(pos.y)];
	if (!visible && !debug) { DesactivateSprites();}
	else{ ActivateSprites(); }
}

void Behaviour::GetTilesInsideRadius()
{
	vec s = game_object->GetTransform()->GetLocalScale();
	if (type == BARRACKS)
	{
		iPoint startingPos(int(pos.x + 4 + s.x * 0.5 - vision_range), int(pos.y - s.y * 0.5 - vision_range));
		iPoint finishingPos(int(startingPos.x + (vision_range * 2)), int(startingPos.y + (vision_range * 2)));

		for (int x = startingPos.x; x < finishingPos.x; x++)
		{
			for (int y = startingPos.y; y < finishingPos.y; y++)
			{
				iPoint tilePos(x, y);
				if (tilePos.DistanceTo(iPoint(int(pos.x + 5), int(pos.y))) <= vision_range * 0.6)
				{
					if (App->fogWar.CheckFoWTileBoundaries(iPoint(tilePos.x, tilePos.y)))FogOfWarManager::fogMap[tilePos.x][tilePos.y] = true;
				}
			}
		}
	}
	else if(type == BASE_CENTER)
	{

		iPoint startingPos(int(pos.x+3 + s.x * 0.5 - vision_range), int(pos.y - s.y * 0.5 - vision_range));
		iPoint finishingPos(int(startingPos.x + (vision_range * 2)), int(startingPos.y + (vision_range * 2)));

		for (int x = startingPos.x; x < finishingPos.x; x++)
		{
			for (int y = startingPos.y; y < finishingPos.y; y++)
			{
				iPoint tilePos(x, y);
				if (tilePos.DistanceTo(iPoint(int(pos.x+3), int(pos.y))) <= vision_range * 0.6)
				{
					if (App->fogWar.CheckFoWTileBoundaries(iPoint(tilePos.x, tilePos.y)))FogOfWarManager::fogMap[tilePos.x][tilePos.y] = true;
				}
			}
		}
	}
	else if (type == LAB)
	{

		iPoint startingPos(int(pos.x + 3 + s.x * 0.5 - vision_range), int(pos.y - s.y * 0.5 - vision_range));
		iPoint finishingPos(int(startingPos.x + (vision_range * 2)), int(startingPos.y + (vision_range * 2)));

		for (int x = startingPos.x; x < finishingPos.x; x++)
		{
			for (int y = startingPos.y; y < finishingPos.y; y++)
			{
				iPoint tilePos(x, y);
				if (tilePos.DistanceTo(iPoint(int(pos.x + 3), int(pos.y))) <= vision_range * 0.6)
				{
					if (App->fogWar.CheckFoWTileBoundaries(iPoint(tilePos.x, tilePos.y)))FogOfWarManager::fogMap[tilePos.x][tilePos.y] = true;
				}
			}
		}
	}
	else
	{
		iPoint startingPos(int(pos.x + s.x * 0.5 - vision_range), int(pos.y - s.y * 0.5 - vision_range));
		iPoint finishingPos(int(startingPos.x + (vision_range * 2)), int(startingPos.y + (vision_range * 2)));

		for (int x = startingPos.x; x < finishingPos.x; x++)
		{
			for (int y = startingPos.y; y < finishingPos.y; y++)
			{
				iPoint tilePos(x, y);
				if (tilePos.DistanceTo(iPoint(int(pos.x), int(pos.y))) <= vision_range * 0.6)
				{
					if (App->fogWar.CheckFoWTileBoundaries(iPoint(tilePos.x, tilePos.y)))FogOfWarManager::fogMap[tilePos.x][tilePos.y] = true;
				}
			}
		}
	}
}


void Behaviour::Load(pugi::xml_node& node)
{
	current_life = node.attribute("current_life").as_int();
	active = true;
}

void Behaviour::Save(pugi::xml_node& node) const
{
	node.append_attribute("current_life").set_value(current_life);
	node.append_attribute("type").set_value((int)type);
}

void Behaviour::Selected()
{
	if (active)
	{
		// Selection mark
		selection_highlight->SetActive();

		// Audio Fx
		audio->Play(SELECT);

		if (bar_go != nullptr) bar_go->SetActive();
		if (creation_bar_go != nullptr) creation_bar_go->SetActive();
		if (selectionPanel != nullptr) selectionPanel->SetActive();
		if (unitInfo != nullptr) unitInfo->SetActive();
	}
}

void Behaviour::UnSelected()
{
	// Selection mark
	selection_highlight->SetInactive();

	//if (bar_go != nullptr) bar_go->SetInactive();
	if (creation_bar_go != nullptr) creation_bar_go->SetInactive();
	if (selectionPanel != nullptr) selectionPanel->SetInactive();
	if (unitInfo != nullptr) unitInfo->SetInactive();
}


void Behaviour::OnDamage(int d, UnitType from)
{	
	if (current_state != DESTROYED && Scene::DamageAllowed())
	{
		if (current_life > 0)
		{
			current_life -= d;

			// Lifebar
			mini_life_bar.Show();
			mini_life_bar.Update(float(current_life) / float(max_life), current_lvl);

			if (current_life <= 0)
			{
				update_health_ui();
				OnKill(type);
			}
			else
			{
				mini_life_bar.Update(float(current_life) / float(max_life), current_lvl);
				update_health_ui();
				AfterDamageAction(from);
			}

			if (!active) buildProgress = current_life;

			if (selection_highlight != nullptr && selection_highlight->IsActive())
			{

				if (unitLife != nullptr)
				{
					std::stringstream ssLife;
					ssLife << "Life: ";
					ssLife << current_life;
					unitLife->text->SetText(ssLife.str().c_str());
				}
			}
		}
	}
}

void Behaviour::OnKill(const UnitType type)
{
	current_life = 0;
	current_state = DESTROYED;
	spriteState = DESTROYED;

	// Lifebar
	OnDestroy();
	mini_life_bar.Hide();

	audio->Play(deathFX);
	game_object->Destroy(dieDelay);
	if (bar_go)
		bar_go->Destroy(dieDelay);

	switch (type)
	{
	case UNIT_MELEE:
	{
		Event::Push(UPDATE_STAT, App->scene, CURRENT_MELEE_UNITS, -1);
		Event::Push(UPDATE_STAT, App->scene, UNITS_LOST, 1);
		break;
	}
	case UNIT_RANGED:
	{
		Event::Push(UPDATE_STAT, App->scene, CURRENT_RANGED_UNITS, -1);
		Event::Push(UPDATE_STAT, App->scene, UNITS_LOST, 1);
		break;
	}
	case UNIT_SUPER:
	{
		Event::Push(UPDATE_STAT, App->scene, CUERRENT_SUPER_UNITS, -1);
		Event::Push(UPDATE_STAT, App->scene, UNITS_LOST, 1);
		break;
	}
	case GATHERER:
	{
		Event::Push(UPDATE_STAT, App->scene, CURRENT_GATHERER_UNITS, -1);
		Event::Push(UPDATE_STAT, App->scene, UNITS_LOST, 1);
		break;
	}
	case ENEMY_MELEE:
	{
		Event::Push(UPDATE_STAT, App->scene, CURRENT_MOB_DROP, 5);
		Event::Push(UPDATE_STAT, App->scene, MOB_DROP_COLLECTED, 5);
		Event::Push(UPDATE_STAT, App->scene, UNITS_KILLED, 1);
		if (App->scene->GetStat(UNITS_KILLED) % 10 == 0) {
			Event::Push(UPDATE_STAT, App->scene, CURRENT_GOLD, 1);
			Event::Push(UPDATE_STAT, App->scene, GOLD_COLLECTED, 1);
		}
		break;
	}
	case ENEMY_RANGED:
	{
		Event::Push(UPDATE_STAT, App->scene, CURRENT_MOB_DROP, 10);
		Event::Push(UPDATE_STAT, App->scene, MOB_DROP_COLLECTED, 10);
		Event::Push(UPDATE_STAT, App->scene, UNITS_KILLED, 1);
		if (App->scene->GetStat(UNITS_KILLED) % 10 == 0) {
			Event::Push(UPDATE_STAT, App->scene, CURRENT_GOLD, 1);
			Event::Push(UPDATE_STAT, App->scene, GOLD_COLLECTED, 1);
		}
		break;
	}
	case ENEMY_SUPER:
	{
		Event::Push(UPDATE_STAT, App->scene, CURRENT_MOB_DROP, 20);
		Event::Push(UPDATE_STAT, App->scene, MOB_DROP_COLLECTED, 20);
		Event::Push(UPDATE_STAT, App->scene, UNITS_KILLED, 1);
		if (App->scene->GetStat(UNITS_KILLED) % 10 == 0) {
			Event::Push(UPDATE_STAT, App->scene, CURRENT_GOLD, 1);
			Event::Push(UPDATE_STAT, App->scene, GOLD_COLLECTED, 1);
		}
		break;
	}
	case BASE_CENTER:
	{
		Event::Push(GAMEPLAY, App->scene, LOSE_BUTTON);
		break;
	}
	case SPAWNER:
	{
		Event::Push(UPDATE_STAT, App->scene, CURRENT_SPAWNERS, -1);
		break;
	}
	case EDGE:
	{
		break;
	}
	case CAPSULE:
	{
		break;
	}
	case BARRACKS:
	{
		break;
	}
	case TOWER:
	{
		break;
	}
	}
	if (!tilesVisited.empty())
	{
		for (std::vector<iPoint>::const_iterator it = tilesVisited.cbegin(); it != tilesVisited.cend(); ++it)
		{
			if (PathfindingManager::unitWalkability[it->x][it->y] != 0)
			{
				PathfindingManager::unitWalkability[it->x][it->y] = 0;
			}
		}
	}
	FreeWalkabilityTiles();
	b_map.erase(GetID());
}

unsigned int Behaviour::GetBehavioursInRange(vec pos, float dist, std::map<float, Behaviour*>& res) const
{
	unsigned int ret = 0;

	for (std::map<double, Behaviour*>::iterator it = b_map.begin(); it != b_map.end(); ++it)
	{
		if (it->first != GetID())
		{
			Transform* t = it->second->game_object->GetTransform();
			if (t)
			{
				float d = t->DistanceTo(pos);
				if (d < dist)
				{
					ret++;
					res.insert({ d, it->second });
				}
			}
		}
	}

	return ret;
}

///////////////////////////
// UNIT BEHAVIOUR
///////////////////////////


B_Unit::B_Unit(Gameobject* go, UnitType t, UnitState s, ComponentType comp_type) :
	Behaviour(go, t, s, comp_type)
{
	//Depending on unit
	atkTime = 1.0;
	
	speed = 5;//MAX SPEED 60
	attack_range = 3.0f;
	vision_range = 10.0f;
	damage = 5;
	deathFX = B_DESTROYED;
	attackFX = SELECT;
	vision_range = 5.0f;

	path = nullptr;
	next = false;
	move = false;
	nextTile.x = 0;
	nextTile.y = 0;
	positiveX = false;
	positiveY = false;
	dirX = 0;
	dirY = 0;
	atkTimer = 0.0f;
	arriveDestination = false;
	goingBase = false;
	new_state = IDLE;
	spriteState = IDLE;
	drawRanges = false;
	gotTile = false;
	game_object->SetStatic(false);
	calculating_path = false;
	atkObj = nullptr;
	chaseObj = nullptr;
	chasing = false;
	moveOrder = false;
	unitLevel = 0;
	active = true;
	foundPoint = false;
	nonMovingCounter = 0.0f;
}

void B_Unit::Update()
{	
	if (providesVisibility) GetTilesInsideRadius();
	else CheckFoWMap();

	if (current_state != DESTROYED)
	{
		if (moveOrder && !chasing) atkObj = nullptr;
		if (atkObj != nullptr) //ATTACK
		{
			if (type == ENEMY_MELEE || type == ENEMY_RANGED || type == ENEMY_SUPER)//IA
			{
				if (atkTimer > atkTime)
				{
					if (!atkObj->IsDestroyed()) //Attack
					{
						DoAttack();
						Event::Push(DAMAGE, atkObj, damage,GetType());
					}
					else atkObj = nullptr;
					atkTimer = 0;
				}
			}
			else//PLAYER
			{
				if (atkTimer > atkTime)
				{
					if (!atkObj->IsDestroyed()) //ATTACK
					{
						DoAttack();						
						Event::Push(DAMAGE, atkObj, damage,GetType());
					}
					else atkObj = nullptr;
					atkTimer = 0;
				}				
			}
		}
		else//CHECK IF MOVES
		{
			if (type == ENEMY_MELEE || type == ENEMY_RANGED || type == ENEMY_SUPER) //IA
			{
				if (chaseObj != nullptr)//CHASE
				{
					if (!chasing)
					{
						vec pos = chaseObj->GetPos();
						Event::Push(UPDATE_PATH, this->AsBehaviour(), int(pos.x), int(pos.y));
						chasing = true;
						goingBase = false;
					}					
				}
				else
				{
					if (Base_Center::baseCenter != nullptr && !goingBase && !chasing)//GO TO BASE
					{
						goingBase = true;
						vec centerPos = Base_Center::baseCenter->GetTransform()->GetGlobalPosition();
						Event::Push(UPDATE_PATH, this->AsBehaviour(), int(centerPos.x+2), int(centerPos.y+2));
					}
				}
			}

			if (moveOrder && (chaseObj != nullptr && !chaseObj->IsDestroyed() && !chasing))
			{
				vec goToPos = chaseObj->GetPos();
				Event::Push(UPDATE_PATH, this->AsBehaviour(), int(goToPos.x), int(goToPos.y));
				chasing = true;
			}

			if (path != nullptr && !path->empty())
				CheckPathTiles();

			if (move && PathfindingManager::unitWalkability[nextTile.x][nextTile.y] == GetID())
			{
				calculating_path = false;
				nonMovingCounter = 0.0f;
				//LOG("move");
				fPoint actualPos = { pos.x, pos.y };

				iPoint tilePos = { int(pos.x), int(pos.y) };
				if (nextTile.x > tilePos.x)
					dirX = 1;
				else if (nextTile.x < tilePos.x)
					dirX = -1;
				else
					dirX = 0;

				if (nextTile.y > tilePos.y)
					dirY = 1;
				else if (nextTile.y < tilePos.y)
					dirY = -1;
				else
					dirY = 0;

				game_object->GetTransform()->MoveX(dirX * speed * App->time.GetGameDeltaTime());//Move x
				game_object->GetTransform()->MoveY(dirY * speed * App->time.GetGameDeltaTime());//Move y

				if (App->pathfinding.ValidTile(int(pos.x), int(pos.y)) == false)
				{
					game_object->GetTransform()->MoveX(-dirX * speed * App->time.GetGameDeltaTime());//Move x back
					game_object->GetTransform()->MoveY(-dirY * speed * App->time.GetGameDeltaTime());//Move y back
				}

				ChangeState();
				CheckDirection(actualPos);
				if (path->empty())
				{
					moveOrder = false;
					move = false;
					chasing = false;
					spriteState = IDLE;
					if (chaseObj != nullptr && chaseObj->IsDestroyed()) chaseObj = nullptr;
				}
			}
			else
			{		
				if(nonMovingCounter > 1.0f) spriteState = IDLE;
				else nonMovingCounter += App->time.GetGameDeltaTime();
			}
		}	

		//Move selection rect
		std::pair<float, float> world = Map::F_MapToWorld(pos.x, pos.y);
		selectionRect.x = world.first + selectionOffset.first;
		selectionRect.y = world.second + selectionOffset.second;

		if (atkTimer < atkTime) atkTimer += App->time.GetGameDeltaTime();	

		if (atkObj != nullptr && atkObj->IsDestroyed()) { spriteState = IDLE;  atkObj = nullptr; }
		if (chaseObj != nullptr && chaseObj->IsDestroyed())
		{
			chaseObj = nullptr;
			chasing = false;
			goingBase = false;
			spriteState = IDLE;
		}

		//Check position for non walkable
		if (App->pathfinding.ValidTile(int(pos.x), int(pos.y)) == false)
		{
			if (!foundPoint)
			{
				bool free = false;
				int distance = 1;
				int maxIterations = 100;
				iPoint startPoint = { int(pos.x),int(pos.y) };
				do
				{
					if (App->pathfinding.ValidTile(startPoint.x + distance, startPoint.y))
					{
						free = true;
						startPoint = { int(pos.x) + (distance+2), int(pos.y) };
					}
					else if (App->pathfinding.ValidTile(startPoint.x - distance, startPoint.y))
					{
						free = true;
						startPoint = { int(pos.x) - (distance + 2), int(pos.y) };
					}
					else if (App->pathfinding.ValidTile(startPoint.x, startPoint.y + distance))
					{
						free = true;
						startPoint = { int(pos.x), int(pos.y) + (distance + 2) };
					}
					else if (App->pathfinding.ValidTile(startPoint.x, startPoint.y - distance))
					{
						free = true;
						startPoint = { int(pos.x), int(pos.y) - (distance + 2) };
					}
					else if (App->pathfinding.ValidTile(startPoint.x + distance, startPoint.y + distance))
					{
						free = true;
						startPoint = { int(pos.x) + (distance + 2), int(pos.y) + (distance + 2) };
					}
					else if (App->pathfinding.ValidTile(startPoint.x - distance, startPoint.y + distance))
					{
						free = true;
						startPoint = { int(pos.x) - (distance + 2),int(pos.y) + (distance + 2) };
					}
					else if (App->pathfinding.ValidTile(startPoint.x + distance, startPoint.y - distance))
					{
						free = true;
						startPoint = { int(pos.x) + (distance + 2), int(pos.y) - (distance + 2) };
					}
					else if (App->pathfinding.ValidTile(startPoint.x - distance, startPoint.y - distance))
					{
						free = true;
						startPoint = { int(pos.x) - (distance + 2), int(pos.y) - (distance +2) };
					}
					distance++;
				} while (!free && distance < maxIterations);

				if (free)
				{
					foundPoint = true;
					freePos = { float(startPoint.x),float(startPoint.y) };
					direction = { freePos.x - pos.x,freePos.y - pos.y };
					float normal = sqrt(pow(direction.x, 2) + pow(direction.y, 2));
					direction = { direction.x / normal,direction.y / normal };
				}
			}
			else
			{
				float d = (abs(pos.x - freePos.x)) + (abs(pos.y - freePos.y));
				if (d > 0.5f)
				{
					game_object->GetTransform()->MoveX(direction.x * 5 * App->time.GetGameDeltaTime());//Move X
					game_object->GetTransform()->MoveY(direction.y * 5 * App->time.GetGameDeltaTime());//Move Y
				}
				else
				{
					foundPoint = false;
				}
			}
		}
			
		//Draw vision and attack range
		if (drawRanges) DrawRanges();

		if (visible)
		{
			if (current_life < max_life)
				mini_life_bar.Show();
		}
		else mini_life_bar.Hide();		
	}
}

void B_Unit::OnCollision(Collider selfCol, Collider col)
{
	if (current_state != DESTROYED)
	{
		if (selfCol.GetColliderTag() == PLAYER_ATTACK_TAG)
		{
			if (col.GetColliderTag() == ENEMY_TAG)
			{
				if (GetType() != GATHERER)
				{
					if(col.parentGo->GetBehaviour()->GetType() != EDGE && col.parentGo->GetBehaviour()->GetType() != CAPSULE)
						atkObj = col.parentGo->GetBehaviour();
				}
				else
					atkObj = col.parentGo->GetBehaviour();			
			}
		}

		if (selfCol.GetColliderTag() == ENEMY_ATTACK_TAG && col.GetColliderTag() == PLAYER_TAG)
			atkObj = col.parentGo->GetBehaviour();

		if (selfCol.GetColliderTag() == ENEMY_VISION_TAG && col.GetColliderTag() == PLAYER_TAG && chaseObj == nullptr)
			chaseObj = col.parentGo->GetBehaviour();
	}
}

int B_Unit::GetUnitLevel() { return unitLevel; }

void B_Unit::UpdatePath(int x, int y)
{
	if (x >= 0 && y >= 0)
	{
		path = App->pathfinding.CreatePath({ int(pos.x), int(pos.y) }, { x, y }, GetID());

		calculating_path = true;
		next = false;
		move = false;
		movDest = { x, y };

		if (!tilesVisited.empty())
		{
			for (std::vector<iPoint>::const_iterator it = tilesVisited.cbegin(); it != tilesVisited.cend(); ++it)
			{
				if (PathfindingManager::unitWalkability[it->x][it->y] != 0.0f)
					PathfindingManager::unitWalkability[it->x][it->y] = 0.0f;
			}

			tilesVisited.clear();
		}
	}
}

void B_Unit::Repath()
{
	if (path != nullptr && !path->empty())
	{
		path = App->pathfinding.CreatePath({ int(pos.x), int(pos.y) }, { movDest.x, movDest.y }, GetID());

		calculating_path = true;
		next = false;
		move = false;

		if (!tilesVisited.empty())
		{
			for (std::vector<iPoint>::const_iterator it = tilesVisited.cbegin(); it != tilesVisited.cend(); ++it)
			{
				if (PathfindingManager::unitWalkability[it->x][it->y] != 0.0f)
					PathfindingManager::unitWalkability[it->x][it->y] = 0.0f;
			}

			tilesVisited.clear();
		}
	}
}

void B_Unit::CheckPathTiles()
{
	if (!next)
	{
		if (PathfindingManager::unitWalkability[nextTile.x][nextTile.y] != 0.0f)
			PathfindingManager::unitWalkability[nextTile.x][nextTile.y] == 0;

		nextTile = path->front();
		next = true;
		move = true;
		gotTile = false;
	}
	else if (PathfindingManager::unitWalkability[nextTile.x][nextTile.y] == 0.0f)
	{
		PathfindingManager::unitWalkability[nextTile.x][nextTile.y] = GetID();
		tilesVisited.push_back(nextTile);
		gotTile = true;
	}
	else if (!gotTile)
	{
		iPoint cell = App->pathfinding.CheckEqualNeighbours(iPoint(int(pos.x), int(pos.y)), nextTile);

		if (cell.x != -1 && cell.y != -1)
			nextTile = cell;
	}
}

void B_Unit::ChangeState()
{
	//Change state to change sprite
	if (dirX == 1 && dirY == 1)//S
	{
		spriteState = MOVING_S;
	}
	else if (dirX == -1 && dirY == -1)//N
	{
		spriteState = MOVING_N;
	}
	else if (dirX == 1 && dirY == -1)//E
	{
		spriteState = MOVING_E;
	}
	else if (dirX == -1 && dirY == 1)//W
	{
		spriteState = MOVING_W;
	}
	else if (dirX == 0 && dirY == 1)//SW
	{
		spriteState = MOVING_SW;
	}
	else if (dirX == 1 && dirY == 0)//SE
	{
		spriteState = MOVING_SE;
	}
	else if (dirX == 0 && dirY == -1)//NE
	{
		spriteState = MOVING_NE;
	}
	else if (dirX == -1 && dirY == 0)//NW
	{
		spriteState = MOVING_NW;
	}
}

void B_Unit::CheckDirection(fPoint actualPos)
{
	if (dirX == 1 && dirY == 1)
	{
		if (actualPos.x >= nextTile.x && actualPos.y >= nextTile.y)
		{
			if (!path->empty()) path->erase(path->begin());
			next = false;
			PathfindingManager::unitWalkability[nextTile.x][nextTile.y] = 0.0f;
		}
	}
	else if (dirX == -1 && dirY == -1)
	{
		if (actualPos.x <= nextTile.x && actualPos.y <= nextTile.y)
		{
			if (!path->empty()) path->erase(path->begin());
			next = false;
			PathfindingManager::unitWalkability[nextTile.x][nextTile.y] = 0.0f;
		}
	}
	else if (dirX == -1 && dirY == 1)
	{
		if (actualPos.x <= nextTile.x && actualPos.y >= nextTile.y)
		{
			if (!path->empty()) path->erase(path->begin());
			next = false;
			PathfindingManager::unitWalkability[nextTile.x][nextTile.y] = 0.0f;
		}
	}
	else if (dirX == 1 && dirY == -1)
	{
		if (actualPos.x >= nextTile.x && actualPos.y <= nextTile.y)
		{
			if (!path->empty()) path->erase(path->begin());
			next = false;
			PathfindingManager::unitWalkability[nextTile.x][nextTile.y] = 0.0f;
		}
	}
	else if (dirX == 0 && dirY == -1)
	{
		if (actualPos.y <= nextTile.y)
		{
			if (!path->empty()) path->erase(path->begin());
			next = false;
			PathfindingManager::unitWalkability[nextTile.x][nextTile.y] = 0.0f;
		}
	}
	else if (dirX == 0 && dirY == 1)
	{
		if (actualPos.y >= nextTile.y)
		{
			if (!path->empty()) path->erase(path->begin());
			next = false;
			PathfindingManager::unitWalkability[nextTile.x][nextTile.y] = 0.0f;
		}
	}
	else if (dirX == 1 && dirY == 0)
	{
		if (actualPos.x >= nextTile.x)
		{
			if (!path->empty()) path->erase(path->begin());
			next = false;
			PathfindingManager::unitWalkability[nextTile.x][nextTile.y] = 0.0f;
		}
	}
	else if (dirX == -1 && dirY == 0)
	{
		if (actualPos.x <= nextTile.x)
		{
			if (!path->empty()) path->erase(path->begin());
			next = false;
			PathfindingManager::unitWalkability[nextTile.x][nextTile.y] = 0.0f;
		}
	}
	else if (dirX == 0 && dirY == 0)
	{
		if(!path->empty()) path->erase(path->begin());
		next = false;
		PathfindingManager::unitWalkability[nextTile.x][nextTile.y] = 0.0f;
	}

	if (path->empty()) new_state = IDLE;
}

void B_Unit::OnDestroy()
{
	App->pathfinding.DeletePath(GetID());
	if (!tilesVisited.empty())
	{
		for (std::vector<iPoint>::const_iterator it = tilesVisited.cbegin(); it != tilesVisited.cend(); ++it)
		{
			if (PathfindingManager::unitWalkability[it->x][it->y] != 0)
				PathfindingManager::unitWalkability[it->x][it->y] = 0;
		}
		tilesVisited.clear();
	}
}

void B_Unit::DrawRanges()
{
	std::pair<float, float> localPos = Map::F_MapToWorld(pos.x, pos.y, pos.z);
	localPos.first += 30.0f;
	localPos.second += 30.0f;
	visionRange = { vision_range * 23, vision_range * 23 };
	atkRange = { attack_range * 23, attack_range * 23 };
	App->render->DrawCircle(localPos, visionRange, { 10, 156, 18, 255 }, FRONT_SCENE, true);//Vision
	App->render->DrawCircle(localPos, atkRange, { 255, 0, 0, 255 }, FRONT_SCENE, true);//Attack
}

void B_Unit::DoAttack()
{
	UnitAttackType();
	std::pair<int, int> Pos(int(pos.x),int(pos.y));
	vec objPos = atkObj->GetPos();
	std::pair<int,int> atkPos(int(objPos.x), int(objPos.y));
	arriveDestination = true;

	audio->Play(attackFX);
	if (atkPos.first == Pos.first && atkPos.second < Pos.second)//N
	{
		spriteState = ATTACKING_N;
	}
	else if (atkPos.first == Pos.first && atkPos.second > Pos.second)//S
	{
		spriteState = ATTACKING_S;
	}
	else if (atkPos.first < Pos.first && atkPos.second == Pos.second)//W
	{
		spriteState = ATTACKING_W;
	}
	else if (atkPos.first > Pos.first && atkPos.second == Pos.second)//E
	{
		spriteState = ATTACKING_E;
	}
	else if (atkPos.first < Pos.first && atkPos.second > Pos.second)//SW
	{
		spriteState = ATTACKING_NW;
	}
	else if (atkPos.first > Pos.first && atkPos.second > Pos.second)//
	{
		spriteState = ATTACKING_SW;
	}
	else if (atkPos.first < Pos.first && atkPos.second < Pos.second)//
	{
		spriteState = ATTACKING_NE;
	}
	else if (atkPos.first > Pos.first && atkPos.second < Pos.second)//
	{
		spriteState = ATTACKING_SE;
	}
}

void B_Unit::UpgradeUnit(int life, int dmg,int lvl)
{
	max_life += life;
	current_life = max_life;
	damage += dmg;
	unitLevel = lvl;
}

void B_Unit::OnRightClick(vec posClick, vec movPos)
{		
	audio->Play(UNIT_MOVES);
	chaseObj = nullptr;
	SDL_Rect cam = App->render->GetCameraRect();
	int x, y;
	App->input->GetMousePosition(x, y);
	x += cam.x;
	y += cam.y;

	for (std::map<double, Behaviour*>::iterator it = Behaviour::b_map.begin(); it != Behaviour::b_map.end(); ++it)
	{
		if (GetType() == GATHERER)
		{
			if ( it->second->GetType() == EDGE || it->second->GetType() == CAPSULE)
			{
				RectF coll = (*it).second->GetSelectionRect();
				if (float(x) > coll.x && float(x) < coll.x + coll.w && float(y) > coll.y && float(y) < coll.y + coll.h)
				{
					chaseObj = (*it).second;
					next = false;
					move = false;
					moveOrder = true;
					chasing = false;
					break;
				}
			}
		}
		else
		{
			if (it->second->GetType() == ENEMY_MELEE || it->second->GetType() == ENEMY_RANGED || it->second->GetType() == ENEMY_SUPER
				|| it->second->GetType() == SPAWNER)
			{
				RectF coll = (*it).second->GetSelectionRect();
				if (float(x) > coll.x && float(x) < coll.x + coll.w && float(y) > coll.y && float(y) < coll.y + coll.h)
				{
					chaseObj = (*it).second;
					next = false;
					move = false;
					moveOrder = true;
					chasing = false;
					break;
				}
			}
		}

		if (!tilesVisited.empty())//Clean path tiles
		{
			for (std::vector<iPoint>::const_iterator it = tilesVisited.cbegin(); it != tilesVisited.cend(); ++it)
			{
				if (PathfindingManager::unitWalkability[it->x][it->y] != 0)
					PathfindingManager::unitWalkability[it->x][it->y] = 0;
			}

			tilesVisited.clear();
		}
	}

	if(chaseObj == nullptr)
	{
		if (movPos.x != -1 && movPos.y != -1)
		{
			path = App->pathfinding.CreatePath({ int(pos.x), int(pos.y) }, { int(movPos.x), int(movPos.y) }, GetID());
			next = false;
			move = false;
			moveOrder = true;
			chasing = false;			
		}
		else if (App->pathfinding.ValidTile(int(posClick.x), int(posClick.y)))
		{
			path = App->pathfinding.CreatePath({ int(pos.x), int(pos.y) }, { int(posClick.x), int(posClick.y) }, GetID());
			next = false;
			move = false;
			moveOrder = true;
			chasing = false;
		}

		if (!tilesVisited.empty())//Clean path tiles
		{
			for (std::vector<iPoint>::const_iterator it = tilesVisited.cbegin(); it != tilesVisited.cend(); ++it)
			{
				if (PathfindingManager::unitWalkability[it->x][it->y] != 0)
				{
					PathfindingManager::unitWalkability[it->x][it->y] = 0;
				}
			}
			tilesVisited.clear();
		}
	}	 
}
 
void Behaviour::Lifebar::Create(Gameobject* parent)
{
	int hud_id = App->tex.Load("textures/Iconos_square_up.png");
	go = new Gameobject("life_bar", parent);
	new Sprite(go, hud_id, { 275, 698, 30, 4 }, FRONT_SCENE, { 2.f, -35.f, 2.f, 2.f }, { 255, 0, 0, 255 });
	green_bar = new Sprite(new Gameobject("GreenBar", go), hud_id, life_starting_section = { 276, 703, 28, 5 }, FRONT_SCENE, { 3.f, -34.f, 2.f, 2.f }, { 0, 255, 0, 255 });
	upgrades = new Sprite(new Gameobject("Upgrades", go), hud_id, upgrades_starting_section = { 0, 0, 0, 0 }, FRONT_SCENE, { 168.f, -184.f, 0.4f, 0.4f });
	Update(1.0f, 0);
}

void Behaviour::Lifebar::Show()
{
	go->SetActive();
}

void Behaviour::Lifebar::Hide()
{
	go->SetInactive();
}

void Behaviour::Lifebar::Update(float life, int lvl)
{
	green_bar->SetSection({ life_starting_section.x, life_starting_section.y, int(float(life_starting_section.w) * life), life_starting_section.h });

	if (lvl <= 0)
		upgrades->SetSection({ 0,0,0,0 });

	else
		upgrades->SetSection({ 16 + 36 * (lvl - 1), 806, 33, 33 });
}

// Queued Unit

SDL_Rect BuildingWithQueue::bar_section;

BuildingWithQueue::QueuedUnit::QueuedUnit(UnitType type, Gameobject* go, vec pos, float time) :
	type(type), pos(pos), time(time), current_time(time)
{
	if (go)
	{
		Gameobject* icon = App->scene->AddGameobject("Queued Unit", go);
		transform = icon->GetTransform();
	}
	else
		transform = nullptr;
}

BuildingWithQueue::QueuedUnit::QueuedUnit(const QueuedUnit& copy) :
	type(copy.type), pos(copy.pos), time(copy.time), current_time(copy.current_time), transform(copy.transform)
{}

float BuildingWithQueue::QueuedUnit::Update()
{
	return (time - (current_time -= App->time.GetGameDeltaTime())) / time;
}

BuildingWithQueue::BuildingWithQueue(Gameobject* go, UnitType type, UnitState starting_state, ComponentType comp_type) : Behaviour(go, type, starting_state, comp_type)
{
	spawnPoint = game_object->GetTransform()->GetLocalPos();
	int texture_id = App->tex.Load("textures/Hud_Sprites.png");
	Gameobject* back_bar = App->scene->AddGameobject("Creation Bar", game_object);
	new Sprite(back_bar, texture_id, { 276, 256, 216, 16 }, FRONT_SCENE, { 0.f, 13.f, 0.29f, 0.2f });
	progress_bar = new Sprite(back_bar, texture_id, bar_section = { 276, 273, 216, 16 }, FRONT_SCENE, { 0.f, 13.f, 0.29f, 0.2f });
	back_bar->SetInactive();

	Gameobject* unit_icon = App->scene->AddGameobject("Unit Icon", game_object);

	icon = new Sprite(unit_icon, texture_id, { 0, 0, 0, 0 }, FRONT_SCENE, { -50.f, 0.f, 0.2f, 0.2f });

	unit_icon->SetInactive();
}

void BuildingWithQueue::Update()
{
	pos = game_object->GetTransform()->GetGlobalPosition();
	if (providesVisibility) GetTilesInsideRadius();

	if (!build_queue.empty())
	{
		bool able_to_build = true;

		switch (build_queue.front().type)
		{
		case GATHERER:
			if (App->scene->GetStat(CURRENT_EDGE) <= GATHERER_COST)
			{
				able_to_build = false;
			}
			break;
		case UNIT_MELEE:
			if (App->scene->GetStat(CURRENT_EDGE) <= MELEE_COST)
			{
				able_to_build = false;
			}
			break;
		case UNIT_RANGED:
			if (App->scene->GetStat(CURRENT_EDGE) <= RANGED_COST)
			{
				able_to_build = false;
			}
			break;
		}

		if (!progress_bar->GetGameobject()->IsActive())
			progress_bar->GetGameobject()->SetActive();

		if (!icon->GetGameobject()->IsActive())
			icon->GetGameobject()->SetActive();

		float percent;
		if (able_to_build)
			percent = build_queue.front().Update();
		else
			percent = 1.0f;
		if (percent >= 1.0f)
		{
			Event::Push(SPAWN_UNIT, App->scene, build_queue.front().type, build_queue.front().pos);

			build_queue.front().transform->GetGameobject()->Destroy();
			build_queue.pop_front();

			if (build_queue.empty())
				progress_bar->GetGameobject()->SetInactive();
				icon->GetGameobject()->SetInactive();
		}
		else
		{
			switch (build_queue.front().type)
			{
			case GATHERER: icon->SetSection({ 121, 292, 43, 35 }); break;
			case UNIT_MELEE: icon->SetSection({ 121, 256, 48, 35 }); break;
			case UNIT_RANGED: icon->SetSection({ 231, 310, 35, 33 }); break;
			case UNIT_SUPER: icon->SetSection({ 121, 335, 35, 33 }); break;
			}

			SDL_Rect section = bar_section;
			section.w = int(float(section.w) * percent);
			progress_bar->SetSection(section);
		}
	}
}

void BuildingWithQueue::AddUnitToQueue(UnitType type, vec pos, float time)
{
	switch (type)
	{
	case GATHERER:
	{
		if (App->scene->GetStat(CURRENT_EDGE) >= GATHERER_COST)
		{
			QueuedUnit unit(type, game_object, pos, time);
			unit.transform->SetY(build_queue.size());
			build_queue.push_back(unit);
		}
		break;
	}
	case UNIT_MELEE:
	{		
		if (App->scene->GetStat(CURRENT_EDGE) >= MELEE_COST)
		{
			QueuedUnit unit(type, game_object, pos, time);
			unit.transform->SetY(build_queue.size());
			build_queue.push_back(unit);
		}
		break;
	}
	case UNIT_RANGED:
	{
		if (App->scene->GetStat(CURRENT_EDGE) >= RANGED_COST)
		{
			QueuedUnit unit(type, game_object, pos, time);
			unit.transform->SetY(build_queue.size());
			build_queue.push_back(unit);
		}
		break;
	}
	case UNIT_SUPER:
	{		
		if (App->scene->GetStat(CURRENT_EDGE) >= SUPER_COST)
		{
			QueuedUnit unit(type, game_object, pos, time);
			unit.transform->SetY(build_queue.size());
			build_queue.push_back(unit);
		}
		break;
	}
	}
}
