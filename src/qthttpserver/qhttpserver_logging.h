#ifndef QHTTPSERVER_LOGGING_H
#define QHTTPSERVER_LOGGING_H

#include <QtCore/QLoggingCategory>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(qHttpServerLogging)

#define qhsDebug() qCDebug(qHttpServerLogging)
#define qhsWarning() qCWarning(qHttpServerLogging)
#define qhsCritical() qCCritical(qHttpServerLogging)
#define qhsInfo() qCInfo(qHttpServerLogging)
#define qhsFatal(...) qCFatal(qHttpServerLogging, __VA_ARGS__)

QT_END_NAMESPACE

#endif // QHTTPSERVER_LOGGING_H
