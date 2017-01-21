
#pragma once
#include <uccm/board.h>
#include <~sudachen/uc_irq/import.h>

#pragma uccm require(end) += [@inc]/~sudachen/uc_waitfor/uc_waitfor.c

enum
{
    UC_EVENT_BY_TIMER = 1,
};

enum
{
    UC_EVENT_ANYID = 0,
};

typedef struct UcEvent UcEvent;
typedef struct UcEventSet UcEventSet;
typedef void (*UcEventCallback)(UcEvent *ev);
typedef bool (*UcEventProbe)(UcEvent *ev);

union uc_waitfor$EventTrig
{
    UcEventProbe    probe;
    uint32_t        delay;
};

struct uc_waitfor$EventOpt
{
   uint8_t id;
   uint8_t repeat: 1;
   uint8_t signalled: 1;
   uint8_t kind: 3;
};

struct UcEvent
{
    UcEvent *next;
    UcEventCallback callback;
    union uc_waitfor$EventTrig t;
    struct uc_waitfor$EventOpt o;
};

struct UcEventSet
{
    UcEvent *timedEvents;
    UcEvent *otherEvents;
};

#define UC_EMPTY_EVENT_SET { NULL, NULL }

void ucAdd_Event(UcEventSet *evset, UcEvent* evt);
void ucDel_Event(UcEventSet *evset, UcEvent* evt);
void ucSignal_Event(UcEventSet *evset, UcEvent* evt);
UcEvent *ucWaitFor_Event(UcEventSet *evset);

__Forceinline
union uc_waitfor$EventTrig uc_waitfor$probeEventTrig(UcEventProbe probe)
{
    union uc_waitfor$EventTrig o = {.probe = probe};
    return o;
}

__Forceinline
union uc_waitfor$EventTrig uc_waitfor$timeEventTrig(uint32_t delayMs)
{
    union uc_waitfor$EventTrig o = {.delay = delayMs};
    return o;
}

__Forceinline
struct uc_waitfor$EventOpt uc_waitfor$rtcEventOpt(uint8_t id, bool repeat)
{
    struct uc_waitfor$EventOpt o = {.id = id, .kind = UC_EVENT_BY_TIMER, .repeat = repeat?1:0};
    return o;
}

#define UC_RTC_REPEAT_EVENT(Id,Delay) { NULL, NULL, uc_waitfor$timeEventTrig(Delay), uc_waitfor$rtcEventOpt(Id,true) }
