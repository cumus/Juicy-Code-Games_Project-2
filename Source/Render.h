#ifndef __RENDER_H__
#define __RENDER_H__

#include "Module.h"
#include "SDL/include/SDL_rect.h"
#include "SDL/include/SDL_pixels.h"
#include "TextureManager.h"
#include "Point.h"
#include <map>
#include <vector>

class Sprite;
class RenderedText;
struct SDL_Renderer;
struct SDL_Texture;

enum Layer : int
{
	BACKGROUND,
	MAP,
	WALKABILITY,
	DEBUG_MAP,
	BACK_SCENE,
	SCENE,
	FRONT_SCENE,
	FOG_OF_WAR,
	DEBUG_SCENE,
	HUD,
	EDITOR,
	FADE,
	CURSOR,
	MAX_LAYERS
};

class Render : public Module
{
public:

	Render();
	~Render();

	void LoadConfig(bool empty_config) override;
	void SaveConfig() const override;

	bool Init() override;
	bool PreUpdate() override;
	bool Update() override;
	bool PostUpdate() override;
	bool CleanUp() override;

	void RecieveEvent(const Event& e) override;

	// Getters & Camera
	SDL_Renderer* GetSDLRenderer() const;
	SDL_Rect GetCameraRect() const;
	RectF GetCameraRectF() const;
	float GetZoom() const;
	void ToggleZoomLocked();
	std::pair<float, float> GetCameraCenter() const;
	bool InsideCam(float x, float y) const;
	iPoint CamToIsometric();
	iPoint ConvertIsoTo2D(iPoint point);

	// Minimap
	int GetMinimap(int width, int height, float scale);

	// Viewport
	void SetupViewPort(float aspect_ratio);
	void ResetViewPort();
	std::pair<int, int> GetViewPortOffset();
	static std::pair<float, float> GetResRatio();

	// Blit
	bool Blit(int texture_id, int x, int y, const SDL_Rect* section = nullptr, Layer layer = SCENE, bool use_cam = true);
	bool Blit_Scale(int texture_id, int x, int y, float scale_x, float scale_y, const SDL_Rect* section = nullptr, Layer layer = SCENE, bool use_cam = true);
	bool BlitNorm(int texture_id, const RectF rect, const SDL_Rect* section = nullptr, Layer layer = SCENE);

	bool BlitMapTile(int texture_id, int x, int y, const SDL_Rect* section = nullptr, Layer layer = SCENE, bool use_cam = true);

	bool Blit_Text(RenderedText* rendered_text, int x, int y, Layer layer = SCENE, bool use_cam = true);
	bool Blit_TextSized(RenderedText* rendered_text, const SDL_Rect size, Layer layer = SCENE, bool use_cam = true);
	//bool BlitParticle(SDL_Texture* texture, int x, int y, const SDL_Rect* section = NULL, const SDL_Rect* rectSize = NULL, SDL_Color color = { 0, 0, 0, 0 }, SDL_BlendMode blendMode = SDL_BLENDMODE_NONE, float speed = 1.0f, double angle = 0, int pivot_x = INT_MAX, int pivot_y = INT_MAX, Layer layer = SCENE) const;

	void DrawQuad(const SDL_Rect rect, const SDL_Color color = { 0, 0, 0, 255 }, bool filled = true, Layer layer = SCENE, bool use_camera = true);
	void DrawQuadNormCoords(const RectF rect, const SDL_Color color = { 0, 0, 0, 255 }, bool filled = true, Layer layer = SCENE);
	void DrawLine(const std::pair<int,int> a, const std::pair<int, int> b, const SDL_Color color = { 0, 0, 0, 255 }, Layer layer = SCENE, bool use_camera = true);
	void DrawCircle(const std::pair<int, int> a, const std::pair<int, int> b, const SDL_Color color = { 0, 0, 0, 255 }, Layer layer = SCENE, bool use_camera = true);

	// Set background color
	void SetBackgroundColor(SDL_Color color);
	bool RenderMinimapFoW(float progress);

	void MoveCamera(float x, float y);

private:

	bool SetDrawColor(SDL_Color color);

private:

	SDL_Renderer* renderer = nullptr;
	SDL_Rect		viewport;
	SDL_Color		background;
	SDL_Color		draw_color;

	//Cursor
	int cursorID;

	// Camera
	RectF cam;
	bool zoom_allowed = false;
	float zoom = 1.0f;
	static std::pair<float, float> target_res;
	static std::pair<float, float> res_ratio;

	// Layer mapping
	struct RenderData
	{
		enum Type
		{
			TEXTURE_FULL,
			TEXTURE_SECTION,
			QUAD_FILLED,
			QUAD_EMPTY,
			LINE,
			CIRCLE,
			MAX_TYPES
		} type;

		RenderData(Type type);
		RenderData(const RenderData& copy);

		SDL_Texture* texture;
		SDL_Rect rect;
		bool camera;
		double angle = 0;
		union ExtraData
		{
			SDL_Rect section;
			SDL_Color color;
		} extra;
	};

	std::map<int, std::vector<RenderData>> layers[MAX_LAYERS];

	// Minimap
	float minimap_scale = 1.0f;
	int minimap_texture[2] = { -1, -1 };
	int last_row = 0;
	bool current_texture = false;
	bool needs_clear = true;

	// Config
	bool accelerated = true;
	bool vsync = false;
	bool target_texture = false;

private:

	inline void AddToLayer(Layer layer, const RenderData& data);
};

#endif // __RENDER_H__