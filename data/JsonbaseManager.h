/**
 * @file jsonbasemanager.h
 * @brief Plik nagłówkowy zawierający definicję klasy JsonBaseManager
 *
 * Klasa odpowiedzialna za zarządzanie danymi w formacie JSON
 */

#ifndef JSONBASEMANAGER_H
#define JSONBASEMANAGER_H

#include <QObject>
#include <QVector>
#include <QString>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QStandardPaths>
#include <QDebug>
#include "Station.h"
#include "Sensor.h"
#include "Measurement.h"
#include "AirQualityIndex.h"

/**
 * @class JsonBaseManager
 * @brief Klasa zarządzająca danymi w formacie JSON
 *
 * Klasa umożliwia zapisywanie i odczytywanie danych stacji, czujników,
 * pomiarów i wskaźników jakości powietrza w pliku JSON.
 */
class JsonBaseManager : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief Konstruktor klasy JsonBaseManager
     * @param parent Wskaźnik na obiekt rodzica (domyślnie nullptr)
     */
    explicit JsonBaseManager(QObject *parent = nullptr);

    // Operacje na stacjach

    /**
     * @brief Zapisuje stację do pliku JSON
     * @param station Obiekt stacji do zapisania
     * @return true jeśli zapis się powiódł, false w przeciwnym przypadku
     */
    bool saveStation(const Station &station);

    /**
     * @brief Wczytuje listę stacji z pliku JSON
     * @return Wektor zawierający wczytane stacje
     */
    QVector<Station> loadStations() const;

    // Operacje na czujnikach

    /**
     * @brief Zapisuje czujnik do pliku JSON
     * @param sensor Obiekt czujnika do zapisania
     * @return true jeśli zapis się powiódł, false w przeciwnym przypadku
     */
    bool saveSensor(const Sensor &sensor);

    /**
     * @brief Wczytuje listę czujników z pliku JSON
     * @return Wektor zawierający wczytane czujniki
     */
    QVector<Sensor> loadSensors() const;

    // Operacje na pomiarach

    /**
     * @brief Zapisuje pomiar do pliku JSON
     * @param measurement Obiekt pomiaru do zapisania
     * @return true jeśli zapis się powiódł, false w przeciwnym przypadku
     */
    bool saveMeasurement(const Measurement &measurement);

    /**
     * @brief Wczytuje listę pomiarów z pliku JSON
     * @return Wektor zawierający wczytane pomiary
     */
    QVector<Measurement> loadMeasurements() const;

    // Operacje na wskaźnikach jakości powietrza

    /**
     * @brief Zapisuje wskaźnik jakości powietrza do pliku JSON
     * @param index Obiekt wskaźnika do zapisania
     * @return true jeśli zapis się powiódł, false w przeciwnym przypadku
     */
    bool saveAirQualityIndex(const AirQualityIndex &index);

    /**
     * @brief Wczytuje listę wskaźników jakości powietrza z pliku JSON
     * @return Wektor zawierający wczytane wskaźniki
     */
    QVector<AirQualityIndex> loadAirQualityIndices() const;

    /**
     * @brief Pobiera ścieżkę do pliku z danymi
     * @return Ścieżka do pliku JSON
     */
    QString getDatabasePath() const;

private:
    QString m_jsonFilePath; /**< Ścieżka do pliku JSON z danymi */

    /**
     * @brief Wczytuje główny obiekt JSON z pliku
     * @return Obiekt JSON z danymi
     */
    QJsonObject loadRootObject() const;

    /**
     * @brief Zapisuje obiekt JSON do pliku
     * @param rootObject Obiekt do zapisania
     * @return true jeśli zapis się powiódł, false w przeciwnym przypadku
     */
    bool saveToFile(const QJsonObject &rootObject) const;

    /**
     * @brief Inicjalizuje plik danych pustą strukturą JSON
     * @return true jeśli inicjalizacja się powiodła, false w przeciwnym przypadku
     */
    bool initializeDataFile();
};

#endif // JSONBASEMANAGER_H
