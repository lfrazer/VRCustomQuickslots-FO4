#pragma once

#include "f4se/GameTypes.h"

class Actor;
class TESObjectREFR;

enum EventResult
{
	kEvent_Continue = 0,
	kEvent_Abort
};


// 08
template <typename T>
class BSTEventSink
{
public:
	virtual ~BSTEventSink() { };
	virtual	EventResult	ReceiveEvent(T * evn, void * dispatcher) { return kEvent_Continue; }; // pure
//	void	** _vtbl;	// 00
};

struct BGSInventoryListEvent
{
	struct Event
	{

	};
};

struct MenuOpenCloseEvent
{
	BSFixedString	menuName;
	bool			isOpen;
};

struct MenuModeChangeEvent
{

};

struct UserEventEnabledEvent
{

};

struct RequestHUDModesEvent
{

};

struct PerkEntryUpdatedEvent
{
	struct PerkValueEvents
	{

	};
};

struct ApplyColorUpdateEvent
{

};

struct BSMovementDataChangedEvent
{

};

struct BSTransformDeltaEvent
{

};

struct BSSubGraphActivationUpdate
{

};

struct bhkCharacterMoveFinishEvent
{

};

struct bhkNonSupportContactEvent
{

};

struct bhkCharacterStateChangeEvent
{

};

struct ChargenCharacterUpdateEvent
{

};

struct QuickContainerStateEvent
{

};

struct TESCombatEvent 
{
	TESObjectREFR	* source;	// 00
	TESObjectREFR	* target;	// 04
	UInt32			state;		// 08
};

struct TESDeathEvent
{
	TESObjectREFR	* source;	// 00
};

struct TESObjectLoadedEvent
{
	UInt32	formId;
	UInt8	loaded; // 01 - loaded, 00 - unloaded
};

struct TESLoadGameEvent
{

};

struct TESFurnitureEvent
{
	Actor			* actor;
	TESObjectREFR	* furniture;
	bool			isGettingUp;
};

struct TESInitScriptEvent
{
	TESObjectREFR * reference;
};

struct TESContainerChangedEvent {
	UInt32          sourceID;
	UInt32          targetID;
	UInt32          formID;
	UInt32          count;
	UInt32          refID;
	UInt8           uniqueID;
};
STATIC_ASSERT(sizeof(TESContainerChangedEvent) == 0x18);

struct BGSOnPlayerFireWeaponEvent
{
	TESForm* weapon;
};

struct WeaponFiredEvent
{
	void*                          weapon;
	TESObjectREFR*                          source;
	void*                                   unk10; // Projectile?
};
STATIC_ASSERT(sizeof(WeaponFiredEvent) == 0x18);

struct TESHitEvent
{
	
};

struct PlayerAmmoCountEvent
{
	UInt32                          ammoCount;
	UInt32                          ammoReserves;
};

struct PlayerWeaponReloadEvent
{
	//
};

struct PlayerSetWeaponStateEvent
{
	
};

struct MeleeAttackJustReleasedEvent
{

};

struct TESEquipEvent
{
	TESObjectREFR*	ref;		// 00
	UInt32			FormID;		// 08
	UINT32			unk0C;		// 0C
	char            pad10[4];	// 10
	bool			equip;		// 14
								//...
};

// 08
template <typename EventT>
class BSTEventDispatcher
{
public:
	typedef BSTEventSink<EventT> SinkT;

	bool AddEventSink(SinkT * sink)
	{
		SimpleLocker locker(&lock);

		// Check for duplicate first
		for (int i = 0; i < eventSinks.count; i++)
		{
			if(eventSinks[i] == sink)
				return false;
		}

		eventSinks.Insert(0, sink);
		return true;
	}

	void RemoveEventSink(SinkT * sink)
	{
		SimpleLocker locker(&lock);

		for (int i = 0; i < eventSinks.count; i++)
		{
			if(eventSinks[i] == sink) {
				eventSinks.Remove(i);
				break;
			}
		}
	}

	

	SimpleLock			lock;				// 000
	tArray<SinkT*>		eventSinks;			// 008
	tArray<SinkT*>		addBuffer;			// 020
	tArray<SinkT*>		removeBuffer;		// 038
	bool				stateFlag;			// 050
	char				pad[3];
};

class BSTGlobalEvent
{
public:
	virtual ~BSTGlobalEvent();

	template <typename T>
	class EventSource
	{
	public:
		virtual ~EventSource();

		// void ** _vtbl;                           // 00
		UInt64                  unk08;              // 08
		BSTEventDispatcher<T>   eventDispatcher;    // 10
	};

	// void ** _vtbl;                               // 00
	UInt64    unk08;                                // 08
	UInt64    unk10;                                // 10
	tArray<EventSource<void*>*> eventSources;       // 18
};

extern RelocPtr <BSTGlobalEvent*> g_globalEvents;
extern RelocPtr <BSTGlobalEvent::EventSource<ApplyColorUpdateEvent>*> g_colorUpdateDispatcher;

template<typename EventT>
BSTEventDispatcher<EventT> * GetEventDispatcher() { };

#define DECLARE_EVENT_DISPATCHER(Event, address) \
template<> inline BSTEventDispatcher<Event> * GetEventDispatcher() \
{ \
	typedef BSTEventDispatcher<Event> * (*_GetEventDispatcher)(); \
	RelocAddr<_GetEventDispatcher> GetDispatcher(address); \
	return GetDispatcher(); \
}

template<typename EventT>
BSTEventDispatcher<EventT> GetGlobalEventDispatcher() { };

template<typename EventT>
BSTEventDispatcher<EventT>* GetSingletonEventDispatcher_Internal() { };

#define DECLARE_GLOBAL_EVENT_DISPATCHER(Event, address)\
template<> inline BSTEventDispatcher<Event> GetGlobalEventDispatcher() {\
    typedef BSTGlobalEvent::EventSource<Event>* (_GetGlobalEventDispatcher);\
    RelocPtr <_GetGlobalEventDispatcher> GetDispatcher(address);\
    return (*GetDispatcher)->eventDispatcher;\
}

#define DECLARE_SINGLETON_EVENT_DISPATCHER(Event, address)\
template<> inline BSTEventDispatcher<Event>* GetSingletonEventDispatcher_Internal() {\
    typedef BSTEventDispatcher<Event>* (_GetSingletonEventDispatcher_Internal);\
    RelocAddr <_GetSingletonEventDispatcher_Internal> GetDispatcher(address);\
    return GetDispatcher;\
}

#define DECLARE_EVENT_CLASS(Event)\
class Event##Handler : public BSTEventSink<Event> {\
public:\
    virtual ~Event##Handler() { };\
    virtual	EventResult	ReceiveEvent(Event* evn, void* dispatcher) override;\
};\
extern Event##Handler g_##Event##Handler

#define GetSingletonEventDispatcher(Event) (*GetSingletonEventDispatcher_Internal<Event>())


// A548D71D41C7C2E9D21B25E06730FB911FC31F47+B4 (struct+A8)
DECLARE_EVENT_DISPATCHER(TESCombatEvent, 0x0042A7F0)      //17900 diff with f4se

// A548D71D41C7C2E9D21B25E06730FB911FC31F47+118 (struct+D0)
DECLARE_EVENT_DISPATCHER(TESDeathEvent, 0x0042AC50)

// A548D71D41C7C2E9D21B25E06730FB911FC31F47+1A4 (struct+110)
DECLARE_EVENT_DISPATCHER(TESFurnitureEvent, 0x0042B330)

// A548D71D41C7C2E9D21B25E06730FB911FC31F47+1F4 (struct+130)
DECLARE_EVENT_DISPATCHER(TESLoadGameEvent, 0x0042B5B0)

// A548D71D41C7C2E9D21B25E06730FB911FC31F47+238 (struct+148)
DECLARE_EVENT_DISPATCHER(TESObjectLoadedEvent, 0x0042B8D0)

// 22274238010A92A75A0E77127FF6D54FDBC6F943+821 (inside call)
DECLARE_EVENT_DISPATCHER(TESInitScriptEvent, 0x0042B470)


DECLARE_EVENT_DISPATCHER(TESContainerChangedEvent, 0x0042ABB0);   

DECLARE_EVENT_DISPATCHER(TESHitEvent, 0x0042CD70);

DECLARE_EVENT_DISPATCHER(TESEquipEvent, 0x0042AF70);

DECLARE_GLOBAL_EVENT_DISPATCHER(PlayerWeaponReloadEvent, 0x05AC1378);

DECLARE_GLOBAL_EVENT_DISPATCHER(PlayerSetWeaponStateEvent, 0x05AC1398);
DECLARE_GLOBAL_EVENT_DISPATCHER(MeleeAttackJustReleasedEvent, 0x05AC0F88);


DECLARE_SINGLETON_EVENT_DISPATCHER(WeaponFiredEvent, 0x5945AC0);    // 'Weapon Equip Slot' | rax, qword ptr cs:unk_14*

/*
__int64 *sub_1404298F0();
__int64 *sub_140429990();
__int64 *sub_140429A30();
__int64 *sub_140429AD0();
__int64 *sub_140429B70();  
__int64 *sub_140429C10();
__int64 *sub_140429CB0();  
__int64 *sub_140429D50();  
__int64 *sub_140429DF0();  
__int64 *sub_140429E90();
__int64 *sub_140429F30();
__int64 *sub_140429FD0();
__int64 *sub_14042A070();  
__int64 *sub_14042A110();
__int64 *sub_14042A1B0();
__int64 *sub_14042A250();
__int64 *sub_14042A2F0();
__int64 *sub_14042A390();  TESActivateEvent
__int64 *sub_14042A430();
__int64 *sub_14042A4D0();
__int64 *sub_14042A570();  TESBookReadEvent
__int64 *sub_14042A610();  TESCellAttachDetachEvent
__int64 *sub_14042A6B0();  TESCellFullyLoadedEvent
__int64 *sub_14042A750();
__int64 *sub_14042A7F0();  TESCombatEvent
__int64 *sub_14042A890();
__int64 *sub_14042A930();
__int64 *sub_14042A9D0();
__int64 *sub_14042AA70();
__int64 *sub_14042AB10();
__int64 *sub_14042ABB0();  TESContainerChangedEvent
__int64 *sub_14042AC50();  TESDeathEvent
__int64 *sub_14042ACF0();
__int64 *sub_14042AD90();
__int64 *sub_14042AE30();
__int64 *sub_14042AED0();  TESEnterSneakingEvent
__int64 *sub_14042AF70();  TESEquipEvent
__int64 *sub_14042B010();
__int64 *sub_14042B0B0();
__int64 *sub_14042B150();
__int64 *sub_14042B1F0();
__int64 *sub_14042B290();
__int64 *sub_14042B330();  TESFurnitureEvent
__int64 *sub_14042B3D0();  TESGrabReleaseEvent
__int64 *sub_14042B470();  TESInitScriptEvent
__int64 *sub_14042B510();  TESLimbCrippleEvent
__int64 *sub_14042B5B0();  TESLoadGameEvent
__int64 *sub_14042B650();
__int64 *sub_14042B6F0();  TESLockChangedEvent
__int64 *sub_14042B790();  TESMagicEffectApplyEvent
__int64 *sub_14042B830();
__int64 *sub_14042B8D0();  TESObjectLoadedEvent
__int64 *sub_14042B970();
__int64 *sub_14042BA10();
__int64 *sub_14042BAB0();
__int64 *sub_14042BB50();
__int64 *sub_14042BBF0();
__int64 *sub_14042BC90();
__int64 *sub_14042BD30();
__int64 *sub_14042BDD0();
__int64 *sub_14042BE70();
__int64 *sub_14042BF10();
__int64 *sub_14042BFB0();
__int64 *sub_14042C050();
__int64 *sub_14042C0F0();
__int64 *sub_14042C190();
__int64 *sub_14042C230();
__int64 *sub_14042C2D0();
__int64 *sub_14042C370();
__int64 *sub_14042C410();  TESSleepStartEvent
__int64 *sub_14042C4B0();  TESSleepStopEvent
__int64 *sub_14042C550();
__int64 *sub_14042C5F0();
__int64 *sub_14042C690();
__int64 *sub_14042C730();
__int64 *sub_14042C7D0();
__int64 *sub_14042C870();
__int64 *sub_14042C910();
__int64 *sub_14042C9B0();
__int64 *sub_14042CA50();
__int64 *sub_14042CAF0();  TESWaitStartEvent
__int64 *sub_14042CB90();  TESWaitStopEvent
__int64 *sub_14042CC30();
__int64 *sub_14042CCD0();
__int64 *sub_14042CD70();  TESHitEvent

*/
