#pragma once
#include <QString>
#include <QDateTime>
#include <QVector>
#include <QJsonObject>

/**
 * @file measurement.h
 * @brief Definicja klasy Measurement reprezentującej serie pomiarowe
 */

/**
 * @class Measurement
 * @brief Klasa reprezentująca serie pomiarów z czujnika
 *
 * Przechowuje historię pomiarów wraz z metadanymi i udostępnia
 * metody do analizy statystycznej oraz filtrowania danych.
 * @ingroup DataModels
 */
class Measurement {
public:
    /**
     * @struct DataPoint
     * @brief Pojedynczy rekord pomiarowy
     */
    struct DataPoint {
        QDateTime timestamp; ///< Data i czas pomiaru
        double value;        ///< Wartość pomiaru (może być NAN dla brakujących danych)
        bool isValid;        ///< Flaga poprawności danych
    };

    /**
     * @brief Konstruktor tworzący serię pomiarów z danych JSON
     * @param json Obiekt JSON z API GIOS zawierający dane pomiarów
     * @throws std::invalid_argument Jeśli brak wymaganych pól w JSON
     */
    Measurement(const QJsonObject &json);

    /// @name Podstawowe gettery
    /// @{
    QString paramCode() const { return m_paramCode; } ///< Zwraca kod parametru (np. "PM10")
    const QVector<DataPoint>& data() const { return m_data; } ///< Zwraca referencję do wszystkich punktów danych
    /// @}

    /// @name Metody pomocnicze
    /// @{
    bool isEmpty() const { return m_data.empty(); } ///< Sprawdza czy brak danych pomiarowych
    QString toString() const; ///< Generuje tekstowy opis serii pomiarów
    /// @}

    /// @name Metody statystyczne
    /// @{
    int validCount() const; ///< Zlicza poprawne pomiary
    double dataCompleteness() const; ///< Oblicza % kompletności danych (0-100)
    double maxValue() const; ///< Zwraca maksymalną wartość pomiaru
    double minValue() const; ///< Zwraca minimalną wartość pomiaru
    double avgValue() const; ///< Oblicza średnią wartość pomiarów
    QDateTime dateOfMaxValue() const; ///< Zwraca datę wystąpienia maksymalnej wartości
    /// @}

    /**
     * @brief Filtruje dane w podanym zakresie czasowym
     * @param from Data początkowa zakresu
     * @param to Data końcowa zakresu
     * @return Wektor punktów pomiarowych z wybranego zakresu
     */
    QVector<DataPoint> filterByDateRange(const QDateTime& from, const QDateTime& to) const;

    /**
     * @struct AnalysisResult
     * @brief Wynik kompleksowej analizy danych pomiarowych
     */
    struct AnalysisResult {
        double minValue;    ///< Minimalna wartość w serii
        double maxValue;    ///< Maksymalna wartość w serii
        double avgValue;    ///< Średnia wartość
        QDateTime minTime;  ///< Czas wystąpienia minimum
        QDateTime maxTime;  ///< Czas wystąpienia maksimum
        QString trend;      ///< Opis trendu (np. "wzrostowy")
        double trendValue;  ///< Współczynnik trendu (ujemny = spadek, dodatni = wzrost)
    };

    /**
     * @brief Przeprowadza kompleksową analizę danych
     * @return Struktura AnalysisResult z wynikami analizy
     * @note Wykorzystuje regresję liniową do określenia trendu
     */
    AnalysisResult analyzeData() const;

    /// @name Identyfikacja
    /// @{
    int sensorId() const; ///< Zwraca ID czujnika źródłowego
    QDateTime timestamp() const; ///< Zwraca czas pierwszego pomiaru w serii
    /// @}

private:
    QString m_paramCode;       ///< Kod parametru pomiarowego (np. "PM2.5")
    QVector<DataPoint> m_data; ///< Kolekcja wszystkich punktów pomiarowych
    int m_sensorId;           ///< ID czujnika z którego pochodzą pomiary
};
