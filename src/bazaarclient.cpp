#include "bazaarclient.h"

#include <QDBusConnection>
#include <QDBusReply>
#include <QDBusMessage>
#include <QDBusArgument>
#include <QDBusVariant>
#include <QDebug>
#include <QDateTime>

using namespace Qt::Literals::StringLiterals;

BazaarClient::BazaarClient() {
    qDebug() << "BazaarClient: Initializing connection to Bazaar";
    
    // Initialize D-Bus interface to Bazaar
    // Based on Bazaar's search provider configuration:
    // BusName=io.github.kolunmi.bazaar
    // ObjectPath=/io/github/kolunmi/bazaar/SearchProvider
    QString serviceName = QStringLiteral("io.github.kolunmi.bazaar");
    
    qDebug() << "BazaarClient: Attempting to connect to D-Bus service:" << serviceName;
    
    m_bazaarInterface = std::make_unique<QDBusInterface>(
        serviceName,
        QStringLiteral("/io/github/kolunmi/bazaar/SearchProvider"),
        QStringLiteral("org.gnome.Shell.SearchProvider2"),
        QDBusConnection::sessionBus()
    );
    
    if (!m_bazaarInterface->isValid()) {
        m_lastError = m_bazaarInterface->lastError().message();
        qWarning() << "BazaarClient: Failed to connect to Bazaar D-Bus interface:" << m_lastError;
        qWarning() << "BazaarClient: Make sure Bazaar is running and the search provider is enabled.";
    } else {
        qDebug() << "BazaarClient: Successfully connected to Bazaar D-Bus service:" << serviceName;
        m_lastError.clear();
    }
}

bool BazaarClient::isConnected() const {
    return m_bazaarInterface && m_bazaarInterface->isValid();
}

QString BazaarClient::lastError() const {
    return m_lastError;
}

QList<AppSuggestion> BazaarClient::search(const QString &term) {
    QList<AppSuggestion> results;
    
    if (!isConnected()) {
        m_lastError = "Not connected to Bazaar D-Bus interface"_L1;
        qDebug() << "BazaarClient::search:" << m_lastError;
        return results;
    }
    
    // Based on Bazaar's start_request function, they filter out:
    // - Single character queries
    // - Very short queries
    if (term.length() < 2) {
        m_lastError = "Search term too short (minimum 2 characters)"_L1;
        return results;
    }
    
    // Split search term into individual terms
    QStringList terms = term.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    
    qDebug() << "BazaarClient::search: Querying Bazaar with terms:" << terms;
    
    // Get initial result set
    QStringList resultIds = getInitialResultSet(terms);
    if (resultIds.isEmpty()) {
        qDebug() << "BazaarClient::search: No results returned from Bazaar for query:" << term;
        return results;
    }
    
    qDebug() << "BazaarClient::search: Bazaar returned" << resultIds.size() << "result IDs:" << resultIds;
    
    // Get metadata for the results
    QList<QVariantMap> metas = getResultMetas(resultIds);
    qDebug() << "BazaarClient::search: Got metadata for" << metas.size() << "results";
    
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
            qWarning() << "BazaarClient::search: Skipping result with empty name:" << suggestion.id;
            continue;
        }
        
        qDebug() << "BazaarClient::search: Found app:" << suggestion.name << "ID:" << suggestion.id;
        results.append(suggestion);
    }
    
    return results;
}

bool BazaarClient::activateResult(const QString &appId, const QStringList &searchTerms) {
    if (!isConnected()) {
        m_lastError = "Not connected to Bazaar D-Bus interface"_L1;
        qWarning() << "BazaarClient::activateResult:" << m_lastError;
        return false;
    }
    
    qDebug() << "BazaarClient::activateResult: Activating app ID:" << appId;
    
    uint timestamp = static_cast<uint>(QDateTime::currentSecsSinceEpoch());
    
    QDBusReply<void> reply = m_bazaarInterface->call(
        QStringLiteral("ActivateResult"), 
        appId, 
        searchTerms, 
        timestamp
    );
    
    if (!reply.isValid()) {
        m_lastError = reply.error().message();
        qWarning() << "BazaarClient::activateResult: Failed to activate result:" << m_lastError;
        return false;
    }
    
    qDebug() << "BazaarClient::activateResult: Successfully activated result:" << appId;
    return true;
}

QStringList BazaarClient::getInitialResultSet(const QStringList &terms) {
    QDBusReply<QStringList> reply = m_bazaarInterface->call(QStringLiteral("GetInitialResultSet"), terms);
    
    if (!reply.isValid()) {
        m_lastError = reply.error().message();
        qWarning() << "BazaarClient::getInitialResultSet: Failed to get search results:" << m_lastError;
        return QStringList();
    }
    
    return reply.value();
}

QList<QVariantMap> BazaarClient::getResultMetas(const QStringList &resultIds) {
    QList<QVariantMap> metas;
    
    qDebug() << "BazaarClient::getResultMetas: Calling GetResultMetas with" << resultIds.size() << "result IDs";
    
    QDBusMessage metaCall = QDBusMessage::createMethodCall(
        QStringLiteral("io.github.kolunmi.bazaar"),
        QStringLiteral("/io/github/kolunmi/bazaar/SearchProvider"),
        QStringLiteral("org.gnome.Shell.SearchProvider2"),
        QStringLiteral("GetResultMetas")
    );
    metaCall << resultIds;
    
    QDBusMessage metaReply = QDBusConnection::sessionBus().call(metaCall);
    
    if (metaReply.type() == QDBusMessage::ErrorMessage) {
        m_lastError = metaReply.errorMessage();
        qWarning() << "BazaarClient::getResultMetas: Failed to get result metadata:" << m_lastError;
        return metas;
    }
    
    qDebug() << "BazaarClient::getResultMetas: GetResultMetas call successful, parsing response...";
    qDebug() << "BazaarClient::getResultMetas: Reply arguments count:" << metaReply.arguments().size();
    
    if (metaReply.arguments().isEmpty()) {
        m_lastError = "No arguments in GetResultMetas reply"_L1;
        qWarning() << "BazaarClient::getResultMetas:" << m_lastError;
        return metas;
    }
    
    // Get the first argument which should be aa{sv}
    QVariant metaVariant = metaReply.arguments().at(0);
    qDebug() << "BazaarClient::getResultMetas: Metadata variant type:" << metaVariant.typeName();
    
    if (metaVariant.canConvert<QDBusArgument>()) {
        qDebug() << "BazaarClient::getResultMetas: Converting QDBusArgument to metadata list";
        QDBusArgument metaArg = metaVariant.value<QDBusArgument>();
        
        // Handle aa{sv} signature - array of array of dictionaries
        metaArg.beginArray();
        while (!metaArg.atEnd()) {
            qDebug() << "BazaarClient::getResultMetas: Processing outer array element...";
            
            // Each outer element is a{sv} - an array of dictionaries
            // We need to begin the inner array directly on the same argument
            metaArg.beginArray();
            while (!metaArg.atEnd()) {
                qDebug() << "BazaarClient::getResultMetas: Processing inner array element (dictionary)...";
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
                    qDebug() << "BazaarClient::getResultMetas:     Key:" << key << "Value:" << value.toString() << "Type:" << value.typeName();
                }
                metaArg.endMap();
                
                metas.append(meta);
                qDebug() << "BazaarClient::getResultMetas:   Added metadata map with" << meta.size() << "keys:" << meta.keys();
            }
            metaArg.endArray();
        }
        metaArg.endArray();
    } else {
        qWarning() << "BazaarClient::getResultMetas: Cannot convert metadata to QDBusArgument, variant type:" << metaVariant.typeName();
        // Try direct conversion as fallback
        if (metaVariant.canConvert<QVariantList>()) {
            QVariantList metaList = metaVariant.toList();
            qDebug() << "BazaarClient::getResultMetas: Trying direct conversion from QVariantList with" << metaList.size() << "items";
            for (const QVariant &item : metaList) {
                if (item.canConvert<QVariantMap>()) {
                    metas.append(item.toMap());
                }
            }
        }
    }
    
    return metas;
}
