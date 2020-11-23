#include <pit.h>
#include <io.h>

static uint64_t started_time = 0;
static bool_t initialized = false;

// up to 255 waiters
uint64_t waiters_edging_tick[256];
void (*waiters[256])();
void* waiters_args[256];
uint8_t max_waiter = 0;

// return true if teh waiter could be added, false otherwise.
bool_t add_waiter(uint64_t wait_ms, void (fun)(), void* arg) {
    if (max_waiter == 255) {
        return false;
    }

    waiters[max_waiter] = fun;
    waiters_edging_tick[max_waiter] = started_time+wait_ms/get_precision_ms();
    waiters_args[max_waiter] = arg;
    max_waiter++;

    return true;
}

void del_waiter(uint8_t idx) {
    if (idx >= max_waiter) {
        return;
    }

    waiters[idx] = waiters[max_waiter];
    waiters_edging_tick[idx] = waiters_edging_tick[max_waiter];
    waiters_args[idx] = waiters_args[max_waiter];

    max_waiter--;
}

void waiter_waker(void* done) {
    *(uint8_t*)done = 1;
}

void sleep(uint64_t nb_ms) {
    volatile uint8_t done = false;

    add_waiter(nb_ms, waiter_waker, (void*)&done);

    while (!done) {}
}

void pit_init() {
    // set channel 0 to 'rate generator'
    out(0x34, 0x43);
    // set reload count to 4096 (~3.6ms / interrupt)
    out(0x0, 0x40);
    out(0x10, 0x40);
}

void time_incr() {
    // since we cannot be sure that the counter was operating as the right rate, we discard the first measurement
    if (!initialized) {
        initialized = true;
        return;
    }

    started_time++;

    // notify waiters
    for (int i = max_waiter-1; i >= 0; i--) { 
        if (started_time > waiters_edging_tick[i]) {
            (waiters[i])(waiters_args[i]);
            del_waiter(i);
        }
    }
}

uint64_t get_time_ms() {
    return (double)started_time*get_precision_ms();
}

double get_precision_ms() {
    return 4096/1.1931816666e3;
}
