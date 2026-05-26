#include "netspeedplugin.h"
#include "mainwidget.h"
#include <cstdio>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>
#include <QDBusInterface>
#include <QDBusConnection>
#include <QUrl>
#include <QNetworkRequest>
#include <QProcess>
#include <QMessageBox>
#include <QAbstractButton>

static constexpr char kPluginStateKey[] = "enable";
static constexpr char kUpdateCheckUrl[] = "https://api.github.com/repos/kali-urs/netspeed-plugin/releases/latest";

static QSettings settings()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
                  + QStringLiteral("/dde-dock-netspeed-plugin");
    QDir().mkpath(dir);
    return QSettings(dir + QStringLiteral("/settings.conf"), QSettings::IniFormat);
}

static void notify(const QString &title, const QString &body)
{
    QDBusInterface("org.deepin.dde.Notification1",
                   "/org/deepin/dde/Notification1",
                   "org.deepin.dde.Notification1",
                   QDBusConnection::sessionBus())
        .call("Notify", "dde-dock-netspeed-plugin", 0,
              "netspeed-monitor", title, body,
              QStringList(), QVariantMap(), 5000);
}

NetSpeedPlugin::NetSpeedPlugin(QObject *parent)
    : QObject(parent)
    , m_refreshTimer(new QTimer(this))
    , m_mainWidget(nullptr)
    , m_tipsWidget(nullptr)
    , m_network(new QNetworkAccessManager(this))
    , m_downloadReply(nullptr)
    , m_downloadFile(nullptr)
    , m_lastProgress(0)
    , m_updating(false)
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
    abortDownload();
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

    QTimer::singleShot(5000, this, &NetSpeedPlugin::checkUpdate);
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

    QMap<QString, QVariant> checkUpdate;
    checkUpdate["itemId"] = "check_update";
    checkUpdate["itemText"] = QString("检查更新 (v%1)").arg(currentVersion());
    checkUpdate["isActive"] = true;
    items.append(checkUpdate);

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
    } else if (menuId == "check_update") {
        checkUpdate();
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

QString NetSpeedPlugin::currentVersion() const
{
    return QStringLiteral(VERSION);
}

void NetSpeedPlugin::checkUpdate()
{
    if (m_updating)
        return;
    m_updating = true;

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    QNetworkRequest req;
    req.setUrl(QUrl(kUpdateCheckUrl));
    req.setRawHeader("Accept", "application/json");
    req.setRawHeader("User-Agent", "dde-dock-netspeed-plugin/" VERSION);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);
#else
    QNetworkRequest req(QUrl(kUpdateCheckUrl));
    req.setRawHeader("Accept", "application/json");
    req.setRawHeader("User-Agent", "dde-dock-netspeed-plugin/" VERSION);
#endif

    QNetworkReply *reply = m_network->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        m_updating = false;

        if (reply->error() != QNetworkReply::NoError) {
            notify("检查更新失败", reply->errorString());
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QString latestVer = doc.object().value("tag_name").toString().remove('v');

        if (latestVer.isEmpty()) {
            notify("检查更新失败", "无法获取最新版本信息");
            return;
        }

        if (latestVer == currentVersion()) {
            notifyNoUpdate();
        } else {
            confirmAndUpdate(latestVer);
        }
    });
}

void NetSpeedPlugin::confirmAndUpdate(const QString &latestVersion)
{
    QMessageBox msgBox(m_mainWidget);
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setWindowTitle("发现新版本");
    msgBox.setText(QString("当前版本: v%1\n最新版本: v%2\n\n是否下载并安装更新？")
                       .arg(currentVersion(), latestVersion));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);
    if (auto *btn = msgBox.button(QMessageBox::Yes))
        btn->setText("下载更新");
    if (auto *btn = msgBox.button(QMessageBox::No))
        btn->setText("取消");

    if (msgBox.exec() != QMessageBox::Yes)
        return;

    m_downloadVersion = latestVersion;
    QString debUrl = QString(
        "https://github.com/kali-urs/netspeed-plugin/releases/download/"
        "v%1/dde-dock-netspeed-plugin_%1_amd64.deb")
                         .arg(latestVersion);
    startDownload(debUrl);
}

void NetSpeedPlugin::startDownload(const QString &url)
{
    QString tmpPath = QString("/tmp/dde-dock-netspeed-plugin_%1_amd64.deb")
                          .arg(m_downloadVersion);
    m_downloadFile = new QFile(tmpPath, this);
    if (!m_downloadFile->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        notify("下载失败", "无法创建临时文件");
        return;
    }

    QNetworkRequest req;
    req.setUrl(QUrl(url));
    req.setRawHeader("User-Agent", "dde-dock-netspeed-plugin/" VERSION);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);

    m_downloadReply = m_network->get(req);
    connect(m_downloadReply, &QNetworkReply::readyRead, this, [this]() {
        m_downloadFile->write(m_downloadReply->readAll());
    });
    connect(m_downloadReply, &QNetworkReply::downloadProgress,
            this, &NetSpeedPlugin::onDownloadProgress);
    connect(m_downloadReply, &QNetworkReply::finished,
            this, &NetSpeedPlugin::onDownloadFinished);

    notify("开始下载",
           QString("正在下载 v%1 更新...").arg(m_downloadVersion));
}

void NetSpeedPlugin::onDownloadProgress(qint64 received, qint64 total)
{
    if (total <= 0)
        return;
    int percent = static_cast<int>(received * 100 / total);
    if (percent - m_lastProgress >= 10 || percent == 100) {
        m_lastProgress = percent;
        notify(QString("下载更新 %1%").arg(percent),
               QString("v%1 (%2 / %3 KB)")
                   .arg(m_downloadVersion)
                   .arg(received / 1024)
                   .arg(total / 1024));
    }
}

void NetSpeedPlugin::onDownloadFinished()
{
    m_downloadFile->flush();
    m_downloadFile->close();

    if (m_downloadReply->error() != QNetworkReply::NoError) {
        notify("下载失败", m_downloadReply->errorString());
        m_downloadFile->remove();
        m_downloadFile->deleteLater();
        m_downloadFile = nullptr;
        m_downloadReply->deleteLater();
        m_downloadReply = nullptr;
        return;
    }

    m_downloadReply->deleteLater();
    m_downloadReply = nullptr;

    notify("下载完成", QString("v%1 已下载，正在安装...").arg(m_downloadVersion));
    installPackage();
}

void NetSpeedPlugin::installPackage()
{
    QString path = QString("/tmp/dde-dock-netspeed-plugin_%1_amd64.deb")
                       .arg(m_downloadVersion);

    QProcess *proc = new QProcess(this);

    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, proc, path](int exitCode, QProcess::ExitStatus status) {
                proc->deleteLater();

                if (status == QProcess::CrashExit || exitCode != 0) {
                    QString errMsg = QString::fromUtf8(proc->readAllStandardError());
                    notify("安装失败",
                           QString("退出码: %1\n%2\n可手动安装: %3")
                               .arg(exitCode)
                               .arg(errMsg.trimmed())
                               .arg(path));
                    return;
                }

                notify("更新完成",
                       QString("v%1 安装成功，正在重启任务栏...")
                           .arg(m_downloadVersion));

                QTimer::singleShot(1500, this, []() {
                    QProcess::startDetached("killall", {"dde-shell"});
                });
            });

    proc->start("pkexec",
                {"env", "DEBIAN_FRONTEND=noninteractive",
                 "dpkg", "-i", path});
}

void NetSpeedPlugin::notifyNoUpdate()
{
    notify("已是最新版本", QString("当前版本: v%1").arg(currentVersion()));
}

void NetSpeedPlugin::abortDownload()
{
    if (m_downloadReply) {
        m_downloadReply->abort();
        m_downloadReply->deleteLater();
        m_downloadReply = nullptr;
    }
    if (m_downloadFile) {
        m_downloadFile->remove();
        m_downloadFile->deleteLater();
        m_downloadFile = nullptr;
    }
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
