
#pragma once
#include <uccm/board.h>
#include <~sudachen/uc_irq/import.h>

#pragma uccm require(source) += [@inc]/~sudachen/uc_waitfor/uc_waitfor.c

#ifdef __nRF5x_UC__
#pragma uccm require(source) += [@inc]/~sudachen/uc_waitfor/uc_nrf_timer.c
#endif

#define EVENT_ID_NONE   0
#define EVENT_ID_FIRST  3

#pragma uccm file(uccm_dynamic_defs.h) ~= \
  #define EVENT_ID_TIMER ({#EVENT_ID:1} + EVENT_ID_FIRST)\n
#pragma uccm file(uccm_dynamic_defs.h) ~= \
  #define EVENT_ID_NRFTIMER ({#EVENT_ID:1} + EVENT_ID_FIRST)\n
#pragma uccm file(uccm_dynamic_defs.h) ~= \
  #define EVENT_ID_CR ({#EVENT_ID:1} + EVENT_ID_FIRST)\n

#define EVENT_IS_BY_TIMER(E) ((E)->o.id == EVENT_ID_TIMER)

typedef struct Event Event;
typedef void (*EventCallback)(Event *ev);
typedef bool (*EventProbe)(Event *ev);

enum
{
    ACTIVATE_BY_TIMER    = 0,
    ACTIVATE_BY_PROBE    = 1,
    ACTIVATE_BY_SIGNAL   = 2,
    CALLBACK_ON_COMPLETE = 3,
};

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

extern const void* uc_waitfor$Nil;
#define EVENT_LIST_NIL ((Event*)&uc_waitfor$Nil)

void list_event(struct Event *);
void unlist_event(struct Event *);
void unlist_allEvents(uint32_t id);
void signal_event(struct Event *);
void complete_event(struct Event *);
Event *wait_forEvent(void);
__Forceinline Event *wait_event(void) { return wait_forEvent(); }

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

#ifdef CR_USES_SWITCH_CASE
typedef int CrPtr;
#else
typedef void *CrPtr;
#endif

typedef struct CrContext CrContext;
struct CrContext
{
    void(*route)(CrContext*);
    CrContext *next;
    CrPtr crPtr;
    Event *event;
    Event e;
};

extern const void* uc_waitfor$CrNil;
#define CR_LIST_NIL ((CrContext*)&uc_waitfor$CrNil)

#ifdef CR_USES_SWITCH_CASE
#define CR_SWITCH(Cr)      switch(Cr->crPtr) if (0) {;} else case 0:
#define CR_MSDELAY(Cr,Ms)  wait_crMs(Ms,Cr,__LINE__); return; case __LINE__:
#define CR_AWAIT(Cr,Ev,Ms) wait_crEvent(Ev,Ms,Cr,__LINE__); return; case __LINE__:
#define CR_YIELD(Cr)       yield_cr(Cr,__LINE__); return; case __LINE__:
#define CR_SLEEP(Cr)       suspend_cr(Cr,__LINE__); return; case __LINE__:
#else
#ifndef __C_HIGHLIGHT
#define ADDRESS_OF_LABEL(L) &&L
#define GOTO_LABEL_PTR(L) goto *L
#endif
#define CR_SWITCH(Cr)      if ( (Cr)->crPtr != NULL ) { GOTO_LABEL_PTR((Cr)->crPtr); } else
#define CR_MSDELAY(Cr,Ms)  wait_crMs(Ms,Cr,ADDRESS_OF_LABEL(C_LOCAL_ID(_CR_))); return; C_LOCAL_ID(_CR_):
#define CR_AWAIT(Cr,Ev,Ms) wait_crEvent(Ev,Ms,Cr,ADDRESS_OF_LABEL(C_LOCAL_ID(_CR_))); return; C_LOCAL_ID(_CR_):
#define CR_YIELD(Cr)       yield_cr(Cr,ADDRESS_OF_LABEL(C_LOCAL_ID(_CR_))); return; C_LOCAL_ID(_CR_):
#define CR_SLEEP(Cr)       suspend_cr(Cr,ADDRESS_OF_LABEL(C_LOCAL_ID(_CR_))); return; C_LOCAL_ID(_CR_):
#endif

void wait_crMs(uint32_t ms, CrContext *cr, CrPtr ptr);
void wait_crEvent(Event *e, uint32_t ms, CrContext *cr, CrPtr ptr);
void suspend_cr(CrContext *cr, CrPtr ptr);
void yield_cr(CrContext *cr, CrPtr ptr);
void resume_cr(CrContext *cr);

void call_cr(CrContext *cr);
Event *arm_cr(CrContext *cr);

typedef struct RtcEvent RtcEvent;
struct RtcEvent
{
    Event e;
};

#define RTC_REPEAT_EVENT_w_CALLBACK(Delay,Callback) \
    { NULL, Callback, {0}, \
        {.id = EVENT_ID_TIMER, \
         .kind = ACTIVATE_BY_TIMER, \
         .repeat = 1, \
         .delay = (Delay)} }

#define RTC_ONESHOT_EVENT_w_CALLBACK(Delay,Callback) \
    { NULL, Callback, {0}, \
        {.id = EVENT_ID_TIMER, \
         .kind = ACTIVATE_BY_TIMER, \
         .repeat = 0, \
         .delay = (Delay)} }

#define RTC_REPEAT_EVENT(Delay) RTC_REPEAT_EVENT_w_CALLBACK(Delay,NULL);
#define RTC_ONESHOT_EVENT(Delay) RTC_ONESHOT_EVENT_w_CALLBACK(Delay,NULL);
