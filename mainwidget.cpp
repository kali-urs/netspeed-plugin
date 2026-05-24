#include "mainwidget.h"
#include <QPalette>
#include <QRegularExpression>

MainWidget::MainWidget(Dock::Position position)
    : m_upNum(nullptr)
    , m_upUnit(nullptr)
    , m_downNum(nullptr)
    , m_downUnit(nullptr)
    , m_layout(nullptr)
    , m_numWidth(0)
    , m_position(position)
{
    m_dpi = QApplication::primaryScreen()->logicalDotsPerInch();
    m_font.setFamily("Noto Mono");
    m_font.setPixelSize((m_dpi * 9) / 72);

    QFontMetrics fm(m_font);
    m_numWidth = fm.horizontalAdvance("↑ 999.9") + 4;

    m_upNum = new QLabel(this);
    m_upUnit = new QLabel(this);
    m_downNum = new QLabel(this);
    m_downUnit = new QLabel(this);

    m_upNum->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_upUnit->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_downNum->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_downUnit->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    m_upNum->setFont(m_font);
    m_upUnit->setFont(m_font);
    m_downNum->setFont(m_font);
    m_downUnit->setFont(m_font);

    m_upNum->setMinimumWidth(m_numWidth);
    m_downNum->setMinimumWidth(m_numWidth);

    m_layout = new QGridLayout;
    m_layout->addWidget(m_upNum, 0, 0);
    m_layout->addWidget(m_upUnit, 0, 1);
    m_layout->addWidget(m_downNum, 1, 0);
    m_layout->addWidget(m_downUnit, 1, 1);
    m_layout->setContentsMargins(4, 0, 4, 0);
    m_layout->setSpacing(0);
    m_layout->setHorizontalSpacing(0);
    setLayout(m_layout);

    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::transparent);
    setPalette(pal);

    updateSpeed({0, 0, "0 B/s", "0 B/s"}, position);
}

MainWidget::~MainWidget() = default;

void MainWidget::parseSpeed(const QString &speed, QString &num, QString &unit) const
{
    QRegularExpression re("^([\\d.]+)\\s*(.*)$");
    auto match = re.match(speed);
    if (match.hasMatch()) {
        num = match.captured(1);
        unit = match.captured(2);
    } else {
        num = speed;
        unit.clear();
    }
}

void MainWidget::updateSpeed(const NetSpeedInfo &info, Dock::Position position)
{
    m_position = position;

    QColor textColor = palette().color(QPalette::WindowText);
    QString colorName = textColor.name();
    QString style = QString("QLabel { color: %1; }").arg(colorName);
    m_upNum->setStyleSheet(style);
    m_upUnit->setStyleSheet(style);
    m_downNum->setStyleSheet(style);
    m_downUnit->setStyleSheet(style);

    QString upNum, upUnit, downNum, downUnit;
    parseSpeed(info.upSpeed, upNum, upUnit);
    parseSpeed(info.downSpeed, downNum, downUnit);

    m_upNum->setText(QString("↑ %1").arg(upNum));
    m_upUnit->setText(QString("%1/s").arg(upUnit));
    m_downNum->setText(QString("↓ %1").arg(downNum));
    m_downUnit->setText(QString("%1/s").arg(downUnit));

    setMinimumSize(0, 0);
    setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    updateGeometry();
}

QSize MainWidget::sizeHint() const
{
    if (!m_upNum || !m_upUnit) {
        return QSize(80, 24);
    }

    QFontMetrics fm(m_font);
    int lineH = fm.height();
    int unitW = fm.horizontalAdvance("MB/s");

    return QSize(m_numWidth + unitW + 8, lineH * 2 + 4);
}
