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

    void setScale(int scale);
    void updateSpeed(const NetSpeedInfo &info, Dock::Position position);
    QSize sizeHint() const override;

private:
    void applyScale();
    void recalcFixedWidths();
    void parseSpeed(const QString &speed, QString &num, QString &unit) const;

    QLabel *m_upArrow;
    QLabel *m_upNum;
    QLabel *m_upUnit;
    QLabel *m_downArrow;
    QLabel *m_downNum;
    QLabel *m_downUnit;
    QGridLayout *m_layout;
    QFont m_font;
    int m_dpi;
    int m_arrowWidth;
    int m_numWidth;
    int m_scale;
    Dock::Position m_position;
};

#endif
