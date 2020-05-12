/*
	VRCustomQuickslots - VR Custom Quickslots F4SE extension for Fallout4 VR
	Copyright (C) 2020 L Frazer
	https://github.com/lfrazer

	This file is part of VRCustomQuickslots.

	VRCustomQuickslots is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	VRCustomQuickslots is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License
	along with VRCustomQuickslots.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "f4se/PluginAPI.h"		// SKSE plugin API
#include "f4se_common/f4se_version.h"
#include "MenuChecker.h"

#include "common/IDebugLog.h"
#include <shlobj.h>				// for use of CSIDL_MYCODUMENTS
#include <algorithm>

#include "quickslots.h"
#include "quickslotutil.h"


static PluginHandle					g_pluginHandle = kPluginHandle_Invalid;
static F4SEPapyrusInterface         * g_papyrus = NULL;
static F4SEMessagingInterface		* g_messaging = NULL;
F4SETaskInterface				* g_task = NULL;

PapyrusVRAPI*	g_papyrusvr = nullptr;
CQuickslotManager* g_quickslotMgr = nullptr;
CUtil*			g_Util = nullptr;
vr::IVRSystem*	g_VRSystem = nullptr; // only set by new RAW api from Hook Mgr

const char* kConfigFile = "Data\\F4SE\\Plugins\\vrcustomquickslots.xml";
const char* kConfigFileUniqueId = "Data\\F4SE\\Plugins\\vrcustomquickslots_%s.xml";
const int VRCUSTOMQUICKSLOTS_VERSION = 5;

extern "C" {

	void OnF4SEMessage(F4SEMessagingInterface::Message* msg);

	bool F4SEPlugin_Query(const F4SEInterface * f4se, PluginInfo * info) {	// Called by SKSE to learn about this plugin and check that it's safe to load it
		gLog.OpenRelative(CSIDL_MYDOCUMENTS, "\\My Games\\Fallout4VR\\F4SE\\VRCustomQuickslots.log");
		gLog.SetPrintLevel(IDebugLog::kLevel_Error);
		gLog.SetLogLevel(IDebugLog::kLevel_DebugMessage);

		// populate info structure
		info->infoVersion = PluginInfo::kInfoVersion;
		info->name = "VRCustomQuickslots-FO4";
		info->version = VRCUSTOMQUICKSLOTS_VERSION;

		// store plugin handle so we can identify ourselves later
		g_pluginHandle = f4se->GetPluginHandle();

		if (f4se->isEditor)
		{
			_MESSAGE("loaded in editor, marking as incompatible");

			return false;
		}
		else if (f4se->runtimeVersion < CURRENT_RELEASE_RUNTIME)
		{
			_MESSAGE("unsupported runtime version %08X", f4se->runtimeVersion);

			return false;
		}

		// ### do not do anything else in this callback
		// ### only fill out PluginInfo and return true/false

		// supported runtime version
		return true;
	}

	bool F4SEPlugin_Load(const F4SEInterface * f4se) 	// Called by F4SE to load this plugin
	{

		_MESSAGE("VRCustomQuickslots loaded");

		g_papyrus = (F4SEPapyrusInterface *)f4se->QueryInterface(kInterface_Papyrus);

		g_task = (F4SETaskInterface *)f4se->QueryInterface(kInterface_Task);

		//Registers for SKSE Messages (PapyrusVR probably still need to load, wait for SKSE message PostLoad)
		_MESSAGE("Registering for F4SE messages");
		g_messaging = (F4SEMessagingInterface*)f4se->QueryInterface(kInterface_Messaging);
		g_messaging->RegisterListener(g_pluginHandle, "F4SE", OnF4SEMessage);



		//wait for PapyrusVR init (during PostPostLoad SKSE Message)

		return true;
	}

	// Legacy API event handlers
	void OnVRButtonEvent(PapyrusVR::VREventType type, PapyrusVR::EVRButtonId buttonId, PapyrusVR::VRDevice deviceId)
	{
		// Use button presses here
		if (type == PapyrusVR::VREventType_Pressed)
		{
			//_MESSAGE("VR Button press deviceId: %d buttonId: %d", deviceId, buttonId);

			g_quickslotMgr->ButtonPress(buttonId, deviceId);
		}
		else if (type == PapyrusVR::VREventType_Released)
		{
			g_quickslotMgr->ButtonRelease(buttonId, deviceId);
		}
	}

	void OnVRUpdateEvent(float deltaTime)
	{
		// Get render poses by default
		PapyrusVR::TrackedDevicePose* leftHandPose = g_papyrusvr->GetVRManager()->GetLeftHandPose();
		PapyrusVR::TrackedDevicePose* rightHandPose = g_papyrusvr->GetVRManager()->GetRightHandPose();
		PapyrusVR::TrackedDevicePose* hmdPose = g_papyrusvr->GetVRManager()->GetHMDPose();

		g_quickslotMgr->Update(hmdPose, leftHandPose, rightHandPose);

	}

	// New RAW API event handlers
	bool OnControllerStateChanged(vr::TrackedDeviceIndex_t unControllerDeviceIndex, const vr::VRControllerState_t* pControllerState, uint32_t unControllerStateSize, vr::VRControllerState_t* pOutputControllerState)
	{
		static uint64_t lastButtonPressedData[PapyrusVR::VRDevice_LeftController + 1] = {0}; // should be size 3 array at least for all 3 vr devices
		static_assert(PapyrusVR::VRDevice_LeftController + 1 >= 3, "lastButtonPressedData array size too small!");
		
		if (!g_quickslotMgr->IsTrackingDataValid())
		{
			return false;
		}

		vr::TrackedDeviceIndex_t leftcontroller = g_VRSystem->GetTrackedDeviceIndexForControllerRole(vr::ETrackedControllerRole::TrackedControllerRole_LeftHand);
		vr::TrackedDeviceIndex_t rightcontroller = g_VRSystem->GetTrackedDeviceIndexForControllerRole(vr::ETrackedControllerRole::TrackedControllerRole_RightHand);

		// NOTE: DO NOT check the packetNum on ControllerState, it seems to cause problems (maybe it should only be checked per controllers?) - at any rate, it seems to change every frame anyway.  

		PapyrusVR::VRDevice deviceId = PapyrusVR::VRDevice_Unknown;
			
		if (unControllerDeviceIndex == leftcontroller)
		{
			deviceId = PapyrusVR::VRDevice_LeftController;
		}
		else if(unControllerDeviceIndex == rightcontroller)
		{
			deviceId = PapyrusVR::VRDevice_RightController;
		}

		if (deviceId != PapyrusVR::VRDevice_Unknown)
		{
			//We get the activate button from config now so, getting it by function.
			const auto buttonId = g_quickslotMgr->GetActivateButton();
			const uint64_t buttonMask = ButtonMaskFromId((vr::EVRButtonId)buttonId);  // annoying issue where PapyrusVR and openvr enums are not type-compatible..

			// right now only check for trigger press.  In future support input binding?
			if (pControllerState->ulButtonPressed & buttonMask && !(lastButtonPressedData[deviceId] & buttonMask))
			{
				bool retVal = g_quickslotMgr->ButtonPress(buttonId, deviceId);

				if (retVal) // mask out input if we touched a quickslot (block the game from receiving it)
				{
					pOutputControllerState->ulButtonPressed &= ~buttonMask;
				}

				QSLOG_INFO("Trigger pressed for deviceIndex: %d deviceId: %d", unControllerDeviceIndex, deviceId);
			}
			else if (!(pControllerState->ulButtonPressed & buttonMask) && (lastButtonPressedData[deviceId] & buttonMask))
			{
				g_quickslotMgr->ButtonRelease(buttonId, deviceId);

				QSLOG_INFO("Trigger released for deviceIndex: %d deviceId: %d", unControllerDeviceIndex, deviceId);
			}
			
			// we need to block all inputs when button is held over top of a quickslot (check last button press and if controller is hovering over a quickslot)
			if (lastButtonPressedData[deviceId] & buttonMask && g_quickslotMgr->FindQuickslotByDeviceId(deviceId))
			{
				pOutputControllerState->ulButtonPressed &= ~buttonMask;
			}

			lastButtonPressedData[deviceId] = pControllerState->ulButtonPressed;
		}
		
		return true;
	}


	vr::EVRCompositorError OnGetPosesUpdate(VR_ARRAY_COUNT(unRenderPoseArrayCount) vr::TrackedDevicePose_t* pRenderPoseArray, uint32_t unRenderPoseArrayCount,
			VR_ARRAY_COUNT(unGamePoseArrayCount) vr::TrackedDevicePose_t* pGamePoseArray, uint32_t unGamePoseArrayCount)
	{
		// actually, we do need to copy the pose memory data here because ButtonPress/Release will refer to it by pointer later.
		static PapyrusVR::TrackedDevicePose sPoseData[vr::k_unMaxTrackedDeviceCount];

		// NOTE: PapyrusVR tracked pose structure must match structure from OpenVR.h exactly.  Be wary of OpenVR updates!  original code from artumino engineered like this ¯\_(@)_/¯
		PapyrusVR::TrackedDevicePose* hmdPose = nullptr;
		PapyrusVR::TrackedDevicePose* leftCtrlPose = nullptr;
		PapyrusVR::TrackedDevicePose* rightCtrlPose = nullptr;

		memcpy_s(sPoseData, sizeof(PapyrusVR::TrackedDevicePose) * vr::k_unMaxTrackedDeviceCount, pRenderPoseArray, sizeof(PapyrusVR::TrackedDevicePose) * unRenderPoseArrayCount);
		

		for (uint32_t i = 0; i < unRenderPoseArrayCount; ++i)
		{
			if (i == PapyrusVR::k_unTrackedDeviceIndex_Hmd)
			{
				hmdPose = (PapyrusVR::TrackedDevicePose*)&sPoseData[i];
			}
			else if (i == g_VRSystem->GetTrackedDeviceIndexForControllerRole(vr::ETrackedControllerRole::TrackedControllerRole_LeftHand))
			{
				leftCtrlPose = (PapyrusVR::TrackedDevicePose*)&sPoseData[i];
			}
			else if (i == g_VRSystem->GetTrackedDeviceIndexForControllerRole(vr::ETrackedControllerRole::TrackedControllerRole_RightHand))
			{
				rightCtrlPose = (PapyrusVR::TrackedDevicePose*)&sPoseData[i];
			}
		}

		// null check to avoid rare crash here
		if (hmdPose && rightCtrlPose && leftCtrlPose)
		{
			g_quickslotMgr->Update(hmdPose, leftCtrlPose, rightCtrlPose);
		}

		return vr::VRCompositorError_None;
	}

	//Listener for PapyrusVR Messages
	void OnPapyrusVRMessage(F4SEMessagingInterface::Message* msg)
	{
		if (msg)
		{
			if (msg->type == kPapyrusVR_Message_Init && msg->data)
			{
				_MESSAGE("PapyrusVR Init Message received with valid data, waiting for init.");
				g_papyrusvr = (PapyrusVRAPI*)msg->data;

			}
		}
	}

	static inline void CreateManagersAndRegister()
	{
		if (g_quickslotMgr == nullptr)
		{
			_MESSAGE("Creating Managers and Registering for PapyrusVR messages from FO4VRTools");  // This log msg may happen before XML is loaded

			// Create objects
			g_Util = new CUtil();
			g_quickslotMgr = new CQuickslotManager();

			_MESSAGE("Register Menu event handler");

			MenuChecker::MenuOpenCloseHandler::Register();

			const bool regResult = g_messaging->RegisterListener(g_pluginHandle, "FO4VRTools", OnPapyrusVRMessage);

			if (regResult)
			{
				_MESSAGE("Registered for PapyrusVR Messages");
			}
			else
			{
				_MESSAGE("Failed to register for PapyrusVR messages!");
			}
		}
	}

	//Listener for SKSE Messages
	void OnF4SEMessage(F4SEMessagingInterface::Message* msg)
	{
		// path to XML config file for unique character ID - will be set on PreLoadGame message
		static char sConfigUniqueIdPath[MAX_PATH] = { 0 };

		if (msg)
		{
			// Debug log
			_MESSAGE("Got F4SE Message Type=%u from sender %s", msg->type, msg->sender ? msg->sender : "Unknown");

			if ( msg->type == F4SEMessagingInterface::kMessage_GameLoaded) //msg->type == F4SEMessagingInterface::kMessage_PostLoad ||
			{
				//if (g_papyrusvr)
				{
					CreateManagersAndRegister();

					_MESSAGE("Initializing VRCustomQuickslots data.");
					_MESSAGE("Reading XML quickslots config vrcustomquickslots.xml");
					g_quickslotMgr->ReadConfig(kConfigFile);

					QSLOG("XML config load complete.");
					QSLOG("VRCustomQuickslots Plugin version: %d", VRCUSTOMQUICKSLOTS_VERSION);

					OpenVRHookManagerAPI* hookMgrAPI = RequestOpenVRHookManagerObject();
					if (hookMgrAPI && !g_quickslotMgr->DisableRawAPI())
					{
						QSLOG("Using new RAW OpenVR Hook API.");

						g_quickslotMgr->SetHookMgr(hookMgrAPI);
						g_VRSystem = hookMgrAPI->GetVRSystem(); // setup VR system before callbacks
						hookMgrAPI->RegisterControllerStateCB(OnControllerStateChanged);
						hookMgrAPI->RegisterGetPosesCB(OnGetPosesUpdate);

					}
					else if(g_papyrusvr)
					{
						QSLOG("Using legacy PapyrusVR API.");

						//Registers for PoseUpdates
						g_papyrusvr->GetVRManager()->RegisterVRButtonListener(OnVRButtonEvent);
						g_papyrusvr->GetVRManager()->RegisterVRUpdateListener(OnVRUpdateEvent);
					}
					else
					{
						_MESSAGE("PapyrusVR was not initialized!");
					}
				}

			}
			else if (msg->type == F4SEMessagingInterface::kMessage_PreLoadGame)
			{
				char saveGameDataFile[MAX_PATH] = { 0 };
				const int NUM_SAVE_TOKENS = 20;
				char* saveNameTokens[NUM_SAVE_TOKENS] = { 0 };
				char* strtok_context;
				int currTokenIdx = 0;

				strncpy_s(saveGameDataFile, (char*)msg->data, std::min<size_t>(MAX_PATH - 1, msg->dataLen));
				
				// split up the string to extract the unique hash from the savefile name that identifies a save game for a specific character
				char* currTok = strtok_s(saveGameDataFile, "_", &strtok_context);
				saveNameTokens[currTokenIdx++] = currTok;
				while (currTok != nullptr)
				{
					currTok = strtok_s(nullptr, "_", &strtok_context);
					if (currTok && currTokenIdx < NUM_SAVE_TOKENS)
					{
						saveNameTokens[currTokenIdx++] = currTok;
					}
				}
				
				// index 1 will always be the uniqueId based on the format of save game names
				const char* playerUID = saveNameTokens[1];
				_MESSAGE("SKSE PreLoadGame message received, save file name: %s UniqueID: %s", (char*)msg->data, playerUID ? playerUID : "NULL");

				
				if (playerUID)
				{
					sprintf_s(sConfigUniqueIdPath, kConfigFileUniqueId, playerUID);
					
					// check if unique ID config file exists
					FILE* fp = nullptr;
					fopen_s(&fp, sConfigUniqueIdPath, "r");
					if (fp)
					{
						fclose(fp); // if so, close handle and try to load XML
						g_quickslotMgr->ReadConfig(sConfigUniqueIdPath);
						QSLOG("XML config for unique playerID load complete.");
					}
					else
					{
						QSLOG("Unable to find player specific config file: %s", sConfigUniqueIdPath);
					}
				}




			}
			else if (msg->type == F4SEMessagingInterface::kMessage_PostLoadGame || msg->type == F4SEMessagingInterface::kMessage_NewGame)
			{
				QSLOG("F4SE PostLoadGame or NewGame message received, type: %d", msg->type);
				g_quickslotMgr->SetInGame(true);
			}
			else if (msg->type == F4SEMessagingInterface::kMessage_PreSaveGame)
			{
				QSLOG("F4SE SaveGame message received.");

				if (g_quickslotMgr->AllowEdit())
				{
					if (strlen(sConfigUniqueIdPath) > 0) // if we have a unique character config path, write to that instead
					{
						g_quickslotMgr->WriteConfig(sConfigUniqueIdPath);
					}
					else
					{
						g_quickslotMgr->WriteConfig(kConfigFile);
					}
				}
			}
		}
	}



};
