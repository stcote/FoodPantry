#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "KeyPad.h"
#include "NameListDlg.h"

#include <stdlib.h>
#include <QDate>
#include <QTcpSocket>
#include <QSettings>
#include <QGuiApplication>
#include <QScreen>
#include <QThread>
#include <QScrollBar>
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
const QString CALWT_STR = "CALWT";

const int    DEFAULT_TARE  = -214054;
const double DEFAULT_SCALE = 0.0000913017;
const float  DEFAULT_CALWT = 10.0;

const float  MIN_VALID_WEIGHT = 0.5;

const int DT_PIN  = 21;
const int SCK_PIN = 20;

const QString CAL_STR_1 = "Empty Scale\nClick Continue";
const QString CAL_STR_2 = "Add %.1f lbs to scale\nClick Continue";

const int NUM_CAL_SAMPLES = 10;
const int NUM_TARE_SAMPLES = 4;

const float BASKET_TARE = 3.5;

const int TOUCHSCREEN_Y = 480;

//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::MainWindow - Constructor
 * @param parent - parent widget
 */
//*****************************************************************************
MainWindow::MainWindow( QWidget *parent ) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //*** initialize vars ***
    connected_  = false;
    svr_        = nullptr;
    client_     = nullptr;
    curCalMode_ = NOCAL_MODE;

    //*** button colors ***
    setStyleSheet(  "QPushButton { background-color: blue; color: white; border: off; } "
                    "QScrollBar:vertical { width: 50px; }" );
    ui->exitBtn->setStyleSheet(  "QPushButton { background-color: red; color: white; border: off; } " );


    //*** make connections ***
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

    connect( ui->shutdownBtn, SIGNAL(clicked()), SLOT(shutdownNow()) );

    connect( ui->weighLbl_1, SIGNAL(clicked()), SLOT(handleTare()) );
    connect( ui->weighLbl_2, SIGNAL(clicked()), SLOT(handleTare()) );
    connect( ui->weighLbl_3, SIGNAL(clicked()), SLOT(handleTare()) );

    connect( ui->exitBtn, SIGNAL(clicked()), SLOT(close()) );
    connect( ui->backBtn, SIGNAL(clicked()), SLOT(handleCancelCalibrate()) );
    connect( ui->changeCalBtn, SIGNAL(clicked() ), SLOT(handleChangeCalWeight()) );
    connect( ui->restoreNameBtn, SIGNAL(clicked()), SLOT(handleRestoreNameBtn()) );

    //*** first page displayed ***
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
    connect( weightTimer_, SIGNAL(timeout()), SLOT(requestWeight()) );
    QTimer::singleShot( 2000, weightTimer_, SLOT(start()) );

    //*** TODO - Remove after testing phase ***
    initFakeData();

    //*** set up the TCP server ***
    setupServer();

    //*** clear all weight displays ***
    ui->weighLbl_1->clear();
    ui->weighLbl_2->clear();
    ui->weighLbl_3->clear();

    //*** determine if we are using the touchscreen ***
    int screenY = QGuiApplication::primaryScreen()->geometry().height();
    if ( screenY == TOUCHSCREEN_Y )
    {
        //*** hide menu bar ***
        ui->menuBar->hide();

        //*** make frameless ***
        setWindowFlags( Qt::FramelessWindowHint );

        //*** full screen ***
        showFullScreen();
    }

    //*** desensitize 'restore name' button until we have a name to restore ***
    ui->restoreNameBtn->setEnabled( false );

    //*** Add cal weight to button ***
    displayCalWeight();
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::~MainWindow - Destructor
 */
//*****************************************************************************
MainWindow::~MainWindow()
{
    //*** save scale calibration values ***
    saveSettings();

    delete ui;

    //*** scale object ***
    if ( hx711_ ) delete hx711_;

    //*** TCP server ***
    if ( svr_ ) delete svr_;

    //*** weight timer ***
    if ( weightTimer_ ) delete weightTimer_;
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::handleConnect - called when the checkin server connects
 */
//*****************************************************************************
void MainWindow::handleConnect()
{
    //*** display name list page ***
    ui->widgetStack->setCurrentIndex( NAME_PAGE );

    //*** set flag to indicate we are connected ***
    connected_ = true;
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::handleDisconnect - called when the checkin server
 *              disconnects
 */
//*****************************************************************************
void MainWindow::handleDisconnect()
{
    //*** display 'waiting to connect' page ***
    ui->widgetStack->setCurrentIndex( CONNECT_PAGE );

    //*** set flag to indicate we are no longer connected ***
    connected_ = false;
}


//*** TODO - remove after testing ***
//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::handleAddName - For testing purposes only
 */
//*****************************************************************************
void MainWindow::handleAddName()
{
t_CheckIn ci;

    //*** do we have more names in fake list? ***
    if ( !fake_.isEmpty() )
    {
        //*** grab the first record ***
        ci = fake_.takeFirst();

        //*** extract the name ***
        QString name = ci.name;

        //*** add to map ***
        clients_[name] = ci;

        //*** add to the name list ***
        addToNameList( name );
        allNames_.append( name );
        ui->restoreNameBtn->setEnabled( true );
    }
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::handleCalibrate - called when calibrate is requested
 */
//*****************************************************************************
void MainWindow::handleCalibrate()
{
    //*** ignore if we are already in calibration mode ***
    if ( curCalMode_ != NOCAL_MODE ) return;

    //*** display the calibration page ***
    ui->widgetStack->setCurrentIndex( CALIBRATE_PAGE );

    //*** display the first user prompt (empty scale) ***
    ui->calibrateLbl->setText( CAL_STR_1 );

    //*** enter the TARE state ***
    curCalMode_ = CAL_TARE_MODE;

    //*** no weight updates while we are doing this ***
    weightTimer_->stop();
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::handleCalibrateContinue - called when the 'continue' button
 *              is clicked during the calibration process. Handles the current
 *              phase.
 */
//*****************************************************************************
void MainWindow::handleCalibrateContinue()
{
QList<int> data;

    //*** ignore if not in calibration mode ***
    if ( curCalMode_ == NOCAL_MODE ) return;

    //*** handle TARE mode ***
    if ( curCalMode_ == CAL_TARE_MODE )
    {
        //*** get tare raw value ***
        calTareVal_ = hx711_->getRawAvg( NUM_CAL_SAMPLES );

        //*** create and display next user prompt (add weight to scale) ***
        QString buf;
        buf.sprintf( qPrintable(CAL_STR_2), calWeight_ );
        ui->calibrateLbl->setText( buf );

        //*** enter CAL WEIGHT mode ***
        curCalMode_ = CAL_WEIGHT_MODE;
    }

    //*** handle CAL WEIGHT mode ***
    else if ( curCalMode_ == CAL_WEIGHT_MODE )
    {
        //*** get average raw value for the weight ***
        calWeightVal_ = hx711_->getRawAvg( NUM_CAL_SAMPLES );

        //*** set new calibration data ***
        hx711_->setCalibrationData( calTareVal_, calWeightVal_, calWeight_ );

        //*** get and save new calculated values ***
        hx711_->getCalibrationData( tare_, scale_ );
        saveSettings();

        //*** exit calibration mode ***
        curCalMode_ = NOCAL_MODE;

        //*** resume weight readings ***
        weightTimer_->start();

        //*** exit appropriately ***
        handleCancelCalibrate();
    }
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::handleCancelCalibrate - exits calibration mode. Determines
 *              the current quiescent state and sets the appropriate display.
 */
//*****************************************************************************
void MainWindow::handleCancelCalibrate()
{
    //*** ensure we are out of calibration mode ***
    curCalMode_ = NOCAL_MODE;

    //*** if we are connected to checkin client ***
    if ( connected_ )
    {
        //*** display the name list page ***
        ui->widgetStack->setCurrentIndex( NAME_PAGE );
    }

    //*** not currently connected to the checkin client ***
    else
    {
        //*** display the 'waiting to connect' page ***
        ui->widgetStack->setCurrentIndex( CONNECT_PAGE );
    }
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::handleShutdown - called when the 'shutdown' button is clicked
 */
//*****************************************************************************
void MainWindow::handleShutdown()
{
    //*** display the 'Shutdown' page ***
    ui->widgetStack->setCurrentIndex( SHUTDOWN_PAGE );
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::handleNameSelected - called when the user clicks on a
 *              name in the name list.
 * @param item - the item that was selected via the click
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

    //*** display name at top of page along with # items ***
    QString txt = QString( "%1 ( %2 )" ).arg(name).arg(clients_[name].numItems);
    ui->nameLbl->setText( txt );

    //*** clear the weights ***
    ui->weightList->clear();
    weights_.clear();

    //*** default to BASKET tare ***
    ui->basketBtn->setChecked( true );

    //*** remove the name from the list ***
    removeFromNameList( name );
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::handleWeigh - called when the 'weigh' button is clicked.
 *              Gets the current weight and adds to the list
 */
//*****************************************************************************
void MainWindow::handleWeigh()
{
    //*** read the scale ***
    float weight = hx711_->getWeight();

    //*** factor in basket? ***
    if ( ui->basketBtn->isChecked() )
    {
        //*** subtract the weight of the basket ***
        weight -= BASKET_TARE;

        //*** we shouldn't have a negative weight ***
        if ( weight < 0.0f )
        {
            weight = 0.0;
        }
    }

    //*** format weight ***
    QString wLine;
    wLine.sprintf( "%.1f", weight );

    //*** no negative 0 weights ***
    if ( wLine == "-0.0" ) wLine = "0.0";

    //*** add to the list of weights ***
    ui->weightList->addItem( wLine );
    weights_.append( weight );
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::handleClearLast - called when the 'clear last' button is
 *              clicked. Removes the last collected weight from the list.
 */
//*****************************************************************************
void MainWindow::handleClearLast()
{
    //*** must be at least one thing in list ***
    if ( ui->weightList->count() > 0 )
    {
        //*** delete the last row fro both the control and the list ***
        delete ui->weightList->takeItem( ui->weightList->count()-1 );
        weights_.takeLast();
    }
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::handleDone - called when the 'Done' button is clicked.
 *              Processes the collected weights for the person and sends the
 *              data back to the checkin client.
 */
//*****************************************************************************
void MainWindow::handleDone()
{
float totalWeight = 0.0;    // accumulator
t_WeightReport wr;          // message struct

    //*** return to the name list display ***
    ui->widgetStack->setCurrentIndex( NAME_PAGE );

    ui->restoreNameBtn->setEnabled( true );

    //*** if no weights, just restore name to list ***
    if ( ui->weightList->count() == 0 )
    {
        //*** add name back to list ***
        addToNameList( curName_ );

        //*** done ***
        return;
    }

    //*** total the weight values ***
    foreach( float w, weights_ )
    {
        totalWeight += w;
    }

    //*** can we send message back to client? ( must have a valid weight ) ***
    if ( connected_ && client_ && (totalWeight > MIN_VALID_WEIGHT) )
    {
        //*** send weight report ***
        memset( &wr, 0, WEIGHT_REPORT_SIZE );
        wr.magic = MAGIC_VAL;
        wr.size = WEIGHT_SIZE_FIELD;
        wr.type = WEIGHT_REPORT_TYPE;

        wr.key = clients_[curName_].key;
        wr.weight = totalWeight;
        wr.day = clients_[curName_].day;

        //*** write it to client socket ***
        client_->write( (char*)&wr, WEIGHT_REPORT_SIZE );
    }
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::handleNewConnection - called when a new client connection
 *              is made to the TCP server.
 */
//*****************************************************************************
void MainWindow::handleNewConnection()
{
    //*** get pending connection ***
    client_ = svr_->nextPendingConnection();

    //*** delete on disconnect ***
    connect( client_, &QAbstractSocket::disconnected, this, &MainWindow::handleDisconnect);
    connect( client_, &QAbstractSocket::disconnected, client_, &QObject::deleteLater);

    //*** set up to receive data (and errors) ***
    connect( client_, &QAbstractSocket::readyRead, this, &MainWindow::clientDataReady );
    connect( client_, SIGNAL(error(QAbstractSocket::SocketError)),
                      SLOT(displayError( QAbstractSocket::SocketError )) );

    //*** handle connection in UI ***
    handleConnect();
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::clientDataReady - called when data is available on the
 *              client socket.
 */
//*****************************************************************************
void MainWindow::clientDataReady()
{
t_CheckIn ci;   // checkin data struct

    //*** get socket that received data ***
    QTcpSocket *sock = (QTcpSocket*)sender();

    //*** get data ***
    while ( sock->bytesAvailable() >= CHECKIN_SIZE )
    {
        //*** read structs worth of data ***
        if ( sock->read( (char*)&ci, CHECKIN_SIZE ) == CHECKIN_SIZE )
        {
            //*** grab name ***
            QString name = ci.name;

            //*** if # items > 0, then add to list ***
            if ( ci.numItems != 0 )
            {
                //*** add to map ***
                clients_[name] = ci;

                //*** display on name screen ***
                addToNameList( name );
                allNames_.append( name );
            }

            //*** if numItems == 0, then remove from list ***
            else
            {
                //*** remove from display ***
                removeFromNameList( name );

                //*** remove all traces ***
                allNames_.removeAll( name );
                clients_.remove( name );
            }
        }
    }
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::displayError - called when a TCP error is detected
 * @param socketError - the error that was detected
 */
//*****************************************************************************
void MainWindow::displayError( QAbstractSocket::SocketError socketError )
{
Q_UNUSED( socketError )

    //*** get socket that received error ***
    QTcpSocket *sock = static_cast<QTcpSocket*>(sender());

    //*** TODO - report error somehow ***

    qDebug() << "Socket Error: " << sock->errorString();
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::handleTare - called when the user requests a new tare
 */
//*****************************************************************************
void MainWindow::handleTare()
{
    //*** get the new tare value ***
    tare_ = hx711_->getRawAvg( NUM_TARE_SAMPLES );

    //*** set it in the scale object ***
    hx711_->setTare( tare_ );

    //*** save current value ***
    saveSettings();
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::requestWeight - called by the weight timer to update the
 *              weight displays
 */
//*****************************************************************************
void MainWindow::requestWeight()
{
    //*** read the scale ***
    float weight = hx711_->getWeight();

    //*** format the weight ***
    QString wLine;
    wLine.sprintf( "%.1f", weight );

    //*** no negative 0s ***
    if ( wLine == "-0.0" ) wLine = "0.0";

    //*** update all the weight displays ***
    ui->weighLbl_1->setText( wLine );
    ui->weighLbl_2->setText( wLine );
    ui->weighLbl_3->setText( wLine );
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::handleChangeCalWeight - called when the user wants to
 *              update the actual weight used for the calibration process.
 */
//*****************************************************************************
void MainWindow::handleChangeCalWeight()
{
KeyPad dlg( "Cal Weight", this );   // dialog used for keypad entry
float newVal = 0;                   // temp weight value
const float MAX_CAL_WEIGHT = 50.0;  // sanity check value

    //*** display the keypad dialog ***
    if ( dlg.exec() == QDialog::Accepted )
    {
        //*** get the value entered ***
        newVal = dlg.getValue();

        //*** if value is ok, save it ***
        if ( newVal <= MAX_CAL_WEIGHT )
        {
            calWeight_ = newVal;
            saveSettings();
            displayCalWeight();
        }
    }
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::handleRestoreNameBtn - called when the user wants to
 *              restore a previously used name.
 */
//*****************************************************************************
void MainWindow::handleRestoreNameBtn()
{
    QStringList dlgNames;

    //*** check ALL names ***
    foreach( QString n, allNames_ )
    {
        //*** if name not currently in person list, add to dialog list ***
        if ( !nameList_.contains( n ) )
        {
            dlgNames.append( n );
        }
    }

    NameListDlg dlg( dlgNames );  // dialog that displays names

    //*** if we got here, we have at least one item in list ***
    if ( dlg.exec() == QDialog::Accepted )
    {
        //*** get selected name from dialog ***
        QString name = dlg.getName();

        //*** if valid name and not in list ***
        if ( !name.isEmpty() )
        {
            //*** add back to list of names ***
            addToNameList( name );
        }
    }
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::shutdownNow - called when the user wants to shut down
 *          the Pi.
 */
//*****************************************************************************
void MainWindow::shutdownNow()
{
    //*** request shutdown from the OS ***
    system( " sudo shutdown -h now" );
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::setupServer - Sets up the TCP server, which waits for
 *              incoming TCP connections from the checkin serve
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
        //*** handle incoming connections ***
        connect (svr_, &QTcpServer::newConnection, this, &MainWindow::handleNewConnection );
    }
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::setupScale - sets up the scale object
 */
//*****************************************************************************
void MainWindow::setupScale()
{
    //*** class that reads serial data from the hx711 board ***
    hx711_ = new HX711( DT_PIN, SCK_PIN, tare_, scale_ );
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::addClient - adds a new incoming client to the map and ui
 * @param ci - structure containing client info
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

        //*** add to ui names list ***
        addToNameList( name );
    }
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::loadSettings - loads the various saved settings
 */
//*****************************************************************************
void MainWindow::loadSettings()
{
QSettings s;    // settings object - uses app names set in constructor

    //*** tare value ***
    tare_ = s.value( TARE_STR, DEFAULT_TARE ).toInt();

    //*** scale value ***
    scale_ = s.value( SCALE_STR, DEFAULT_SCALE ).toDouble();

    //*** cal weight setting ***
    calWeight_ = s.value( CALWT_STR, DEFAULT_CALWT ).toFloat();
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::saveSettings - saves the various settings values
 */
//*****************************************************************************
void MainWindow::saveSettings()
{
QSettings s;    // settings object - uses app names set in constructor

    //*** save values ***
    s.setValue( TARE_STR, tare_ );
    s.setValue( SCALE_STR, scale_ );
    s.setValue( CALWT_STR, calWeight_ );
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::displayCalWeight - display the cal weight in the change
 *              button.
 */
//*****************************************************************************
void MainWindow::displayCalWeight()
{
QString buf;

    buf.sprintf( "Change Cal Weight ( %.1f )", calWeight_ );
    ui->changeCalBtn->setText( buf );
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::addToNameList - adds a name to the person list
 * @param name - name to add
 */
//*****************************************************************************
void MainWindow::addToNameList( QString name )
{
    //*** add if not already there ***
    if ( !nameList_.contains( name ) )
    {
        nameList_.append( name );
    }

    //*** redisplay ***
    refreshNameList();
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::removeFromNameList - removes a name from the person list
 * @param name - name to remove
 */
//*****************************************************************************
void MainWindow::removeFromNameList( QString name )
{
    //*** remove from the list ***
    nameList_.removeAll( name );

    //*** redisplay ***
    refreshNameList();
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::refreshNameList - redisplays the person list with the
 *              current set of names
 */
//*****************************************************************************
void MainWindow::refreshNameList()
{
    //*** clear the list control ***
    ui->nameList->clear();

    //*** set the current list of names in the control ***
    ui->nameList->addItems( nameList_ );
}


//*** TODO - remove after initial testing ***
//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::initFakeData - used to simulate data received from
 *              checkin server
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
