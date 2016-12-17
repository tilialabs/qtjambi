
#include <QFileInfo>
#include <QStringList>
#include <QDir>
#include <QDebug>

#include <cstdio>

#include "preprocesshandler.h"
#include "wrapper.h"

PreprocessHandler::PreprocessHandler(QString sourceFile, QString targetFile, const QString &phononInclude,
    const QStringList &includePathList, const QStringList &inputDirectoryList, int verbose) :
        preprocess(env),
        verbose(verbose),
        ppconfig(":/trolltech/generator/parser/rpp/pp-qt-configuration"),
        sourceFile(sourceFile),
        targetFile(targetFile),
        phononInclude(phononInclude),
        includePathList(includePathList),
        inputDirectoryList(inputDirectoryList) {
    //empty space for useless comments
    preprocess.verbose = verbose;
}

bool PreprocessHandler::checkDefineUndefine(const QString &name, int check) const {
    return false;
}

bool PreprocessHandler::dumpCheck(int kind) const {
    if((verbose & kind) == kind)
        return true;
    return false;
}

void PreprocessHandler::dump(int kind) const {
    std::string stage;
    if(kind == DEBUGLOG_DUMP_BEFORE)
        stage = "STAGE1";
    else if(kind == DEBUGLOG_DUMP_MIDDLE)
        stage = "STAGE2";
    else if(kind == DEBUGLOG_DUMP_AFTER)
        stage = "AFTER";
    else
        return;

    std::cout << "DUMP " << stage << " (begin)" << std::endl;

    std::cout << "DUMP " << stage << " (end)" << std::endl;
}


bool PreprocessHandler::handler() {
    QFile file(ppconfig);
    if (!file.open(QFile::ReadOnly)) {
        std::fprintf(stderr, "Preprocessor configuration file not found '%s'\n", ppconfig);
        return false;
    }

    QByteArray ba = file.readAll();
    file.close();

// FIXME: Dump empty at start
// FIXME: If any debug mode, enable showing DEFINE/UNDEF/INCLUDE(summary/verbose)
// FIXME: If nothing set on cmdline enable #define Q_OS_OS2
// PROCESS string of "#define name value"
// FIXME: Replace "null_out" with stdout
    preprocess.operator()(ba.constData(), ba.constData() + ba.size(), null_out);
// FIXME: Dump defines set
// FIXME: Restore normal debug mode, showing DEFINE/UNDEF/INCLUDE(summary/verbose)
    QStringList includes = setIncludes();

    foreach(QString include, includes) {
#if QT_VERSION >= 0x050000
        preprocess.push_include_path(toStdString(QDir::toNativeSeparators(include)));
#else
        preprocess.push_include_path(toStdString(QDir::convertSeparators(include)));
#endif
    }
// FIXME: Dump defines set

    QString currentDir = QDir::current().absolutePath();

    writeTargetFile(sourceFile, targetFile, currentDir);

    return true;
}

void PreprocessHandler::writeTargetFile(QString sourceFile, QString targetFile, QString currentDir) {

    QFileInfo sourceInfo(sourceFile);
    QDir::setCurrent(sourceInfo.absolutePath());

    std::string result;
    result.reserve(20 * 1024);  // 20K

    result += "# 1 \"builtins\"\n";
    result += "# 1 \"";
    result += toStdString(sourceFile);
    result += "\"\n";

    qDebug() << "Processing source" << sourceInfo.absolutePath() << sourceInfo.fileName();
    preprocess.file(toStdString(sourceInfo.fileName()),
                    rpp::pp_output_iterator<std::string> (result));

    QDir::setCurrent(currentDir);

    QFile f(targetFile);
    if (!f.open(QIODevice::Append | QIODevice::Text)) {
        std::fprintf(stderr, "Failed to write preprocessed file: %s\n", qPrintable(targetFile));
    }
    f.write(result.c_str(), result.length());
}

QStringList PreprocessHandler::setIncludes() {

    QStringList includes;

    // It is important any explicitly given phonon include dir is before the main Qt include
    //  directory in the search order.  This is so that on a build system that has both a Qt
    //  Phonon and another Phonon implementation (like from KDE) it should find the ecplicitly
    //  given include location first.
    QString phonon_include_dir;
    if (!phononInclude.isEmpty()) {
        phonon_include_dir = phononInclude;
    } else {
#if defined(Q_OS_MAC)
        phonon_include_dir = "/Library/Frameworks/phonon.framework/Headers";
        if(!phonon_include_dir.isNull())
            std::fprintf(stdout, "Appending built-in --phonon-include: %s\n", qPrintable(phonon_include_dir));
#endif
    }
    if (!phonon_include_dir.isNull())
        includes << phonon_include_dir;

    // Include Qt
    QString includedir;
    if (!Wrapper::include_directory.isNull()) {
        includedir = Wrapper::include_directory;
    } else {
#if defined(Q_OS_MAC)
        includedir = "/Library/Frameworks";
#else
        includedir = "/usr/include/qt4";
#endif
        if(!includedir.isNull())
            std::fprintf(stdout, "Appending built-in --qt-include-directory: %s\n", qPrintable(includedir));
    }
    if (!includedir.isNull())
        includes << includedir;

    // Additional include locations from command line
    if (!includePathList.isEmpty())
        includes << includePathList;

    return includes;
}
