
#pragma once
#include <uccm/board.h>
#include <~sudachen/uc_irq/import.h>

#pragma uccm require(source) += [@inc]/~sudachen/uc_waitfor/uc_waitfor.c

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
    bool            signalled;
};

struct uc_waitfor$EventOpt
{
   uint32_t id: 7;
   uint32_t repeat: 1;
   uint32_t kind: 2;
   uint32_t delay: 22;
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

extern const UcEvent uc_waitfor$Nil;
#define UC_EVENT_LIST_NIL ((UcEvent*)&uc_waitfor$Nil)
#define UC_EMPTY_EVENT_SET { UC_EVENT_LIST_NIL, UC_EVENT_LIST_NIL }

void ucAdd_Event(UcEventSet *evset, UcEvent* evt);
void ucDel_Event(UcEventSet *evset, UcEvent* evt);
void ucSignal_Event(UcEventSet *evset, UcEvent* evt);
UcEvent *ucWaitFor_Event(UcEventSet *evset);

#define UC_RTC_REPEAT_EVENT(Delay) { NULL, NULL, {.onTick=0}, {.id = 0, .kind = UC_ACTIVATE_BY_TIMER, .repeat = 1, .delay = (Delay)} }
