/**
 * @file databasemanager.h
 * @brief Plik nagłówkowy zawierający definicję klasy DatabaseManager
 *
 * Klasa odpowiedzialna za zarządzanie bazą danych SQLite przechowującą dane o jakości powietrza
 */

#pragma once
#include <QObject>
#include <QtSql>
#include <QSqlDatabase>
#include "Station.h"
#include "Sensor.h"
#include "Measurement.h"
#include "AirQualityIndex.h"

/**
 * @class DatabaseManager
 * @brief Klasa zarządzająca bazą danych SQLite
 *
 * Klasa zapewnia interfejs do przechowywania i odczytywania danych:
 * - stacji pomiarowych
 * - czujników
 * - pomiarów
 * - wskaźników jakości powietrza
 */
class DatabaseManager : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Konstruktor klasy DatabaseManager
     * @param parent Wskaźnik na obiekt rodzica (domyślnie nullptr)
     */
    explicit DatabaseManager(QObject *parent = nullptr);

    /**
     * @brief Destruktor klasy DatabaseManager
     *
     * Zamyka połączenie z bazą danych jeśli jest otwarte
     */
    ~DatabaseManager();

    /**
     * @brief Inicjalizuje połączenie z bazą danych
     * @param dbPath Ścieżka do pliku bazy danych (domyślnie "air_quality.db")
     * @return true jeśli inicjalizacja się powiodła, false w przeciwnym przypadku
     */
    bool initDatabase(const QString &dbPath = "air_quality.db");

    /**
     * @brief Zapisuje stację do bazy danych
     * @param station Obiekt stacji do zapisania
     */
    void saveStation(const Station &station);

    /**
     * @brief Zapisuje czujnik do bazy danych
     * @param sensor Obiekt czujnika do zapisania
     */
    void saveSensor(const Sensor &sensor);

    /**
     * @brief Zapisuje pomiary do bazy danych
     * @param measurement Obiekt pomiarów do zapisania
     * @param sensorId ID czujnika powiązanego z pomiarami
     */
    void saveMeasurement(const Measurement &measurement, int sensorId);

    /**
     * @brief Zapisuje wskaźnik jakości powietrza do bazy danych
     * @param index Obiekt wskaźnika jakości powietrza do zapisania
     */
    void saveAirQualityIndex(const AirQualityIndex &index);

    /**
     * @brief Wczytuje listę stacji z bazy danych
     * @return Wektor zawierający wczytane stacje
     */
    QVector<Station> loadStations();

    /**
     * @brief Wczytuje listę czujników dla określonej stacji
     * @param stationId ID stacji
     * @return Wektor zawierający wczytane czujniki
     */
    QVector<Sensor> loadSensors(int stationId);

    /**
     * @brief Wczytuje pomiary dla określonego czujnika w podanym zakresie czasowym
     * @param sensorId ID czujnika
     * @param from Data początkowa zakresu
     * @param to Data końcowa zakresu
     * @return Wektor zawierający wczytane punkty pomiarowe
     */
    QVector<Measurement::DataPoint> loadMeasurements(int sensorId, const QDateTime &from, const QDateTime &to);

    /**
     * @brief Wczytuje wskaźnik jakości powietrza dla określonej stacji
     * @param stationId ID stacji
     * @return Obiekt wskaźnika jakości powietrza
     */
    AirQualityIndex loadAirQualityIndex(int stationId);

private:
    QSqlDatabase m_db; /**< Obiekt bazy danych SQLite */

    /**
     * @brief Tworzy tabele w bazie danych jeśli nie istnieją
     * @return true jeśli operacja się powiodła, false w przeciwnym przypadku
     */
    bool createTables();
};
