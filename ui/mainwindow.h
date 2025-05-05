/**
 * @file mainwindow.h
 * @brief Plik nagłówkowy zawierający definicję klasy MainWindow
 *
 * Klasa głównego okna aplikacji monitorującej jakość powietrza
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include "ApiHandler.h"
#include "Station.h"
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QDateTimeAxis>
#include <QtCharts/QValueAxis>
#include <QDateEdit>
#include <QLabel>
#include "jsonbasemanager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

/**
 * @class MainWindow
 * @brief Główne okno aplikacji monitorującej jakość powietrza
 *
 * Klasa odpowiedzialna za interfejs użytkownika, wyświetlanie danych
 * i zarządzanie interakcjami z użytkownikiem.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief Konstruktor klasy MainWindow
     * @param parent Wskaźnik na obiekt rodzica (domyślnie nullptr)
     */
    explicit MainWindow(QWidget *parent = nullptr);

    /**
     * @brief Destruktor klasy MainWindow
     */
    ~MainWindow();

private slots:
    /**
     * @brief Slot obsługujący pobranie listy stacji
     * @param stations Wektor zawierający listę stacji
     */
    void handleStationsFetched(const QVector<Station>& stations);

    /**
     * @brief Slot obsługujący pobranie listy czujników
     * @param sensors Wektor zawierający listę czujników
     */
    void handleSensorsFetched(const QVector<Sensor>& sensors);

    /**
     * @brief Slot obsługujący pobranie pomiarów
     * @param measurement Obiekt zawierający dane pomiarowe
     */
    void handleMeasurementsFetched(const Measurement& measurement);

    /**
     * @brief Slot obsługujący pobranie wskaźnika jakości powietrza
     * @param index Obiekt zawierający wskaźnik jakości powietrza
     */
    void handleAirQualityFetched(const AirQualityIndex& index);

    /**
     * @brief Slot obsługujący błąd sieciowy
     * @param message Komunikat błędu
     */
    void handleNetworkError(const QString& message);

    /**
     * @brief Slot obsługujący kliknięcie na stację
     * @param item Wskaźnik na element listy stacji
     */
    void handleStationClicked(QListWidgetItem *item);

    /**
     * @brief Slot obsługujący kliknięcie na czujnik
     * @param item Wskaźnik na element listy czujników
     */
    void handleSensorClicked(QListWidgetItem *item);

    /**
     * @brief Slot obsługujący kliknięcie przycisku odświeżania
     */
    void handleRefreshClicked();

    /**
     * @brief Slot obsługujący kliknięcie przycisku filtrowania
     */
    void handleFilterClicked();

    /**
     * @brief Slot obsługujący wyszukiwanie stacji w pobliżu
     */
    void handleSearchNearby();

    /**
     * @brief Slot obsługujący wynik geokodowania
     * @param lat Szerokość geograficzna
     * @param lon Długość geograficzna
     */
    void handleGeocodingResult(double lat, double lon);

    /**
     * @brief Slot obsługujący błąd geokodowania
     * @param message Komunikat błędu
     */
    void handleGeocodingError(const QString& message);

    /**
     * @brief Slot obsługujący zastosowanie zakresu dat
     */
    void handleDateRangeApplied();

    /**
     * @brief Inicjalizacja domyślnych stacji
     */
    void initializeStations();

private:
    Ui::MainWindow *ui;                          /**< Wskaźnik na interfejs użytkownika */
    ApiHandler *m_apiHandler;                    /**< Wskaźnik na obiekt obsługi API */
    QVector<Station> m_allStations;              /**< Lista wszystkich stacji */
    double m_referenceLat = NAN;                 /**< Referencyjna szerokość geograficzna */
    double m_referenceLon = NAN;                 /**< Referencyjna długość geograficzna */
    QChart *m_chart;                             /**< Wskaźnik na obiekt wykresu */
    QChartView *m_chartView;                     /**< Wskaźnik na widok wykresu */
    Measurement* m_currentMeasurement = nullptr; /**< Aktualne pomiary */
    QDateEdit* dateFromEdit;                     /**< Widget edycji daty początkowej */
    QDateEdit* dateToEdit;                       /**< Widget edycji daty końcowej */
    QDateTimeAxis *m_axisX = nullptr;            /**< Oś X wykresu (czas) */
    QValueAxis *m_axisY = nullptr;               /**< Oś Y wykresu (wartości) */
    QLabel* airQualityLabel;                     /**< Etykieta wyświetlająca jakość powietrza */
    bool m_connectionErrorShown = false;         /**< Flaga wskazująca czy wyświetlono błąd połączenia */

    /**
     * @brief Wyświetla listę stacji
     * @param stations Wektor stacji do wyświetlenia
     */
    void displayStations(const QVector<Station>& stations);

    /**
     * @brief Wyświetla listę czujników
     * @param sensors Wektor czujników do wyświetlenia
     */
    void displaySensors(const QVector<Sensor>& sensors);

    /**
     * @brief Wyświetla pomiary
     * @param measurement Obiekt pomiarów do wyświetlenia
     */
    void displayMeasurement(const Measurement& measurement);

    /**
     * @brief Wyświetla wskaźnik jakości powietrza
     * @param index Obiekt wskaźnika jakości powietrza
     */
    void displayAirQuality(const AirQualityIndex& index);

    /**
     * @brief Loguje wiadomość
     * @param message Wiadomość do zalogowania
     */
    void logMessage(const QString& message);

    /**
     * @brief Aktualizuje wykres
     * @param data Dane do wyświetlenia na wykresie
     * @param paramName Nazwa parametru
     */
    void updateChart(const QVector<Measurement::DataPoint>& data, const QString& paramName);

    /**
     * @brief Aktualizuje tabelę z przefiltrowanymi danymi
     * @param data Przefiltrowane dane do wyświetlenia
     */
    void updateTableWithFilteredData(const QVector<Measurement::DataPoint>& data);

    /**
     * @brief Sprawdza połączenie z internetem
     * @return true jeśli jest połączenie, false w przeciwnym przypadku
     */
    bool isOnline() const;

    /**
     * @brief Zdarzenie pokazania okna
     * @param event Zdarzenie pokazania
     */
    void showEvent(QShowEvent *event);

    /**
     * @brief Wyświetla analizę danych
     * @param analysis Wyniki analizy
     */
    void displayAnalysis(const Measurement::AnalysisResult& analysis);

    /**
     * @brief Analizuje przefiltrowane dane
     * @param filteredData Dane do analizy
     * @return Wyniki analizy
     */
    Measurement::AnalysisResult analyzeFilteredData(const QVector<Measurement::DataPoint>& filteredData);

    /**
     * @brief Zwraca wskaźnik na menedżera bazy danych
     * @return Wskaźnik na DatabaseManager
     */
    DatabaseManager* databaseManager() const;

    /**
     * @brief Tworzy obiekt stacji
     * @param id ID stacji
     * @param name Nazwa stacji
     * @param lat Szerokość geograficzna
     * @param lon Długość geograficzna
     * @param street Ulica
     * @param city Miasto
     * @return Obiekt stacji
     */
    Station createStation(int id, const QString& name, double lat, double lon, const QString& street, const QString& city);

};

#endif // MAINWINDOW_H
