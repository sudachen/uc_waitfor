
#include <~sudachen/uc_waitfor/import.h>
#include <app_timer.h>

typedef struct NrfTimerEvent NrfTimerEvent;
struct NrfTimerEvent
{
    Event e;
    app_timer_timeout_handler_t callback;
    void *context;
};

__Static_Assert( sizeof(NrfTimerEvent) <= sizeof(app_timer_t) );

void uc_waitfor$nrfTimerCallback(Event *e)
{
    NrfTimerEvent *ne = (NrfTimerEvent*)e;
    ne->callback(ne->context);
}

uint32_t app_timer_create(
    app_timer_id_t const *      p_timer_id,
    app_timer_mode_t            mode,
    app_timer_timeout_handler_t timeout_handler)
{
    if ((timeout_handler == NULL) || (p_timer_id == NULL))
    {
        return NRF_ERROR_INVALID_PARAM;
    }

    NrfTimerEvent *ne = (NrfTimerEvent*)*p_timer_id; // yep, it's really is not const!
    memset(ne,0,sizeof(*ne));

    ne->e.next = NULL;
    ne->e.o.id = EVENT_ID_NRFTIMER;
    ne->e.o.kind = ACTIVATE_BY_TIMER;
    ne->e.o.repeat = (mode == APP_TIMER_MODE_REPEATED)?1:0;
    ne->e.callback = uc_waitfor$nrfTimerCallback;
    ne->callback = timeout_handler;
    ne->context = NULL;

    return NRF_SUCCESS;
}

uint32_t app_timer_start(
    app_timer_id_t timer_id,
    uint32_t timeout_ticks,
    void * p_context)
{
    uint32_t ms;
    NrfTimerEvent *ne = (NrfTimerEvent*)timer_id; // yep, it's really is not const!

    if ((timeout_ticks < APP_TIMER_MIN_TIMEOUT_TICKS))
    {
        return NRF_ERROR_INVALID_PARAM;
    }

    ms = ((uint32_t)ROUNDED_DIV(timeout_ticks * 1000 * (APP_TIMER_PRESCALER + 1),
                               (uint32_t)APP_TIMER_CLOCK_FREQ));

    if ( ms >= MAX_TIMER_DELAY )
    {
        return NRF_ERROR_INVALID_PARAM;
    }

    if (is_listedEvent(&ne->e)) unlist_event(&ne->e);

    __Assert(ne->e.o.id == EVENT_ID_NRFTIMER);
    ne->context = p_context;
    ne->e.o.delay = ms;
    list_event(&ne->e);

    return NRF_SUCCESS;
}

uint32_t app_timer_stop(app_timer_id_t timer_id)
{
    NrfTimerEvent *ne = (NrfTimerEvent*)timer_id; // yep, it's really is not const!
    if (is_listedEvent(&ne->e))
        unlist_event(&ne->e);
    return NRF_SUCCESS;
}

uint32_t app_timer_stop_all(void)
{
    unlist_allEvents(EVENT_ID_NRFTIMER);
    return NRF_SUCCESS;
}
