#ifndef HX711_H
#define HX711_H

#include <QList>

    //****************
    //*** typedefs ***
    //****************
   typedef long long NSecTime;


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

//   NSecTime H_getNSecTime();

   int H_extendSign( int val );

   //*** Interrupt Service Routine ***
   static void H_fallingEdgeISR();


#endif
