#include <Arduino.h>
#include <eosalx.h>
#include <flashes.h>

/*
  mcu_flashes
  Example to load and start flashes library 
  echo server.
 */
 
#define N_LEDS 3
const int leds[N_LEDS] = {PB0, PB14, PB7};
os_timer ledtimer;

static void toggle_leds(void)
{
    static int lednr = 0;
    if (os_elapsed(&ledtimer, 30))
    {
      digitalWrite(leds[lednr], LOW);
      if (++lednr >= N_LEDS) lednr = 0;
      digitalWrite(leds[lednr], HIGH);
      os_get_timer(&ledtimer);
    }
}

void setup() 
{
    int lednr;

    // initialize the digital pin as an output.
    for (lednr = 0; lednr < N_LEDS; lednr++)
    {
        pinMode(leds[lednr], OUTPUT);
    }

    // Set up serial port for trace output.
    Serial.begin(9600);
    while (!Serial)
    {
        toggle_leds();
    }
    Serial.println("Starting...");

    // Initialize OS abtraction layer and start flashes on socket.
    osal_initialize(OSAL_INIT_DEFAULT);
    flashes_socket_setup();
}

void loop()
{
    flashes_socket_loop();
    toggle_leds();
}

