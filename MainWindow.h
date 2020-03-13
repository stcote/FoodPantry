#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include <QHash>
#include <QTcpServer>
#include <QTimer>
#include "HX711.h"

namespace Ui {
class MainWindow;
}

const int FP_NAME_MAX = 127;

typedef struct
{
    int key;
    char name[FP_NAME_MAX+1];
    int  numItems;
    qint64 day;
} t_CheckIn;

typedef struct
{
    quint32 magic;
    quint32 size;
    quint32 type;
    int     key;
    float   weight;
    qint64  day;
} t_WeightReport;

const int MAGIC_VAL = 0x3e3e3e3e;

const int CHECKIN_SIZE = sizeof( t_CheckIn );

const int WEIGHT_REPORT_SIZE = sizeof( t_WeightReport );
const int WEIGHT_SIZE_FIELD = WEIGHT_REPORT_SIZE - ( 2 * sizeof(quint32) );
const int WEIGHT_REPORT_TYPE = 0x0001;


typedef enum { NOCAL_MODE, CAL_TARE_MODE, CAL_WEIGHT_MODE, TARE_MODE } CalMode;

//*****************************************************************************
//*****************************************************************************
/**
 * @brief The MainWindow class
 */
//*****************************************************************************
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:

    void handleConnect();
    void handleDisconnect();
    void handleAddName();

    void handleCalibrate();
    void handleShutdown();
    void handleCancelCalibrate();
    void handleCalibrateContinue();

    void handleNameSelected( QListWidgetItem* item );

    void handleWeigh();
    void handleClearLast();
    void handleDone();

    void handleNewConnection();
    void clientDataReady();
    void displayError( QAbstractSocket::SocketError socketError );

    void handleTare();

    void requestWeight();

    void handleChangeCalWeight();

    void handleRestoreNameBtn();

    void shutdownNow();


private:

    //*** set up the TCP server ***
    void setupServer();

    //*** set up scale ***
    void setupScale();

    //*** add a new client to the list ***
    void addClient( t_CheckIn &ci );

    //*** settings ***
    void loadSettings();
    void saveSettings();

    //*** display the cal weight in the change button ***
    void displayCalWeight();

    //*** initialize the fake data for testing ***
    void initFakeData();

    //*** UI ***
    Ui::MainWindow *ui;

    //*** scale reader object ***
    HX711 *hx711_;

    //*** map client name to client data ***
    QHash<QString,t_CheckIn> clients_;

    //*** current name being processed ***
    QString curName_;

    //*** flag that shows if the fpSvr is connected ***
    bool connected_;

    //*** TCP server ***
    QTcpServer *svr_;

    //*** connected client ***
    QTcpSocket *client_;

    //*** timer to display weight ***
    QTimer *weightTimer_;

    //*** list of weight values ***
    QList<float> weights_;

    //*** list of previously used names ***
    QStringList prevNames_;

    //*** scale vars ***
    int tare_;
    double scale_;
    float calWeight_;

    CalMode curCalMode_;
    int calTareVal_;
    int calWeightVal_;

    //*** list of fake data for testing ***
    QList<t_CheckIn> fake_;

};

#endif // MAINWINDOW_H
