#ifndef NAU7802_H
#define NAU7802_H

#include <QList>
#include <QQueue>
#include <QTimer>
#include <QObject>

typedef QList<int> t_DataSet;

//Register Map
typedef enum
{
  NAU7802_PU_CTRL = 0x00,
  NAU7802_CTRL1,
  NAU7802_CTRL2,
  NAU7802_OCAL1_B2,
  NAU7802_OCAL1_B1,
  NAU7802_OCAL1_B0,
  NAU7802_GCAL1_B3,
  NAU7802_GCAL1_B2,
  NAU7802_GCAL1_B1,
  NAU7802_GCAL1_B0,
  NAU7802_OCAL2_B2,
  NAU7802_OCAL2_B1,
  NAU7802_OCAL2_B0,
  NAU7802_GCAL2_B3,
  NAU7802_GCAL2_B2,
  NAU7802_GCAL2_B1,
  NAU7802_GCAL2_B0,
  NAU7802_I2C_CONTROL,
  NAU7802_ADCO_B2,
  NAU7802_ADCO_B1,
  NAU7802_ADCO_B0,
  NAU7802_ADC = 0x15, //Shared ADC and OTP 32:24
  NAU7802_OTP_B1,     //OTP 23:16 or 7:0?
  NAU7802_OTP_B0,     //OTP 15:8
  NAU7802_PGA = 0x1B,
  NAU7802_PGA_PWR = 0x1C,
  NAU7802_DEVICE_REV = 0x1F,
} Scale_Registers;

//Bits within the PU_CTRL register
typedef enum
{
  NAU7802_PU_CTRL_RR = 0,
  NAU7802_PU_CTRL_PUD,
  NAU7802_PU_CTRL_PUA,
  NAU7802_PU_CTRL_PUR,
  NAU7802_PU_CTRL_CS,
  NAU7802_PU_CTRL_CR,
  NAU7802_PU_CTRL_OSCS,
  NAU7802_PU_CTRL_AVDDS,
} PU_CTRL_Bits;

//Bits within the CTRL1 register
typedef enum
{
  NAU7802_CTRL1_GAIN = 2,
  NAU7802_CTRL1_VLDO = 5,
  NAU7802_CTRL1_DRDY_SEL = 6,
  NAU7802_CTRL1_CRP = 7,
} CTRL1_Bits;

//Bits within the CTRL2 register
typedef enum
{
  NAU7802_CTRL2_CALMOD = 0,
  NAU7802_CTRL2_CALS = 2,
  NAU7802_CTRL2_CAL_ERROR = 3,
  NAU7802_CTRL2_CRS = 4,
  NAU7802_CTRL2_CHS = 7,
} CTRL2_Bits;

//Bits within the PGA register
typedef enum
{
  NAU7802_PGA_CHP_DIS = 0,
  NAU7802_PGA_INV = 3,
  NAU7802_PGA_BYPASS_EN,
  NAU7802_PGA_OUT_EN,
  NAU7802_PGA_LDOMODE,
  NAU7802_PGA_RD_OTP_SEL,
} PGA_Bits;

//Bits within the PGA PWR register
typedef enum
{
  NAU7802_PGA_PWR_PGA_CURR = 0,
  NAU7802_PGA_PWR_ADC_CURR = 2,
  NAU7802_PGA_PWR_MSTR_BIAS_CURR = 4,
  NAU7802_PGA_PWR_PGA_CAP_EN = 7,
} PGA_PWR_Bits;

//Allowed Low drop out regulator voltages
typedef enum
{
  NAU7802_LDO_2V4 = 0b111,
  NAU7802_LDO_2V7 = 0b110,
  NAU7802_LDO_3V0 = 0b101,
  NAU7802_LDO_3V3 = 0b100,
  NAU7802_LDO_3V6 = 0b011,
  NAU7802_LDO_3V9 = 0b010,
  NAU7802_LDO_4V2 = 0b001,
  NAU7802_LDO_4V5 = 0b000,
} NAU7802_LDO_Values;

//Allowed gains
typedef enum
{
  NAU7802_GAIN_128 = 0b111,
  NAU7802_GAIN_64 = 0b110,
  NAU7802_GAIN_32 = 0b101,
  NAU7802_GAIN_16 = 0b100,
  NAU7802_GAIN_8 = 0b011,
  NAU7802_GAIN_4 = 0b010,
  NAU7802_GAIN_2 = 0b001,
  NAU7802_GAIN_1 = 0b000,
} NAU7802_Gain_Values;

//Allowed samples per second
typedef enum
{
  NAU7802_SPS_320 = 0b111,
  NAU7802_SPS_80 = 0b011,
  NAU7802_SPS_40 = 0b010,
  NAU7802_SPS_20 = 0b001,
  NAU7802_SPS_10 = 0b000,
} NAU7802_SPS_Values;

//Select between channel values
typedef enum
{
  NAU7802_CHANNEL_1 = 0,
  NAU7802_CHANNEL_2 = 1,
} NAU7802_Channels;

//Calibration state
typedef enum
{
  NAU7802_CAL_SUCCESS = 0,
  NAU7802_CAL_IN_PROGRESS = 1,
  NAU7802_CAL_FAILURE = 2,
} NAU7802_Cal_Status;


//************************************************************************
//************************************************************************
class NAU7802 : public QObject
{
    Q_OBJECT

public:

    //*** constructor ***
    NAU7802( int rawTare, double scale );

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


public slots:

    void takeReading();

private:

    bool available();                                        //Returns true if Cycle Ready bit is set (conversion is complete)
    qint32 getReading();                                     //Returns 24-bit reading. Assumes CR Cycle Ready bit (ADC conversion complete) has been checked by .available()
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
    bool calibrateAFE();                                     //Synchronous calibration of the analog front end of the NAU7802. Returns true if CAL_ERR bit is 0 (no error)
    void beginCalibrateAFE();                                //Begin asynchronous calibration of the analog front end of the NAU7802. Poll for completion with calAFEStatus() or wait with waitForCalibrateAFE().
    bool waitForCalibrateAFE(quint32 timeout_ms = 0);         //Wait for asynchronous AFE calibration to complete with optional timeout.
    NAU7802_Cal_Status calAFEStatus();                       //Check calibration status.


    //*** gets the number of requested samples from the collected queue ***
    //*** returns the number of samples actually retrieved ***
    int getSamples( int numSamples, t_DataSet &data );

    //*** fd for i2c device ***
    int nau7802_;

    //*** samples queue ***
    QQueue<int> collectedData_;

    //*** raw tare value (zero weight) ***
    int tare_;

    //*** scale factor to produce desired weight units ***
    //*** produced as part of calibration
    double scale_;

    //*** sample collection timer ***
    QTimer *sampleTimer_;

};

#endif
