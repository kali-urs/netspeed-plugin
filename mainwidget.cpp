#include "mainwidget.h"
#include <QPalette>

MainWidget::MainWidget(Dock::Position position)
    : m_upLabel(nullptr)
    , m_downLabel(nullptr)
    , m_layout(nullptr)
    , m_fixedW(0)
    , m_position(position)
{
    m_dpi = QApplication::primaryScreen()->logicalDotsPerInch();
    m_font.setFamily("Noto Mono");
    m_font.setPixelSize((m_dpi * 9) / 72);

    QFontMetrics fm(m_font);
    m_fixedW = fm.horizontalAdvance("▲ 999.9 MB/s") + 8;

    m_upLabel = new QLabel(this);
    m_downLabel = new QLabel(this);

    m_upLabel->setAlignment(Qt::AlignCenter);
    m_downLabel->setAlignment(Qt::AlignCenter);

    m_layout = new QBoxLayout(QBoxLayout::TopToBottom);
    m_layout->addWidget(m_upLabel);
    m_layout->addWidget(m_downLabel);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);
    setLayout(m_layout);

    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::transparent);
    setPalette(pal);

    updateSpeed({0, 0, "0 B/s", "0 B/s"}, position);
}

MainWidget::~MainWidget() = default;

void MainWidget::updateSpeed(const NetSpeedInfo &info, Dock::Position position)
{
    m_position = position;
    m_upLabel->setFont(m_font);
    m_downLabel->setFont(m_font);

    QColor textColor = palette().color(QPalette::WindowText);
    QString colorName = textColor.name();
    QString style = QString("QLabel { color: %1; }").arg(colorName);
    m_upLabel->setStyleSheet(style);
    m_downLabel->setStyleSheet(style);

    m_upLabel->setText(QString("▲ %1").arg(info.upSpeed));
    m_downLabel->setText(QString("▼ %1").arg(info.downSpeed));

    setMinimumSize(0, 0);
    setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    updateGeometry();
}

int MainWidget::fixedWidth() const
{
    return m_fixedW;
}

QSize MainWidget::sizeHint() const
{
    if (!m_upLabel || !m_downLabel) {
        return QSize(60, 24);
    }

    QFontMetrics fm(m_font);
    int lineH = fm.height();

    return QSize(fixedWidth(), lineH * 2 + 4);
}
