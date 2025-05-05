#include "DatabaseManager.h"

DatabaseManager::DatabaseManager(QObject *parent) : QObject(parent) {
    initDatabase();
}

DatabaseManager::~DatabaseManager() {
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool DatabaseManager::initDatabase(const QString &dbPath) {
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(dbPath);

    qDebug() << "Ścieżka do bazy danych:" << m_db.databaseName();

    if (!m_db.open()) {
        qCritical() << "Nie można otworzyć bazy danych:" << m_db.lastError();
        return false;
    }

    return createTables();
}

bool DatabaseManager::createTables() {
    QSqlQuery query;

    //tabela stacji
    query.exec("CREATE TABLE IF NOT EXISTS stations ("
               "id INTEGER PRIMARY KEY,"
               "name TEXT,"
               "latitude REAL,"
               "longitude REAL,"
               "city_id INTEGER,"
               "city_name TEXT,"
               "commune_name TEXT,"
               "district_name TEXT,"
               "province_name TEXT,"
               "street_name TEXT)");

    //tabela czujników
    query.exec("CREATE TABLE IF NOT EXISTS sensors ("
               "id INTEGER PRIMARY KEY,"
               "station_id INTEGER,"
               "param_name TEXT,"
               "param_formula TEXT,"
               "param_code TEXT,"
               "param_id INTEGER,"
               "FOREIGN KEY(station_id) REFERENCES stations(id))");

    //tabela pomiarów
    query.exec("CREATE TABLE IF NOT EXISTS measurements ("
               "sensor_id INTEGER,"
               "timestamp TEXT,"
               "value REAL,"
               "is_valid INTEGER,"
               "FOREIGN KEY(sensor_id) REFERENCES sensors(id))");

    //tabela indeksu jakości powietrza
    query.exec("CREATE TABLE IF NOT EXISTS air_quality ("
               "station_id INTEGER PRIMARY KEY,"
               "calc_date TEXT,"
               "overall_index_id INTEGER,"
               "overall_index_name TEXT,"
               "source_data_date TEXT)");

    return !query.lastError().isValid();
}

void DatabaseManager::saveStation(const Station &station) {
    QSqlQuery query;
    query.prepare("INSERT OR REPLACE INTO stations VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(station.id());
    query.addBindValue(station.name());
    query.addBindValue(station.latitude());
    query.addBindValue(station.longitude());
    query.addBindValue(station.address().cityId);
    query.addBindValue(station.address().cityName);
    query.addBindValue(station.address().communeName);
    query.addBindValue(station.address().districtName);
    query.addBindValue(station.address().provinceName);
    query.addBindValue(station.address().streetName);
    query.exec();
    if (!query.exec()) {
        qDebug() << "Błąd zapisywania stacji:" << query.lastError().text();
    } else {
        qDebug() << "Stancja została zapisana (ID:" << station.id() << "):" << station.name();
    }
}

void DatabaseManager::saveSensor(const Sensor &sensor) {
    QSqlQuery query;
    query.prepare("INSERT OR REPLACE INTO sensors VALUES (?, ?, ?, ?, ?, ?)");
    query.addBindValue(sensor.id());
    query.addBindValue(sensor.stationId());
    query.addBindValue(sensor.parameter().name);
    query.addBindValue(sensor.parameter().formula);
    query.addBindValue(sensor.parameter().code);
    query.addBindValue(sensor.parameter().id);
    query.exec();
}

void DatabaseManager::saveMeasurement(const Measurement &measurement, int sensorId) {
    if (!m_db.isOpen()) {
        qWarning() << "Baza danych nie jest otwarta!";
        return;
    }

    if (!m_db.transaction()) {
        qWarning() << "Nie udało się rozpocząć transakcji:" << m_db.lastError().text();
        return;
    }

    try {
        //przygotowujemy zapytanie jeden raz
        QSqlQuery query;
        query.prepare("INSERT OR REPLACE INTO measurements VALUES (?, ?, ?, ?)");

        //otrzymujemy dane do wstawienia
        const auto& dataPoints = measurement.data();

        //rezerwujemy pamięć, aby przyspieszyć wstawianie
        query.setForwardOnly(true);

        int batchSize = 0;
        const int maxBatchSize = 100; //optymalny rozmiar pakietu

        for (const auto &point : dataPoints) {
            //wiążemy parametry
            query.addBindValue(sensorId);
            query.addBindValue(point.timestamp.toString(Qt::ISODate));
            query.addBindValue(point.value);
            query.addBindValue(point.isValid ? 1 : 0);

            if (!query.exec()) {
                throw std::runtime_error(
                    QString("Nie udało się wstawić pomiaru: %1 (Sensor ID: %2, Time: %3)")
                        .arg(query.lastError().text())
                        .arg(sensorId)
                        .arg(point.timestamp.toString())
                        .toStdString());
            }

            if (++batchSize >= maxBatchSize) {
                m_db.commit();
                if (!m_db.transaction()) {
                    throw std::runtime_error(
                        QString("Nie udało się rozpocząć nowej transakcji: %1")
                            .arg(m_db.lastError().text())
                            .toStdString());
                }
                batchSize = 0;
            }
        }

        //finalizujemy transakcję
        if (!m_db.commit()) {
            throw std::runtime_error(
                QString("Nie udało się zatwierdzić transakcji: %1")
                    .arg(m_db.lastError().text())
                    .toStdString());
        }

        qDebug() << "Pomyślnie zapisane" << dataPoints.size()
                 << "pomiary dla czujnika" << sensorId;

    } catch (const std::exception &e) {
        m_db.rollback();
        qCritical() << "Zapis pomiaru nie powiódł się:" << e.what();
        throw; //rzucamy wyjątek dalej
    }
}

QVector<Station> DatabaseManager::loadStations() {
    QVector<Station> stations;
    QSqlQuery query("SELECT * FROM stations");

    while (query.next()) {
        QJsonObject json;
        json["id"] = query.value("id").toInt();
        json["stationName"] = query.value("name").toString();
        json["gegrLat"] = QString::number(query.value("latitude").toDouble());
        json["gegrLon"] = QString::number(query.value("longitude").toDouble());

        QJsonObject city;
        city["id"] = query.value("city_id").toInt();
        city["name"] = query.value("city_name").toString();
        city["commune"] = QJsonObject{
            {"communeName", query.value("commune_name").toString()},
            {"districtName", query.value("district_name").toString()},
            {"provinceName", query.value("province_name").toString()}
        };
        json["city"] = city;
        json["addressStreet"] = query.value("street_name").toString();

        stations.append(Station(json));
    }

    return stations;
}
