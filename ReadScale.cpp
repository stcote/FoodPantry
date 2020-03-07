#include "HX711.h"
#include <wiringPi.h>
#include <stdio.h>

int main(void)
{
int raw = 0;
long total = 0;
long numSamples = 30;
    
    wiringPiSetupGpio() ;

    HX711_init( 21, 20, -267362, 0.00022527 );
    
    HX711_setCalibrationData( -267362, 61112, 29.922 );
    

    delay(100);
    
#if 0    
    for ( int i=0; i<10; i++ )
    {
        int raw = HX711_getRawReading();
        total += raw;
        printf( "%d\n", raw );
        delay(300);
    }
    printf( "Avg: %d\n", total / 10 );
#endif
    
    while ( 1 )
    {
        float weight = HX711_getWeight();
        
        printf( "weight: %.1f\n", weight );
        
        delay( 300 );
    }

#if 0
    printf( "Nothing on scale, press enter\n" );
    scanf("");
    
    for ( int i=0; i<numSamples; i++ )
    {
        total += HX711_getRawReading();
        delay( 150 );
    }
    int tareAvg = (int)(total / numSamples);
    printf( "Tare: %d\nPut weight on scale\n", tareAvg );
    delay(5);
    
    total = 0;
    for ( int i=0; i<numSamples; i++ )
    {
        total += HX711_getRawReading();
        delay( 150 );
    }
    int scaleAvg = (int)(total / numSamples);
    float scale = 4.024 / (float)(scaleAvg - tareAvg);
    printf( "scaleAvg: %d  Scale: %.6f\n", scaleAvg, scale );
    
//    for ( int i=0; i<1000; i++ )
//    {
//        raw = HX711_getRawReading();
//        
//        printf( "Reading %d: %d\n", i+1, raw );
//        
//        delay(150);
//    }
#endif
}
