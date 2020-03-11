#include "KeyPad.h"
#include "ui_KeyPad.h"

#include <QList>
#include <QPushButton>


//*****************************************************************************
//*****************************************************************************
/**
 * @brief KeyPad::KeyPad
 * @param parent
 */
//*****************************************************************************
KeyPad::KeyPad(QString title, QWidget *parent ) :
    QDialog(parent),
    ui(new Ui::KeyPad)
{
QList<QPushButton*> btns;

    btns << ui->btn_0 << ui->btn_1 << ui->btn_2 << ui->btn_3 << ui->btn_4
         << ui->btn_5 << ui->btn_6 << ui->btn_7 << ui->btn_8 << ui->btn_9;

    ui->setupUi(this);

    //*** set up signal mapper ***
    mapper_ = new QSignalMapper( this );

    foreach( auto btn, btns )
        connect( btn, SIGNAL(clicked()), mapper_, SLOT(map()));
    for ( int i=0; i<btns.size(); i++ )
        mapper_->setMapping( btns[i], i );

    connect( mapper_, SIGNAL(mapped(int)), this, SLOT(handleNumClick(int)) );

    //*** other buttons ***
    connect( ui->delBtn,    SIGNAL(clicked()), SLOT(handleDel()) );
    connect( ui->clrBtn,    SIGNAL(clicked()), SLOT(handleClear()) );
    connect( ui->okBtn,     SIGNAL(clicked()), SLOT(accept()) );
    connect( ui->cancelBtn, SIGNAL(clicked()), SLOT(reject()) );

    //*** clear the initial value ***
    ui->dataLbl->clear();

    //*** display label text ***
    ui->mainLbl->setText( title );
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief KeyPad::~KeyPad
 */
//*****************************************************************************
KeyPad::~KeyPad()
{
    delete ui;
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief KeyPad::handleNumClick
 * @param val
 */
//*****************************************************************************
void KeyPad::handleNumClick( int val )
{
    valStr_ += QString::number( val );
    ui->dataLbl->setText( valStr_ );
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief KeyPad::handleDel
 */
//*****************************************************************************
void KeyPad::handleDel()
{
    if ( !valStr_.isEmpty() )
    {
        valStr_.chop( 1 );
        ui->dataLbl->setText( valStr_ );
    }
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief KeyPad::handleClear
 */
//*****************************************************************************
void KeyPad::handleClear()
{
    valStr_.clear();
    ui->dataLbl->setText( valStr_ );
}
