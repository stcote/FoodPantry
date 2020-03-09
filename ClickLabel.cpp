#include "ClickLabel.h"


void ClickLabel::mousePressEvent( QMouseEvent* )
{
    emit clicked();
}
