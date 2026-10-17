/*
 * SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: LGPL-2.1-only
 */
#include "intenturl.h"
#include <QtTest/QtTest>

class Ut_IntentUrl : public QObject
{
    Q_OBJECT

private slots:
    void terminator_data();
    void terminator();
    void targetSemicolons();
    void validation_data();
    void validation();
};

void Ut_IntentUrl::terminator_data()
{
    QTest::addColumn<QString>("ending");
    QTest::addColumn<bool>("valid");
    QTest::newRow("standard") << QStringLiteral(";end") << true;
    QTest::newRow("google-trailing-semicolon") << QStringLiteral(";end;") << true;
    QTest::newRow("multiple-trailing-semicolons") << QStringLiteral(";end;;") << true;
    QTest::newRow("trailing-field") << QStringLiteral(";end;scheme=http") << false;
    QTest::newRow("missing-end") << QStringLiteral(";") << false;
    QTest::newRow("empty-field") << QStringLiteral(";;end;") << true;
    QTest::newRow("trailing-whitespace") << QStringLiteral(";end; ") << false;
}

void Ut_IntentUrl::terminator()
{
    QFETCH(QString, ending);
    QFETCH(bool, valid);
    const auto intent = Lipstick::IntentUrl::parse(QStringLiteral(
            "intent://www.google.com/maps/?q=Paris#Intent;scheme=https;"
            "package=com.google.android.apps.maps;"
            "S.browser_fallback_url=https%3A%2F%2Fwww.google.com%2Fmaps%2F") + ending);
    QCOMPARE(intent.valid, valid);
    QCOMPARE(intent.isAllowed(), valid);
    if (valid) {
        QCOMPARE(intent.target, QUrl(QStringLiteral("https://www.google.com/maps/?q=Paris")));
        QCOMPARE(intent.package, QStringLiteral("com.google.android.apps.maps"));
        QCOMPARE(intent.allowedFallback(), QUrl(QStringLiteral("https://www.google.com/maps/")));
    }
}

void Ut_IntentUrl::targetSemicolons()
{
    const auto intent = Lipstick::IntentUrl::parse(QStringLiteral(
            "intent://example.com/a;b?q=x;y#Intent;;;scheme=https;;;"
            "package=com.example.maps;;S.browser_fallback_url=https%3A%2F%2Fexample.com%2Fa%3Bb;;end;;"));
    QVERIFY(intent.isAllowed());
    QCOMPARE(intent.target, QUrl(QStringLiteral("https://example.com/a;b?q=x;y")));
    QCOMPARE(intent.package, QStringLiteral("com.example.maps"));
    QCOMPARE(intent.allowedFallback(), QUrl(QStringLiteral("https://example.com/a;b")));
}

void Ut_IntentUrl::validation_data()
{
    QTest::addColumn<QString>("fields");
    QTest::addColumn<bool>("valid");
    QTest::addColumn<bool>("allowed");
    QTest::newRow("only-empty-fields") << QStringLiteral(";;") << false << false;
    QTest::newRow("empty-tokens") << QStringLiteral(";;scheme=https;;;package=com.example.maps;;") << true << true;
    QTest::newRow("empty-key") << QStringLiteral("scheme=https;;=value") << false << false;
    QTest::newRow("duplicate-scheme") << QStringLiteral("scheme=https;scheme=http") << false << false;
    QTest::newRow("bad-escape") << QStringLiteral("scheme=https;package=com.example.%GG") << false << false;
    QTest::newRow("encoded-control") << QStringLiteral("scheme=https;package=com.example.%0A") << false << false;
    QTest::newRow("unsafe-scheme") << QStringLiteral("scheme=javascript") << true << false;
    QTest::newRow("unsafe-package") << QStringLiteral("scheme=https;package=com.jolla.nativeapp.test") << true << false;
    QTest::newRow("unsafe-action") << QStringLiteral("scheme=https;action=android.intent.action.SEND") << true << false;
}

void Ut_IntentUrl::validation()
{
    QFETCH(QString, fields);
    QFETCH(bool, valid);
    QFETCH(bool, allowed);
    for (const QString &ending : {QStringLiteral(";end"), QStringLiteral(";end;")}) {
        const auto intent = Lipstick::IntentUrl::parse(
                QStringLiteral("intent://example.com/#Intent;") + fields + ending);
        QCOMPARE(intent.valid, valid);
        QCOMPARE(intent.isAllowed(), allowed);
    }
}

QTEST_APPLESS_MAIN(Ut_IntentUrl)
#include "ut_intenturl.moc"
