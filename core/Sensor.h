#pragma once
#include <QString>
#include <QJsonObject>

/**
 * @file sensor.h
 * @brief Definicja klasy Sensor reprezentującej czujnik pomiarowy
 */

/**
 * @class Sensor
 * @brief Klasa reprezentująca pojedynczy czujnik pomiarowy na stacji
 *
 * Przechowuje informacje o parametrze mierzonym przez czujnik
 * oraz jego powiązaniu ze stacją pomiarową.
 * @ingroup DataModels
 */
class Sensor {
public:
    /**
     * @struct Param
     * @brief Struktura opisująca mierzony parametr
     */
    struct Param {
        QString name;    ///< Pełna nazwa parametru (np. "dwutlenek siarki")
        QString formula; ///< Formuła chemiczna (np. "SO2")
        QString code;    ///< Kod parametru w systemie GIOS (np. "SO2")
        int id;         ///< ID parametru w systemie GIOS
    };

    /**
     * @brief Konstruktor tworzący czujnik z danych JSON
     * @param json Obiekt JSON z API GIOS zawierający dane czujnika
     * @throws std::invalid_argument Jeśli brak wymaganych pól w JSON
     */
    Sensor(const QJsonObject &json);

    /// @name Podstawowe gettery
    /// @{
    int id() const { return m_id; }          ///< Zwraca ID czujnika
    int stationId() const { return m_stationId; } ///< Zwraca ID stacji macierzystej
    Param parameter() const { return m_param; } ///< Zwraca pełne dane parametru
    /// @}

    /**
     * @brief Generuje tekstową reprezentację czujnika
     * @return String w formacie "Czujnik [ID] (Parametr: [nazwa])"
     */
    QString toString() const;

    /// @name Gettery dla danych JSON
    /// @{
    QString paramName() const; ///< Zwraca nazwę parametru (alias dla parameter().name)
    QString paramCode() const; ///< Zwraca kod parametru (alias dla parameter().code)
    /// @}

private:
    int m_id;           ///< Unikalny identyfikator czujnika
    int m_stationId;    ///< ID stacji do której należy czujnik
    Param m_param;      ///< Dane mierzonego parametru

    // Pola pomocnicze dla kompatybilności JSON
    QString m_paramName; ///< Kopia nazwy parametru (dla uproszczenia dostępu)
    QString m_paramCode; ///< Kopia kodu parametru (dla uproszczenia dostępu)
};
