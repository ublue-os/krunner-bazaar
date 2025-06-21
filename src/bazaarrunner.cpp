/*
    SPDX-FileCopyrightText: 2025 Adam Fidel <adam@fidel.cloud>

    SPDX-License-Identifier: Apache-2.0
*/

#include "bazaarrunner.h"

#include <KLocalizedString>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusVariant>
#include <QDBusArgument>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QProcess>
#include <QStandardPaths>
#include <QDebug>
#include <QDateTime>

BazaarRunner::BazaarRunner(QObject *parent, const KPluginMetaData &data)
    : KRunner::AbstractRunner(parent, data)
    , m_bazaarInterface(nullptr)
{
    qDebug() << "BazaarRunner: Constructor called";
    qDebug() << "BazaarRunner: Plugin name:" << data.name();
    qDebug() << "BazaarRunner: Plugin ID:" << data.pluginId();
    
    // Disallow short queries (based on Bazaar's filtering logic)
    // Bazaar filters out single character queries, we set minimum to 2
    setMinLetterCount(2);
    
    qDebug() << "BazaarRunner: Minimum letter count set to 2";
    
    // Initialize D-Bus interface to Bazaar
    // Based on Bazaar's search provider configuration:
    // BusName=io.github.kolunmi.bazaar
    // ObjectPath=/io/github/kolunmi/bazaar/SearchProvider
    QString serviceName = QStringLiteral("io.github.kolunmi.bazaar");
    
    qDebug() << "BazaarRunner: Attempting to connect to D-Bus service:" << serviceName;
    
    m_bazaarInterface = std::make_unique<QDBusInterface>(
        serviceName,
        QStringLiteral("/io/github/kolunmi/bazaar/SearchProvider"),
        QStringLiteral("org.gnome.Shell.SearchProvider2"),
        QDBusConnection::sessionBus()
    );
    
    if (!m_bazaarInterface->isValid()) {
        qWarning() << "BazaarRunner: Failed to connect to Bazaar D-Bus interface:" << m_bazaarInterface->lastError().message();
        qWarning() << "BazaarRunner: Make sure Bazaar is running and the search provider is enabled.";
    } else {
        qDebug() << "BazaarRunner: Successfully connected to Bazaar D-Bus service:" << serviceName;
    }
    
    addSyntax(QStringLiteral(":q:"), i18n("Search for Flatpak applications in Bazaar"));
    qDebug() << "BazaarRunner: Constructor completed successfully";
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
    qDebug() << "Calling GetResultMetas with" << resultIds.size() << "result IDs";
    
    QDBusMessage metaCall = QDBusMessage::createMethodCall(
        QStringLiteral("io.github.kolunmi.bazaar"),
        QStringLiteral("/io/github/kolunmi/bazaar/SearchProvider"),
        QStringLiteral("org.gnome.Shell.SearchProvider2"),
        QStringLiteral("GetResultMetas")
    );
    metaCall << resultIds;
    
    QDBusMessage metaReply = QDBusConnection::sessionBus().call(metaCall);
    
    if (metaReply.type() == QDBusMessage::ErrorMessage) {
        qWarning() << "Failed to get result metadata from Bazaar:" << metaReply.errorMessage();
        return results;
    }
    
    qDebug() << "GetResultMetas call successful, parsing response...";
    qDebug() << "Reply arguments count:" << metaReply.arguments().size();
    
    if (metaReply.arguments().isEmpty()) {
        qWarning() << "No arguments in GetResultMetas reply";
        return results;
    }
    
    // Get the first argument which should be aa{sv}
    QVariant metaVariant = metaReply.arguments().at(0);
    qDebug() << "Metadata variant type:" << metaVariant.typeName();
    
    QList<QVariantMap> metas;
    
    if (metaVariant.canConvert<QDBusArgument>()) {
        qDebug() << "Converting QDBusArgument to metadata list";
        QDBusArgument metaArg = metaVariant.value<QDBusArgument>();
        
        // Handle aa{sv} signature - array of array of dictionaries
        metaArg.beginArray();
        while (!metaArg.atEnd()) {
            qDebug() << "Processing outer array element...";
            
            // Each outer element is a{sv} - an array of dictionaries
            // We need to begin the inner array directly on the same argument
            metaArg.beginArray();
            while (!metaArg.atEnd()) {
                qDebug() << "Processing inner array element (dictionary)...";
                QVariantMap meta;
                
                // Each inner element is {sv} - a dictionary of string to variant
                metaArg.beginMap();
                while (!metaArg.atEnd()) {
                    metaArg.beginMapEntry();
                    
                    // Read the key (string)
                    QString key;
                    metaArg >> key;
                    
                    // Read the value (variant)
                    QDBusVariant dbusVariant;
                    metaArg >> dbusVariant;
                    QVariant value = dbusVariant.variant();
                    
                    metaArg.endMapEntry();
                    
                    meta[key] = value;
                    qDebug() << "    Key:" << key << "Value:" << value.toString() << "Type:" << value.typeName();
                }
                metaArg.endMap();
                
                metas.append(meta);
                qDebug() << "  Added metadata map with" << meta.size() << "keys:" << meta.keys();
            }
            metaArg.endArray();
        }
        metaArg.endArray();
    } else {
        qWarning() << "Cannot convert metadata to QDBusArgument, variant type:" << metaVariant.typeName();
        // Try direct conversion as fallback
        if (metaVariant.canConvert<QVariantList>()) {
            QVariantList metaList = metaVariant.toList();
            qDebug() << "Trying direct conversion from QVariantList with" << metaList.size() << "items";
            for (const QVariant &item : metaList) {
                if (item.canConvert<QVariantMap>()) {
                    metas.append(item.toMap());
                }
            }
        }
    }
    
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
    qDebug() << "BazaarRunner::match called with query:" << term;
    
    if (term.length() < 2) {
        qDebug() << "BazaarRunner::match: Query too short, minimum 2 characters required";
        return;
    }

    qDebug() << "BazaarRunner::match: Querying Bazaar for:" << term;
    QList<AppSuggestion> results = queryBazaar(term);
    qDebug() << "BazaarRunner::match: Got" << results.size() << "results from Bazaar";

    int addedMatches = 0;
    for (const auto &app : results) {
        if (isInstalled(app.id)) {
            qDebug() << "BazaarRunner::match: Skipping already installed app:" << app.name;
            continue;
        }

        KRunner::QueryMatch match(this);
        //match.setType(KRunner::QueryMatch::PossibleMatch);
        match.setIconName(app.iconName);
        match.setText(i18n("Install %1 via Bazaar", app.name));
        match.setSubtext(app.description);
        match.setData(app.id);
        match.setRelevance(0.9);
        context.addMatch(match);
        addedMatches++;
        
        qDebug() << "BazaarRunner::match: Added match for:" << app.name;
    }
    
    qDebug() << "BazaarRunner::match: Added" << addedMatches << "matches to context";
}

void BazaarRunner::run(const KRunner::RunnerContext &context, const KRunner::QueryMatch &match)
{
    qDebug() << "BazaarRunner::run called";
    
    const QString appId = match.data().toString();
    if (appId.isEmpty()) {
        qWarning() << "BazaarRunner::run: No app ID provided for installation";
        return;
    }
    
    qDebug() << "BazaarRunner::run: Activating Bazaar result for app ID:" << appId;
    
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
