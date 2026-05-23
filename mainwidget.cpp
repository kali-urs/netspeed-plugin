#include "mainwidget.h"
#include <DGuiApplicationHelper>

MainWidget::MainWidget(Dock::Position position)
    : m_upLabel(nullptr)
    , m_downLabel(nullptr)
    , m_layout(nullptr)
{
    m_dpi = QApplication::primaryScreen()->logicalDotsPerInch();
    m_font.setFamily("Noto Mono");

    m_upLabel = new QLabel(this);
    m_downLabel = new QLabel(this);

    m_upLabel->setAlignment(Qt::AlignCenter);
    m_downLabel->setAlignment(Qt::AlignCenter);

    QBoxLayout::Direction dir = (position == Dock::Top || position == Dock::Bottom)
                                    ? QBoxLayout::LeftToRight
                                    : QBoxLayout::TopToBottom;
    m_layout = new QBoxLayout(dir);
    m_layout->addWidget(m_upLabel);
    m_layout->addWidget(m_downLabel);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(4);
    setLayout(m_layout);

    updateSpeed({0, 0, "0 B/s", "0 B/s"}, position);
}

MainWidget::~MainWidget() = default;

void MainWidget::updateSpeed(const NetSpeedInfo &info, Dock::Position position)
{
    QBoxLayout::Direction newDir = (position == Dock::Top || position == Dock::Bottom)
                                       ? QBoxLayout::LeftToRight
                                       : QBoxLayout::TopToBottom;
    if (m_layout->direction() != newDir) {
        delete m_layout;
        m_layout = new QBoxLayout(newDir);
        m_layout->addWidget(m_upLabel);
        m_layout->addWidget(m_downLabel);
        m_layout->setContentsMargins(0, 0, 0, 0);
        m_layout->setSpacing(4);
        setLayout(m_layout);
    }

    m_font.setPixelSize((m_dpi * 9) / 72);
    m_upLabel->setFont(m_font);
    m_downLabel->setFont(m_font);

    auto *helper = DGuiApplicationHelper::instance();
    bool isLight = (helper->themeType() == DGuiApplicationHelper::LightType);
    QString color = isLight ? "#000000" : "#ffffff";
    QString style = QString("QLabel { color: %1; }").arg(color);
    m_upLabel->setStyleSheet(style);
    m_downLabel->setStyleSheet(style);

    m_upLabel->setText(QString("▲ %1").arg(info.upSpeed));
    m_downLabel->setText(QString("▼ %1").arg(info.downSpeed));

    setMinimumSize(0, 0);
    setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    updateGeometry();
}

QSize MainWidget::sizeHint() const
{
    if (!m_upLabel || !m_downLabel) {
        return QSize(60, 24);
    }

    int upW = QFontMetrics(m_font).horizontalAdvance(m_upLabel->text());
    int downW = QFontMetrics(m_font).horizontalAdvance(m_downLabel->text());
    int upH = QFontMetrics(m_font).height();
    int downH = QFontMetrics(m_font).height();

    int w = qMax(upW, downW) + 8;
    int h = upH + downH + 6;

    return QSize(w, h);
}
