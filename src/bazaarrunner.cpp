/*
    SPDX-FileCopyrightText: 2025 Adam Fidel <adam@fidel.cloud>

    SPDX-License-Identifier: Apache-2.0
*/

#include "bazaarrunner.h"

#include <KLocalizedString>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusVariant>
#include <QProcess>
#include <QStandardPaths>
#include <QDebug>
#include <QDateTime>

BazaarRunner::BazaarRunner(QObject *parent, const KPluginMetaData &data)
    : KRunner::AbstractRunner(parent, data)
    , m_bazaarInterface(nullptr)
{
    // Disallow short queries (3 characters or less due to OBS)
    setMinLetterCount(3);
    
    // Initialize D-Bus interface to Bazaar
    // Note: You may need to adjust the service name based on how Bazaar registers itself
    // Common patterns: "org.domain.AppName", "com.domain.AppName", or "io.domain.AppName"
    QString serviceName = QStringLiteral("org.gnome.Bazaar"); // Adjust this as needed
    
    m_bazaarInterface = new QDBusInterface(
        serviceName,
        QStringLiteral("/org/gnome/Shell/SearchProvider2"),
        QStringLiteral("org.gnome.Shell.SearchProvider2"),
        QDBusConnection::sessionBus(),
        this
    );
    
    if (!m_bazaarInterface->isValid()) {
        qWarning() << "Failed to connect to Bazaar D-Bus interface:" << m_bazaarInterface->lastError().message();
        qDebug() << "Trying alternative service names...";
        
        // Try alternative service names
        QStringList alternativeNames = {
            QStringLiteral("io.github.giantpinkrobots.flatsweep"), // Example pattern
            QStringLiteral("com.github.giantpinkrobots.bazaar"),   // Another pattern
            QStringLiteral("org.freedesktop.Bazaar"),              // FreeDesktop pattern
            QStringLiteral("io.github.bazaar.Bazaar")              // GitHub pattern
        };
        
        for (const QString &altName : alternativeNames) {
            delete m_bazaarInterface;
            m_bazaarInterface = new QDBusInterface(
                altName,
                QStringLiteral("/org/gnome/Shell/SearchProvider2"),
                QStringLiteral("org.gnome.Shell.SearchProvider2"),
                QDBusConnection::sessionBus(),
                this
            );
            
            if (m_bazaarInterface->isValid()) {
                qDebug() << "Successfully connected to Bazaar using service name:" << altName;
                break;
            }
        }
        
        if (!m_bazaarInterface->isValid()) {
            qWarning() << "Could not connect to Bazaar D-Bus service. Make sure Bazaar is running.";
        }
    }
    
    addSyntax(QStringLiteral(":q:"), i18n("Search for Flatpak applications in Bazaar"));
}

bool BazaarRunner::isInstalled(const QString &appId) {
    // Check if Flatpak is installed first
    QProcess flatpakProcess;
    flatpakProcess.start(QStringLiteral("flatpak"), {QStringLiteral("--version")});
    if (!flatpakProcess.waitForFinished(3000) || flatpakProcess.exitCode() != 0) {
        return false; // Flatpak not available
    }
    
    // Check if the specific app is installed
    QProcess checkProcess;
    checkProcess.start(QStringLiteral("flatpak"), {
        QStringLiteral("info"), 
        QStringLiteral("--user"), 
        appId
    });
    checkProcess.waitForFinished(3000);
    
    if (checkProcess.exitCode() == 0) {
        return true; // Installed in user repo
    }
    
    // Check system repo
    checkProcess.start(QStringLiteral("flatpak"), {
        QStringLiteral("info"), 
        QStringLiteral("--system"), 
        appId
    });
    checkProcess.waitForFinished(3000);
    
    return checkProcess.exitCode() == 0;
}

QList<AppSuggestion> BazaarRunner::queryBazaar(const QString &term) {
    QList<AppSuggestion> results;
    
    if (!m_bazaarInterface || !m_bazaarInterface->isValid()) {
        return results;
    }
    
    // Call GetInitialResultSet method
    QStringList terms = term.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    QDBusReply<QStringList> reply = m_bazaarInterface->call(QStringLiteral("GetInitialResultSet"), terms);
    
    if (!reply.isValid()) {
        qWarning() << "Failed to get search results from Bazaar:" << reply.error().message();
        return results;
    }
    
    QStringList resultIds = reply.value();
    if (resultIds.isEmpty()) {
        return results;
    }
    
    // Get metadata for the results
    QDBusReply<QList<QVariantMap>> metaReply = m_bazaarInterface->call(QStringLiteral("GetResultMetas"), resultIds);
    
    if (!metaReply.isValid()) {
        qWarning() << "Failed to get result metadata from Bazaar:" << metaReply.error().message();
        return results;
    }
    
    QList<QVariantMap> metas = metaReply.value();
    
    for (int i = 0; i < resultIds.size() && i < metas.size(); ++i) {
        const QVariantMap &meta = metas[i];
        
        AppSuggestion suggestion;
        suggestion.id = resultIds[i];
        
        // Try different possible keys for name
        suggestion.name = meta.value(QStringLiteral("name")).toString();
        if (suggestion.name.isEmpty()) {
            suggestion.name = meta.value(QStringLiteral("title")).toString();
        }
        if (suggestion.name.isEmpty()) {
            suggestion.name = meta.value(QStringLiteral("id")).toString();
        }
        
        // Try different possible keys for description
        suggestion.description = meta.value(QStringLiteral("description")).toString();
        if (suggestion.description.isEmpty()) {
            suggestion.description = meta.value(QStringLiteral("subtitle")).toString();
        }
        
        // Try different possible keys for icon
        suggestion.iconName = meta.value(QStringLiteral("icon")).toString();
        if (suggestion.iconName.isEmpty()) {
            suggestion.iconName = meta.value(QStringLiteral("icon-data")).toString();
        }
        if (suggestion.iconName.isEmpty()) {
            suggestion.iconName = meta.value(QStringLiteral("gicon")).toString();
        }
        
        // If no icon is provided, use a default Flatpak icon
        if (suggestion.iconName.isEmpty()) {
            suggestion.iconName = QStringLiteral("application-x-flatpak");
        }
        
        // Skip if we couldn't get a name
        if (suggestion.name.isEmpty()) {
            continue;
        }
        
        results.append(suggestion);
    }
    
    return results;
}

void BazaarRunner::match(KRunner::RunnerContext &context)
{
    const QString term = context.query();
    if (term.length() < 3) return;

    QList<AppSuggestion> results = queryBazaar(term);

    for (const auto &app : results) {
        if (isInstalled(app.id)) continue;

        KRunner::QueryMatch match(this);
        match.setType(KRunner::QueryMatch::PossibleMatch);
        match.setIconName(app.iconName);
        match.setText(i18n("Install %1 via Bazaar", app.name));
        match.setSubtext(app.description);
        match.setData(app.id);
        match.setRelevance(0.9);
        context.addMatch(match);
    }
}

void BazaarRunner::run(const KRunner::RunnerContext &context, const KRunner::QueryMatch &match)
{
    const QString appId = match.data().toString();
    if (appId.isEmpty()) {
        qWarning() << "No app ID provided for installation";
        return;
    }
    
    // Activate the result in Bazaar, which should trigger the installation
    if (m_bazaarInterface && m_bazaarInterface->isValid()) {
        QStringList terms = context.query().split(QLatin1Char(' '), Qt::SkipEmptyParts);
        uint timestamp = static_cast<uint>(QDateTime::currentSecsSinceEpoch());
        
        QDBusReply<void> reply = m_bazaarInterface->call(
            QStringLiteral("ActivateResult"), 
            appId, 
            terms, 
            timestamp
        );
        
        if (!reply.isValid()) {
            qWarning() << "Failed to activate result in Bazaar:" << reply.error().message();
            
            // Fallback: try to launch Bazaar with search terms
            QProcess::startDetached(QStringLiteral("bazaar"), QStringList() << QStringLiteral("--search") << context.query());
        } else {
            qDebug() << "Successfully activated result:" << appId << "in Bazaar";
        }
    } else {
        qWarning() << "Bazaar D-Bus interface not available";
        
        // Fallback: try to launch Bazaar directly
        QProcess::startDetached(QStringLiteral("bazaar"), QStringList());
    }
}

K_PLUGIN_CLASS_WITH_JSON(BazaarRunner, "bazaarrunner.json")

// needed for the QObject subclass declared as part of K_PLUGIN_CLASS_WITH_JSON
#include "bazaarrunner.moc"

#include "moc_bazaarrunner.cpp"
