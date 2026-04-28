#include "mainwindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QLibraryInfo>
#include <QLocale>
#include <QTranslator>

namespace {
QString normalizedLocaleName(QString value)
{
    value = value.trimmed();
    if (value.isEmpty()) {
        return {};
    }

    value = value.section(QLatin1Char('.'), 0, 0);
    value = value.section(QLatin1Char('@'), 0, 0);
    value.replace(QLatin1Char('-'), QLatin1Char('_'));
    return value;
}

QString requestedLocaleName()
{
    const QList<QByteArray> variables = {"QT_IMG_VIEWER_LANG", "LC_ALL", "LC_MESSAGES", "LANG"};
    for (const QByteArray &variable : variables) {
        const QString localeName = normalizedLocaleName(qEnvironmentVariable(variable.constData()));
        if (!localeName.isEmpty() && localeName != QStringLiteral("C") && localeName != QStringLiteral("POSIX")) {
            return localeName;
        }
    }

    return QLocale::system().name();
}

QStringList localeCandidates(const QString &localeName)
{
    QStringList candidates;
    const QString normalized = normalizedLocaleName(localeName);
    if (!normalized.isEmpty()) {
        candidates.append(normalized);
        const QString language = normalized.section(QLatin1Char('_'), 0, 0);
        if (!language.isEmpty()) {
            candidates.append(language);
        }
        if (language == QStringLiteral("zh")) {
            candidates.append(QStringLiteral("zh_CN"));
        }
    }

    candidates.removeDuplicates();
    return candidates;
}

void installTranslations(QApplication &app, QTranslator &appTranslator, QTranslator &qtTranslator)
{
    const QString localeName = requestedLocaleName();
    QLocale::setDefault(QLocale(localeName));

    for (const QString &candidate : localeCandidates(localeName)) {
        if (appTranslator.load(QStringLiteral("qt-img-viewer_") + candidate, QStringLiteral(":/i18n"))) {
            app.installTranslator(&appTranslator);
            break;
        }
    }

    const QString qtTranslationsPath = QLibraryInfo::path(QLibraryInfo::TranslationsPath);
    for (const QString &candidate : localeCandidates(localeName)) {
        if (qtTranslator.load(QStringLiteral("qtbase_") + candidate, qtTranslationsPath)) {
            app.installTranslator(&qtTranslator);
            break;
        }
    }
}
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QTranslator appTranslator;
    QTranslator qtTranslator;
    installTranslations(app, appTranslator, qtTranslator);

    QApplication::setApplicationName(QCoreApplication::translate("main", "Qt Image Viewer"));
    QApplication::setDesktopFileName(QStringLiteral("qt-img-viewer"));
    QApplication::setOrganizationName(QStringLiteral("Local"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::translate("main", "Qt image viewer"));
    parser.addHelpOption();
    parser.addPositionalArgument(QStringLiteral("path"), QCoreApplication::translate("main", "Image file or directory to open."), QStringLiteral("[path]"));
    parser.process(app);

    MainWindow window;
    window.show();
    const QStringList paths = parser.positionalArguments();
    if (!paths.isEmpty()) {
        window.openPath(paths.first());
    }

    return app.exec();
}
