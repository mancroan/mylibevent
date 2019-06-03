#include <iostream>

#include <event.h>
#include <event2/http.h>

using namespace std;

// Time callback function
void onTime(int sock, short event, void *arg)
{
    static int cnt = 0;
    cout << "Game Over! " << cnt++ << endl;

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    if (cnt < 5) {
        // Add timer event
        event_add((struct event *) arg, &tv);
    }
    else {
        cout << "onTime is over" << endl;
    }
}

int main(int argc, char **argv)
{
    cout << event_get_version() << endl;

    struct event_base *base = event_init();
    struct event ev;

    evtimer_set(&ev, onTime, &ev);
    struct timeval timeevent;
    timeevent.tv_sec = 1;
    timeevent.tv_usec = 0;
    event_add(&ev, &timeevent);

    event_base_dispatch(base);
    event_base_free(base);

    return 0;
}

#include <iostream>
#include <event2/event.h>

struct cb_arg
{
    struct event *ev;
    struct timeval tv;
};

void timeout_cb(int fd, short event, void *params)
{
    struct cb_arg *arg = (struct cb_arg*)params;
    struct event *ev = arg->ev;
    struct timeval tv = arg->tv;

    evtimer_add(ev, &tv);
}

int main()
{
    struct event_base *base = event_base_new();
    struct event *timeout = NULL;
    struct timeval tv = {1, 0};
    struct cb_arg arg;

    timeout = evtimer_new(base, timeout_cb, &arg);
    arg.ev = timeout;
    arg.tv = tv;
    evtimer_add(timeout, &tv);
    event_base_dispatch(base);
    evtimer_del(timeout);

    return 0;
}

