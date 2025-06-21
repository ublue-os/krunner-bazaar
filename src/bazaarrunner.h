/*
    SPDX-FileCopyrightText: 2025 Adam Fidel <adam@fidel.cloud>

    SPDX-License-Identifier: Apache-2.0
*/

#pragma once

#include <KRunner/AbstractRunner>

class BazaarRunner : public Plasma::AbstractRunner {
    Q_OBJECT

public:
    BazaarRunner(QObject *parent, const QVariantList &args);
        : Plasma::AbstractRunner(parent, args)
    /*{
        setObjectName("BazaarRunner");
        setDefaultMatchRegex(QRegExp(".*"));
        setPriority(HighPriority);
    }*/

    void match(Plasma::RunnerContext &context) override; {

    }

    void run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &match) override;

private:

};
