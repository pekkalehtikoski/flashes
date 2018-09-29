#include <Arduino.h>
#include <eosal.h>
#include <eosalx.h>
#include <flashes.h>

/*
  mcu_flashes
  Example to load and start flashes library 
  echo server.
 */
 
#define N_LEDS 3
const int leds[N_LEDS] = {PB0, PB14, PB7};

static void toggle_leds(void)
{
    static int lednr = 0;

    digitalWrite(leds[lednr], HIGH);
    delay(30);
    digitalWrite(leds[lednr], LOW);
    delay(50);
    if (++lednr >= N_LEDS) lednr = 0;
}


// Include code for eacho server (at the time of writing IP 192.168.1.201, TCP port 6001)
#include "/coderoot/eosal/examples/simple_socket_server/code/simple_socket_server_example.c"

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
    Serial.println("Arduino starting...");

    // Initialize OS abtraction layer and start flashes on socket.
    osal_initialize(OSAL_INIT_DEFAULT);
    flashes_socket_setup();
}

void loop()
{
    flashes_socket_loop();
    toggle_leds();
}

