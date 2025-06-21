/*
    SPDX-FileCopyrightText: 2025 Adam Fidel <adam@fidel.cloud>

    SPDX-License-Identifier: Apache-2.0
*/

#pragma once

#include <KRunner/AbstractRunner>
#include <QDBusInterface>
#include <QDBusReply>

struct AppSuggestion {
    QString id;
    QString name;
    QString description;
    QString iconName;
};

class BazaarRunner : public KRunner::AbstractRunner {
    Q_OBJECT

public:
    BazaarRunner(QObject *parent, const KPluginMetaData &data);

    void match(KRunner::RunnerContext &context) override;
    void run(const KRunner::RunnerContext &context, const KRunner::QueryMatch &match) override;

private:
    QList<AppSuggestion> queryBazaar(const QString &term);
    bool isInstalled(const QString &appId);
    
    std::unique_ptr<QDBusInterface> m_bazaarInterface;
};
