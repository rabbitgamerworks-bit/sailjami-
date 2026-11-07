/*
 *  Copyright (C) 2026 SailJami contributors
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include <sailfishapp.h>

#include <QDir>
#include <QGuiApplication>
#include <QQmlContext>
#include <QQuickView>
#include <QScopedPointer>
#include <QStandardPaths>
#include <QtQml>

#include "conversationlistmodel.h"
#include "jamibackend.h"
#include "messagemodel.h"

namespace {

/**
 * libjami stores its data in $XDG_{CONFIG,DATA,CACHE}_HOME/jami. Sailjail only
 * lets the app write to its own per-application directories, so point the XDG
 * variables there before the daemon starts.
 */
void setupDaemonDirectories()
{
    const struct { const char* var; QStandardPaths::StandardLocation loc; } dirs[] = {
        {"XDG_CONFIG_HOME", QStandardPaths::AppConfigLocation},
        {"XDG_DATA_HOME", QStandardPaths::AppDataLocation},
        {"XDG_CACHE_HOME", QStandardPaths::CacheLocation},
    };
    for (const auto& d : dirs) {
        const QString path = QStandardPaths::writableLocation(d.loc);
        QDir().mkpath(path);
        qputenv(d.var, QFile::encodeName(path));
    }
}

} // namespace

int main(int argc, char* argv[])
{
    QScopedPointer<QGuiApplication> app(SailfishApp::application(argc, argv));
    app->setOrganizationName(QStringLiteral("org.sailjami"));
    app->setApplicationName(QStringLiteral("sailjami"));

    setupDaemonDirectories();

    qmlRegisterUncreatableType<ConversationListModel>("org.sailjami", 1, 0, "ConversationListModel",
                                                      QStringLiteral("Provided by Jami"));
    qmlRegisterUncreatableType<MessageModel>("org.sailjami", 1, 0, "MessageModel",
                                             QStringLiteral("Provided by Jami"));

    JamiBackend jami;
    if (!jami.start())
        qWarning("Jami daemon could not be started");

    QScopedPointer<QQuickView> view(SailfishApp::createView());
    view->rootContext()->setContextProperty(QStringLiteral("Jami"), &jami);
    view->setSource(SailfishApp::pathToMainQml());
    view->show();

    const int ret = app->exec();
    jami.stop();
    return ret;
}
