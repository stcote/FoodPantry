#ifndef HX711_H
#define HX711_H

#include <QList>

typedef QList<int> t_DataSet;

class HX711 : public QObject
{
    Q_OBJECT

public:

    enum CollectMode { NO_MODE, WEIGHT_MODE, RAW_MODE, RAW_AVG_MODE };

    //*** constructor ***
    HX711( int DT_GPIO, int SCK_GPIO, int rawTare, double scale );

    //*** destructor ***
    ~HX711();

    //*** request the current scale weight ***
    //*** signal 'weight()' at end
    bool getWeight();

    //*** collect n samples - reject outliers ***
    //*** signal 'rawData()' at end ***
    bool collectRawData( int numSamples );

    //*** get the raw average of n samples - reject outliers ***
    //*** signal 'rawAvg()' at end ***
    bool getRawAvg( int numSamples );


    void setCalibrationData( int tareVal, int weightVal, float actualWeight );

    void getCalibrationData( int &rawTareValue, double &scaleValue );\

    //*** invoked by ISR when collection complete ***
    void dataCollected();


signals:

    void weight( float wt );

    void rawAvg( int avg );

    void rawData( t_DataSet data );

    //*** private signal ***
    void collectionComplete( t_DataSet data );



private slots:

    void handleCollectionDone( t_DataSet data );


private:

    //*** raw tare value (zero weight) ***
    int tare_;

    //*** scale factor to produce desired weight units ***
    //*** produced as part of calibration
    double scale_;

    //*** current collection mode ***
    CollectMode curCollectMode_;

};

#if 0
   //************************
   //*** Public Functions ***
   //************************

   void  HX711_init( int DT_Pin, int SC_Pin, int rawTare, double scale );

   float HX711_getWeight();

   int   HX711_getRawReading();

   void  HX711_collectRawData( int numSamples, QList<int> &data );

   int   HX711_getRawAverage(int numSamples , QList<int> &data );

   void  HX711_setCalibrationData( int tareVal, int weightVal, float actualWeight );

   void  HX711_getCalibrationData( int &rawTareValue, double &scaleValue );


   //***********************
   //*** local functions ***
   //***********************

   void H_pulseDelay();

   int H_extendSign( int val );

   //*** Interrupt Service Routine ***
//   static void H_fallingEdgeISR();
#endif

#endif
