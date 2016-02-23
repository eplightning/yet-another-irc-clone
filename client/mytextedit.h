#ifndef MYTEXTEDIT_H
#define MYTEXTEDIT_H

#include <QTextEdit>
#include <QKeyEvent>

class MyTextEdit : public QTextEdit
{
    Q_OBJECT
public:
    MyTextEdit(QWidget *parent = 0);

signals:
    void enterPressed();

private:
    void keyPressEvent(QKeyEvent *e);

public slots:
};

#endif // MYTEXTEDIT_H
