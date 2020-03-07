#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QDate>
#include <QTcpSocket>
#include <QSettings>
#include <wiringPi.h>
#include "HX711.h"

//*** page constants ***
const int CONNECT_PAGE   = 0;
const int NAME_PAGE      = 1;
const int WEIGH_PAGE     = 2;
const int CALIBRATE_PAGE = 3;
const int SHUTDOWN_PAGE  = 4;

const quint16 SCALE_PORT = 29456;

const int WEIGHT_TIMER_MSEC = 333;

const QString ORG_NAME = "FoodPantry";
const QString APP_NAME = "FoodPantry";

const QString TARE_STR  = "TARE";
const QString SCALE_STR = "SCALE";

const int    DEFAULT_TARE  = -214054;
const double DEFAULT_SCALE = 0.0000913017;

const int DT_PIN = 21;
const int SCK_PIN = 20;

const QString CAL_STR_1 = "Empty Scale\nClick Continue";
const QString CAL_STR_2 = "Add 10 lbs to scale\nClick Continue";
const QString CAL_STR_3 = "Calibration Complete";

const float CAL_WEIGHT = 10.0;

const float BASKET_TARE = 3.5;


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::MainWindow
 * @param parent
 */
//*****************************************************************************
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connected_ = false;
    svr_ = nullptr;
    curCalMode_ = NOCAL_MODE;

    ui->weighBtn->setStyleSheet( "background-color: green" );
    ui->clearLastBtn->setStyleSheet( "background-color: red" );
    ui->doneBtn->setStyleSheet( "background-color: blue" );

    connect( ui->actionConnect, SIGNAL(triggered()), SLOT(handleConnect()) );
    connect( ui->actionDisconnect, SIGNAL(triggered()), SLOT(handleDisconnect()) );
    connect( ui->actionAdd_name, SIGNAL(triggered()), SLOT(handleAddName()) );

    connect( ui->nameList, SIGNAL(itemPressed(QListWidgetItem*)), SLOT(handleNameSelected(QListWidgetItem*)) );

    connect( ui->weighBtn, SIGNAL(clicked()), SLOT(handleWeigh()) );
    connect( ui->clearLastBtn, SIGNAL(clicked()), SLOT(handleClearLast()) );
    connect( ui->doneBtn, SIGNAL(clicked()), SLOT(handleDone()) );

    connect( ui->calibrateBtn_1, SIGNAL(clicked()), SLOT(handleCalibrate()) );
    connect( ui->calibrateBtn_2, SIGNAL(clicked()), SLOT(handleCalibrate()) );
    connect( ui->shutdownBtn_1, SIGNAL(clicked()), SLOT(handleShutdown()) );
    connect( ui->shutdownBtn_2, SIGNAL(clicked()), SLOT(handleShutdown()) );
    connect( ui->cancelCalibrateBtn, SIGNAL(clicked()), SLOT(handleCancelCalibrate()) );
    connect( ui->continueBtn, SIGNAL(clicked()), SLOT(handleCalibrateContinue()) );

    ui->widgetStack->setCurrentIndex( CONNECT_PAGE );

    //*** initialize for settings ***
    QCoreApplication::setOrganizationName( ORG_NAME );
    QCoreApplication::setApplicationName( APP_NAME );

    //*** load the settings ***
    loadSettings();

    //*** set up the scale ***
    setupScale();

    //*** weight timer ***
    weightTimer_ = new QTimer( this );
    weightTimer_->setInterval( WEIGHT_TIMER_MSEC );
    connect( weightTimer_, SIGNAL(timeout()), SLOT(displayWeight()) );
    weightTimer_->start();

    initFakeData();

    //*** set up the TCP server ***
    setupServer();

    //*** clear all weight displays ***
    ui->weighLbl_1->clear();
    ui->weighLbl_2->clear();
    ui->weighLbl_3->clear();
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::~MainWindow
 */
//*****************************************************************************
MainWindow::~MainWindow()
{
    //*** save scale calibration values ***
    saveSettings();

    delete ui;

    //*** TCP server ***
    if ( svr_ ) delete svr_;

    //*** weight timer ***
    if ( weightTimer_ ) delete weightTimer_;
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::handleConnect
 */
//*****************************************************************************
void MainWindow::handleConnect()
{
    ui->widgetStack->setCurrentIndex( NAME_PAGE );

    connected_ = true;
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::handleDisconnect
 */
//*****************************************************************************
void MainWindow::handleDisconnect()
{
    ui->widgetStack->setCurrentIndex( CONNECT_PAGE );

    connected_ = false;
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::handleAddName
 */
//*****************************************************************************
void MainWindow::handleAddName()
{
t_CheckIn ci;

    if ( !fake_.isEmpty() )
    {
        ci = fake_.takeFirst();

        QString name = ci.name;

        clients_[name] = ci;

        ui->nameList->addItem( name );
    }
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::handleCalibrate
 */
//*****************************************************************************
void MainWindow::handleCalibrate()
{
    if ( curCalMode_ != NOCAL_MODE ) return;

    ui->widgetStack->setCurrentIndex( CALIBRATE_PAGE );

    ui->calibrateLbl->setText( CAL_STR_1 );
    curCalMode_ = TARE_MODE;

    weightTimer_->stop();
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::handleCalibrateContinue
 */
//*****************************************************************************
void MainWindow::handleCalibrateContinue()
{
QList<int> data;

    if ( curCalMode_ == NOCAL_MODE ) return;

    if ( curCalMode_ == TARE_MODE )
    {
        //*** get average raw value for tare ***
        calTareVal_ = HX711_getRawAverage( 10, data );

//        qDebug() << "calTareVal_:" << calTareVal_;

        ui->calibrateLbl->setText( CAL_STR_2 );

        curCalMode_ = WEIGHT_MODE;
    }

    else if ( curCalMode_ == WEIGHT_MODE )
    {
        //*** get average raw value for the weight ***
        calWeightVal_ = HX711_getRawAverage( 10, data );

        //*** set new calibration data ***
        HX711_setCalibrationData( calTareVal_, calWeightVal_, CAL_WEIGHT );

        //*** get and save new values ***
        HX711_getCalibrationData( tare_, scale_ );
        saveSettings();

        ui->calibrateLbl->setText( CAL_STR_3 );

        curCalMode_ = NOCAL_MODE;

        weightTimer_->start();

        handleCancelCalibrate();
    }
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::handleShutdown
 */
//*****************************************************************************
void MainWindow::handleShutdown()
{
    ui->widgetStack->setCurrentIndex( SHUTDOWN_PAGE );
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::handleCancelCalibrate
 */
//*****************************************************************************
void MainWindow::handleCancelCalibrate()
{
    if ( connected_ )
    {
        ui->widgetStack->setCurrentIndex( NAME_PAGE );
    }
    else
    {
        ui->widgetStack->setCurrentIndex( CONNECT_PAGE );
    }
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::handleNameSelected
 * @param item
 */
//*****************************************************************************
void MainWindow::handleNameSelected( QListWidgetItem *item )
{
    //*** get the selected name ***
    QString name = item->text();

    //*** display the WEIGH page ***
    ui->widgetStack->setCurrentIndex( WEIGH_PAGE );

    //*** set current name ***
    curName_ = name;

    //*** display name at top of page ***
    QString txt = QString( "%1 ( %2 )" ).arg(name).arg(clients_[name].numItems);
    ui->nameLbl->setText( txt );

    //*** clear the weights ***
    ui->weightList->clear();

    //*** default to BASKET tare ***
    ui->basketBtn->setChecked( true );

    //*** remove the name from the list ***
    delete ui->nameList->takeItem( ui->nameList->row( item ) );
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::handleWeigh
 */
//*****************************************************************************
void MainWindow::handleWeigh()
{
    //*** read the scale ***
    float weight = HX711_getWeight();

    if ( ui->basketBtn->isChecked() )
    {
        weight -= BASKET_TARE;
        if ( weight < 0.0 )
        {
            weight = 0.0;
        }
    }

    QString wLine;
    wLine.sprintf( "%.1lf", weight );

    //*** add to the list of weights ***
    ui->weightList->addItem( wLine );
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::handleClearLast
 */
//*****************************************************************************
void MainWindow::handleClearLast()
{
    //*** must be at least one thing in list ***
    if ( ui->weightList->count() > 0 )
    {
        //*** delete the last row ***
        delete ui->weightList->takeItem( ui->weightList->count()-1 );
    }
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::handleDone
 */
//*****************************************************************************
void MainWindow::handleDone()
{
    ui->widgetStack->setCurrentIndex( NAME_PAGE );
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::handleNewConnection
 */
//*****************************************************************************
void MainWindow::handleNewConnection()
{
    //*** get pending connection ***
    QTcpSocket *client = svr_->nextPendingConnection();

    //*** delete on disconnect ***
    connect( client, &QAbstractSocket::disconnected, this, &MainWindow::handleDisconnect);
    connect( client, &QAbstractSocket::disconnected, client, &QObject::deleteLater);

    connect( client, &QAbstractSocket::readyRead, this, &MainWindow::clientDataReady );
    connect( client, SIGNAL(error(QAbstractSocket::SocketError)),
                     SLOT(displayError( QAbstractSocket::SocketError )) );

    //*** handle connection ***
    handleConnect();
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::clientDataReady
 */
//*****************************************************************************
void MainWindow::clientDataReady()
{
t_CheckIn ci;

    //*** get socket that received data ***
    QTcpSocket *sock = (QTcpSocket*)sender();

    //*** get data ***
    while ( sock->bytesAvailable() >= CHECKIN_SIZE )
    {
        if ( sock->read( (char*)&ci, CHECKIN_SIZE ) == CHECKIN_SIZE )
        {
            //*** grab name ***
            QString name = ci.name;

            //*** add to map ***
            clients_[name] = ci;

            //*** display on name screen ***
            ui->nameList->addItem( name );
        }
    }
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::displayError
 * @param socketError
 */
//*****************************************************************************
void MainWindow::displayError( QAbstractSocket::SocketError socketError )
{
Q_UNUSED( socketError )

    //*** get socket that received error ***
    QTcpSocket *sock = static_cast<QTcpSocket*>(sender());

    qDebug() << "Socket Error: " << sock->errorString();
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::displayWeight
 */
//*****************************************************************************
void MainWindow::displayWeight()
{
    float weight = HX711_getWeight();

    QString buf;
    buf.sprintf( "%.1f", weight );

    ui->weighLbl_1->setText( buf );
    ui->weighLbl_2->setText( buf );
    ui->weighLbl_3->setText( buf );
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::setupServer
 */
//*****************************************************************************
void MainWindow::setupServer()
{
    //*** don't set up twice ***
    if ( svr_ ) return;

    //*** create the server ***
    svr_ = new QTcpServer( this );

    //*** start listening on our port ***
    if ( !svr_->listen( QHostAddress::Any, SCALE_PORT ) )
    {
        qDebug() << "Error listening on TCP port!!!";
        ui->connectLbl->setText( "Server Error" );
    }
    else
    {
        connect (svr_, &QTcpServer::newConnection, this, &MainWindow::handleNewConnection );
    }
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::setupScale
 */
//*****************************************************************************
void MainWindow::setupScale()
{
    wiringPiSetupGpio() ;

    HX711_init( DT_PIN, SCK_PIN, tare_, scale_ );
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::addClient
 * @param ci
 */
//*****************************************************************************
void MainWindow::addClient( t_CheckIn &ci )
{
    //*** grab name ***
    QString name = ci.name;

    //*** add to map ***
    if ( !clients_.contains( name ) )
    {
        clients_[name] = ci;

        ui->nameList->addItem( name );
    }
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::loadSettings
 */
//*****************************************************************************
void MainWindow::loadSettings()
{
QSettings s;

    //*** tare value ***
    tare_ = s.value( TARE_STR, DEFAULT_TARE ).toInt();

    //*** scale value ***
    scale_ = s.value( SCALE_STR, DEFAULT_SCALE ).toDouble();
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::saveSettings
 */
//*****************************************************************************
void MainWindow::saveSettings()
{
QSettings s;

    //*** save values ***
    s.setValue( TARE_STR, tare_ );
    s.setValue( SCALE_STR, scale_ );

}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::initFakeData
 */
//*****************************************************************************
void MainWindow::initFakeData()
{
t_CheckIn f1 = { 82,  "Cote, Steven", 24, 0 };
t_CheckIn f2 = { 75,  "Barr, Chris",  30, 0 };
t_CheckIn f3 = { 16,  "Cunha, Lenny", 24, 0 };
t_CheckIn f4 = { 98,  "Dichard, Bob", 18, 0 };
t_CheckIn f5 = { 128, "Cote, Lucy",   30, 0 };

    fake_.append( f1 );
    fake_.append( f2 );
    fake_.append( f3 );
    fake_.append( f4 );
    fake_.append( f5 );
}
