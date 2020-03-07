#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include <QHash>
#include <QTcpServer>
#include <QTimer>

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

const int CHECKIN_SIZE = sizeof( t_CheckIn );

typedef enum { NOCAL_MODE, TARE_MODE, WEIGHT_MODE } CalMode;

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

    void displayWeight();


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

    //*** initialize the fake data for testing ***
    void initFakeData();

    //*** UI ***
    Ui::MainWindow *ui;

    //*** map client name to client data ***
    QHash<QString,t_CheckIn> clients_;

    //*** current name being processed ***
    QString curName_;

    //*** flag that shows if the fpSvr is connected ***
    bool connected_;

    //*** TCP server ***
    QTcpServer *svr_;

    //*** timer to display weight ***
    QTimer *weightTimer_;

    //*** scale vars ***
    int tare_;
    double scale_;

    CalMode curCalMode_;
    int calTareVal_;
    int calWeightVal_;

    //*** list of fake data for testing ***
    QList<t_CheckIn> fake_;

};

#endif // MAINWINDOW_H
