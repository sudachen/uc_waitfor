
#include <~sudachen/uc_waitfor/import.h>

#define is_NIL(Ev) (Ev == EVENT_LIST_NIL)
#define NIL EVENT_LIST_NIL

#ifdef __stm32Fx_UC__
#define TIMER_TICKS(MS) (MS) // sysTimer configured on 1ms itick interwal
#elif defined __nRF5x_UC__
#define TIMER_TICKS(MS) APP_TIMER_TICKS((MS), APP_TIMER_PRESCALER)
#endif

typedef struct EventSet EventSet;
struct EventSet
{
    Event *timedEvents;
    Event *otherEvents;
};

static uint32_t uc_waitfor$Tick = 0;
const Event uc_waitfor$Nil = {NULL,NULL,};
static EventSet uc_waitfor$EvSet = {NIL,NIL};

static void on_timedIrqHandler(IrqHandler* self)
{
    (void)self;
    ++uc_waitfor$Tick;
}

void enable_handleTimedIqr(bool enable)
{
    static IrqHandler h = { on_timedIrqHandler, NULL };

    if ( enable )
    {
        if ( h.next == NULL )
            register_1msHandler(&h);
    }
    else
    {
        if ( h.next != NULL )
        {
            unregister_1msHandler(&h);
            uc_waitfor$Tick = 0;
        }
    }
}

void list_event(Event *ev)
{
    __Assert( ev != NULL );
    __Assert( ev != EVENT_LIST_NIL );

    EventSet *evset = &uc_waitfor$EvSet;
    enable_handleTimedIqr(true);

    __Critical
    {
        if ( ev->next == NULL ) // unlisted event
        {
            if ( ev->o.kind == ACTIVATE_BY_TIMER )
            {
                __Assert( ev->o.kind == ACTIVATE_BY_TIMER );
                __Assert( ev->next == NULL );

                ev->t.onTick = uc_waitfor$Tick + TIMER_TICKS(ev->o.delay);
                C_SLIST_LINK_WHEN((_->t.onTick > ev->t.onTick),Event,evset->timedEvents,ev,NIL);
            }
            else
            {
                C_SLIST_LINK_BACK(Event,evset->otherEvents,ev,NIL);
            }
        }
    }

    __Assert( ev->next != NULL );
}

void unlist_event(Event *ev)
{
    __Assert( ev != NULL );
    __Assert( ev != EVENT_LIST_NIL );

    EventSet *evset = &uc_waitfor$EvSet;

    __Critical
    {
        if ( ev->next != NULL )
        {
            Event** const l = (ev->o.kind == ACTIVATE_BY_TIMER)?&evset->timedEvents:&evset->otherEvents;
            C_SLIST_UNLINK(Event,(*l),ev,NIL);
        }
    }

    __Assert(ev->next == NULL);
}

void unlist_allEvents(uint32_t id)
{
    EventSet *evset = &uc_waitfor$EvSet;

    __Critical
    {
        Event** const l0 = &evset->timedEvents;
        C_SLIST_UNLINK_WHEN((_->o.id == id),Event,(*l0),NIL);
        if ( id != EVENT_ID_TIMER && id != EVENT_ID_NRFTIMER )
        {
            Event** const l1 = &evset->otherEvents;
            C_SLIST_UNLINK_WHEN((_->o.id == id),Event,(*l1),NIL);
        }
    }
}

void signal_event(Event *ev)
{
    __Assert( ev != NULL );
    __Assert( ev != EVENT_LIST_NIL );
    __Assert( ev->o.kind == ACTIVATE_BY_SIGNAL ||
              ev->o.kind == CALLBACK_ON_COMPLETE );

    if ( ev->o.kind == ACTIVATE_BY_SIGNAL ||
         ev->o.kind == CALLBACK_ON_COMPLETE )
    {
        list_event(ev);
        ev->t.is.signalled = true;
    }
}

void complete_event(Event *ev)
{
    __Assert( ev != NULL );
    __Assert( ev != EVENT_LIST_NIL );

    switch(ev->o.kind)
    {
        case ACTIVATE_BY_SIGNAL:
            ev->t.is.completed = true;
            break;
        case CALLBACK_ON_COMPLETE:
            if ( !ev->t.is.completed )
            {
                ev->t.is.completed = true;
                if ( ev->callback ) ev->callback(ev);
            }
            break;
        default:
            ;
    }
}

Event *wait_forEvent()
{
    EventSet *evset = &uc_waitfor$EvSet;

    for(;;)
    {
        uint32_t onTick = uc_waitfor$Tick;
        Event *ev, *tev;

        for(;;)
        {
            ev = NULL;

            __Critical
            {
                if ( !is_NIL(evset->timedEvents) && evset->timedEvents->t.onTick <= onTick )
                {
                    ev = evset->timedEvents;
                    evset->timedEvents = ev->next;
                }
            }

            if ( !ev ) break;

            __Assert ( ev->o.kind == ACTIVATE_BY_TIMER );
            ev->next = NULL; // is unlinked

            if ( ev->o.repeat )
                list_event(ev); // insert to wating list in appropriate possition
            // new event with the same uc_waitfor$Tick trigger will added the last
            if ( ev->callback )
            {
                ev->callback(ev);
                ev = NULL;
            }
            else // function should return event to caller
                return ev;
        }

        // now, there is nothing before onTick moment
        __Critical
        {
            if ( onTick > 0x7fffffff )
            {
                uc_waitfor$Tick -= onTick;
                for( ev = evset->timedEvents ; !is_NIL(ev); ev = ev->next )
                    ev->t.onTick -= onTick;
            }

            ev = evset->otherEvents;
        }

        while ( !is_NIL(ev) )
        {
            bool triggered = false;
            switch (ev->o.kind)
            {
                case ACTIVATE_BY_PROBE:
                    __Assert ( ev->t.probe != NULL );
                    if ( ev->t.probe != NULL )
                    {
                        triggered = ev->t.probe(ev);
                    }
                    break;
                case ACTIVATE_BY_SIGNAL:
                case CALLBACK_ON_COMPLETE:
                    if (( triggered = ev->t.is.signalled ))
                    {
                        ev->t.is.signalled = false;
                        ev->t.is.completed = false;
                    }
                    break;
                default:
                    __Unreachable();
            }

            __Critical
            {
                tev = ev;
                ev = ev->next;
            }

            if (triggered)
            {
                if ( !tev->o.repeat )
                    unlist_event(tev);

                if ( tev->callback && tev->o.kind != CALLBACK_ON_COMPLETE )
                    tev->callback(tev);
                else
                    return tev;
            }
        }

        __WFE();
    }
}
