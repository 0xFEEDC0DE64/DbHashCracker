#include "workerthread.h"

#include <QQueue>
#include <QPair>
#include <QCryptographicHash>
#include <QDebug>
#include <QProcessEnvironment>
#include <QSqlError>

QAtomicInt WorkerThread::m_dbIdentifier;

QAtomicInteger<quint64> WorkerThread::m_counter;
QAtomicInt WorkerThread::m_speed;

WorkerThread::WorkerThread(const QStringList &prefixes, const QStringList &suffixes, const QString &tableName,
                   const int bufferSize, const bool noReplace, const bool noDelayed, QObject *parent) :
    QThread(parent), m_prefixes(prefixes), m_suffixes(suffixes), m_tableName(tableName),
    m_bufferSize(bufferSize), m_noReplace(noReplace), m_noDelayed(noDelayed)
{
}

void WorkerThread::run()
{
    QSqlDatabase db = getDatabase(QStringLiteral("thread%0").arg(m_dbIdentifier++));
    if(!db.isOpen())
        return;

    QSqlQuery query = getQuery(db, m_tableName, m_noReplace, m_noDelayed, m_bufferSize);
    //TODO error handling

    QQueue<QPair<QString, QString> > queue;

    for(const auto &prefix : m_prefixes)
        for(const auto &suffix : m_suffixes)
        {
            const auto word = prefix + suffix;
            queue.enqueue(qMakePair(QCryptographicHash::hash(word.toUtf8(), QCryptographicHash::Md5).toHex(), word));

            m_counter++;
            m_speed++;

            if(queue.count() == m_bufferSize)
                if(!bindAndExecute(query, queue))
                    return;

            if(isInterruptionRequested())
                return;
        }

    if(!queue.isEmpty())
    {
        query = getQuery(db, m_tableName, m_noReplace, m_noDelayed, queue.count());
        //TODO error handling

        if(!bindAndExecute(query, queue))
            return;
    }
}

QSqlDatabase WorkerThread::getDatabase(const QString &connectionName)
{
    qDebug() << "connecting to database for" << connectionName;

    QSqlDatabase db(QSqlDatabase::addDatabase(QStringLiteral("QMYSQL"), connectionName));
    db.setHostName(QProcessEnvironment::systemEnvironment().value(QStringLiteral("MYSQL_HOSTNAME"), QStringLiteral("localhost")));
    db.setUserName(QProcessEnvironment::systemEnvironment().value(QStringLiteral("MYSQL_USERNAME"), QStringLiteral("hashcracker")));
    db.setPassword(QProcessEnvironment::systemEnvironment().value(QStringLiteral("MYSQL_PASSWORD"), QStringLiteral("topsecret")));
    db.setDatabaseName(QProcessEnvironment::systemEnvironment().value(QStringLiteral("MYSQL_DATABASE"), QStringLiteral("hashcracker")));

    if(!db.open())
    {
        qCritical() << db.lastError().text();
        qCritical() << db.lastError().databaseText();
        qFatal("could not connect to database");
    }

    return db;
}

QSqlQuery WorkerThread::getQuery(const QSqlDatabase &db, const QString &tableName, const bool noReplace, const bool noDelayed, const int rowCount)
{
    QSqlQuery query(db);

    QString columns;
    for(int i = 0; i < rowCount; i++)
    {
        if(i > 0)
            columns.append(',');
        columns.append(QStringLiteral(" (:Hash%0, :Value%0)").arg(i));
    }

    if(!query.prepare(QStringLiteral("%0%1 INTO `%2` (`Hash`, `Value`) VALUES%3")
                      .arg(noReplace ? QStringLiteral("INSERT") : QStringLiteral("REPLACE"))
                      .arg(noDelayed ? QString() : QStringLiteral(" DELAYED"))
                      .arg(tableName)
                      .arg(columns)))
    {
        qCritical() << query.lastError().text();
        qCritical() << query.lastError().databaseText();
        qFatal("could not prepare query");
    }

    return query;
}

bool WorkerThread::bindAndExecute(QSqlQuery &query, QQueue<QPair<QString, QString> > &queue)
{
    for(int i = 0; !queue.isEmpty(); i++)
    {
        const auto pair = queue.dequeue();
        query.bindValue(QStringLiteral(":Hash%0").arg(i), pair.first);
        query.bindValue(QStringLiteral(":Value%0").arg(i), pair.second);
    }

    const auto result = query.exec();
    if(!result)
    {
        qCritical() << query.lastError().text();
        qCritical() << query.lastError().databaseText();
        qFatal("could not execute query");
    }

    return result;
}
