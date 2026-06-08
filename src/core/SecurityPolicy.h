#pragma once

#include <QString>
#include <QtGlobal>

class SecurityPolicy
{
public:
    static bool requiresPassword(quint8 command);
    static bool isPasswordValid(const QString &password);
};
