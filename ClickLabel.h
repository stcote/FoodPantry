#ifndef CLICKLABEL_H
#define CLICKLABEL_H

#include <QLabel>

class ClickLabel : public QLabel
{
    Q_OBJECT

public:
    ClickLabel( QWidget *parent = Q_NULLPTR ) : QLabel(parent) {}
    ~ClickLabel() {}

signals:

    void clicked();

protected:

    void mousePressEvent( QMouseEvent* );

};

#endif // CLICKLABEL_H
