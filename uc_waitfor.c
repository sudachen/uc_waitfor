
#include <~sudachen/uc_waitfor/import.h>

static uint32_t uc_waitfor$tick = 0;

static void uc_waitfor$IrqHandler(UcIrqHandler* self)
{
    (void)self;
    ++uc_waitfor$tick;
}

static void uc_waitfor$enable(bool enable)
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

void ucAdd_Event(UcEventSet *evset, UcEvent* ev)
{
    __Assert( ev->next == NULL );
    if ( ev->next != NULL ) return;

    uc_waitfor$enable(true);

    if ( ev->o.kind == UC_EVENT_BY_TIMER )
    {
        ev->next = evset->timedEvents;
        evset->timedEvents = ev;
    } else {
        ev->next = evset->otherEvents;
        evset->otherEvents = ev;
    }
}

UcEvent *ucWaitFor_Event(UcEventSet *evset)
{
    return NULL;
}
