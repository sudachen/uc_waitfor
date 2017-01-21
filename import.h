
#pragma once
#include <uccm/board.h>
#include <~sudachen/uc_irq/import.h>

#pragma uccm require(end) += [@inc]/~sudachen/uc_waitfor/uc_waitfor.c

enum
{
    UC_ACTIVATE_BY_TIMER  = 1,
    UC_ACTIVATE_BY_PROBE  = 2,
    UC_ACTIVATE_BY_SIGNAL = 3,
};

enum
{
    UC_EVENT_NOID = 0,
};

typedef struct UcEvent UcEvent;
typedef struct UcEventSet UcEventSet;
typedef void (*UcEventCallback)(UcEvent *ev);
typedef bool (*UcEventProbe)(UcEvent *ev);

union uc_waitfor$EventTrig
{
    UcEventProbe    probe;
    uint32_t        onTick;
};

struct uc_waitfor$EventOpt
{
   uint32_t repeat: 1;
   uint32_t signalled: 1;
   uint32_t kind: 2;
   uint32_t delay: 28;
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

#define UC_RTC_REPEAT_EVENT(Delay) { NULL, NULL, {.onTick=0}, {.kind = UC_ACTIVATE_BY_TIMER, .repeat = 1, .signalled = 0, .delay = (Delay)} }
