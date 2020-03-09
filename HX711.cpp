#include "HX711.h"
#include <wiringPi.h>
#include <stdio.h>
#include <QElapsedTimer>
#include <QDebug>

//*****************
//*** CONSTANTS ***
//*****************

const int SAMPLES_PER_WEIGHT = 4;

//*** Delay for one pulse length ***
#define PulseDelay delayMicroseconds( 5 );


//***********************************
//*** ISR routine and static vars ***
//***********************************
static void H_fallingEdgeISR();

static HX711 *hx711_ = nullptr;

//*** GPIO pins ***
static int DT_Pin_  = 0;
static int SCK_Pin_ = 0;

//*** num samples left to collect ***
static volatile int samplesToCollect_ = 0;

//*** array of sample values ***
static QList<int> collectedData_;

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

    curCollectMode_ = NO_MODE;

    hx711_ = this;

    //*** ensure wiring pi library is set up for GPIO numbering ***
    wiringPiSetupGpio() ;

    //*** set up serial shift pins ***
    pinMode( DT_GPIO, INPUT );
    pinMode( SCK_GPIO, OUTPUT );

    //*** Set up interrupt Service Routine on falling edge of DT pin ***
    wiringPiISR( DT_GPIO, INT_EDGE_FALLING, H_fallingEdgeISR );

    qRegisterMetaType<t_DataSet>("t_DataSet");

    //*** connect to data complete signal ***
    connect( this, SIGNAL(collectionComplete(t_DataSet)), SLOT(handleCollectionDone(t_DataSet)) );
}


//*****************************************************************************
//*****************************************************************************
HX711::~HX711()
{

}


//*****************************************************************************
//*****************************************************************************
void HX711::handleCollectionDone( t_DataSet data )
{
double weightTotal = 0;
long long rawTotal = 0;
float weightVal = 0;
int rawAverage = 0;
int numSamples = data.size();
int median = 0;
int variant = 0;

    //*** if raw mode, just pass back what we got ***
    if ( curCollectMode_ == RAW_MODE )
    {
        curCollectMode_ = NO_MODE;

        emit rawData( data );

        //*** done ***
        return;
    }

    //*** determine median value ***
    qSort( data );

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

    //*** remove outliers and add to total ***
    for ( int i=0; i<numSamples; i++ )
    {
        int diff = abs( data[i] - median );
        if ( diff > abs(variant) )
        {
            data[i] = median;
        }

        rawTotal += data[i];
    }

    //*** check mode ***
    if ( curCollectMode_ == WEIGHT_MODE )
    {
        //*** calculate average weight ***
        foreach( int sample, data )
        {
            weightTotal += ( ((double)sample - (double)tare_) * scale_ );
        }

        //*** compute the average weight ***
        weightVal = (float)( weightTotal / (double)numSamples );

        curCollectMode_ = NO_MODE;

        emit weight( weightVal );
    }

    else if ( curCollectMode_ == RAW_AVG_MODE )
    {
        //*** determine raw average ***
        rawAverage = (int)(rawTotal / numSamples );

        curCollectMode_ = NO_MODE;

        emit rawAvg( rawAverage );
    }
}


//*****************************************************************************
//*****************************************************************************
bool HX711::getWeight()
{
    //*** if we are currently collecting, return error ***
    if ( samplesToCollect_ > 0 || curCollectMode_ != NO_MODE ) return false;

    //*** set collect mode ***
    curCollectMode_ = WEIGHT_MODE;

    //*** request samples ***
    samplesToCollect_ = SAMPLES_PER_WEIGHT;

    return true;
}


//*****************************************************************************
//*****************************************************************************
bool HX711::collectRawData( int numSamples )
{
    //*** if we are currently collecting, return error ***
    if ( samplesToCollect_ > 0 || curCollectMode_ != NO_MODE ) return false;

    //*** set collect mode ***
    curCollectMode_ = RAW_MODE;

    //*** request samples ***
    samplesToCollect_ = numSamples;

    return true;
}


//*****************************************************************************
//*****************************************************************************
bool HX711::getRawAvg( int numSamples )
{
    //*** if we are currently collecting, return error ***
    if ( samplesToCollect_ > 0 || curCollectMode_ != NO_MODE ) return false;

    //*** set collect mode ***
    curCollectMode_ = RAW_AVG_MODE;

    //*** request samples ***
    samplesToCollect_ = numSamples;

    return true;
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
void HX711::dataCollected()
{
    //*** grab a copy of the data ***
    t_DataSet data = collectedData_;

    //*** clear the data ***
    collectedData_.clear();

    //*** asynchronous notification ***
    emit collectionComplete( data );
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
    if ( samplesToCollect_ < 1 || readingData_ || digitalRead( DT_Pin_ ) == 1 ) return;

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

    //*** handle data collection ***
    collectedData_.append( -extendSign(sample) );
    samplesToCollect_--;

    //*** reset flag ***
    readingData_ = false;

    //*** if data collection complete, let object know ***
    if ( samplesToCollect_ < 1 && hx711_ )
    {
        hx711_->dataCollected();
    }
}

