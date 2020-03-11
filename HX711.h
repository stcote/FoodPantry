#ifndef HX711_H
#define HX711_H

#include <QList>
#include <QQueue>

typedef QList<int> t_DataSet;

class HX711
{
public:

    //*** constructor ***
    HX711( int DT_GPIO, int SCK_GPIO, int rawTare, double scale );

    //*** destructor ***
    ~HX711();

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
