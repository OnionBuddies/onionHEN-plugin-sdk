#include <onion/plugin.h>

#include <stdio.h>
#include <unistd.h>

ONION_PLUGIN_DEFINE("ONIO00001", "1.00", "Onion hello", 
                    ONION_PLUGIN_CAP_NOTIFY, ONION_PLUGIN_FLAG_LONG_RUNNING);

int main(void) {
    puts("OnionHEN hello plugin started");
    for (;;) {
        sleep(60);
    }
    return 0;
}

