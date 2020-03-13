#include "Application.h"
#include "Log.h"

#ifdef DEBUG
#include "mmgr\mmgr.h"
#endif

#include "SDL/include/SDL.h"
#ifdef PLATFORMx86
#pragma comment( lib, "SDL/libx86/SDL2.lib" )
#pragma comment( lib, "SDL/libx86/SDL2main.lib" )
#elif PLATFORMx64
#pragma comment( lib, "SDL/libx64/SDL2.lib" )
#pragma comment( lib, "SDL/libx64/SDL2main.lib" )
#endif

Application* App = nullptr;

int main(int argc, char* args[])
{
	int main_return = EXIT_FAILURE;

	LOG("Engine starting ... %d");

	App = new Application(argc, args);

	LOG("Initializing Application Systems");

	if (App->Init())
	{
		LOG("Entering Application's Main Loop");

		int update_return = 1;

		while (update_return == 1)
			update_return = App->Update();

		if (update_return == 0)
		{
			LOG("Shuting Application Systems");

			if (App->CleanUp())
			{
				delete App;
				main_return = EXIT_SUCCESS;
				LOG("EXIT SUCCESS");
/*#ifdef DEBUG
				LOG("Memory Leaks: %d!\n", (m_getMemoryStatistics().totalAllocUnitCount));
#endif*/
			}
			else
			{
				LOG("Application CleanUp exits with ERROR");
			}
		}
		else
		{
			LOG("Application Main Loop ERROR");
		}
	}
	else
	{
		LOG("Application Init exits with ERROR");
	}

	LOG("... Bye! :)\n");

	return main_return;
}