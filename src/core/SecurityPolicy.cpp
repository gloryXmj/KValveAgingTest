#include "src/core/SecurityPolicy.h"

#include "src/core/CommandMap.h"

namespace
{
constexpr auto kFixedPassword = "Keye@1234";
}

bool SecurityPolicy::requiresPassword(const quint8 command)
{
    return CommandMap::isProtectedTimingCommand(command);
}

bool SecurityPolicy::isPasswordValid(const QString &password)
{
    return password == QLatin1String(kFixedPassword);
}
