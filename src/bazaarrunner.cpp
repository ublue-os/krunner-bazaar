/*
    SPDX-FileCopyrightText: 2025 Adam Fidel <adam@fidel.cloud>

    SPDX-License-Identifier: Apache-2.0
*/

#include "bazaarrunner.h"

#include <KLocalizedString>

struct AppSuggestion {};

bool isInstalled(const QString &appId) {
    // Use Flatpak API or cache to check install status
    return false;
}

QList<AppSuggestion> queryBazaar(const QString &term) {
    // Connect to Bazaar API
    return {};
}

BazaarRunner::BazaarRunner(QObject *parent, const KPluginMetaData &data)
    : KRunner::AbstractRunner(parent, data)
{
    // Disallow short queries (3 characters or less due to OBS)
    setMinLetterCount(2);
    addSyntax(QStringLiteral("sometriggerword :q:"), i18n("Description for this syntax"));
}

void BazaarRunner::match(KRunner::RunnerContext &context)
{
    /*const QString term = context.query();
    if (term.compare(QLatin1String("hello"), Qt::CaseInsensitive) == 0) {
        KRunner::QueryMatch match(this);
        match.setText(i18n("Hello from BazaarRunner"));
        context.addMatch(match);
    }*/

    const QString term = context.query();
    if (term.length() < 3) return;

    QList<AppSuggestion> results = queryBazaar(term);

    for (const auto &app : results) {
        if (isInstalled(app.id)) continue;

        Plasma::QueryMatch match(this);
        match.setType(Plasma::QueryMatch::PossibleMatch);
        match.setIconName(app.iconName);
        match.setText(QStringLiteral("Install %1 via Bazaar").arg(app.name));
        match.setData(app.id);
        match.setRelevance(0.9);
        context.addMatch(match);
    }
}

void BazaarRunner::run(const KRunner::RunnerContext &context, const KRunner::QueryMatch &match)
{
    Q_UNUSED(context)
    Q_UNUSED(match)

    // TODO
}

K_PLUGIN_CLASS_WITH_JSON(BazaarRunner, "bazaarrunner.json")

// needed for the QObject subclass declared as part of K_PLUGIN_CLASS_WITH_JSON
#include "bazaarrunner.moc"

#include "moc_bazaarrunner.cpp"
