#include "MainWindow.h"
#include "ui_MainWindow.h"

const int CONNECT_PAGE = 0;
const int NAME_PAGE = 1;
const int WEIGH_PAGE = 2;


//*****************************************************************************
//*****************************************************************************
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    names_ << "Cote, Steven" << "Barr, Chris" << "Cunha, Lenny" << "Dichard, Bob" << "Cote, Lucy";
    numItems_[names_[0]] = 24;
    numItems_[names_[1]] = 30;
    numItems_[names_[2]] = 18;
    numItems_[names_[3]] = 24;
    numItems_[names_[4]] = 30;

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

    ui->widgetStack->setCurrentIndex( CONNECT_PAGE );
}


//*****************************************************************************
//*****************************************************************************
MainWindow::~MainWindow()
{
    delete ui;
}


//*****************************************************************************
//*****************************************************************************
void MainWindow::handleConnect()
{
    ui->widgetStack->setCurrentIndex( NAME_PAGE );
}


//*****************************************************************************
//*****************************************************************************
void MainWindow::handleDisconnect()
{
    ui->widgetStack->setCurrentIndex( CONNECT_PAGE );
}


//*****************************************************************************
//*****************************************************************************
void MainWindow::handleAddName()
{
    if ( !names_.isEmpty() )
    {
        QString name = names_.takeFirst();

        ui->nameList->addItem( name );
    }
}


//*****************************************************************************
//*****************************************************************************
void MainWindow::handleNameSelected( QListWidgetItem *item )
{
    //*** get the selected name ***
    QString name = item->text();

    //*** display the WEIGH page ***
    ui->widgetStack->setCurrentIndex( WEIGH_PAGE );

    //*** display name at top of page ***
    ui->nameLbl->setText( name + " ( " + QString::number( numItems_[name] ) + " )" );

    //*** clear the weights ***
    ui->weightList->clear();

    //*** default to BASKET tare ***
    ui->basketBtn->setChecked( true );

    //*** remove the name from the list ***
    delete ui->nameList->takeItem( ui->nameList->row( item ) );
}


//*****************************************************************************
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
void MainWindow::handleDone()
{
    ui->widgetStack->setCurrentIndex( NAME_PAGE );
}
