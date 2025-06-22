#pragma once

#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QDBusInterface>
#include <memory>

struct AppSuggestion {
    QString id;
    QString name;
    QString description;
    QString iconName;
};

class BazaarClient {
public:
    BazaarClient();

    bool isConnected() const;
    QString lastError() const;
    QList<AppSuggestion> search(const QString &term);
    
    // Activate/launch an application in Bazaar
    bool activateResult(const QString &appId, const QStringList &searchTerms);

private:
    std::unique_ptr<QDBusInterface> m_bazaarInterface;
    QString m_lastError;
    
    QStringList getInitialResultSet(const QStringList &terms);
    QList<QVariantMap> getResultMetas(const QStringList &resultIds);
};
