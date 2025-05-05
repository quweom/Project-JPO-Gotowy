#include "JsonBaseManager.h"
#include <QDir>

JsonBaseManager::JsonBaseManager(QObject *parent)
    : QObject(parent)
{
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(appDataPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    m_jsonFilePath = appDataPath + "/air_quality_data.json";

    if (!QFile::exists(m_jsonFilePath)) {
        initializeDataFile();
    }
}

//operacje dla zapisywania stacji
bool JsonBaseManager::saveStation(const Station &station)
{
    QJsonObject root = loadRootObject();
    QJsonArray stationsArray = root["stations"].toArray();

    //sprawdzamy istnienie stacji
    bool exists = false;
    for (auto &&item : stationsArray) {
        if (item.toObject()["id"].toInt() == station.id()) {
            exists = true;
            break;
        }
    }

    if (!exists) {
        QJsonObject stationObj;
        stationObj["id"] = station.id();
        stationObj["stationName"] = station.name();
        stationObj["gegrLat"] = QString::number(station.latitude(), 'f', 6);
        stationObj["gegrLon"] = QString::number(station.longitude(), 'f', 6);
        stationObj["addressStreet"] = station.addressStreet();

        QJsonObject addressObj;
        addressObj["city"] = station.cityName();
        stationObj["address"] = addressObj;

        stationsArray.append(stationObj);
        root["stations"] = stationsArray;
        return saveToFile(root);
    }

    return true;
}

QVector<Station> JsonBaseManager::loadStations() const
{
    QJsonObject root = loadRootObject();
    QJsonArray stationsArray = root["stations"].toArray();
    QVector<Station> stations;

    for (const QJsonValue &value : stationsArray) {
        QJsonObject obj = value.toObject();
        Station station;

        station.setId(obj["id"].toInt());
        station.setName(obj["stationName"].toString());
        station.setLatitude(obj["gegrLat"].toString().toDouble());
        station.setLongitude(obj["gegrLon"].toString().toDouble());
        station.setAddressStreet(obj["addressStreet"].toString());

        if (obj.contains("address")) {
            QJsonObject address = obj["address"].toObject();
            station.setCityName(address["city"].toString());
        }

        stations.append(station);
    }

    return stations;
}

//operacje dla zapisywania czujnika
bool JsonBaseManager::saveSensor(const Sensor &sensor)
{
    QJsonObject root = loadRootObject();
    QJsonArray sensorsArray = root["sensors"].toArray();

    //sprawdzamy istnienie czujnika
    bool exists = false;
    for (auto &&item : sensorsArray) {
        if (item.toObject()["id"].toInt() == sensor.id()) {
            exists = true;
            break;
        }
    }

    if (!exists) {
        QJsonObject sensorObj;
        sensorObj["id"] = sensor.id();
        sensorObj["stationId"] = sensor.stationId();

        sensorObj["paramName"] = sensor.paramName();
        sensorObj["paramCode"] = sensor.paramCode();

        sensorsArray.append(sensorObj);
        root["sensors"] = sensorsArray;
        return saveToFile(root);
    }

    return true;
}

QVector<Sensor> JsonBaseManager::loadSensors() const
{
    QJsonObject root = loadRootObject();
    QJsonArray sensorsArray = root["sensors"].toArray();
    QVector<Sensor> sensors;

    for (const QJsonValue &value : sensorsArray) {
        QJsonObject obj = value.toObject();
        Sensor sensor(obj);
        sensors.append(sensor);
    }

    return sensors;
}

//operacje dla zapisywania pomiarów
bool JsonBaseManager::saveMeasurement(const Measurement &measurement)
{
    QJsonObject root = loadRootObject();
    QJsonArray measurementsArray = root["measurements"].toArray();

    //tworzymy unikalny klucz do pomiaru
    QString key = QString("%1_%2")
                      .arg(measurement.sensorId())
                      .arg(measurement.timestamp().toString(Qt::ISODate));

    //sprawdzamy istnienie wymiaru
    bool exists = false;
    for (auto &&item : measurementsArray) {
        if (item.toObject()["key"].toString() == key) {
            exists = true;
            break;
        }
    }

    if (!exists) {
        QJsonObject measurementObj;
        measurementObj["key"] = key;
        measurementObj["sensorId"] = measurement.sensorId();
        measurementObj["values"] = QJsonArray();

        measurementsArray.append(measurementObj);
        root["measurements"] = measurementsArray;
        return saveToFile(root);
    }

    return true;
}

QVector<Measurement> JsonBaseManager::loadMeasurements() const
{
    QJsonObject root = loadRootObject();
    QJsonArray measurementsArray = root["measurements"].toArray();
    QVector<Measurement> measurements;

    for (const QJsonValue &value : measurementsArray) {
        measurements.append(Measurement(value.toObject()));
    }

    return measurements;
}

// Air Quality Index operations
bool JsonBaseManager::saveAirQualityIndex(const AirQualityIndex &index)
{
    QJsonObject root = loadRootObject();
    QJsonArray indicesArray = root["airQualityIndices"].toArray();

    //sprawdzamy istnienie indeksu
    bool exists = false;
    for (auto &&item : indicesArray) {
        if (item.toObject()["id"].toInt() == index.stationId()) {
            exists = true;
            break;
        }
    }

    if (!exists) {
        QJsonObject indexObj;
        indexObj["id"] = index.stationId();
        indicesArray.append(indexObj);
        root["airQualityIndices"] = indicesArray;
        return saveToFile(root);
    }

    return true;
}

QVector<AirQualityIndex> JsonBaseManager::loadAirQualityIndices() const
{
    QJsonObject root = loadRootObject();
    QJsonArray indicesArray = root["airQualityIndices"].toArray();
    QVector<AirQualityIndex> indices;

    for (const QJsonValue &value : indicesArray) {
        indices.append(AirQualityIndex(value.toObject()));
    }

    return indices;
}

QString JsonBaseManager::getDatabasePath() const
{
    return m_jsonFilePath;
}

QJsonObject JsonBaseManager::loadRootObject() const
{
    QFile file(m_jsonFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Nie udało się otworzyć pliku do odczytu:" << file.errorString();
        return QJsonObject();
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (doc.isNull()) {
        qWarning() << "Nie udało się przeanalizować pliku JSON";
        return QJsonObject();
    }

    return doc.object();
}

bool JsonBaseManager::saveToFile(const QJsonObject &rootObject) const
{
    QFile file(m_jsonFilePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Nie udało się otworzyć pliku do zapisu:" << file.errorString();
        return false;
    }

    QJsonDocument doc(rootObject);
    if (file.write(doc.toJson()) == -1) {
        qWarning() << "Nie udało się zapisać do pliku:" << file.errorString();
        file.close();
        return false;
    }

    file.close();
    return true;
}

bool JsonBaseManager::initializeDataFile()
{
    QJsonObject root;
    root["stations"] = QJsonArray();
    root["sensors"] = QJsonArray();
    root["measurements"] = QJsonArray();
    root["airQualityIndices"] = QJsonArray();

    return saveToFile(root);
}
