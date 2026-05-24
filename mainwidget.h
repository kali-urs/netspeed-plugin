#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QGridLayout>
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
    void parseSpeed(const QString &speed, QString &num, QString &unit) const;

    QLabel *m_upNum;
    QLabel *m_upUnit;
    QLabel *m_downNum;
    QLabel *m_downUnit;
    QGridLayout *m_layout;
    QFont m_font;
    int m_dpi;
    int m_numWidth;
    Dock::Position m_position;
};

#endif
