#include "Sensor.h"
#include <QDebug>

Sensor::Sensor(const QJsonObject &json) {
    //parsowanie podstawowych pól
    m_id = json["id"].toInt();
    m_stationId = json["stationId"].toInt();

    //parsowanie zagnieżdżonej struktury parametru
    QJsonObject param = json["param"].toObject();
    m_param.name = param["paramName"].toString();
    m_param.formula = param["paramFormula"].toString();
    m_param.code = param["paramCode"].toString();
    m_param.id = param["idParam"].toInt();
}

QString Sensor::toString() const {
    return QString("Czujnik %1 (Stacja: %2)\nParametr: %3 (%4)")
    .arg(m_id)
        .arg(m_stationId)
        .arg(m_param.name)
        .arg(m_param.formula)
        .arg(m_param.code);
}

QString Sensor::paramName() const { return m_paramName; }
QString Sensor::paramCode() const { return m_paramCode; }
