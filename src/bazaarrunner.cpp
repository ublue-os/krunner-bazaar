/*
    SPDX-FileCopyrightText: 2025 Adam Fidel <adam@fidel.cloud>

    SPDX-License-Identifier: Apache-2.0
*/

#include "bazaarrunner.h"

#include <KLocalizedString>
#include <QProcess>
#include <QStandardPaths>
#include <QDebug>

BazaarRunner::BazaarRunner(QObject *parent, const KPluginMetaData &data)
    : KRunner::AbstractRunner(parent, data)
{
    qDebug() << "BazaarRunner: Constructor called";
    qDebug() << "BazaarRunner: Plugin name:" << data.name();
    qDebug() << "BazaarRunner: Plugin ID:" << data.pluginId();
    

    setMinLetterCount(2);
    
    if (!m_bazaarClient.isConnected()) {
        qWarning() << "BazaarRunner: Failed to connect to Bazaar:" << m_bazaarClient.lastError();
    } else {
        qDebug() << "BazaarRunner: Successfully initialized Bazaar client";
    }

    addSyntax(QStringLiteral(":q:"), i18n("Search for Flatpak applications in Bazaar"));
    qDebug() << "BazaarRunner: Constructor completed successfully";
}

void BazaarRunner::match(KRunner::RunnerContext &context)
{
    const QString term = context.query();

    if (term.length() < 2) {
        qDebug() << "BazaarRunner::match: Query too short, minimum 2 characters required";
        return;
    }

    QList<AppSuggestion> results = m_bazaarClient.search(term);

    int addedMatches = 0;
    for (const auto &app : results) {

        KRunner::QueryMatch match(this);
        match.setIconName(app.iconName);
        match.setText(i18n("Install %1", app.name));
        match.setSubtext(app.description);
        match.setData(app.id);
        match.setRelevance(0.9);
        context.addMatch(match);
        addedMatches++;
    
        qDebug() << "BazaarRunner::match: Added match for:" << app.name;
    }
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
    
    QStringList terms = context.query().split(QLatin1Char(' '), Qt::SkipEmptyParts);
    bool success = m_bazaarClient.activateResult(appId, terms);
    
    if (!success) {
        qWarning() << "Failed to activate result in Bazaar:" << m_bazaarClient.lastError();
    } else {
        qDebug() << "Successfully activated result:" << appId << "in Bazaar";
    }
}

K_PLUGIN_CLASS_WITH_JSON(BazaarRunner, "bazaarrunner.json")

// needed for the QObject subclass declared as part of K_PLUGIN_CLASS_WITH_JSON
#include "bazaarrunner.moc"

#include "moc_bazaarrunner.cpp"
