#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include <QHash>
#include <QTcpServer>

namespace Ui {
class MainWindow;
}

const int NAME_MAX = 127;

typedef struct
{
    int key;
    char name[NAME_MAX+1];
    int  numItems;
    qint64 day;
} t_CheckIn;

const int CHECKIN_SIZE = sizeof( t_CheckIn );


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

    void handleNameSelected( QListWidgetItem* item );

    void handleWeigh();
    void handleClearLast();
    void handleDone();

    void handleNewConnection();
    void clientDataReady();
    void displayError( QAbstractSocket::SocketError socketError );


private:

    //*** set up the TCP server ***
    void setupServer();

    //*** add a new client to the list ***
    void addClient( t_CheckIn &ci );

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

    //*** list of fake data for testing ***
    QList<t_CheckIn> fake_;

};

#endif // MAINWINDOW_H
