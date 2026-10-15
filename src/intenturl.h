/*
 * SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: LGPL-2.1-only
 */
#ifndef LIPSTICK_INTENTURL_H
#define LIPSTICK_INTENTURL_H

#include <QUrl>
#include <QSet>
#include <QStringList>
#include <QRegularExpression>

namespace Lipstick {
struct IntentUrl {
    bool valid = false;
    QUrl target;
    QUrl fallback;
    QString package;
    QString action = QStringLiteral("android.intent.action.VIEW");
    QString category = QStringLiteral("android.intent.category.BROWSABLE");

    // Launch policy shared by support queries and actual dispatch.
    bool isAllowed() const
    {
        const QSet<QString> forbidden = {QStringLiteral("intent"), QStringLiteral("javascript"),
            QStringLiteral("data"), QStringLiteral("file"), QStringLiteral("content"),
            QStringLiteral("about"), QStringLiteral("chrome"), QStringLiteral("resource")};
        return valid && !forbidden.contains(target.scheme())
                && !package.startsWith(QLatin1String("com.jolla.nativeapp."))
                && target.userInfo().isEmpty()
                && action == QLatin1String("android.intent.action.VIEW")
                && category == QLatin1String("android.intent.category.BROWSABLE");
    }

    // A disallowed fallback does not invalidate the target intent.
    QUrl allowedFallback() const
    {
        if (!fallback.isValid() || fallback.host().isEmpty()
                || !fallback.userInfo().isEmpty()
                || (fallback.scheme() != QLatin1String("http") && fallback.scheme() != QLatin1String("https"))) {
            return QUrl();
        }
        return fallback;
    }

    static IntentUrl parse(const QString &value)
    {
        IntentUrl result;
        if (!value.startsWith(QLatin1String("intent:")) || value.size() > 65536
                || value.contains(QRegularExpression(QStringLiteral("[\\x00-\\x20\\x7f]")))) {
            return result;
        }
        const int marker = value.indexOf(QLatin1String("#Intent;"));
        if (marker < 0 || !value.endsWith(QLatin1String(";end"))
                || value.indexOf(QLatin1String("#Intent;"), marker + 1) >= 0) {
            return result;
        }
        const QString fields = value.mid(marker + 8, value.size() - marker - 12);
        QString scheme;
        QString fallback;
        QSet<QString> seen;
        const QStringList tokens = fields.split(QLatin1Char(';'));
        for (const QString &token : tokens) {
            const int equals = token.indexOf(QLatin1Char('='));
            if (equals <= 0) {
                return result;
            }
            const QString key = token.left(equals);
            const QString encoded = token.mid(equals + 1);
            if (seen.contains(key) || encoded.contains(QRegularExpression(QStringLiteral("%(?![0-9a-fA-F]{2})")))) {
                return result;
            }
            seen.insert(key);
            const QString decoded = QUrl::fromPercentEncoding(encoded.toUtf8());
            if (decoded.contains(QRegularExpression(QStringLiteral("[\\x00-\\x1f\\x7f]")))) {
                return result;
            }
            if (key == QLatin1String("scheme")) {
                scheme = decoded.toLower();
            } else if (key == QLatin1String("package")) {
                result.package = decoded;
            } else if (key == QLatin1String("S.browser_fallback_url")) {
                fallback = decoded;
            } else if (key == QLatin1String("action")) {
                result.action = decoded;
            } else if (key == QLatin1String("category")) {
                result.category = decoded;
            } else {
                return result;
            }
        }
        static const QRegularExpression schemePattern(QStringLiteral("^[a-z][a-z0-9+.-]*$"));
        static const QRegularExpression packagePattern(QStringLiteral("^[a-zA-Z][a-zA-Z0-9_]*(\\.[a-zA-Z][a-zA-Z0-9_]*)+$"));
        if (!schemePattern.match(scheme).hasMatch()
                || (!result.package.isEmpty() && !packagePattern.match(result.package).hasMatch())) {
            return result;
        }
        result.target = QUrl(scheme + value.mid(6, marker - 6), QUrl::StrictMode);
        if (!result.target.isValid() || result.target.isRelative()) {
            return result;
        }
        result.fallback = QUrl(fallback, QUrl::StrictMode);
        result.valid = true;
        return result;
    }
};
}
#endif
