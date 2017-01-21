
#include <~sudachen/uc_waitfor/import.h>

static const uint32_t TICK_GAP = 10;
typedef uint32_t uc_waitfor$tick_t;
static uc_waitfor$tick_t uc_waitfor$tick = 0;

void uc_waitfor$IrqHandler(UcIrqHandler* self)
{
    (void)self;
    ++uc_waitfor$tick;
}

void uc_waitfor$enable(bool enable)
{
    static UcIrqHandler irq = { uc_waitfor$IrqHandler, NULL };

    if ( enable )
    {
        if ( irq.next == NULL )
            ucRegister_IrqHandler(&irq,&UC_TIMED_IRQ);
    }
    else
    {
        if ( irq.next != NULL )
            ucUnregister_IrqHandler(&irq,&UC_TIMED_IRQ);
    }
}

void uc_waitfor$reload(UcEventSet *evset,UcEvent* ev)
{
    __Assert( ev->o.kind == UC_ACTIVATE_BY_TIMER );

    ev->next = evset->timedEvents;
    evset->timedEvents = ev;
}

void ucAdd_Event(UcEventSet *evset, UcEvent* ev)
{
    __Assert( ev->next == NULL );
    if ( ev->next != NULL ) return;

    uc_waitfor$enable(true);

    if ( ev->o.kind == UC_ACTIVATE_BY_TIMER )
    {
        uc_waitfor$reload(evset,ev);
    }
    else
    {
        ev->next = evset->otherEvents;
        evset->otherEvents = ev;
    }
}

UcEvent *ucWaitFor_Event(UcEventSet *evset)
{
    for(;;)
    {
        uc_waitfor$tick_t onTick = uc_waitfor$tick;

        if ( evset->timedEvents )
        {
            UcEvent **evPtr = &evset->timedEvents;
            for(; *evPtr != NULL &&
                (*evPtr)->t.onTick <= onTick &&
                ((*evPtr)->t.onTick + TICK_GAP) > onTick; )
            {
                UcEvent *ev = *evPtr;
                __Assert ( ev->o.kind == UC_ACTIVATE_BY_TIMER );

                (*evPtr) = ev->next;

                if ( ev->o.repeat )
                    uc_waitfor$reload(evset,ev);

                if ( ev->callback )
                    ev->callback(ev);
                else
                    return ev;
            }
        }

        if ( evset->otherEvents )
        {
            UcEvent **evPtr = &evset->otherEvents;
            while( *evPtr != NULL )
            {
                bool triggered = false;
                UcEvent *ev = *evPtr;
                switch (ev->o.kind)
                {
                case UC_ACTIVATE_BY_PROBE:
                    __Assert ( ev->t.probe != NULL );
                    if ( ev->t.probe != NULL ) triggered = ev->t.probe(ev);
                    break;
                case UC_ACTIVATE_BY_SIGNAL:
                    triggered = ev->o.signalled != 0;
                    ev->o.signalled = 0;
                    break;
                default:
                    __Unreachable();
                }

                if (triggered)
                {
                    if ( !ev->o.repeat )
                        (*evPtr) = ev->next;
                    else
                        evPtr = &ev->next;

                    if ( ev->callback )
                        ev->callback(ev);
                    else
                        return ev;
                }
                else
                   evPtr = &ev->next;
            }
        }

        __WFE();
    }
    return NULL;
}
