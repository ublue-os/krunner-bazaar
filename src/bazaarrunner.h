/*
    SPDX-FileCopyrightText: 2025 Adam Fidel <adam@fidel.cloud>

    SPDX-License-Identifier: Apache-2.0
*/

#pragma once

#include <KRunner/AbstractRunner>
#include "bazaarclient.h"

class BazaarRunner : public KRunner::AbstractRunner {
    Q_OBJECT

public:
    BazaarRunner(QObject *parent, const KPluginMetaData &data);

    void match(KRunner::RunnerContext &context) override;
    void run(const KRunner::RunnerContext &context, const KRunner::QueryMatch &match) override;

private:
    bool isInstalled(const QString &appId);
    
    BazaarClient m_bazaarClient;
};
