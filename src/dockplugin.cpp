#include "dockplugin.h"

#include "dockcontainer.h"
#include "dockwidget.h"
#include "dockarea.h"
#include "docktabbar.h"
#include "style/abstractstyle.h"

#include <QQmlApplicationEngine>
#include <QFontDatabase>

//QObject *createDockStyle(QQmlEngine *, QJSEngine *)
//{
//    return dockStyle;
//}
DockPlugin::DockPlugin(QObject *parent) : QObject(parent)
{

}

void DockPlugin::registerDockPlugin()
{
    qmlRegisterType<DockContainer>("Kaj.Dock", 1, 0, "DockContainer");
    qmlRegisterType<DockWidget>("Kaj.Dock", 1, 0, "DockWidget");
    qmlRegisterType<DockArea>("Kaj.Dock", 1, 0, "DockArea");
    qmlRegisterType<DockTabBar>("Kaj.Dock", 1, 0, "DockTabBar");
    qmlRegisterUncreatableMetaObject(Dock::staticMetaObject,
                                     "Kaj.Dock",
                                     1,
                                     0,
                                     "Dock",
                                     "Error: only enums");

//    qmlRegisterSingletonType<DockStyle>("Kaj.Dock", 1, 0, "DockStyle", createDockStyle);

    qRegisterMetaType<QList<DockWidget *>>();

    QFontDatabase::addApplicationFont(":/icons.ttf");
    QFontDatabase::addApplicationFont(":/icons/dock_font_default.ttf");
}
