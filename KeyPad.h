#ifndef KEYPAD_H
#define KEYPAD_H

#include <QDialog>
#include <QSignalMapper>

namespace Ui {
class KeyPad;
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief The KeyPad class
 */
//*****************************************************************************
class KeyPad : public QDialog
{
    Q_OBJECT

public:
    explicit KeyPad( QString title, QWidget *parent = nullptr );
    ~KeyPad();

    float getValue() { return valStr_.toFloat(); }

private slots:

    void handleNumClick( int val );
    void handleDel();
    void handleClear();

signals:
      void clicked( int val );


private:
    Ui::KeyPad *ui;

    QSignalMapper *mapper_;

    QString valStr_;
};

#endif // KEYPAD_H
