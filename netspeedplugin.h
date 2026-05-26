#ifndef NETSPEEDPLUGIN_H
#define NETSPEEDPLUGIN_H

#include <QObject>
#include <QTimer>
#include <QLabel>
#include <QSettings>
#include <pluginsiteminterface.h>
#include "type.h"

class MainWidget;

class NetSpeedPlugin : public QObject, PluginsItemInterface
{
    Q_OBJECT
    Q_INTERFACES(PluginsItemInterface)
    Q_PLUGIN_METADATA(IID ModuleInterface_iid FILE "netspeed.json")

public:
    explicit NetSpeedPlugin(QObject *parent = nullptr);
    ~NetSpeedPlugin() override;

    const QString pluginDisplayName() const override;
    const QString pluginName() const override;
    void init(PluginProxyInterface *proxyInter) override;

    QWidget *itemWidget(const QString &itemKey) override;
    QWidget *itemTipsWidget(const QString &itemKey) override;
    QWidget *itemPopupApplet(const QString &itemKey) override;

    bool pluginIsAllowDisable() override;
    bool pluginIsDisable() override;
    void pluginStateSwitched() override;

    const QString itemContextMenu(const QString &itemKey) override;
    void invokedMenuItem(const QString &itemKey, const QString &menuId, const bool checked) override;
    void displayModeChanged(const Dock::DisplayMode displayMode) override;
    void positionChanged(const Dock::Position position) override;

    PluginSizePolicy pluginSizePolicy() const override;

private slots:
    void refreshInfo();

private:
    QString formatSpeed(unsigned long bytes);
    void applyScale(int scale);

    QTimer *m_refreshTimer;
    MainWidget *m_mainWidget;
    QLabel *m_tipsWidget;

    unsigned long m_oldRx, m_oldTx;
    unsigned long m_curRx, m_curTx;
    NetSpeedInfo m_info;

    int m_scale;
    Dock::Position m_position;
};

#endif
