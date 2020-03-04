#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QDate>
#include <QTcpSocket>

//*** page constants ***
const int CONNECT_PAGE   = 0;
const int NAME_PAGE      = 1;
const int WEIGH_PAGE     = 2;
const int CALIBRATE_PAGE = 3;
const int SHUTDOWN_PAGE  = 4;

const quint16 SCALE_PORT = 29456;


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

    ui->widgetStack->setCurrentIndex( CONNECT_PAGE );

    initFakeData();

    //*** set up the TCP server ***
    setupServer();
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief MainWindow::~MainWindow
 */
//*****************************************************************************
MainWindow::~MainWindow()
{
    delete ui;

    if ( svr_ ) delete svr_;
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
    ui->widgetStack->setCurrentIndex( CALIBRATE_PAGE );
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
const double MAXVAL = (double)RAND_MAX;

    //*** create a number between 5 and 25 ***
    double w = ((double)qrand() / MAXVAL) * 20 + 5;
    QString wLine;
    wLine.sprintf( "%.1lf", w );

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
