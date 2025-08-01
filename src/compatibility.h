#if defined(ARDUINO)
    #include <stdio.h>
    #include <unistd.h>
    #define sleep_ms(msecs) sleep(msecs / 1000.0);
#else
    #include <windows.h>
    #define sleep_ms(msecs) Sleep(msecs);
#endif
