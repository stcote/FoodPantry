#ifndef NAMELISTDLG_H
#define NAMELISTDLG_H

#include <QDialog>

namespace Ui {
class NameListDlg;
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief The NameListDlg class
 */
//*****************************************************************************
class NameListDlg : public QDialog
{
    Q_OBJECT

public:
    explicit NameListDlg( QStringList &list, QWidget *parent = nullptr);
    ~NameListDlg();

    QString getName();

private slots:

    void selectionChanged();

private:
    Ui::NameListDlg *ui;
};

#endif // NAMELISTDLG_H
