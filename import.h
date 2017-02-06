
#pragma once
#include <uccm/board.h>
#include <~sudachen/uc_irq/import.h>

#pragma uccm require(source) += [@inc]/~sudachen/uc_waitfor/uc_waitfor.c

#ifdef __nRF5x_UC__
#pragma uccm require(source) += [@inc]/~sudachen/uc_waitfor/uc_nrf_timer.c
#endif

enum
{
    UC_ACTIVATE_BY_TIMER    = 0,
    UC_ACTIVATE_BY_PROBE    = 1,
    UC_ACTIVATE_BY_SIGNAL   = 2,
    UC_CALLBACK_ON_COMPLETE = 3,
};

#define UC_EVENT_ID_NONE   0
#define UC_EVENT_ID_FIRST  3

#pragma uccm file(uccm_dynamic_defs.h) ~= #define UC_EVENT_ID_TIMER ({#UC_EVENT_ID:1} + UC_EVENT_ID_FIRST)\n
#pragma uccm file(uccm_dynamic_defs.h) ~= #define UC_EVENT_ID_NRFTIMER ({#UC_EVENT_ID:1} + UC_EVENT_ID_FIRST)\n

typedef struct UcEvent UcEvent;
typedef struct UcEventSet UcEventSet;
typedef void (*UcEventCallback)(UcEvent *ev);
typedef bool (*UcEventProbe)(UcEvent *ev);

union uc_waitfor$EventTrig
{
    UcEventProbe    probe;
    uint32_t        onTick;
    struct
    {
        bool        signalled;
        bool        completed;
    } is;
};

#define UC_MAX_TIMER_DELAY (1u<<22)

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

void list_event(struct UcEvent *);
void unlist_event(struct UcEvent *);
void unlist_allEvents(uint32_t id);
void signal_event(struct UcEvent *);
void complete_event(struct UcEvent *);
UcEvent *wait_forEvent(void);

__Forceinline
bool is_unlistedEvent(struct UcEvent *e)
{
    return e->next == NULL;
}

__Forceinline
bool is_listedEvent(struct UcEvent *e)
{
    return e->next != NULL;
}

#define UC_RTC_REPEAT_EVENT(Delay) { NULL, NULL, {.onTick=UC_EVENT_ID_TIMER}, {.id = 0, .kind = UC_ACTIVATE_BY_TIMER, .repeat = 1, .delay = (Delay)} }
#define UC_RTC_ONESHOT_EVENT(Delay) { NULL, NULL, {.onTick=UC_EVENT_ID_TIMER}, {.id = 0, .kind = UC_ACTIVATE_BY_TIMER, .repeat = 0, .delay = (Delay)} }
