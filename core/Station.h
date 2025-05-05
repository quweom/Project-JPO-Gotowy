#pragma once
#include <QString>
#include <QJsonObject>

/**
* @file station.h
* @brief Definicja klasy Station reprezentującej stację monitoringu jakości powietrza
*/

/**
 * @class Station
 * @brief Klasa reprezentująca stację pomiarową jakości powietrza
 *
 * Przechowuje informacje o lokalizacji stacji, danych adresowych
 * oraz udostępnia metody do obliczeń geograficznych i formatowania danych.
 * @ingroup DataModels
 */

class Station {
public:

    /**
     * @struct Address
     * @brief Struktura przechowująca dane adresowe stacji
     */
    struct Address {
        int cityId;          ///< ID miasta w systemie GIOS
        QString cityName;    ///< Nazwa miasta
        QString communeName; ///< Nazwa gminy
        QString districtName;///< Nazwa dzielnicy
        QString provinceName;///< Nazwa województwa
        QString streetName;  ///< Nazwa ulicy
    };

    /**
     * @brief Konstruktor domyślny
     */
    Station();

    /**
     * @brief Konstruktor tworzący obiekt na podstawie danych JSON
     * @param json Obiekt JSON z danymi stacji z API GIOS
     * @throws std::invalid_argument Jeśli brak wymaganych pól w JSON
     */
    Station(const QJsonObject &json);

    /// @name Gettery podstawowych właściwości
    /// @{
    int id() const { return m_id; }                ///< Zwraca ID stacji
    QString name() const { return m_name; }        ///< Zwraca nazwę stacji
    double latitude() const { return m_latitude; } ///< Zwraca szerokość geograficzną
    double longitude() const { return m_longitude; }///< Zwraca długość geograficzną
    Address address() const { return m_address; }  ///< Zwraca pełne dane adresowe
    /// @}

    /**
     * @brief Generuje string z podstawowymi informacjami (do debugowania)
     * @return Sformatowany string z danymi stacji
     */
    QString toString() const;

    /**
     * @brief Sprawdza czy stacja znajduje się w podanym mieście
     * @param city Nazwa miasta do sprawdzenia
     * @return true jeśli stacja jest w podanym mieście (porównanie case-insensitive)
     */
    bool isInCity(const QString& city) const;

    /**
     * @brief Oblicza odległość do podanych współrzędnych
     * @param lat Szerokość geograficzna docelowa
     * @param lon Długość geograficzna docelowa
     * @return Odległość w kilometrach
     * @note Wykorzystuje formułę haversine do obliczeń
     * @see https://en.wikipedia.org/wiki/Haversine_formula
     */
    double distanceTo(double lat, double lon) const;

    /**
     * @brief Generuje krótki opis stacji (do wyświetlania na liście)
     * @return String w formacie "Nazwa (Miasto)"
     */
    QString toShortString() const;

    /**
     * @brief Generuje pełny opis stacji z opcjonalną odległością
     * @param refLat Referencyjna szerokość geograficzna (opcjonalna)
     * @param refLon Referencyjna długość geograficzna (opcjonalna)
     * @return Sformatowany string ze wszystkimi danymi stacji
     * @note Jeśli podano refLat i refLon, zawiera obliczoną odległość
     */
    QString toFullString(double refLat = NAN, double refLon = NAN) const;   //pełne dane

    /// @name Gettery dla nowych funkcjonalności
    /// @{
    QString cityName() const { return m_address.cityName; }///< Zwraca nazwę miasta
    /**
     * @brief Zwraca odległość jako sformatowany string
     * @param lat Szerokość geograficzna
     * @param lon Długość geograficzna
     * @return String w formacie "X.XX km"
     */
    QString distanceStringTo(double lat, double lon) const;
    /// @}

    /// @name Settery dla operacji JSON
    /// @{
    void setId(int id);                     ///< Ustawia ID stacji
    void setName(const QString &name);      ///< Ustawia nazwę stacji
    void setLatitude(double lat);          ///< Ustawia szerokość geograficzną
    void setLongitude(double lon);         ///< Ustawia długość geograficzną
    void setAddressStreet(const QString &street); ///< Ustawia nazwę ulicy
    void setCityName(const QString &city); ///< Ustawia nazwę miasta
    /// @}

    /**
     * @brief Zwraca nazwę ulicy
     * @return Nazwa ulicy gdzie znajduje się stacja
     */
    QString addressStreet() const;

private:
    // m - member variable
    int m_id;               ///< Unikalny identyfikator stacji
    QString m_name;         ///< Nazwa stacji
    double m_latitude;      ///< Szerokość geograficzna (WGS84)
    double m_longitude;     ///< Długość geograficzna (WGS84)
    Address m_address;      ///< Pełne dane adresowe

    // Pola pomocnicze dla JSON
    QString m_addressStreet; ///< Nazwa ulicy (dla kompatybilności)
    QString m_cityName;     ///< Nazwa miasta (dla kompatybilności)
};
