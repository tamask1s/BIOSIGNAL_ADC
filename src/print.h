#if defined(ARDUINO)
    #include <Arduino.h>
    #define init_print(baud_rate) Serial.begin(baud_rate)
    #define print(msg) Serial.print(msg)
    #define println(msg) Serial.println(msg)
#else
    #include <iostream>
    #define print(msg) std::cout << (msg)
    #define println(msg) std::cout << (msg) << std::endl
    #define init_print(baud_rate)
#endif
