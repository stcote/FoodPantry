#ifndef NAU7802_H
#define NAU7802_H

#include <QList>
#include <QQueue>

typedef QList<int> t_DataSet;


class NAU7802
{
public:

    //*** constructor ***
    NAU7802( int interruptPin, int rawTare, double scale );

    //*** destructor ***
    ~NAU7802();

    //*** set up the scale ***
    bool setup();

    //*** request the current scale weight ***
    float getWeight();

    //*** collect n samples  ***
    int collectRawData( int numSamples, t_DataSet &data );

    //*** collect n samples - reject outliers ***
    int collectProcessedData( int numSamples, t_DataSet &data );

    //*** get the raw average of n samples - reject outliers ***
    int getRawAvg( int numSamples );

    //*** set the data used for calibration ***
    void setCalibrationData( int tareVal, int weightVal, float actualWeight );

    //*** get the data derived from the calibration ***
    void getCalibrationData( int &rawTareValue, double &scaleValue );

    //*** start using a new tare value ***
    void setTare( int tareVal );


private:

    bool setGain(quint8 gainValue);                          //Set the gain. x1, 2, 4, 8, 16, 32, 64, 128 are available
    bool setLDO(quint8 ldoValue);                            //Set the onboard Low-Drop-Out voltage regulator to a given value. 2.4, 2.7, 3.0, 3.3, 3.6, 3.9, 4.2, 4.5V are avaialable
    bool setSampleRate(quint8 rate);                         //Set the readings per second. 10, 20, 40, 80, and 320 samples per second is available
    bool setChannel(quint8 channelNumber);                   //Select between 1 and 2
    bool reset();                                            //Resets all registers to Power Of Defaults
    bool powerUp();                                          //Power up digital and analog sections of scale, ~2mA
    bool powerDown();                                        //Puts scale into low-power 200nA mode
    bool setIntPolarityHigh();                               //Set Int pin to be high when data is ready (default)
    bool setIntPolarityLow();                                //Set Int pin to be low when data is ready
    quint8 getRevisionCode();                                //Get the revision code of this IC. Always 0x0F.
    bool setBit(quint8 bitNumber, quint8 registerAddress);   //Mask & set a given bit within a register
    bool clearBit(quint8 bitNumber, quint8 registerAddress); //Mask & clear a given bit within a register
    bool getBit(quint8 bitNumber, quint8 registerAddress);   //Return a given bit within a register
    quint8 getRegister(quint8 registerAddress);              //Get contents of a register
    bool setRegister(quint8 registerAddress, quint8 value);  //Send a given value to be written to given address. Return true if successful
    qint32 getReading();                                     //Returns 24-bit reading. Assumes CR Cycle Ready bit (ADC conversion complete) has been checked by .available()



    //*** gets the number of requested samples from the collected queue ***
    //*** returns the number of samples actually retrieved ***
    int getSamples( int numSamples, t_DataSet &data );

    //*** raw tare value (zero weight) ***
    int tare_;

    //*** scale factor to produce desired weight units ***
    //*** produced as part of calibration
    double scale_;

};

#endif
