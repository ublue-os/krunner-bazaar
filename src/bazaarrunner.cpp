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
    // Disallow short queries (based on Bazaar's filtering logic)
    // Bazaar filters out single character queries, we set minimum to 2
    setMinLetterCount(2);
    
    // Initialize D-Bus interface to Bazaar
    // Based on Bazaar's search provider configuration:
    // BusName=io.github.kolunmi.bazaar
    // ObjectPath=/io/github/kolunmi/bazaar/SearchProvider
    QString serviceName = QStringLiteral("io.github.kolunmi.bazaar");
    
    m_bazaarInterface = std::make_unique<QDBusInterface>(
        serviceName,
        QStringLiteral("/io/github/kolunmi/bazaar/SearchProvider"),
        QStringLiteral("org.gnome.Shell.SearchProvider2"),
        QDBusConnection::sessionBus()
    );
    
    if (!m_bazaarInterface->isValid()) {
        qWarning() << "Failed to connect to Bazaar D-Bus interface:" << m_bazaarInterface->lastError().message();
        qWarning() << "Make sure Bazaar is running and the search provider is enabled.";
    } else {
        qDebug() << "Successfully connected to Bazaar D-Bus service:" << serviceName;
    }
    
    addSyntax(QStringLiteral(":q:"), i18n("Search for Flatpak applications in Bazaar"));
}

bool BazaarRunner::isInstalled(const QString &appId) {
    // Check if Flatpak is installed first
    QProcess flatpakProcess;
    flatpakProcess.start(QStringLiteral("flatpak"), {QStringLiteral("--version")});
    if (!flatpakProcess.waitForFinished(3000) || flatpakProcess.exitCode() != 0) {
        return false;
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
        qDebug() << "Bazaar D-Bus interface not available for query:" << term;
        return results;
    }
    
    // Based on Bazaar's start_request function, they filter out:
    // - Single character queries
    // - Very short queries (handled by our setMinLetterCount)
    if (term.length() < 2) {
        return results;
    }
    
    // Call GetInitialResultSet method
    // Bazaar expects an array of terms (split by spaces)
    QStringList terms = term.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    
    qDebug() << "Querying Bazaar with terms:" << terms;
    
    QDBusReply<QStringList> reply = m_bazaarInterface->call(QStringLiteral("GetInitialResultSet"), terms);
    
    if (!reply.isValid()) {
        qWarning() << "Failed to get search results from Bazaar:" << reply.error().message();
        return results;
    }
    
    QStringList resultIds = reply.value();
    if (resultIds.isEmpty()) {
        qDebug() << "No results returned from Bazaar for query:" << term;
        return results;
    }
    
    qDebug() << "Bazaar returned" << resultIds.size() << "result IDs:" << resultIds;
    
    // Get metadata for the results
    QDBusReply<QList<QVariantMap>> metaReply = m_bazaarInterface->call(QStringLiteral("GetResultMetas"), resultIds);
    
    if (!metaReply.isValid()) {
        qWarning() << "Failed to get result metadata from Bazaar:" << metaReply.error().message();
        return results;
    }
    
    QList<QVariantMap> metas = metaReply.value();
    qDebug() << "Got metadata for" << metas.size() << "results";
    
    for (int i = 0; i < resultIds.size() && i < metas.size(); ++i) {
        const QVariantMap &meta = metas[i];
        
        AppSuggestion suggestion;
        suggestion.id = resultIds[i];
        
        // Based on Bazaar's get_result_metas implementation:
        // - "id": the result ID
        // - "name": the application name (from bz_entry_get_title)
        // - "description": optional description (from bz_entry_get_description)
        // - "gicon": icon as string (from g_icon_to_string)
        // - "icon": serialized icon variant (fallback)
        
        suggestion.name = meta.value(QStringLiteral("name")).toString();
        suggestion.description = meta.value(QStringLiteral("description")).toString();
        
        // Try different icon keys in order of preference
        suggestion.iconName = meta.value(QStringLiteral("gicon")).toString();
        if (suggestion.iconName.isEmpty()) {
            // Handle serialized icon variant
            QVariant iconVariant = meta.value(QStringLiteral("icon"));
            if (iconVariant.isValid()) {
                // For now, use a fallback icon since handling serialized GIcon is complex
                suggestion.iconName = QStringLiteral("application-x-flatpak");
            }
        }
        
        // If no icon is provided, use a default Flatpak icon
        if (suggestion.iconName.isEmpty()) {
            suggestion.iconName = QStringLiteral("application-x-flatpak");
        }
        
        // Skip if we couldn't get a name
        if (suggestion.name.isEmpty()) {
            qWarning() << "Skipping result with empty name:" << suggestion.id;
            continue;
        }
        
        qDebug() << "Found app:" << suggestion.name << "ID:" << suggestion.id;
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
    
    qDebug() << "Activating Bazaar result for app ID:" << appId;
    
    // Activate the result in Bazaar, which should trigger the installation
    // Based on Bazaar's activate_result function, it triggers the "search" action
    // with the result ID, which opens Bazaar and navigates to that app
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
            
            // Fallback: try to launch Bazaar directly
            // Use the flatpak run command if Bazaar is installed as Flatpak
            if (!QProcess::startDetached(QStringLiteral("flatpak"), 
                                       QStringList() << QStringLiteral("run") 
                                                    << QStringLiteral("io.github.kolunmi.bazaar"))) {
                // Final fallback: try system bazaar command
                QProcess::startDetached(QStringLiteral("bazaar"), QStringList());
            }
        } else {
            qDebug() << "Successfully activated result:" << appId << "in Bazaar";
        }
    } else {
        qWarning() << "Bazaar D-Bus interface not available";
        
        // Fallback: try to launch Bazaar directly
        if (!QProcess::startDetached(QStringLiteral("flatpak"), 
                                   QStringList() << QStringLiteral("run") 
                                                << QStringLiteral("io.github.kolunmi.bazaar"))) {
            QProcess::startDetached(QStringLiteral("bazaar"), QStringList());
        }
    }
}

K_PLUGIN_CLASS_WITH_JSON(BazaarRunner, "bazaarrunner.json")

// needed for the QObject subclass declared as part of K_PLUGIN_CLASS_WITH_JSON
#include "bazaarrunner.moc"

#include "moc_bazaarrunner.cpp"
