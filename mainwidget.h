#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QBoxLayout>
#include <QScreen>
#include <QApplication>
#include <pluginsiteminterface.h>
#include "type.h"

class MainWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MainWidget(Dock::Position position);
    ~MainWidget() override;

    void updateSpeed(const NetSpeedInfo &info, Dock::Position position);
    QSize sizeHint() const override;

private:
    int fixedWidth() const;

    QLabel *m_upLabel;
    QLabel *m_downLabel;
    QBoxLayout *m_layout;
    QFont m_font;
    int m_dpi;
    int m_fixedW;
    Dock::Position m_position;
};

#endif
