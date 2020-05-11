#pragma once
#include "f4se/GameReferences.h"
#include "f4se/GameMenus.h"
#include "f4se/PapyrusVM.h"
#include <unordered_map>

// Thanks to Shizof for this

namespace MenuChecker
{
	extern std::vector<std::string> gameStoppingMenus;

	extern std::vector<std::string> gameStoppingMenusNoDialogue;

	extern std::unordered_map<std::string, bool> menuTypes;

	bool isGameStopped();

	bool isGameStoppedNoDialogue();

	bool isVatsActive();


	class MenuOpenCloseHandler : public BSTEventSink<MenuOpenCloseEvent>
	{
	public:
		virtual ~MenuOpenCloseHandler() { };
		virtual	EventResult	ReceiveEvent(MenuOpenCloseEvent * evn, void * dispatcher) override
		{
			if (!evn)
				return kEvent_Continue;

			const char * menuName = evn->menuName.c_str();

			if (evn->isOpen) //Menu opened
			{
				//LOG("Menu %s opened.", menuName);
				if (menuTypes.find(menuName) != menuTypes.end())
				{
					menuTypes[menuName] = true;
				}
			}
			else  //Menu closed
			{
				//LOG("Menu %s closed.", menuName);
				if (menuTypes.find(menuName) != menuTypes.end())
				{
					menuTypes[menuName] = false;
				}
				/*if(!isGameStopped())
				{
					_MESSAGE("WeaponFiredEvent sinks before count: %d", GetSingletonEventDispatcher(WeaponFiredEvent).eventSinks.count);

				GetSingletonEventDispatcher(WeaponFiredEvent).AddEventSink(&TactsuitVR::g_WeaponFiredEventHandler);

				_MESSAGE("WeaponFiredEvent sinks after count: %d", GetSingletonEventDispatcher(WeaponFiredEvent).eventSinks.count);
				}*/
			}

			return kEvent_Continue;
		};

		static void Register()
		{
			static auto * pHandler = new MenuOpenCloseHandler();
			(*g_ui)->menuOpenCloseEventSource.AddEventSink(pHandler); //V1.10.26
		}
	};	
}