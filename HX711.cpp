#include "HX711.h"
#include <wiringPi.h>
#include <stdio.h>
#include <QElapsedTimer>
#include <QDebug>

//*****************
//*** CONSTANTS ***
//*****************

const int SAMPLES_PER_WEIGHT = 4;

const int SAMPLES_PER_SECOND = 10;
const int SECONDS_QUEUED     = 2;
const int QUEUE_SIZE         = SAMPLES_PER_SECOND * SECONDS_QUEUED;


//*** Delay for one pulse length ***
#define PulseDelay delayMicroseconds( 5 );


//***********************************
//*** ISR routine and static vars ***
//***********************************
static void H_fallingEdgeISR();

//static HX711 *hx711_ = nullptr;

//*** GPIO pins ***
static int DT_Pin_  = 0;
static int SCK_Pin_ = 0;

//*** num samples left to collect ***
//static volatile int samplesToCollect_ = 0;

//*** array of sample values ***
static QQueue<int> collectedData_;

//*** flag to indicate that we are reading in data ***
static volatile bool readingData_ = false;

//*** sign extend the 24 bit value ***
int extendSign( int rawVal )
{
    //*** if negative ***
    if ( (rawVal & (0x00800000)) > 0 )
    {
        //*** sign extend top byte ***
        rawVal |= 0xFF000000;
    }

    return rawVal;
}




//*****************************************************************************
//*****************************************************************************
HX711::HX711( int DT_GPIO, int SCK_GPIO, int rawTare, double scale )
{
    //*** save initialization values ***
    DT_Pin_  = DT_GPIO;
    SCK_Pin_ = SCK_GPIO;
    tare_    = rawTare;
    scale_   = scale;

    //*** ensure wiring pi library is set up for GPIO numbering ***
    wiringPiSetupGpio() ;

    //*** set up serial shift pins ***
    pinMode( DT_GPIO, INPUT );
    pinMode( SCK_GPIO, OUTPUT );

    //*** Set up interrupt Service Routine on falling edge of DT pin ***
    wiringPiISR( DT_GPIO, INT_EDGE_FALLING, H_fallingEdgeISR );
}


//*****************************************************************************
//*****************************************************************************
HX711::~HX711()
{

}



//*****************************************************************************
//*****************************************************************************
float HX711::getWeight()
{
t_DataSet data;
float totalWeight = 0.0;

    //*** get samples for weight ***
    collectProcessedData( SAMPLES_PER_WEIGHT, data );

    //*** no data ***
    if ( data.isEmpty() ) return -1.0;

    //*** calculate average weight ***
    foreach( int sample, data )
    {
        totalWeight += ( ((double)sample - (double)tare_) * scale_ );
    }

    //*** compute the average weight ***
    float weightVal = (float)( totalWeight / (float)data.size() );

    return weightVal;
}


//*****************************************************************************
//*****************************************************************************
int HX711::getRawAvg( int numSamples )
{
t_DataSet data;
long long totalVal = 0;

    //*** get samples for weight ***
    collectProcessedData( numSamples, data );

    //*** no data ***
    if ( data.isEmpty() ) return 0;

    //*** calculate average value ***
    foreach( int sample, data )
    {
        totalVal += sample;
    }

    //*** compute the average weight ***
    int avgVal = (int)( totalVal / data.size() );

    return avgVal;
}


//*****************************************************************************
//*****************************************************************************
void HX711::setCalibrationData( int tareVal, int weightVal, float actualWeight )
{
    //*** save new raw tare value ***
    tare_ = tareVal;

    //*** calculate scale factor ***
    double weight = static_cast<double>(actualWeight);
    scale_ = (double)( weight / ((double)weightVal - (double)tareVal) );
}


//*****************************************************************************
//*****************************************************************************
void HX711::getCalibrationData( int &rawTareValue, double &scaleValue )
{
    rawTareValue = tare_;
    scaleValue = scale_;
}


//*****************************************************************************
//*****************************************************************************
void HX711::setTare( int tareVal )
{
    tare_ = tareVal;
}


//*****************************************************************************
//*****************************************************************************
int HX711::collectRawData( int numSamples, t_DataSet &data )
{
QQueue<int> samples;    // place to hold copy of collected data

    //*** clear output data ***
    data.clear();

    //*** get current queue size ***
    int qSize = collectedData_.size();

    // must have samples ***
    if ( qSize == 0 ) return 0;

    //*** copy data to minimize chance of it changing in the middle of processing ***
    samples = collectedData_;

    //*** use as many samples as we can get, up to requested amount ***
    numSamples = qMin( qSize, numSamples );

    //*** grab requested samples from end of queue and add to list ***
    int lastIdx = qSize - 1;
    int firstIdx = lastIdx  - numSamples + 1;
    for ( int idx = firstIdx; idx <= lastIdx; idx++ )
    {
        data.append( samples[idx] );
    }

    return data.size();

}


//*****************************************************************************
//*****************************************************************************
int HX711::collectProcessedData(int numSamples, t_DataSet &data)
{
    //*** get raw samples ***
    int actualNumSamples = collectRawData( numSamples, data );

    //*** must have samples to do processing ***
    if ( actualNumSamples < 1 ) return 0;

    //*** determine median value ***
    qSort( data );

    int median = 0;
    int variant = 0;
    int mid = numSamples / 2;
    if ( (numSamples%2 == 0) )
    {
        //*** even # samples ***
        median = ( data[mid] + data[mid-1] ) / 2;
    }
    else
    {
        //*** odd # samples ***
        median = data[mid];
    }

    //*** determine variant threshold ***
    variant = median / 10;

    //*** remove outliers ***
    for ( int i=0; i<numSamples; i++ )
    {
        int diff = abs( data[i] - median );
        if ( diff > abs(variant) )
        {
            //*** replace outlier with median value ***
            data[i] = median;
        }
    }

    return data.size();
}


//*****************************************************************************
//********************************** ISR **************************************
//*****************************************************************************
static void H_fallingEdgeISR()
{
int i = 0;
int sample = 0;
const int NUM_BITS = 24;    // 24 bit A/D converter

    //*** make sure we are valid to read ***
    //*** reading flag should be reset and DT oin should be low ***
    if ( readingData_ || digitalRead( DT_Pin_ ) == 1 ) return;

    //*** set reading flag ***
    readingData_ = true;

    //*** need delay before reading data ***
    PulseDelay

    //*** get all bits ***
    for ( i=0; i<NUM_BITS; i++ )
    {
        //*** bring clock high ***
        digitalWrite( SCK_Pin_, HIGH );

        //*** shift current value to make room for bit ***
        sample <<= 1;

        //*** Delay for typical pulse width on clock ***
        PulseDelay

        //*** bring clock low ***
        digitalWrite( SCK_Pin_, LOW );

        //*** if HIGH, add the bit to the value ***
        if ( digitalRead( DT_Pin_ ) )
        {
            sample |= 0x0001;
        }
    }

    //*** need one more pulse to indicate a gain of 128 ***
    //*** 2 pulses = gain of 32, 3 pulses = gain of 64  ***
    PulseDelay
    digitalWrite( SCK_Pin_, HIGH );
    PulseDelay
    digitalWrite( SCK_Pin_, LOW );

    //*** save data ***
    collectedData_.enqueue( -extendSign(sample) );

    //*** limit queue size - get rid of oldest data ***
    while ( collectedData_.size() > QUEUE_SIZE ) collectedData_.dequeue();

    //*** reset flag ***
    readingData_ = false;
}

