#include "mytextedit.h"

MyTextEdit::MyTextEdit(QWidget *parent):
    QTextEdit(parent)
{

}

void MyTextEdit::keyPressEvent(QKeyEvent *e)
{
    if ((e->key() == Qt::Key_Return) || (e->key() == Qt::Key_Enter))
    {
        emit enterPressed();
    }
}
