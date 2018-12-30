#include <stdbool.h>

#define RELAY_PIN (gpio_num_t) 2

int RELAY_CONNECTED = 0;
bool RELAY_CHANGED = 0;
char* RELAY_CHANNEL = "heat/switch"; 