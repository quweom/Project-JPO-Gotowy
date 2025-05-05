#include "Measurement.h"
#include <QDebug>
#include <algorithm>
#include <QJsonArray>
#include <cmath>

Measurement::Measurement(const QJsonObject &json) {

    //parsowanie podstawowych pól
    if (json.contains("sensorId")) {
        m_sensorId = json["sensorId"].toInt();
    }

    if (json.contains("paramCode")) {
        m_paramCode = json["paramCode"].toString();
    }
    else if (json.contains("key")) {
        QString key = json["key"].toString();
        m_paramCode = key.split('_').first();
    }
    else {
        m_paramCode = "Nieznany";
        qWarning() << "Nie można zdefiniować kodu parametru w JSON:" << json;
    }

    //parsowanie danych pomiarowych
    if (json.contains("values") && json["values"].isArray()) {
        QJsonArray valuesArray = json["values"].toArray();

        for (const QJsonValue &value : valuesArray) {
            if (!value.isObject()) continue;

            QJsonObject pointObj = value.toObject();
            DataPoint point;

            //parsowanie timestampu
            if (pointObj.contains("date")) {
                point.timestamp = QDateTime::fromString(
                    pointObj["date"].toString(),
                    "yyyy-MM-dd HH:mm:ss"
                    );
            }

            //parsowanie wartości
            if (pointObj.contains("value")) {
                if (pointObj["value"].isNull()) {
                    point.value = NAN;
                    point.isValid = false;
                } else {
                    bool ok;
                    double val = pointObj["value"].toVariant().toDouble(&ok);
                    point.value = ok ? val : NAN;
                    point.isValid = ok;
                }
            }

            m_data.append(point);
        }
    }

}

QString Measurement::toString() const {
    QString result = QString("Parametr: %1\nMeasurements:\n").arg(m_paramCode);
    for (const DataPoint& point : m_data) {
        result += QString("- %1: %2\n")
        .arg(point.timestamp.toString("yyyy-MM-dd HH:mm"),
             std::isnan(point.value) ? "NULL" : QString::number(point.value));
    }
    return result;
}

//implementacje metod analizy danych
double Measurement::maxValue() const {
    double max = NAN;
    for (const DataPoint& point : m_data) {
        if (point.isValid && (!std::isnan(point.value) && (std::isnan(max) || point.value > max))) {
            max = point.value;
        }
    }
    return max;
}

double Measurement::avgValue() const {
    double sum = 0;
    int count = 0;
    for (const DataPoint& point : m_data) {
        if (point.isValid && !std::isnan(point.value)) {
            sum += point.value;
            count++;
        }
    }
    return count > 0 ? sum / count : NAN;
}

int Measurement::validCount() const {
    return std::count_if(m_data.begin(), m_data.end(),
                         [](const DataPoint& p) { return p.isValid; });
}

double Measurement::dataCompleteness() const {
    if (m_data.empty()) return 0;
    return (validCount() * 100.0) / m_data.size();
}

QVector<Measurement::DataPoint> Measurement::filterByDateRange(const QDateTime& from, const QDateTime& to) const
{
    QVector<DataPoint> result;

    for (const auto& point : m_data) {
        bool matches = true;
        if (from.isValid() && point.timestamp < from) {
            matches = false;
        }
        if (to.isValid() && point.timestamp > to) {
            matches = false;
        }
        if (matches) {
            result.append(point);
        }
    }

    return result;
}

Measurement::AnalysisResult Measurement::analyzeData() const
{
    AnalysisResult result;
    result.minValue = std::numeric_limits<double>::max();
    result.maxValue = std::numeric_limits<double>::lowest();
    result.avgValue = 0;
    double sum = 0;
    int count = 0;
    QVector<double> values;
    QVector<double> times;

    // zbieramy dane i obliczamy min/max/avg
    for (const DataPoint& point : m_data) {
        if (point.isValid && !std::isnan(point.value)) {
            if (point.value < result.minValue) {
                result.minValue = point.value;
                result.minTime = point.timestamp;
            }
            if (point.value > result.maxValue) {
                result.maxValue = point.value;
                result.maxTime = point.timestamp;
            }
            sum += point.value;
            count++;

            values.append(point.value);
            times.append(point.timestamp.toSecsSinceEpoch());
        }
    }

    // obliczenie średniej
    if (count > 0) {
        result.avgValue = sum / count;
    } else {
        result.minValue = NAN;
        result.maxValue = NAN;
        result.avgValue = NAN;
    }

    if (count > 1) {
        double sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0;
        int n = 0;

        for (const DataPoint& p : m_data) {
            if (p.isValid) {
                double x = p.timestamp.toSecsSinceEpoch() / 3600.0;
                double y = p.value;
                sumX += x;
                sumY += y;
                sumXY += x * y;
                sumX2 += x * x;
                n++;
            }
        }

        double a = (n * sumXY - sumX * sumY) / (n * sumX2 - sumX * sumX);
        double avgChangePerHour = a;

        // klasyfikacja trendu
        if (abs(avgChangePerHour) < 0.01) {
            result.trend = "<span style='color:black;'>Stabilne</span>";
        } else if (avgChangePerHour >= 0.01) {
            result.trend = QString("<span style='color:green;'>Wzrost (%1/h)</span>")
            .arg(avgChangePerHour, 0, 'f', 3);
        } else {
            result.trend = QString("<span style='color:red;'>Spadek (%1/h)</span>")
            .arg(avgChangePerHour, 0, 'f', 3);
        }
        result.trendValue = avgChangePerHour;
    }

    return result;
}

int Measurement::sensorId() const { return m_sensorId; }

QDateTime Measurement::timestamp() const { return m_data.isEmpty() ? QDateTime() : m_data.first().timestamp; }
