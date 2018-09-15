#pragma once

#include <QThread>
#include <QAtomicInteger>
#include <QAtomicInt>
#include <QStringList>
#include <QString>
#include <QSqlDatabase>
#include <QSqlQuery>

template <class T> class QQueue;
template <class T1, class T2> struct QPair;

class WorkerThread : public QThread
{
    Q_OBJECT

    static QAtomicInt m_dbIdentifier;

public:
    static QAtomicInteger<quint64> m_counter;
    static QAtomicInt m_speed;

    explicit WorkerThread(const QStringList &prefixes, const QStringList &suffixes, const QString &tableName,
                      const int bufferSize, const bool noReplace, const bool noDelayed, QObject *parent = Q_NULLPTR);

    void run() Q_DECL_OVERRIDE;

    static QSqlDatabase getDatabase(const QString &connectionName = QLatin1String(QSqlDatabase::defaultConnection));
    static QSqlQuery getQuery(const QSqlDatabase &db, const QString &tableName, const bool noReplace, const bool noDelayed, const int rowCount);
    static bool bindAndExecute(QSqlQuery &query, QQueue<QPair<QString, QString> > &queue);

private:
    const QStringList m_prefixes;
    const QStringList m_suffixes;
    const QString m_tableName;
    const int m_bufferSize;
    const bool m_noReplace;
    const bool m_noDelayed;
};
