#ifndef __TIMEMANAGER_H__
#define __TIMEMANAGER_H__

#include <list>

class Timer
{
public:
	Timer(const bool start_active = true);
	Timer(const Timer& timer);

	bool IsPlaying() const;
	void Start();
	void Pause();
	void Stop();

	unsigned int Read() const;
	int ReadI() const;
	float ReadF() const;

private:

	bool paused;
	unsigned int started_at;
	unsigned int paused_at;
};

class TimeManager
{
public:

	TimeManager();
	~TimeManager();

	bool Init();

	float UpdateDeltaTime(); // returns updated dt
	int ManageFrameTimers(); // returns extra ms for frame

	void	Delay(unsigned int ms) const;
	
	void	SetMaxFPS(float max_fps); // Set to 0 uncap fps
	float	GetMaxFPS() const;
	float	GetDeltaTime() const;
	float	GetGameDeltaTime() const;
	unsigned int GetCappedMS() const;
	unsigned int GetFpsCounter() const;
	unsigned int GetLastMs() const;
	unsigned int GetLastFPS() const;
	float GetEngineTimer() const;

	float GetGameTimer() const;
	void StartGameTimer();
	void PauseGameTimer();
	void StopGameTimer();

private:

	unsigned long	frames_counter = 0u;
	unsigned int	fps_counter = 0u;
	unsigned int	last_fps_count = 0u;
	unsigned int	last_ms_count = 0u;

	float	dt = 0.f;
	float	capped_fps = 60.f;
	unsigned int	capped_ms = 0u;

	Timer	ms_timer; // read every frame
	Timer	fps_timer; // read every second

	Timer	engine_timer;
	Timer	game_timer;
	float	game_dt;
};

#endif // __TIMEMANAGER_H__