
#include <~sudachen/uc_waitfor/import.h>

#define is_NIL(Ev) (Ev == UC_EVENT_LIST_NIL)
#define NIL UC_EVENT_LIST_NIL

#ifdef __stm32Fx_UC__
#define TIMER_TICKS(MS) (MS) // sysTimer configured on 1ms itick interwal
#elif defined __nRF5x_UC__
#define TIMER_TICKS(MS) APP_TIMER_TICKS((MS), APP_TIMER_PRESCALER)
#endif

static uint32_t tick = 0;
const UcEvent uc_waitfor$Nil = {NULL,NULL,};
static UcEventSet uc_waitfor$evset = {NIL,NIL};

static void on_timedIrqHandler(UcIrqHandler* self)
{
    (void)self;
    ++tick;
}

void enable_handleTimedIqr(bool enable)
{
    static UcIrqHandler h = { on_timedIrqHandler, NULL };

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
            tick = 0;
        }
    }
}

static void reload(UcEventSet *evset,UcEvent* ev)
{
    __Assert( ev->o.kind == UC_ACTIVATE_BY_TIMER );
    __Assert( ev->next == NULL );

    ev->t.onTick = tick + TIMER_TICKS(ev->o.delay);

    __Critical
    {
        C_SLIST_LINK_WHEN((_->t.onTick > ev->t.onTick),UcEvent,evset->timedEvents,ev,NIL);
    }
}

void list_event(UcEvent *ev)
{
    UcEventSet *evset = &uc_waitfor$evset;

    __Assert( ev->next == NULL );
    if ( ev->next != NULL ) return;

    enable_handleTimedIqr(true);

    if ( ev->o.kind == UC_ACTIVATE_BY_TIMER )
    {
        reload(evset,ev);
    }
    else
    {
        __Critical
        {
            C_SLIST_LINK_BACK(UcEvent,evset->otherEvents,ev,NIL);
        }
    }

    __Assert( ev->next != NULL );
}

void unlist_event(UcEvent *ev)
{
    UcEventSet *evset = &uc_waitfor$evset;

    __Assert(ev->next != NULL);
    if ( ev->next == NULL ) return;

    __Critical
    {
        UcEvent** const l = (ev->o.kind == UC_ACTIVATE_BY_TIMER)?&evset->timedEvents:&evset->otherEvents;
        C_SLIST_UNLINK(UcEvent,(*l),ev,NIL);
    }

    __Assert(ev->next == NULL);
}

void unlist_allEvents(uint32_t id)
{
    UcEventSet *evset = &uc_waitfor$evset;

    __Critical
    {
        UcEvent** const l0 = &evset->timedEvents;
        C_SLIST_UNLINK_WHEN((_->o.id == id),UcEvent,(*l0),NIL);
        if ( id != UC_EVENT_ID_TIMER && id != UC_EVENT_ID_NRFTIMER )
        {
            UcEvent** const l1 = &evset->otherEvents;
            C_SLIST_UNLINK_WHEN((_->o.id == id),UcEvent,(*l1),NIL);
        }
    }
}

void complete_event(UcEvent *ev)
{
    __Assert( ev->o.kind == UC_ACTIVATE_BY_SIGNAL ||
              ev->o.kind == UC_CALLBACK_ON_COMPLETE );

    if ( ev->o.kind == UC_ACTIVATE_BY_SIGNAL )
        ev->t.is.completed = true;
    else if ( ev->o.kind == UC_CALLBACK_ON_COMPLETE )
    {
        ev->t.is.completed = true;
        if ( ev->callback ) ev->callback(ev);
    }
}

UcEvent *wait_forEvent()
{
    UcEventSet *evset = &uc_waitfor$evset;

    for(;;)
    {
        uint32_t onTick = tick;
        UcEvent *ev;

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

            __Assert ( ev->o.kind == UC_ACTIVATE_BY_TIMER );
            ev->next = NULL; // is unlinked

            if ( ev->o.repeat )
                reload(evset,ev); // insert to wating list in appropriate possition
            // new event with the same tick trigger will added the last
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
                tick -= onTick;
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
                case UC_ACTIVATE_BY_PROBE:
                    __Assert ( ev->t.probe != NULL );
                    if ( ev->t.probe != NULL )
                    {
                        triggered = ev->t.probe(ev);
                    }
                    break;
                case UC_ACTIVATE_BY_SIGNAL:
                case UC_CALLBACK_ON_COMPLETE:
                    if (( triggered = ev->t.is.signalled ))
                    {
                        ev->t.is.signalled = false;
                        ev->t.is.completed = false;
                    }
                    break;
                default:
                    __Unreachable();
            }

            if (triggered)
            {
                if ( !ev->o.repeat )
                    unlist_event(ev);

                if ( ev->callback && ev->o.kind != UC_CALLBACK_ON_COMPLETE )
                    ev->callback(ev);
                else
                    return ev;
            }

            __Critical
            {
                ev = ev->next;
            }
        }

        __WFE();
    }
}
