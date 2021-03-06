SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(SEMI_AUTOMATIC);


// User defined variables
#define TIMEZONE        -5  // Your desired standard time zone offset
#define USE_DST         1   // 1 = use daylight savings time, 0 = do NOT use daylight savings time
#define TWENTYFOUR_HOUR 1   // 1 = use 24-hour (military) time, 0 = use 12 hour time


// 7-segment display libraries
#include "Adafruit_GFX.h"
#include "Adafruit_LEDBackpack.h"
Adafruit_7segment matrix = Adafruit_7segment();


// Global variables
uint16_t time_sync_time = 0;
uint16_t the_time = 0;
uint8_t last_second = 61;
bool colon_display = 0;


void setup() {
    Particle.connect();

    // Initialize the display
    matrix.begin(0x70);

    // Display an animation while we wait to connect to the cloud
    connectWait();

    // Pick a random time to synchronize the time from the cloud
    time_sync_time = random(0, 2399);

    // Set the time zone
    Time.zone(TIMEZONE);

} // end setup()


void loop() {
    everySecond();
} // end loop()


// Run every second
void everySecond() {
    // Exit early if 1 second has NOT elapsed
    if(Time.second()==last_second)
        return;

    // Get the current time in HHMM format
    the_time = Time.hour()*100+Time.minute();


    // Display the time in 24 hour format
    if(TWENTYFOUR_HOUR==1) {
        matrix.print(the_time);
        colonDisplay(colon_display, false);

    // We have to do some mangling to do 12-hour time
    } else {
        // Calculate 12-hour time
        uint16_t new_time = the_time;
        if(new_time>=1200)
            new_time = new_time-1200;

        matrix.print(new_time);

        // Show PM dot
        if(the_time>=1200)
            colonDisplay(colon_display, true);
        else
            colonDisplay(colon_display, false);
    }


    // Toggle the blinking colon
    colon_display = !colon_display;

    // Update the display
    matrix.writeDisplay();

    // Do our routine chores
    timeChores();

    // Set this so we don't run this block again for another second
    last_second = Time.second();
}


// Manage colon display (for blinking + PM dot)
void colonDisplay(bool middle, bool PM) {
    uint8_t colon = 0;

    // Show the middle colon
    if(middle)
        colon |= 2;

    // Show the PM dot
    if(PM)
        colon |= 4;

    matrix.writeDigitRaw(2, colon);
    matrix.writeDisplay();
}


// Chores to maintain optimal time
void timeChores() {
    // Synchronize the time
    if(Time.second()==0 && the_time==time_sync_time)
        Particle.syncTime();

    // If we're not using DST, return early
    if(!USE_DST)
        return;

    // If we ARE using DST, check to see if DST is in effect
    // and act accordingly.
    if(Time.isDST())
        Time.beginDST();
    else
        Time.endDST();
} // end timeChores()


// Display an animation while we wait to connect to the cloud
void connectWait() {
    uint8_t segment = 1;

    // Loop through 6 segments
    for(uint8_t i=0; i<6; i++) {
        // Loop through 4 digits
        for(uint8_t n=0; n<5; n++) {
            // If we get connected, exit this function
            if(Particle.connected())
                return;

            // 2 is the colon and other dots
            if(n==2)
                continue;

            matrix.writeDigitRaw(n, segment);
        }

        // Shift the bit to the next segment
        segment = segment << 1;

        matrix.writeDisplay();

        // Never hurts to add this in a tight loop
        Particle.process();
        delay(50);
    }
} // end connectWait()