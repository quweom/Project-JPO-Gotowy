#include "AirQualityIndex.h"
#include <QDebug>
#include <QColor>

AirQualityIndex::AirQualityIndex(const QJsonObject &json) {
    //parsowanie podstawowych pól
    m_stationId = json["id"].toInt();
    m_calcDate = QDateTime::fromString(json["stCalcDate"].toString(), Qt::ISODate);
    m_sourceDataDate = QDateTime::fromString(json["stSourceDataDate"].toString(), Qt::ISODate);

    //parsowanie głównego indeksu
    QJsonObject indexObj = json["stIndexLevel"].toObject();
    m_overallIndex.id = indexObj["id"].toInt();
    m_overallIndex.name = indexObj["indexLevelName"].toString();

    //parsowanie danych stanowisk pomiarowych
    QJsonArray stations = json["stations"].toArray();
    for (const QJsonValue &stationVal : stations) {
        QJsonObject stationObj = stationVal.toObject();
        StationData data;

        data.paramName = stationObj["paramName"].toString();
        data.calcDate = QDateTime::fromString(stationObj["calcDate"].toString(), Qt::ISODate);

        QJsonObject levelObj = stationObj["indexLevel"].toObject();
        data.level.id = levelObj["id"].toInt();
        data.level.name = levelObj["indexLevelName"].toString();

        m_stationReadings.append(data);
    }
}

QString AirQualityIndex::toString() const {
    if (!isValid()) {
        return "Brak danych o jakości powietrza";
    }
    return QString("Wskaźnik jakości powietrza: %1").arg(m_overallIndex.name);
}

bool AirQualityIndex::isValid() const {
    return m_stationId > 0 && m_calcDate.isValid();
}

QColor AirQualityIndex::getQualityColor() const {
    if (!isValid()) return Qt::gray;

    switch (m_overallIndex.id) {
    case 0: return QColor(0, 228, 0);
    case 1: return QColor(177, 255, 129);
    case 2: return QColor(255, 255, 0);
    case 3: return QColor(255, 126, 0);
    case 4: return QColor(255, 0, 0);
    case 5: return QColor(126, 0, 35);
    default: return Qt::gray;
    }
}

//json
QDateTime AirQualityIndex::calcDate() const { return m_calcDate; }

