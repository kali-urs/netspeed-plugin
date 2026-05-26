#include "netspeedplugin.h"
#include "mainwidget.h"
#include <cstdio>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>

static constexpr char kPluginStateKey[] = "enable";

static QSettings settings()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
                  + QStringLiteral("/dde-dock-netspeed-plugin");
    QDir().mkpath(dir);
    return QSettings(dir + QStringLiteral("/settings.conf"), QSettings::IniFormat);
}

NetSpeedPlugin::NetSpeedPlugin(QObject *parent)
    : QObject(parent)
    , m_refreshTimer(new QTimer(this))
    , m_mainWidget(nullptr)
    , m_tipsWidget(nullptr)
    , m_oldRx(0)
    , m_oldTx(0)
    , m_curRx(0)
    , m_curTx(0)
    , m_scale(100)
    , m_position(Dock::Bottom)
{
}

NetSpeedPlugin::~NetSpeedPlugin()
{
    if (m_mainWidget) {
        m_mainWidget->deleteLater();
        m_mainWidget = nullptr;
    }
    if (m_tipsWidget) {
        m_tipsWidget->deleteLater();
        m_tipsWidget = nullptr;
    }
}

const QString NetSpeedPlugin::pluginDisplayName() const
{
    return QStringLiteral("实时网速");
}

const QString NetSpeedPlugin::pluginName() const
{
    return QStringLiteral(PLUGIN_NAME);
}

void NetSpeedPlugin::init(PluginProxyInterface *proxyInter)
{
    PluginsItemInterface::m_proxyInter = proxyInter;

    m_mainWidget = new MainWidget(position());
    m_tipsWidget = new QLabel;

    m_position = position();
    m_scale = settings().value("scale", 100).toInt();
    m_mainWidget->setScale(m_scale);

    if (!pluginIsDisable()) {
        m_proxyInter->itemAdded(this, pluginName());
    }

    m_refreshTimer->start(1000);
    connect(m_refreshTimer, &QTimer::timeout, this, &NetSpeedPlugin::refreshInfo, Qt::QueuedConnection);
}

QWidget *NetSpeedPlugin::itemWidget(const QString &itemKey)
{
    if (itemKey != pluginName())
        return nullptr;
    return m_mainWidget;
}

QWidget *NetSpeedPlugin::itemTipsWidget(const QString &itemKey)
{
    if (itemKey != pluginName())
        return nullptr;

    m_tipsWidget->setText(QString("↑ %1/s\n↓ %2/s")
                              .arg(m_info.upSpeed)
                              .arg(m_info.downSpeed));
    return m_tipsWidget;
}

QWidget *NetSpeedPlugin::itemPopupApplet(const QString &itemKey)
{
    Q_UNUSED(itemKey);
    return nullptr;
}

bool NetSpeedPlugin::pluginIsAllowDisable()
{
    return true;
}

bool NetSpeedPlugin::pluginIsDisable()
{
    return !settings().value(kPluginStateKey, true).toBool();
}

void NetSpeedPlugin::pluginStateSwitched()
{
    const bool disabledNew = !pluginIsDisable();
    QSettings s = settings();
    s.setValue(kPluginStateKey, !disabledNew);

    if (disabledNew) {
        m_proxyInter->itemRemoved(this, pluginName());
    } else {
        m_proxyInter->itemAdded(this, pluginName());
    }
}

const QString NetSpeedPlugin::itemContextMenu(const QString &itemKey)
{
    Q_UNUSED(itemKey);

    QList<QVariant> items;

    QMap<QString, QVariant> enlarge;
    enlarge["itemId"] = "enlarge";
    enlarge["itemText"] = "放大";
    enlarge["isActive"] = true;
    items.append(enlarge);

    QMap<QString, QVariant> shrink;
    shrink["itemId"] = "shrink";
    shrink["itemText"] = "缩小";
    shrink["isActive"] = true;
    items.append(shrink);

    QMap<QString, QVariant> refresh;
    refresh["itemId"] = "refresh";
    refresh["itemText"] = "刷新";
    refresh["isActive"] = true;
    items.append(refresh);

    QMap<QString, QVariant> menu;
    menu["items"] = items;
    menu["checkableMenu"] = false;
    menu["singleCheck"] = false;

    return QJsonDocument::fromVariant(menu).toJson();
}

void NetSpeedPlugin::invokedMenuItem(const QString &itemKey, const QString &menuId, const bool checked)
{
    Q_UNUSED(itemKey);
    Q_UNUSED(checked);
    if (menuId == "refresh") {
        m_oldRx = 0;
        m_oldTx = 0;
    } else if (menuId == "enlarge") {
        m_scale = qMin(m_scale + 10, 200);
        applyScale(m_scale);
    } else if (menuId == "shrink") {
        m_scale = qMax(m_scale - 10, 50);
        applyScale(m_scale);
    }
}

void NetSpeedPlugin::displayModeChanged(const Dock::DisplayMode displayMode)
{
    Q_UNUSED(displayMode);
}

void NetSpeedPlugin::positionChanged(const Dock::Position position)
{
    m_position = position;
    if (m_mainWidget) {
        m_mainWidget->updateSpeed(m_info, position);
    }
}

PluginsItemInterface::PluginSizePolicy NetSpeedPlugin::pluginSizePolicy() const
{
    return PluginSizePolicy::Custom;
}

QString NetSpeedPlugin::formatSpeed(unsigned long bytes)
{
    double speed = static_cast<double>(bytes);
    if (speed < 1024.0) {
        return QString("%1 B").arg(static_cast<int>(speed));
    } else if (speed < 1024.0 * 1024.0) {
        return QString("%1 KB").arg(speed / 1024.0, 0, 'f', 1);
    } else if (speed < 1024.0 * 1024.0 * 1024.0) {
        return QString("%1 MB").arg(speed / (1024.0 * 1024.0), 0, 'f', 1);
    } else {
        return QString("%1 GB").arg(speed / (1024.0 * 1024.0 * 1024.0), 0, 'f', 1);
    }
}

void NetSpeedPlugin::refreshInfo()
{
    FILE *fp = fopen("/proc/net/dev", "r");
    if (!fp) {
        return;
    }

    char buffer[1024];
    char ifname[64];
    unsigned long rx, tx;
    unsigned long totalRx = 0, totalTx = 0;

    fgets(buffer, sizeof(buffer), fp);
    fgets(buffer, sizeof(buffer), fp);

    while (fgets(buffer, sizeof(buffer), fp)) {
        sscanf(buffer, "%s %lu %*lu %*lu %*lu %*lu %*lu %*lu %*lu %lu",
               ifname, &rx, &tx);
        if (strcmp(ifname, "lo:") == 0) {
            continue;
        }
        totalRx += rx;
        totalTx += tx;
    }
    fclose(fp);

    m_curRx = totalRx;
    m_curTx = totalTx;

    unsigned long upBytes = (m_oldTx == 0) ? 0 : (m_curTx - m_oldTx);
    unsigned long downBytes = (m_oldRx == 0) ? 0 : (m_curRx - m_oldRx);

    m_info.upBytes = m_curTx;
    m_info.downBytes = m_curRx;
    m_info.upSpeed = formatSpeed(upBytes);
    m_info.downSpeed = formatSpeed(downBytes);

    m_oldRx = m_curRx;
    m_oldTx = m_curTx;

    if (m_mainWidget) {
        m_mainWidget->updateSpeed(m_info, m_position);
    }

    m_proxyInter->itemUpdate(this, pluginName());
}

void NetSpeedPlugin::applyScale(int scale)
{
    m_scale = scale;
    QSettings s = settings();
    s.setValue("scale", m_scale);
    if (m_mainWidget) {
        m_mainWidget->setScale(m_scale);
    }
}
