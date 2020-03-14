#include "NameListDlg.h"
#include "ui_NameListDlg.h"


//*****************************************************************************
//*****************************************************************************
/**
 * @brief NameListDlg::NameListDlg
 * @param parent
 */
//*****************************************************************************
NameListDlg::NameListDlg( QStringList &list, QWidget *parent ) :
    QDialog(parent),
    ui(new Ui::NameListDlg)
{
    ui->setupUi(this);

    //*** add all names to the list widget ***
    foreach( QString name, list )
    {
        ui->nameList->addItem( name );
    }

    setStyleSheet(  "QPushButton { background-color: blue; color: white; border: off; } "
                    "QScrollBar:vertical { width: 50px; }" );

    //*** disable OK until selection is made ***
    ui->okBtn->setEnabled( false );

    //*** connect to change in selection ***
    connect( ui->nameList, SIGNAL(itemSelectionChanged()), SLOT(selectionChanged()) );

    connect( ui->okBtn, SIGNAL(clicked()), SLOT(accept()) );
    connect( ui->cancelBtn, SIGNAL(clicked()), SLOT(reject()) );
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief NameListDlg::~NameListDlg
 */
//*****************************************************************************
NameListDlg::~NameListDlg()
{
    delete ui;
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief NameListDlg::getName
 * @return
 */
//*****************************************************************************
QString NameListDlg::getName()
{
QString retName;

    //*** get list of selections (should be only one) ***
    auto items = ui->nameList->selectedItems();

    //*** must have at leaast one ***
    if ( !items.isEmpty() )
    {
        //*** grab the text (name) ***
        retName = items.first()->text();
    }

    return retName;
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief NameListDlg::selectionChanged
 */
//*****************************************************************************
void NameListDlg::selectionChanged()
{
    //*** enable the OK button when there is a selection ***
    ui->okBtn->setEnabled( !ui->nameList->selectedItems().isEmpty() );
}
