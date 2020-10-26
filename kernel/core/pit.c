#include <pit.h>
#include <io.h>

static uint64_t started_time = 0;
static bool_t initialized = false;

void pit_init() {
    // set channel 0 to 'rate generator'
    out(0x34, 0x43);
    // set reload count to 0 (=65536), 
    out(0x0, 0x40);
    out(0x0, 0x40);
}

void time_incr() {
    // since we cannot be sure that the counter was operating as the right rate, we discard the first measurement
    if (!initialized) {
        initialized = true;
        return;
    }

    started_time++;
}

uint64_t get_time_ms() {
    return (double)started_time*65536/1.1931816666e3;
}
