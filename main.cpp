#include <QCoreApplication>
#include <qlogging.h>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QString>
#include <QDebug>
#include <QStringList>
#include <QFile>
#include <QTextStream>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVector>
#include <QStringList>
#include <QDateTime>
#include <QTimer>

#include <algorithm>

#include "workerthread.h"

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    qSetMessagePattern(QStringLiteral("%{time HH:mm:ss.zzz} "
                                      "["
                                      "%{if-debug}D%{endif}"
                                      "%{if-info}I%{endif}"
                                      "%{if-warning}W%{endif}"
                                      "%{if-critical}C%{endif}"
                                      "%{if-fatal}F%{endif}"
                                      "] "
                                      "%{message}"));

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOptions({
        {
            { QStringLiteral("f"), QStringLiteral("filename") },
            QCoreApplication::translate("main", "File containing the wordlist"),
            QCoreApplication::translate("main", "filename"),
            QStringLiteral("wordlist.txt")
        },
        {
            { QStringLiteral("n"), QStringLiteral("table-name") },
            QCoreApplication::translate("main", "Name of the table to be filled"),
            QCoreApplication::translate("main", "table"),
            QStringLiteral("Hashes")
        },
        {
            { QStringLiteral("d"), QStringLiteral("no-drop-table") },
            QCoreApplication::translate("main", "Don't drop the (old) hash table at the beginning"),
        },
        {
            { QStringLiteral("c"), QStringLiteral("no-create-table") },
            QCoreApplication::translate("main", "Don't create a table at the beginning"),
        },
        {
            { QStringLiteral("i"), QStringLiteral("no-index") },
            QCoreApplication::translate("main", "Don't add an index on hash column"),
        },
        {
            { QStringLiteral("p"), QStringLiteral("no-primary") },
            QCoreApplication::translate("main", "Don't add an primary key on value column"),
        },
        {
            { QStringLiteral("t"), QStringLiteral("threads") },
            QCoreApplication::translate("main", "Thread count"),
            QCoreApplication::translate("main", "threads"),
            QString::number(QThread::idealThreadCount())
        },
        {
            { QStringLiteral("b"), QStringLiteral("buffer-size") },
            QCoreApplication::translate("main", "Buffer size (per thread)"),
            QCoreApplication::translate("main", "buffer-size"),
            QString::number(100)
        },
        {
            { QStringLiteral("r"), QStringLiteral("no-replace-into") },
            QCoreApplication::translate("main", "Use INSERT INTO instead of REPLACE INTO")
        },
        {
            QStringLiteral("no-delayed"),
            QCoreApplication::translate("main", "Dont add DELAYED keyword when inserting")
        }
    });

    parser.process(app);
    bool ok;

    const auto filename = parser.value(QStringLiteral("filename"));
    const auto tableName = parser.value(QStringLiteral("table-name"));
    const auto noDropTable = parser.isSet(QStringLiteral("no-drop-table"));
    const auto noCreateTable = parser.isSet(QStringLiteral("no-create-table"));
    const auto noIndex = parser.isSet(QStringLiteral("no-index"));
    const auto noPrimary = parser.isSet(QStringLiteral("no-primary"));

    const auto threads = parser.value(QStringLiteral("threads")).toInt(&ok);
    if(!ok)
    {
        qFatal("could not parse threads");
        return -1;
    }


    const auto bufferSize = parser.value(QStringLiteral("buffer-size")).toInt(&ok);;
    if(!ok)
    {
        qFatal("could not parse bufferSize");
        return -2;
    }

    const auto noReplace = parser.isSet(QStringLiteral("no-replace-into"));
    const auto noDelayed = parser.isSet(QStringLiteral("no-delayed"));

    qDebug() << "filename =" << filename;
    qDebug() << "tableName =" << tableName;
    qDebug() << "noDropTable =" << noDropTable;
    qDebug() << "noCreateTable =" << noCreateTable;
    qDebug() << "noIndex =" << noIndex;
    qDebug() << "noPrimary =" << noPrimary;
    qDebug() << "threads =" << threads;
    qDebug() << "bufferSize =" << bufferSize;
    qDebug() << "noReplace =" << noReplace;
    qDebug() << "noDelayed =" << noDelayed;

    qDebug() << "reading wordlist...";

    QStringList wordlist;

    {
        QFile file(filename);
        if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            qCritical() << file.errorString();
            return -1;
        }

        QTextStream textStream(&file);
        while(!textStream.atEnd())
            wordlist.append(textStream.readLine());
    }

    {
        QSqlDatabase db = WorkerThread::getDatabase();

        if(!noDropTable)
        {
            qDebug() << "dropping old table...";

            auto query = db.exec(QStringLiteral("DROP TABLE IF EXISTS `%0`").arg(tableName));
            if(query.lastError().isValid())
            {
                qCritical() << query.lastError().text();
                qCritical() << query.lastError().databaseText();
                qFatal("could not delete old table");
                return -3;
            }
        }

        if(!noCreateTable)
        {
            qDebug() << "creating table...";

            QString indexSql;

            if(noIndex)
                qWarning() << "will not add an index for hash";
            else
                indexSql.append(QStringLiteral(",INDEX(`Hash`)"));

            if(noPrimary)
                qWarning() << "will not add an primary for value";
            else
                indexSql.append(QStringLiteral(",PRIMARY KEY(`Value`)"));

            auto query = db.exec(QStringLiteral("CREATE TABLE `%0` ("
                                                "    `Hash` CHAR(32) NOT NULL,"
                                                "    `Value` VARCHAR(64) NOT NULL"
                                                "    %1"
                                                ")").arg(tableName).arg(indexSql));
            if(query.lastError().isValid())
            {
                qCritical() << query.lastError().text();
                qCritical() << query.lastError().databaseText();
                qFatal("could not create table");
                return -4;
            }
        }
    }

    QVector<QStringList> prefixes(threads);

    for(auto iter = wordlist.constBegin(); iter != wordlist.constEnd(); )
        for(int i = 0; i < threads && iter != wordlist.constEnd(); i++, iter++)
            prefixes[i].append(*iter);

    const quint64 allcount = quint64(wordlist.count()) * quint64(wordlist.count());

    QVector<WorkerThread*> workers(threads);

    for(int i = 0; i < threads; i++)
    {
        workers[i] = new WorkerThread(prefixes.at(i), wordlist, tableName, bufferSize, noReplace, noDelayed);
        workers.at(i)->start();
        QObject::connect(workers.at(i), &QThread::finished, [&workers](){
            qDebug() << "thread finished";

            const auto allFinished = std::all_of(workers.constBegin(), workers.constEnd(), [](const WorkerThread *worker){
                return worker->isFinished();
            });

            if(allFinished)
            {
                qDeleteAll(workers);
                workers.clear();
                qApp->quit();
            }
        });
    }

    const auto started = QDateTime::currentDateTime();

    QTimer timer;
    timer.setInterval(1000);
    QObject::connect(&timer, &QTimer::timeout, [allcount, started](){
        try {
            auto formatTime = [](quint64 total){
                auto seconds = total % 60;
                total -= seconds;
                total /= 60;

                auto minutes = total % 60;
                total -= minutes;
                total /= 60;

                const auto hours = total;

                return QString("%0:%1:%2")
                        .arg(hours)
                        .arg(minutes, 2, 10, QLatin1Char('0'))
                        .arg(seconds, 2, 10, QLatin1Char('0'));
            };

            const quint64 sum = WorkerThread::m_counter;
            const auto speed = WorkerThread::m_speed.fetchAndStoreOrdered(0);

            const auto elapsed = started.secsTo(QDateTime::currentDateTime());
            const auto avgSpeed = elapsed == 0 ? 0 : sum / elapsed;

            const auto remaining = allcount-sum;

            qInfo().nospace().noquote() << formatTime(elapsed) << " | "
                                        << sum << "/" << allcount
                                        << " (" << QString::number(qreal(sum)/allcount*100., 'f', 2) << "%) | NOW "
                                        << speed << "/sec "
                                        << formatTime(speed == 0 ? 0 : remaining/speed) << " | AVG "
                                        << avgSpeed << "/sec "
                                        << formatTime(avgSpeed == 0 ? 0 : remaining/avgSpeed);
        } catch (...) {
            qCritical() << "exception while printing stats";
        }
    });
    timer.start();

    const auto result = app.exec();
    qDebug() << "finished.";
    return result;
}
