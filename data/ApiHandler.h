/**
 * @file apihandler.h
 * @brief Plik nagłówkowy zawierający definicję klasy ApiHandler
 *
 * Klasa odpowiedzialna za komunikację z API jakości powietrza i zarządzanie danymi stacji
 */

#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include "Station.h"
#include "DatabaseManager.h"
#include "Sensor.h"
#include "Measurement.h"
#include "AirQualityIndex.h"
#include <QMutex>

/**
 * @class ApiHandler
 * @brief Klasa zarządzająca komunikacją z API jakości powietrza
 *
 * Klasa obsługuje pobieranie danych ze stacji pomiarowych, czujników, pomiarów
 * oraz wskaźników jakości powietrza. Wykorzystuje wielowątkowość do efektywnego
 * zarządzania żądaniami sieciowymi.
 */
class ApiHandler : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Konstruktor klasy ApiHandler
     * @param parent Wskaźnik na obiekt rodzica (domyślnie nullptr)
     */
    explicit ApiHandler(QObject *parent = nullptr);

    // Główne metody API

    /**
     * @brief Pobiera listę wszystkich stacji pomiarowych
     */
    void fetchStations();

    /**
     * @brief Pobiera listę czujników dla określonej stacji
     * @param stationId ID stacji
     */
    void fetchSensors(int stationId);

    /**
     * @brief Pobiera pomiary z określonego czujnika
     * @param sensorId ID czujnika
     * @param from Data początkowa zakresu (opcjonalna)
     * @param to Data końcowa zakresu (opcjonalna)
     */
    void fetchMeasurements(int sensorId,
                           const QDateTime& from = QDateTime(),
                           const QDateTime& to = QDateTime());

    /**
     * @brief Pobiera wskaźnik jakości powietrza dla stacji
     * @param stationId ID stacji
     */
    void fetchAirQualityIndex(int stationId);

    // Metody pomocnicze

    /**
     * @brief Ustawia bazowy URL API
     * @param url Nowy URL
     */
    void setApiUrl(const QString &url);

    /**
     * @brief Sprawdza, czy handler jest zajęty przetwarzaniem żądania
     * @return true jeśli handler jest zajęty, false w przeciwnym przypadku
     */
    bool isBusy() const;

    /**
     * @brief Filtruje stacje według miasta
     * @param city Nazwa miasta
     */
    void filterStationsByCity(const QString& city);

    /**
     * @brief Znajduje stacje w określonym promieniu od współrzędnych
     * @param lat Szerokość geograficzna
     * @param lon Długość geograficzna
     * @param radiusKm Promień w kilometrach
     */
    void findStationsInRadius(double lat, double lon, double radiusKm);

    /**
     * @brief Znajduje stacje w pobliżu adresu
     * @param address Adres do geokodowania
     * @param radiusKm Promień w kilometrach
     */
    void findStationsByAddress(const QString& address, double radiusKm);

    /**
     * @brief Zwraca wskaźnik do menedżera bazy danych
     * @return Wskaźnik do DatabaseManager
     */
    DatabaseManager* databaseManager() const { return m_dbManager; }

    /**
     * @brief Aktualizuje listę stacji
     * @param stations Nowa lista stacji
     */
    void updateStations(const QVector<Station>& stations);

    /**
     * @brief Pobiera wszystkie stacje
     * @return Wektor zawierający wszystkie stacje
     */
    QVector<Station> getAllStations() const;

public slots:
    /**
     * @brief Slot znajdujący stacje w pobliżu adresu
     * @param address Adres do geokodowania
     * @param radiusKm Promień w kilometrach
     */
    void findStationsNearAddress(const QString& address, double radiusKm);

signals:
    // Sygnały z wynikami

    /**
     * @brief Sygnał emitowany po pobraniu stacji
     * @param stations Lista stacji
     */
    void stationsFetched(const QVector<Station> &stations);

    /**
     * @brief Sygnał emitowany po pobraniu czujników
     * @param sensors Lista czujników
     */
    void sensorsFetched(const QVector<Sensor> &sensors);

    /**
     * @brief Sygnał emitowany po pobraniu pomiarów
     * @param measurement Dane pomiarowe
     */
    void measurementsFetched(const Measurement &measurement);

    /**
     * @brief Sygnał emitowany po pobraniu wskaźnika jakości powietrza
     * @param index Wskaźnik jakości powietrza
     */
    void airQualityIndexFetched(const AirQualityIndex &index);

    /**
     * @brief Sygnał emitowany po przefiltrowaniu stacji
     * @param stations Przefiltrowana lista stacji
     */
    void stationsFiltered(const QVector<Station>& stations);

    /**
     * @brief Sygnał emitowany po zakończeniu geokodowania
     * @param latitude Szerokość geograficzna
     * @param longitude Długość geograficzna
     */
    void geocodingFinished(double latitude, double longitude);

    // Sygnały błędów

    /**
     * @brief Sygnał emitowany w przypadku błędu sieci
     * @param message Komunikat błędu
     */
    void networkError(const QString &message);

    /**
     * @brief Sygnał emitowany w przypadku błędu API
     * @param errorCode Kod błędu
     */
    void apiError(const QString &errorCode);

    /**
     * @brief Sygnał emitowany w przypadku błędu geokodowania
     * @param message Komunikat błędu
     */
    void geocodingError(const QString& message);

private slots:
    /**
     * @brief Slot obsługujący odpowiedź z listą stacji
     */
    void handleStationsReply();

    /**
     * @brief Slot obsługujący odpowiedź z listą czujników
     */
    void handleSensorsReply();

    /**
     * @brief Slot obsługujący odpowiedź z pomiarami
     */
    void handleMeasurementsReply();

    /**
     * @brief Slot obsługujący odpowiedź ze wskaźnikiem jakości powietrza
     */
    void handleAirQualityIndexReply();

    /**
     * @brief Slot obsługujący odpowiedź geokodowania
     */
    void handleGeocodingReply();

    /**
     * @brief Slot obsługujący błędy sieciowe
     * @param code Kod błędu
     */
    void handleNetworkError(QNetworkReply::NetworkError code);

private:
    QNetworkAccessManager m_manager;                 /**< Menedżer połączeń sieciowych */
    QString m_apiBaseUrl = "https://api.gios.gov.pl/pjp-api/rest"; /**< Bazowy URL API */
    bool m_isBusy = false;                          /**< Flaga wskazująca czy trwa przetwarzanie żądania */
    QVector<Station> m_allStations;                 /**< Cache wszystkich stacji */
    QNetworkAccessManager m_geocoderManager;        /**< Menedżer połączeń dla geokodowania */
    DatabaseManager *m_dbManager;                   /**< Wskaźnik do menedżera bazy danych */
    QThreadPool m_threadPool;                       /**< Pula wątków dla operacji asynchronicznych */
    mutable QMutex m_dataMutex;                     /**< Mutex do synchronizacji dostępu do danych */

    // Metody prywatne

    /**
     * @brief Tworzy URL na podstawie endpointu
     * @param endpoint Ścieżka endpointu
     * @return Pełny URL
     */
    QUrl buildUrl(const QString &endpoint) const;

    /**
     * @brief Tworzy obiekt żądania sieciowego
     * @param url URL żądania
     * @return Obiekt QNetworkRequest
     */
    QNetworkRequest createRequest(const QUrl &url) const;

    /**
     * @brief Obsługuje błędy sieciowe
     * @param reply Obiekt odpowiedzi
     */
    void handleError(QNetworkReply *reply);

    /**
     * @brief Parsuje odpowiedź w formacie JSON array
     * @param reply Obiekt odpowiedzi
     * @return Tablica JSON
     */
    QJsonArray parseJsonArrayReply(QNetworkReply *reply);

    /**
     * @brief Parsuje odpowiedź w formacie JSON object
     * @param reply Obiekt odpowiedzi
     * @return Obiekt JSON
     */
    QJsonObject parseJsonObjectReply(QNetworkReply *reply);

    /**
     * @brief Wykonuje geokodowanie adresu
     * @param address Adres do geokodowania
     */
    void performGeocoding(const QString& address);

    /**
     * @brief Implementacja obsługi odpowiedzi z stacjami
     * @param reply Obiekt odpowiedzi
     */
    void handleStationsReplyImpl(QNetworkReply* reply);

    /**
     * @brief Implementacja obsługi odpowiedzi z czujnikami
     * @param reply Obiekt odpowiedzi
     */
    void handleSensorsReplyImpl(QNetworkReply* reply);

    /**
     * @brief Implementacja obsługi odpowiedzi z pomiarami
     * @param reply Obiekt odpowiedzi
     * @param sensorId ID czujnika
     * @param from Data początkowa
     * @param to Data końcowa
     */
    void handleMeasurementsReplyImpl(QNetworkReply* reply, int sensorId, const QDateTime& from, const QDateTime& to);

    /**
     * @brief Sprawdza dostępność internetu
     * @return true jeśli internet jest dostępny, false w przeciwnym przypadku
     */
    bool isInternetAvailable() const;
};
