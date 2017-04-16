
#pragma once
#include <uccm/board.h>
#include <~sudachen/uc_irq/import.h>

#pragma uccm require(source) += [@inc]/~sudachen/uc_waitfor/uc_waitfor.c

#ifdef __nRF5x_UC__
#pragma uccm require(source) += [@inc]/~sudachen/uc_waitfor/uc_nrf_timer.c
#endif

enum
{
    ACTIVATE_BY_TIMER    = 0,
    ACTIVATE_BY_PROBE    = 1,
    ACTIVATE_BY_SIGNAL   = 2,
    CALLBACK_ON_COMPLETE = 3,
};

#define EVENT_ID_NONE   0
#define EVENT_ID_FIRST  3

#pragma uccm file(uccm_dynamic_defs.h) ~= #define EVENT_ID_TIMER ({#EVENT_ID:1} + EVENT_ID_FIRST)\n
#pragma uccm file(uccm_dynamic_defs.h) ~= #define EVENT_ID_NRFTIMER ({#EVENT_ID:1} + EVENT_ID_FIRST)\n

#define EVENT_IS_BY_TIMER(E) ((E)->o.id == EVENT_ID_TIMER)

typedef struct Event Event;
typedef void (*EventCallback)(Event *ev);
typedef bool (*EventProbe)(Event *ev);

union uc_waitfor$EventTrig
{
    EventProbe  probe;
    uint32_t    onTick;
    struct
    {
        bool    signalled;
        bool    completed;
    } is;
};

#define MAX_TIMER_DELAY (1u<<22)

struct uc_waitfor$EventOpt
{
   uint32_t id: 7;
   uint32_t repeat: 1;
   uint32_t kind: 2;
   uint32_t delay: 22;
};

struct Event
{
    Event *next;
    EventCallback callback;
    union uc_waitfor$EventTrig t;
    struct uc_waitfor$EventOpt o;
};

extern const Event uc_waitfor$Nil;
#define EVENT_LIST_NIL ((Event*)&uc_waitfor$Nil)

void list_event(struct Event *);
void unlist_event(struct Event *);
void unlist_allEvents(uint32_t id);
void signal_event(struct Event *);
void complete_event(struct Event *);
Event *wait_forEvent(void);

__Forceinline
bool is_unlistedEvent(struct Event *e)
{
    __Assert(e != NULL);
    return e->next == NULL;
}

__Forceinline
bool is_listedEvent(struct Event *e)
{
    __Assert(e != NULL);
    return e->next != NULL;
}

#define RTC_REPEAT_EVENT(Delay) { NULL, NULL, {0}, {.id = EVENT_ID_TIMER, .kind = ACTIVATE_BY_TIMER, .repeat = 1, .delay = (Delay)} }
#define RTC_ONESHOT_EVENT(Delay) { NULL, NULL, {0}, {.id = EVENT_ID_TIMER, .kind = ACTIVATE_BY_TIMER, .repeat = 0, .delay = (Delay)} }
