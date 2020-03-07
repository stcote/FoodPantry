#include "HX711.h"
#include <time.h>
#include <wiringPi.h>
#include <stdio.h>
#include <QList>
#include <QDebug>

//*****************
//*** CONSTANTS ***
//*****************
const long NSecsPerSec = 1000000000;
const long USecsPerSec = 1000000;

const int SAMPLES_PER_WEIGHT = 8;

//*****************
//*** VARIABLES ***
//*****************

//*** raw tare value (zero weight) ***
static int tare_ = 0;

//*** scale factor to produce desired weight units ***
//*** produced as part of calibration
static double scale_ = 0.0;

//*** GPIO pins ***
static int DT_Pin_ = 0;
static int SCK_Pin_ = 0;

//*** last value read and time ***
static volatile int readValue_ = 0;;
static volatile int samplesToCollect_ = 0;
static QList<int> collectedData_;

//*** flag to indicate that we are reading in data ***
static volatile bool readingData_ = false;


//*****************
//*** Functions ***
//*****************

//*****************************************************************************
//*****************************************************************************
void HX711_init( int DT_Pin, int SCK_Pin, int rawTare, double scale )
{
    //*** save initialization values ***
    DT_Pin_  = DT_Pin;
    SCK_Pin_ = SCK_Pin;
    tare_    = rawTare;
    scale_   = scale;

    //*** set up serial shift pins ***
    pinMode( DT_Pin_, INPUT );
    pinMode( SCK_Pin_, OUTPUT );

    //*** Set up interrupt Service Routine on falling edge of DT pin ***
    wiringPiISR( DT_Pin_, INT_EDGE_FALLING, H_fallingEdgeISR );
}


//*****************************************************************************
//*****************************************************************************
void HX711_collectRawData( int numSamples, QList<int> &data )
{
    //*** set # samples to collect ***
    samplesToCollect_ = numSamples;

    //*** wait for it to be collected ***
    while( samplesToCollect_ > 0 )
    {
        delay( 5 );
    }

    //*** copy the data ***
    data = collectedData_;
    collectedData_.clear();
}


//*****************************************************************************
//*****************************************************************************
int HX711_getRawAverage( int numSamples, QList<int> &data )
{
long long int total = 0;
int rawAvg = 0;

    //*** clear the output array ***
    data.clear();

    //*** collect the desired # samples ***
    HX711_collectRawData( numSamples, data );

    //*** determine median value ***
    int mid = numSamples / 2;
    int median = ( data[mid] + data[mid-1] ) / 2;
    int variant = median / 10;

    //*** remove outliers and add to total ***
    for ( int i=0; i<numSamples; i++ )
    {
        int diff = abs( data[i] - median );
        if ( diff > abs(variant) )
        {
            data[i] = median;
        }

        total += data[i];
    }

    //*** calc average ***
    rawAvg = total / numSamples;

    return rawAvg;
}


//*****************************************************************************
//*****************************************************************************
float HX711_getWeight()
{
QList<int> data;
double total = 0;
float weightVal = 0;

    //*** collect raw data, removing outliers ***
    HX711_getRawAverage( SAMPLES_PER_WEIGHT, data );

    //*** calculate weight average ***
    for ( int i=0; i<SAMPLES_PER_WEIGHT; i++ )
    {
        total += ( ((double)data[i] - (double)tare_) * scale_ );
    }
        
    //*** compute the average weight ***
    weightVal = (float)( total / (double)SAMPLES_PER_WEIGHT );
    
    //*** bound by 0 ***
    if ( weightVal < 0 ) weightVal = 0;

    return weightVal;
}


//*****************************************************************************
//*****************************************************************************
int HX711_getRawReading()
{
    return -H_extendSign( readValue_ );
}


//*****************************************************************************
//*****************************************************************************
void HX711_setCalibrationData( int tareVal, int weightVal, float actualWeight )
{
    //*** save new raw tare value ***
    tare_ = tareVal;

    //*** calculate scale factor ***
    scale_ = (double)( actualWeight / ((double)weightVal - (double)tareVal) );
}


//*****************************************************************************
//*****************************************************************************
void HX711_getCalibrationData( int &rawTareValue, double &scaleValue )
{
    rawTareValue = tare_;
    scaleValue = scale_;
}


//*****************************************************************************
//*****************************************************************************
static void H_fallingEdgeISR()
{
int i = 0;
int tempReadValue = 0;
const int NUM_BITS = 24;    // 24 bit A/D converter
QString bits;

    //*** make sure we are valid to read ***
    //*** reading flag should be reset and DT oin should be low ***
    if ( readingData_ || digitalRead( DT_Pin_ ) == 1 ) return;
    
    //*** set reading flag ***
    readingData_ = true;

    //*** need delay before reading data ***
    H_pulseDelay();

    //*** get all bits ***
    for ( i=0; i<NUM_BITS; i++ )
    {
        //*** bring clock high ***
        digitalWrite( SCK_Pin_, HIGH );

        //*** shift current value to make room for bit ***
        tempReadValue <<= 1;

        //*** Delay for typical pulse width on clock ***
        H_pulseDelay();

        //*** bring clock low ***
        digitalWrite( SCK_Pin_, LOW );

        //*** if HIGH, add the bit to the value ***
        if ( digitalRead( DT_Pin_ ) ) 
        {
            tempReadValue |= 0x0001;
            bits += "1";
        }
        else
        {
            bits += "0";
        }
    }
    
    //*** have all bits, save the data ***
    readValue_ = tempReadValue;

    //*** check if collecting data ***
    if ( samplesToCollect_ > 0 )
    {
        collectedData_.append( (int)-H_extendSign(readValue_) );
        samplesToCollect_--;
//        qDebug() << bits;
//        if ( samplesToCollect_ == 0 ) qDebug() << "";
    }

    //*** need one more pulse to indicate a gain of 128 ***
    //*** 2 pulses = gain of 32, 3 pulses = gain of 64  ***
    H_pulseDelay();
    digitalWrite( SCK_Pin_, HIGH );
    H_pulseDelay();
    digitalWrite( SCK_Pin_, LOW );

    //*** reset flag ***
    readingData_ = false;
}


//*****************************************************************************
//*****************************************************************************
void H_pulseDelay()
{
    delayMicroseconds( 2 );
}


//*****************************************************************************
//*****************************************************************************
int H_extendSign( int val )
{
    //*** if negative ***
    if ( (val & (0x00800000)) > 0 )
    {
        //*** sign extend top byte ***
        val |= 0xFF000000;
    } 

    return val;
}

