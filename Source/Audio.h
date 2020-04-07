#ifndef __AUDIO_H__
#define __AUDIO_H__

#include "Module.h"
#include <map>

struct _Mix_Music;
struct Mix_Chunk;
class Transform;
enum Scenes;

enum Audio_FX : int
{
	LOGO,
	HAMMER,
	SELECT,
	TITLE,
	B_DESTROYED,
	B_BUILDED,
	UNIT_DIES,

	//Structures//
	EDGE_FX,
	BASE_CENTER_FX,
	TOWER_FX,
	TOWER_ATK_FX,

	//Units//
	GATHERER_ATK_FX,
	MELEE_ATK_FX,
	RANGED_ATK_FX,
	SUPER_ATK_FX,
	GATHERER_DIE_FX,
	MELEE_DIE_FX,
	RANGED_DIE_FX,
	SUPER_DIE_FX,

	IA_MELEE_ATK_FX,
	IA_RANGED_ATK_FX,
	IA_SUPER_ATK_FX,
	IA_SPECIAL_ATK_FX,
	IA_MELEE_DIE_FX,
	IA_RANGED_DIE_FX,
	IA_SUPER_DIE_FX,
	IA_SPECIAL_DIE_FX,

	MAX_FX
};

class Audio : public Module
{
public:

	Audio();
	~Audio();

	void LoadConfig(bool empty_config) override;
	void SaveConfig() const override;

	bool Init() override;
	bool Update() override;
	bool CleanUp() override;

	void RecieveEvent(const Event& e) override;

	// Music
	bool PlayMusic(const char* path, float fade_time = 2.0f);
	void PauseMusic(float fade_time) const;
	// void ClearMusic(float fade_time);

	// FX
	bool LoadFx(Audio_FX audio_fx);
	void UnloadFx();
	bool PlayFx(Audio_FX audio_fx, int repeat = 0);
	bool PlaySpatialFx(Audio_FX audio_fx, double id, const std::pair<float, float> position, int repeat = 0, int ticks = -1, int fade_ms = -1);
	bool StopFXChannel(double id, int ms = 0, bool fade = false);

private:

	_Mix_Music*	music = nullptr;

	struct SpatialData
	{
		SpatialData();
		SpatialData(const SpatialData& copy);

		int channel;
		float angle, distance;
		std::pair<float, float> pos;
	};
	std::map<double, SpatialData> sources;
	Mix_Chunk* fx[MAX_FX];
	
	int	total_channels = 1;

	// Mixer Codecs
	bool using_FLAC	= false;
	bool using_MOD	= false;
	bool using_MP3	= false;
	bool using_OGG	= true;
	bool using_MID	= false;
	bool using_OPUS = false;
};

#endif // __AUDIO_H__