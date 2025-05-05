/**
 * @file airqualityindex.h
 * @brief Plik nagłówkowy zawierający definicję klasy AirQualityIndex
 *
 * Klasa reprezentuje wskaźnik jakości powietrza wraz z danymi pomiarowymi
 */

#pragma once
#include <QString>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QVector>
#include <QColor>

/**
 * @class AirQualityIndex
 * @brief Klasa reprezentująca wskaźnik jakości powietrza
 *
 * Klasa przechowuje informacje o jakości powietrza, w tym ogólny wskaźnik,
 * dane poszczególnych stacji pomiarowych oraz metadane.
 */
class AirQualityIndex {
public:
    /**
     * @brief Konstruktor klasy
     * @param index Wartość indeksu jakości powietrza (domyślnie 0)
     */
    explicit AirQualityIndex(int index = 0) : m_index(index) {}

    /**
     * @struct IndexLevel
     * @brief Struktura reprezentująca poziom indeksu jakości powietrza
     */
    struct IndexLevel {
        int id;                /**< Identyfikator poziomu (0-5) */
        QString name;          /**< Nazwa poziomu (np. "Bardzo dobry") */
    };

    /**
     * @struct StationData
     * @brief Struktura reprezentująca dane stanowiska pomiarowego
     */
    struct StationData {
        QString paramName;     /**< Nazwa mierzonego parametru (np. PM10) */
        IndexLevel level;      /**< Poziom indeksu dla parametru */
        QDateTime calcDate;    /**< Data obliczenia wartości */
    };

    /**
     * @brief Konstruktor tworzący obiekt na podstawie danych JSON
     * @param json Obiekt JSON zawierający dane o jakości powietrza
     */
    AirQualityIndex(const QJsonObject &json);

    // Funkcje dostępowe

    /**
     * @brief Pobiera identyfikator stacji
     * @return Identyfikator stacji pomiarowej
     */
    int stationId() const { return m_stationId; }

    /**
     * @brief Pobiera datę obliczenia wskaźnika
     * @return Data i czas obliczenia
     */
    QDateTime calculationDate() const { return m_calcDate; }

    /**
     * @brief Pobiera ogólny wskaźnik jakości powietrza
     * @return Struktura IndexLevel zawierająca dane ogólnego wskaźnika
     */
    IndexLevel overallIndex() const { return m_overallIndex; }

    /**
     * @brief Pobiera datę źródłowych danych
     * @return Data i czas źródłowych danych pomiarowych
     */
    QDateTime sourceDataDate() const { return m_sourceDataDate; }

    /**
     * @brief Pobiera odczyty ze stacji pomiarowych
     * @return Wektor zawierający dane ze stacji pomiarowych
     */
    const QVector<StationData>& stationReadings() const { return m_stationReadings; }

    /**
     * @brief Pobiera kolor odpowiadający jakości powietrza
     * @return Kolor reprezentujący aktualny poziom jakości powietrza
     */
    QColor getQualityColor() const;

    // Metody pomocnicze

    /**
     * @brief Konwertuje dane do postaci tekstowej
     * @return Tekstowa reprezentacja jakości powietrza
     */
    QString toString() const;

    /**
     * @brief Sprawdza poprawność danych
     * @return true jeśli dane są poprawne, false w przeciwnym przypadku
     */
    bool isValid() const;

    /**
     * @brief Pobiera datę obliczenia (alias dla calculationDate())
     * @return Data i czas obliczenia
     */
    QDateTime calcDate() const;

private:
    int m_index;                /**< Wartość indeksu jakości powietrza */
    int m_stationId;            /**< Identyfikator stacji pomiarowej */
    QDateTime m_calcDate;       /**< Data i czas obliczenia wskaźnika */
    IndexLevel m_overallIndex;  /**< Ogólny wskaźnik jakości powietrza */
    QDateTime m_sourceDataDate; /**< Data i czas źródłowych danych pomiarowych */
    QVector<StationData> m_stationReadings; /**< Dane ze stacji pomiarowych */
};
