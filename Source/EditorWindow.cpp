#include "EditorWindow.h"
#include "Application.h"
#include "Input.h"
#include "Render.h"
#include "JuicyMath.h"
#include "UI_Image.h"
#include "UI_Button.h"
#include "UI_Text.h"
#include "UI_TextButton.h"
#include "UI_ButtonImage.h"

#include "optick-1.3.0.0/include/optick.h"

float EditorWindow::margin = 0.002f;
float EditorWindow::min_size = 0.02f;

EditorWindow::EditorWindow(const RectF window_area, SDL_Color color) : rect(window_area), color(color)
{
	// Cap min pos
	if (rect.x < 0.0f) rect.x = 0.0f;
	if (rect.y < 0.0f) rect.y = 0.0f;

	// Cap min size
	if (rect.w < min_size) rect.w = min_size;
	if (rect.h < min_size) rect.h = min_size;
}

EditorWindow::~EditorWindow()
{
	if (!elements.empty())
		CleanUp();
}

void EditorWindow::CleanUp()
{
	for (std::vector<UI_Element*>::iterator it = elements.begin(); it != elements.end(); ++it)
		DEL(*it);

	elements.clear();
}

const WindowState EditorWindow::Update(float mouse_x, float mouse_y, KeyState mouse_left_button, bool sizing)
{
	if (sizing)
	{
		// Check if user is editing window size
		if (!state.dragging)
		{
			// Check mouse inside rect with margin
			if (JMath::PointInsideRect(mouse_x, mouse_y, { rect.x - margin,  rect.y - margin,  rect.x + rect.w + margin, rect.y + rect.h + margin }))
			{
				state.mouse_inside = true;

				// Check mouse hovering side
				if (JMath::PointInsideRect(mouse_x, mouse_y, GetBorderN_Norm()))
				{
					if (JMath::PointInsideRect(mouse_x, mouse_y, GetBorderW_Norm())) state.hovering = CORNER_NW;
					else if (JMath::PointInsideRect(mouse_x, mouse_y, GetBorderE_Norm())) state.hovering = CORNER_NE;
					else state.hovering = SIDE_N;
				}
				else if (JMath::PointInsideRect(mouse_x, mouse_y, GetBorderS_Norm()))
				{
					if (JMath::PointInsideRect(mouse_x, mouse_y, GetBorderW_Norm())) state.hovering = CORNER_SW;
					else if (JMath::PointInsideRect(mouse_x, mouse_y, GetBorderE_Norm())) state.hovering = CORNER_SE;
					else state.hovering = SIDE_S;
				}
				else if (JMath::PointInsideRect(mouse_x, mouse_y, GetBorderW_Norm())) state.hovering = SIDE_W;
				else if (JMath::PointInsideRect(mouse_x, mouse_y, GetBorderE_Norm())) state.hovering = SIDE_E;
				else state.hovering = INSIDE;

				if (state.hovering > OUTSIDE && mouse_left_button == KEY_DOWN)
						state.dragging = true;
			}
			else
			{
				state.mouse_inside = false;
				state.hovering = OUTSIDE;
			}
		}
		else if (mouse_left_button != KEY_REPEAT)
		{
			state.dragging = false;
			state.hovering = OUTSIDE;
		}
		else
		{
			// Editing window sizes
			switch (state.hovering)
			{
			case SIDE_N: MouseDrag_N(mouse_x, mouse_y); break;
			case SIDE_W: MouseDrag_W(mouse_x, mouse_y); break;
			case SIDE_E: MouseDrag_E(mouse_x, mouse_y); break;
			case SIDE_S: MouseDrag_S(mouse_x, mouse_y); break;
			case CORNER_NW: MouseDrag_N(mouse_x, mouse_y); MouseDrag_W(mouse_x, mouse_y); break;
			case CORNER_NE: MouseDrag_N(mouse_x, mouse_y); MouseDrag_E(mouse_x, mouse_y); break;
			case CORNER_SW: MouseDrag_S(mouse_x, mouse_y); MouseDrag_W(mouse_x, mouse_y); break;
			case CORNER_SE: MouseDrag_S(mouse_x, mouse_y); MouseDrag_E(mouse_x, mouse_y); break;
			}
		}
	}
	else if (JMath::PointInsideRect(mouse_x, mouse_y, rect))
	{
		// Mouse is inside window
		if (!state.mouse_inside)
		{
			state.mouse_inside = true;
			Event::Push(HOVER_IN, this, -1);
		}

		// Iterate window's UI_Elements. Call all given events
		int id = 0;
		for (std::vector<UI_Element*>::iterator it = elements.begin(); it != elements.end(); ++it)
		{
			if (JMath::PointInsideRect(mouse_x, mouse_y, (*it)->GetTargetNormRect()))
			{
				if (!(*it)->mouse_inside)
				{
					Event::Push(HOVER_IN, this, id);
					(*it)->mouse_inside = true;
				}
				if (mouse_left_button == KEY_DOWN)
					Event::Push(MOUSE_DOWN, this, id);

				else if (mouse_left_button == KEY_REPEAT)
					Event::Push(MOUSE_REPEAT, this, id);

				else if (mouse_left_button == KEY_UP)
					Event::Push(MOUSE_UP, this, id);
			}
			else if ((*it)->mouse_inside)
			{
				Event::Push(HOVER_OUT, this, id);
				(*it)->mouse_inside = false;
			}

			++id;
		}
	}
	else if (state.mouse_inside)
	{
		state.mouse_inside = false;
		Event::Push(HOVER_OUT, this, -1);
	}

	_Update();

	return state;
}

void EditorWindow::Draw(bool draw_border) const
{
	// Draw background
	App->render->DrawQuadNormCoords(rect, color, true, EDITOR);

	// Draw contents
	for (std::vector<UI_Element*>::const_iterator it = elements.begin(); it != elements.end(); ++it)
		(*it)->Draw();

	// Draw Border
	if (draw_border)
	{
		App->render->DrawQuadNormCoords(GetBorderN_Norm(), { 150, (state.hovering == SIDE_N || state.hovering == CORNER_NW || state.hovering == CORNER_NE ? 150u : 0), 0, 200 }, true, EDITOR);
		App->render->DrawQuadNormCoords(GetBorderW_Norm(), { 150, (state.hovering == SIDE_W || state.hovering == CORNER_NW || state.hovering == CORNER_SW ? 150u : 0), 0, 200 }, true, EDITOR);
		App->render->DrawQuadNormCoords(GetBorderE_Norm(), { 150, (state.hovering == SIDE_E || state.hovering == CORNER_SE || state.hovering == CORNER_NE ? 150u : 0), 0, 200 }, true, EDITOR);
		App->render->DrawQuadNormCoords(GetBorderS_Norm(), { 150, (state.hovering == SIDE_S || state.hovering == CORNER_SE || state.hovering == CORNER_SW ? 150u : 0), 0, 200 }, true, EDITOR);
	}
}

RectF EditorWindow::GetBorderN_Norm() const
{
	return { rect.x - margin,
		rect.y - margin,
		rect.w + margin * 2.0f,
		margin * 2.0f };
}

RectF EditorWindow::GetBorderW_Norm() const
{
	return { rect.x - margin,
		rect.y - margin,
		margin * 2.0f,
		rect.h + margin * 2.0f };
}

RectF EditorWindow::GetBorderE_Norm() const
{
	return { rect.x + rect.w - margin,
		rect.y - margin,
		margin * 2.0f,
		rect.h + margin * 2.0f };
}

RectF EditorWindow::GetBorderS_Norm() const
{
	return { rect.x - margin,
		rect.y + rect.h - margin,
		rect.w + margin * 2.0f,
		margin * 2.0f };
}

void EditorWindow::MouseDrag_N(float mouse_x, float mouse_y)
{
	if (rect.h + rect.y - mouse_y >= min_size)
	{
		rect.h += rect.y - mouse_y;
		rect.y = mouse_y;
	}
	else
	{
		rect.y += rect.h - min_size;
		rect.h = min_size;
	}
}

void EditorWindow::MouseDrag_W(float mouse_x, float mouse_y)
{
	if (rect.w + rect.x - mouse_x >= min_size)
	{
		rect.w += rect.x - mouse_x;
		rect.x = mouse_x;
	}
	else
	{
		rect.x += rect.w - (min_size);
		rect.w = min_size;
	}
}

void EditorWindow::MouseDrag_E(float mouse_x, float mouse_y)
{
	if ((rect.w = mouse_x - rect.x) < min_size)
		rect.w = min_size;
}

void EditorWindow::MouseDrag_S(float mouse_x, float mouse_y)
{
	if ((rect.h = mouse_y - rect.y) < min_size)
		rect.h = min_size;
}
