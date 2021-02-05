#include "NAU7802.h"
#include <QtCore>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <stdio.h>
#include <QElapsedTimer>
#include <QDebug>

//*****************
//*** CONSTANTS ***
//*****************

const int SAMPLES_PER_WEIGHT = 5;

const int SAMPLES_PER_SECOND = 10;
const int SECONDS_QUEUED     = 2;
const int QUEUE_SIZE         = SAMPLES_PER_SECOND * SECONDS_QUEUED;

const int I2C_ADDR           = 0x2A;


//*****************************************************************************
//*****************************************************************************
NAU7802::NAU7802( int rawTare, double scale )
    : QObject()
{
    //*** save initialization values ***
    tare_    = rawTare;
    scale_   = scale;
    nau7802_ = 0;

    //*** ensure wiring pi library is set up for GPIO numbering ***
//    wiringPiSetupGpio() ;

    //*** set up interrupt line from chip ***
//    pinMode( interruptPin, INPUT );

    //*** Set up interrupt Service Routine on falling edge of interrupt line ***
//    wiringPiISR( interruptPin, INT_EDGE_FALLING, H_fallingEdgeISR );

    //*** now set up the nau7802 chip ***
    setup();

    //*** setup sample timer ***
    sampleTimer_ = new QTimer(this);
    sampleTimer_->setInterval( 100 );
    connect( sampleTimer_, SIGNAL(timeout()), SLOT(takeReading()));
    sampleTimer_->start();
}


//*****************************************************************************
//*****************************************************************************
NAU7802::~NAU7802()
{
    //*** stop and clean up timer ***
    sampleTimer_->stop();
    delete sampleTimer_;
}


//*****************************************************************************
//*****************************************************************************
bool NAU7802::setup()
{
bool result = true;

    //*** open i2c device ***
    nau7802_ = wiringPiI2CSetup( I2C_ADDR );

    if ( nau7802_ < 0 )
    {
        qDebug() << "Error accessing NAU7802 device over I2C";
        return false;
    }
    else
    {
        qDebug() << "Connected to NAU7802 I2C device";
    }

    result &= reset(); //Reset all registers

    result &= powerUp(); //Power on analog and digital sections of the scale

    result &= setLDO(NAU7802_LDO_3V3); //Set LDO to 3.3V

    result &= setGain(NAU7802_GAIN_64); //Set gain to 64

    result &= setSampleRate(NAU7802_SPS_20); //Set samples per second to 20

    result &= setRegister(NAU7802_ADC, 0x30); //Turn off CLK_CHP. From 9.1 power on sequencing.

    result &= setBit(NAU7802_PGA_PWR_PGA_CAP_EN, NAU7802_PGA_PWR); //Enable 330pF decoupling cap on chan 2. From 9.14 application circuit note.

    result &= calibrateAFE(); //Re-cal analog front end when we change gain, sample rate, or channel

    qDebug() << "result:" << result;

    return  result;
}



//*****************************************************************************
//*****************************************************************************
float NAU7802::getWeight()
{
t_DataSet data;
float totalWeight = 0.0;
QString buf,buf2;

    //*** get samples for weight ***
    collectProcessedData( SAMPLES_PER_WEIGHT, data );

    //*** no data ***
    if ( data.isEmpty() ) return -1.0;

    buf = "getWeight() ";

    //*** calculate average weight ***
    foreach( int sample, data )
    {
        buf2.sprintf( "%.2lf ", ((double)sample - (double)tare_) * scale_ );
        buf += buf2;
        totalWeight += ( ((double)sample - (double)tare_) * scale_ );
    }

    //*** compute the average weight ***
    float weightVal = (float)( totalWeight / (float)data.size() );

    buf += " totalWeight: " + QString::number(totalWeight);
    buf += " weightVal: " + QString::number(weightVal);
    qDebug() << buf;

    return weightVal;
}


//*****************************************************************************
//*****************************************************************************
int NAU7802::getRawAvg( int numSamples )
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
    int avgVal = static_cast<int>( totalVal / data.size() );

    return avgVal;
}


//*****************************************************************************
//*****************************************************************************
void NAU7802::setCalibrationData( int tareVal, int weightVal, float actualWeight )
{
    //*** save new raw tare value ***
    tare_ = tareVal;

    //*** calculate scale factor ***
    double weight = static_cast<double>(actualWeight);
    scale_ = (double)( weight / ((double)weightVal - (double)tareVal) );
}


//*****************************************************************************
//*****************************************************************************
void NAU7802::getCalibrationData( int &rawTareValue, double &scaleValue )
{
    rawTareValue = tare_;
    scaleValue = scale_;
}


//*****************************************************************************
//*****************************************************************************
void NAU7802::setTare( int tareVal )
{
    tare_ = tareVal;
}


//*****************************************************************************
//*****************************************************************************
int NAU7802::collectRawData( int numSamples, t_DataSet &data )
{
QQueue<int> samples;    // place to hold copy of collected data
QString buf;

    //*** clear output data ***
    data.clear();

    //*** get current queue size ***
    int qSize = collectedData_.size();

    buf.sprintf( "Q: %d D: %d  ", qSize, data.size() );

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

        buf += QString::number( samples[idx] ) + " ";
    }

    qDebug() << buf;

    return data.size();

}


//*****************************************************************************
//*****************************************************************************
int NAU7802::collectProcessedData(int numSamples, t_DataSet &data)
{
    //*** get raw samples ***
    int actualNumSamples = collectRawData( numSamples, data );

    //*** must have samples to do processing ***
    if ( actualNumSamples < 2 ) return 0;

#if 0
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
#endif

    return data.size();
}


//*****************************************************************************
//******************************** NAU7802 ************************************
//*****************************************************************************

//Set Int pin to be high when data is ready (default)
bool NAU7802::setIntPolarityHigh()
{
  return (clearBit(NAU7802_CTRL1_CRP, NAU7802_CTRL1)); //0 = CRDY pin is high active (ready when 1)
}


//*****************************************************************************
//*****************************************************************************
//Set Int pin to be low when data is ready
bool NAU7802::setIntPolarityLow()
{
  return (setBit(NAU7802_CTRL1_CRP, NAU7802_CTRL1)); //1 = CRDY pin is low active (ready when 0)
}


//*****************************************************************************
//*****************************************************************************
//Mask & set a given bit within a register
bool NAU7802::setBit(quint8 bitNumber, quint8 registerAddress)
{
  quint8 value = getRegister(registerAddress);
  value |= (1 << bitNumber); //Set this bit
  return (setRegister(registerAddress, value));
}


//*****************************************************************************
//*****************************************************************************
//Mask & clear a given bit within a register
bool NAU7802::clearBit(quint8 bitNumber, quint8 registerAddress)
{
  quint8 value = getRegister(registerAddress);
  value &= ~(1 << bitNumber); //Set this bit
  return (setRegister(registerAddress, value));
}


//*****************************************************************************
//*****************************************************************************
//Return a given bit within a register
bool NAU7802::getBit(quint8 bitNumber, quint8 registerAddress)
{
  quint8 value = getRegister(registerAddress);
  value &= (1 << bitNumber); //Clear all but this bit
  return (value);
}


//*****************************************************************************
//*****************************************************************************
//Get contents of a register
quint8 NAU7802::getRegister(quint8 registerAddress)
{
    return static_cast<quint8>(wiringPiI2CReadReg8( nau7802_, registerAddress));
}


//*****************************************************************************
//*****************************************************************************
//Send a given value to be written to given address
//Return true if successful
bool NAU7802::setRegister(quint8 registerAddress, quint8 value)
{
    return (wiringPiI2CWriteReg8( nau7802_, registerAddress, value ) == 0);
}

//*****************************************************************************
//*****************************************************************************
//Returns true if Cycle Ready bit is set (conversion is complete)
bool NAU7802::available()
{
  return (getBit(NAU7802_PU_CTRL_CR, NAU7802_PU_CTRL));
}


//*****************************************************************************
//*****************************************************************************
//Set the readings per second
//10, 20, 40, 80, and 320 samples per second is available
bool NAU7802::setSampleRate(quint8 rate)
{
  if (rate > 0b111)
    rate = 0b111; //Error check

  quint8 value = getRegister(NAU7802_CTRL2);
  value &= 0b10001111; //Clear CRS bits
  value |= rate << 4;  //Mask in new CRS bits

  return (setRegister(NAU7802_CTRL2, value));
}


//*****************************************************************************
//*****************************************************************************
//Select between 1 and 2
bool NAU7802::setChannel(quint8 channelNumber)
{
  if (channelNumber == NAU7802_CHANNEL_1)
    return (clearBit(NAU7802_CTRL2_CHS, NAU7802_CTRL2)); //Channel 1 (default)
  else
    return (setBit(NAU7802_CTRL2_CHS, NAU7802_CTRL2)); //Channel 2
}


//*****************************************************************************
//*****************************************************************************
//Power up digital and analog sections of scale
bool NAU7802::powerUp()
{
  setBit(NAU7802_PU_CTRL_PUD, NAU7802_PU_CTRL);
  setBit(NAU7802_PU_CTRL_PUA, NAU7802_PU_CTRL);

  //Wait for Power Up bit to be set - takes approximately 200us
  quint8 counter = 0;
  while (1)
  {
    if (getBit(NAU7802_PU_CTRL_PUR, NAU7802_PU_CTRL) == true)
      break; //Good to go
    delay(1);
    if (counter++ > 100)
      return (false); //Error
  }
  return (true);
}


//*****************************************************************************
//*****************************************************************************
//Puts scale into low-power mode
bool NAU7802::powerDown()
{
  clearBit(NAU7802_PU_CTRL_PUD, NAU7802_PU_CTRL);
  return (clearBit(NAU7802_PU_CTRL_PUA, NAU7802_PU_CTRL));
}


//*****************************************************************************
//*****************************************************************************
//Resets all registers to Power Of Defaults
bool NAU7802::reset()
{
  setBit(NAU7802_PU_CTRL_RR, NAU7802_PU_CTRL); //Set RR
  delay(1);
  return (clearBit(NAU7802_PU_CTRL_RR, NAU7802_PU_CTRL)); //Clear RR to leave reset state
}


//*****************************************************************************
//*****************************************************************************
//Set the onboard Low-Drop-Out voltage regulator to a given value
//2.4, 2.7, 3.0, 3.3, 3.6, 3.9, 4.2, 4.5V are available
bool NAU7802::setLDO(quint8 ldoValue)
{
  if (ldoValue > 0b111)
    ldoValue = 0b111; //Error check

  //Set the value of the LDO
  quint8 value = getRegister(NAU7802_CTRL1);
  value &= 0b11000111;    //Clear LDO bits
  value |= ldoValue << 3; //Mask in new LDO bits
  setRegister(NAU7802_CTRL1, value);

  return (setBit(NAU7802_PU_CTRL_AVDDS, NAU7802_PU_CTRL)); //Enable the internal LDO
}


//*****************************************************************************
//*****************************************************************************
//Set the gain
//x1, 2, 4, 8, 16, 32, 64, 128 are avaialable
bool NAU7802::setGain(quint8 gainValue)
{
  if (gainValue > 0b111)
    gainValue = 0b111; //Error check

  quint8 value = getRegister(NAU7802_CTRL1);
  value &= 0b11111000; //Clear gain bits
  value |= gainValue;  //Mask in new bits

  return (setRegister(NAU7802_CTRL1, value));
}


//*****************************************************************************
//*****************************************************************************
//Get the revision code of this IC
quint8 NAU7802::getRevisionCode()
{
  quint8 revisionCode = getRegister(NAU7802_DEVICE_REV);
  return (revisionCode & 0x0F);
}


//*****************************************************************************
//*****************************************************************************
//Returns 24-bit reading
//Assumes CR Cycle Ready bit (ADC conversion complete) has been checked to be 1
qint32 NAU7802::getReading()
{
quint8 msb,midsb,lsb;

    //*** get the three byte values ***
    msb   = getRegister( NAU7802_ADCO_B2 );
    midsb = getRegister( NAU7802_ADCO_B1 );
    lsb   = getRegister( NAU7802_ADCO_B0 );

    //*** assemble them ***
    quint32 valueRaw = (static_cast<quint32>(msb) << 16) |
                       (static_cast<quint32>(midsb)  << 8) |
                        static_cast<quint32>(lsb);

    // the raw value coming from the ADC is a 24-bit number, so the sign bit now
    // resides on bit 23 (0 is LSB) of the quint32 container. By shifting the
    // value to the left, I move the sign bit to the MSB of the qint32 container.
    // By casting to a signed qint32 container I now have properly recovered
    // the sign of the original value
    qint32 valueShifted = static_cast<qint32>(valueRaw << 8);

    // shift the number back right to recover its intended magnitude
    qint32 value = (valueShifted >> 8);

    return (value);
}


//*****************************************************************************
//*****************************************************************************
void NAU7802::takeReading()
{
qint32 sample = 0;

    //*** ensure there is a sample available ***
    if ( available() )
    {
        //*** get the 24 bit sample ***
        sample = getReading();

        qDebug() << sample;

        //*** save data ***
        collectedData_.enqueue( sample );

        //*** limit queue size - get rid of oldest data ***
        while ( collectedData_.size() > QUEUE_SIZE ) collectedData_.dequeue();
    }

}


//*****************************************************************************
//*****************************************************************************
//Calibrate analog front end of system. Returns true if CAL_ERR bit is 0 (no error)
//Takes approximately 344ms to calibrate; wait up to 1000ms.
//It is recommended that the AFE be re-calibrated any time the gain, SPS, or channel number is changed.
bool NAU7802::calibrateAFE()
{
  beginCalibrateAFE();
  return waitForCalibrateAFE(1000);
}


//*****************************************************************************
//*****************************************************************************
//Begin asynchronous calibration of the analog front end.
// Poll for completion with calAFEStatus() or wait with waitForCalibrateAFE()
void NAU7802::beginCalibrateAFE()
{
  setBit(NAU7802_CTRL2_CALS, NAU7802_CTRL2);
}


//*****************************************************************************
//*****************************************************************************
//Check calibration status.
NAU7802_Cal_Status NAU7802::calAFEStatus()
{
  if (getBit(NAU7802_CTRL2_CALS, NAU7802_CTRL2))
  {
    return NAU7802_CAL_IN_PROGRESS;
  }

  if (getBit(NAU7802_CTRL2_CAL_ERROR, NAU7802_CTRL2))
  {
    return NAU7802_CAL_FAILURE;
  }

  // Calibration passed
  return NAU7802_CAL_SUCCESS;
}


//*****************************************************************************
//*****************************************************************************
//Wait for asynchronous AFE calibration to complete with optional timeout.
//If timeout is not specified (or set to 0), then wait indefinitely.
//Returns true if calibration completes succsfully, otherwise returns false.
bool NAU7802::waitForCalibrateAFE(quint32 timeout_ms)
{
  quint32 begin = millis();
  NAU7802_Cal_Status cal_ready;

  while ((cal_ready = calAFEStatus()) == NAU7802_CAL_IN_PROGRESS)
  {
    if ((timeout_ms > 0) && ((millis() - begin) > timeout_ms))
    {
      break;
    }
    delay(1);
  }

  if (cal_ready == NAU7802_CAL_SUCCESS)
  {
    return (true);
  }
  return (false);
}


