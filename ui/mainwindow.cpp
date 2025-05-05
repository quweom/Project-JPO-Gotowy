#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QMessageBox>
#include <QDateTime>
#include <QHeaderView>
#include <QTableWidget>
#include <QChart>
#include <QTcpSocket>
#include <QNetworkInterface>
#include <QtConcurrent>
#include <QFutureWatcher>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_apiHandler(new ApiHandler(this)),
    m_allStations(),
    m_chart(new QChart()),
    m_chartView(nullptr),
    m_currentMeasurement(nullptr)
{
    ui->setupUi(this);

    QTimer::singleShot(1500, this, [this]() {
        //automatyczna kontrola przy starcie
        if (!isOnline()) {
            handleNetworkError("");
        }

        //próbujemy załadować dane
        m_apiHandler->fetchStations();
    });

    QTimer::singleShot(100, this, &MainWindow::initializeStations);

    if (auto table = findChild<QTableWidget*>("measurementTable")) {
        table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }

    this->setMinimumSize(1000, 600);
    this->resize(1200, 700);

    ui->mainVerticalLayout->setContentsMargins(10, 10, 10, 10);
    ui->horizontalLayout->setContentsMargins(0, 0, 0, 0);
    ui->leftColumn->setContentsMargins(0, 0, 0, 0);
    ui->rightColumn->setContentsMargins(0, 0, 0, 0);
    ui->rightColumn->setStretch(0, 1);
    ui->rightColumn->setStretch(1, 1);

    airQualityLabel = new QLabel(this);
    airQualityLabel->setAlignment(Qt::AlignCenter);
    airQualityLabel->setFont(QFont("Arial", 14, QFont::Bold));
    ui->mainVerticalLayout->addWidget(airQualityLabel);

    dateFromEdit = ui->timeToolBar->findChild<QDateEdit*>("dateFromEdit");
    dateToEdit = ui->timeToolBar->findChild<QDateEdit*>("dateToEdit");

    if (dateFromEdit && dateToEdit) {
        QDate currentDate = QDate::currentDate();

        dateFromEdit->setMaximumDate(currentDate);
        dateToEdit->setMaximumDate(currentDate);

        //dodatkowe ustawienia
        dateFromEdit->setMinimumDate(QDate(2020, 1, 1)); //minimalna dostępna Data
        dateToEdit->setMinimumDate(QDate(2020, 1, 1));

        dateFromEdit->setDate(currentDate); //ustawienie dzisiejszej daty jako domyślnej
        dateToEdit->setDate(currentDate);
    }

    //podlaczenie przycisków
    connect(ui->refreshButton, &QPushButton::clicked, this, &MainWindow::handleRefreshClicked);
    connect(ui->filterButton, &QPushButton::clicked, this, &MainWindow::handleFilterClicked);
    connect(ui->searchNearbyButton, &QPushButton::clicked, this, &MainWindow::handleSearchNearby);
    connect(ui->timeToolBar->findChild<QPushButton*>("applyDateRangeButton"), &QPushButton::clicked, this, &MainWindow::handleDateRangeApplied);

    //podlaczenie list
    connect(ui->stationList, &QListWidget::itemClicked, this, &MainWindow::handleStationClicked);
    connect(ui->sensorList, &QListWidget::itemClicked, this, &MainWindow::handleSensorClicked);

    //podlaczenie sygnalow ApiHandler
    connect(m_apiHandler, &ApiHandler::stationsFetched, this, &MainWindow::handleStationsFetched);
    connect(m_apiHandler, &ApiHandler::sensorsFetched, this, &MainWindow::handleSensorsFetched);
    connect(m_apiHandler, &ApiHandler::measurementsFetched, this, &MainWindow::handleMeasurementsFetched);
    connect(m_apiHandler, &ApiHandler::airQualityIndexFetched, this, &MainWindow::handleAirQualityFetched);
    connect(m_apiHandler, &ApiHandler::networkError, this, &MainWindow::handleNetworkError);
    connect(m_apiHandler, &ApiHandler::stationsFiltered, this, &MainWindow::displayStations);
    connect(m_apiHandler, &ApiHandler::geocodingFinished, this, &MainWindow::handleGeocodingResult);
    connect(m_apiHandler, &ApiHandler::geocodingError, this, &MainWindow::handleGeocodingError);


    //konfiguracja ui
    ui->statusbar->showMessage("System ready", 3000);
    m_apiHandler->fetchStations();

    //inicjalizacja wykresu
    m_chart = new QChart();
    m_chart->setTitle("Air Quality Measurements");
    m_chart->setAnimationOptions(QChart::SeriesAnimations);

    //tworzenie chart view
    m_chartView = new QChartView(m_chart);
    m_chartView->setRenderHint(QPainter::Antialiasing);

    ui->chartsBrowser->setChart(m_chart);
    ui->chartsBrowser->setRenderHint(QPainter::Antialiasing);

    m_chart->setTheme(QChart::ChartThemeLight);
    m_chart->legend()->setVisible(true);
    m_chart->legend()->setAlignment(Qt::AlignBottom);
}

MainWindow::~MainWindow()
{
    delete m_axisX;
    delete m_axisY;
    delete m_chart;  //wyczyszczenie danych pomiarowych
    delete m_currentMeasurement;
    delete ui;
}

void MainWindow::handleRefreshClicked() {
    ui->statusbar->showMessage("Sprawdzanie połączenia...", 2000);

    //zawsze najpierw próbujemy danych lokalnych
    try {
        QVector<Station> localStations = databaseManager()->loadStations();
        if (!localStations.isEmpty()) {
            displayStations(localStations);
            ui->statusbar->showMessage("Dane lokalne załadowane", 3000);
        }
    } catch (...) {}

    //próbujemy zaktualizować przez API, jeśli istnieje połączenie
    if (isOnline()) {
        ui->statusbar->showMessage("Pobieranie aktualnych danych...", 0);
        m_apiHandler->fetchStations();
    } else {
        ui->statusbar->showMessage("Brak połączenia - używane dane lokalne", 5000);
    }
}

void MainWindow::handleFilterClicked()
{
    QString city = ui->cityFilterEdit->text().trimmed();
    if (city.isEmpty()) {
        displayStations(m_allStations);
        return;
    }

    QVector<Station> filtered;
    std::copy_if(m_allStations.begin(), m_allStations.end(), std::back_inserter(filtered),
                 [city](const Station& s) {
                     return s.cityName().contains(city, Qt::CaseInsensitive);
                 });

    displayStations(filtered);
    logMessage(QString("Zastosowano filtr według miasta: %1").arg(city));
}


void MainWindow::handleStationClicked(QListWidgetItem *item)
{
    int stationId = item->data(Qt::UserRole).toInt();
    m_apiHandler->fetchSensors(stationId);
    m_apiHandler->fetchAirQualityIndex(stationId);
    logMessage(QString("Wybrana stacja ID: %1").arg(stationId));
}

void MainWindow::handleSensorClicked(QListWidgetItem *item)
{
    if (!item) return;

    int sensorId = item->data(Qt::UserRole).toInt();
    m_apiHandler->fetchMeasurements(sensorId, QDateTime(), QDateTime());
}


//procesory danych z API
void MainWindow::handleStationsFetched(const QVector<Station>& stations)
{
    qDebug() << "Otrzymane stacje:" << stations.size();
    if (stations.isEmpty()) {
        qDebug() << "Lista stacji jest pusta! Sprawdź API.";
    }
    m_allStations = stations;
    displayStations(stations);
}

void MainWindow::handleSensorsFetched(const QVector<Sensor>& sensors)
{
    displaySensors(sensors);
    logMessage(QString("Otrzymane czujniki: %1").arg(sensors.size()));
}

void MainWindow::handleMeasurementsFetched(const Measurement& measurement) {
    //czyszczenie poprzednich danych
    if (m_currentMeasurement) {
        delete m_currentMeasurement;
    }

    //zapisujemy nowe dane
    m_currentMeasurement = new Measurement(measurement);

    //znajdujemy minimalne i maksymalne daty w otrzymanych danych
    QDateTime minDate, maxDate;
    bool first = true;
    for (const auto& point : measurement.data()) {
        if (first) {
            minDate = maxDate = point.timestamp;
            first = false;
        } else {
            if (point.timestamp < minDate) minDate = point.timestamp;
            if (point.timestamp > maxDate) maxDate = point.timestamp;
        }
    }

    //ustawiamy daty w interfejsie
    if (dateFromEdit && dateToEdit) {
        dateFromEdit->setDateTime(minDate);
        dateToEdit->setDateTime(maxDate);
    }

    //aktualizujemy interfejs z pełnym zakresem danych
    updateChart(measurement.data(), measurement.paramCode());
    displayMeasurement(measurement); //pokazujemy wszystkie dane w tabeli

    //analiza danych
    Measurement::AnalysisResult analysis = m_currentMeasurement->analyzeData();
    displayAnalysis(analysis);

}

void MainWindow::handleAirQualityFetched(const AirQualityIndex& index)
{
    displayAirQuality(index);
    logMessage("Uzyskano wskaźnik jakości powietrza");
}

void MainWindow::handleNetworkError(const QString& message) {
    Q_UNUSED(message);

    if (m_connectionErrorShown) {
        return;
    }
    m_connectionErrorShown = true;

    //opóźniamy wyświetlanie, aby uniknąć konfliktów z innymi wiadomościami
    QTimer::singleShot(100, this, [this]() {
        try {
            QVector<Station> localStations = databaseManager()->loadStations();

            QString displayMessage;
            if (!localStations.isEmpty()) {
                displayMessage = "Brak połączenia z internetem. Wykorzystuję zapisane dane lokalne.";
                displayStations(localStations);
            } else {
                displayMessage = "Brak połączenia z internetem i brak danych lokalnych.";
            }

            ui->statusbar->showMessage(displayMessage, 5000);
            qDebug() << displayMessage;

        } catch (...) {
            qDebug() << "Błąd podczas ładowania danych lokalnych";
        }

        //zresetuje flagę po 5 sekundach
        QTimer::singleShot(5000, this, [this]() {
            m_connectionErrorShown = false;
        });
    });
}

//metody pomocnicze
void MainWindow::displayStations(const QVector<Station>& stations) {
    ui->stationList->clear();

    //sprawdzanie czy mamy współrzędne referencyjne (z geokodowania)
    bool hasReference = !std::isnan(m_referenceLat) && !std::isnan(m_referenceLon);

    for (const auto& station : stations) {
        QString text = station.toShortString();
        if (hasReference) {
            text += " (" + station.distanceStringTo(m_referenceLat, m_referenceLon) + ")";
        }

        QListWidgetItem *item = new QListWidgetItem(text);
        item->setData(Qt::UserRole, station.id());
        ui->stationList->addItem(item);
    }
}

void MainWindow::displaySensors(const QVector<Sensor>& sensors) {
    qDebug() << "Wywołujemy displaySensors(), liczbę czujników: " << sensors.size();

    ui->sensorList->clear();
    for (const auto& sensor : sensors) {
        QListWidgetItem *item = new QListWidgetItem(sensor.toString());
        item->setData(Qt::UserRole, sensor.id());
        ui->sensorList->addItem(item);
    }
}

void MainWindow::displayMeasurement(const Measurement& measurement)
{
    auto table = ui->measurementTable;
    table->clear();
    table->setColumnCount(3);
    table->setHorizontalHeaderLabels({"Czas", "Wartość", "Status"});
    table->setRowCount(measurement.data().size());

    int row = 0;
    for (const auto& point : measurement.data()) {
        //czas
        QTableWidgetItem* timeItem = new QTableWidgetItem(point.timestamp.toString("yyyy-MM-dd HH:mm"));
        timeItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        table->setItem(row, 0, timeItem);

        //wartość
        QTableWidgetItem* valueItem = new QTableWidgetItem();
        if (point.isValid && !std::isnan(point.value)) {
            valueItem->setText(QString::number(point.value, 'f', 2));
            valueItem->setTextAlignment(Qt::AlignCenter);
            table->setItem(row, 1, valueItem);
            table->setItem(row, 2, new QTableWidgetItem("OK"));
        } else {
            valueItem->setText("--");
            valueItem->setTextAlignment(Qt::AlignCenter);
            table->setItem(row, 1, valueItem);
            table->setItem(row, 2, new QTableWidgetItem("Brak danych"));
        }

        //status
        QTableWidgetItem* statusItem = table->item(row, 2);
        statusItem->setTextAlignment(Qt::AlignCenter);

        row++;
    }

    //dostosowanie wyglądu table
    table->resizeColumnsToContents();
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    table->horizontalHeader()->setStretchLastSection(true);
    table->setColumnWidth(0, 150);
}

void MainWindow::displayAirQuality(const AirQualityIndex& index) {

    const QString baseStyle = "padding: 5px; border-radius: 5px;";

    if (!index.isValid()) {
        airQualityLabel->setText("Wskaźnik jakości powietrza: Brak informacji");
        airQualityLabel->setStyleSheet(baseStyle + "background-color: gray; color: white;");
        return;
    }

    QString qualityText = index.overallIndex().name;
    if (qualityText == "Brak indeksu") {
        qualityText = "Brak informacji";
    }

    airQualityLabel->setText("Wskaźnik jakości powietrza: " + qualityText);

    //ustawiamy kolor w zależności od jakości
    QColor bgColor = index.getQualityColor();
    QString textColor = "white";

    airQualityLabel->setStyleSheet(
        baseStyle + QString("background-color: %1; color: %2;")
                        .arg(bgColor.name())
                        .arg(textColor)
        );
}

void MainWindow::logMessage(const QString& message)
{
    if (!ui->logBrowser) {
        qWarning() << "Przeglądarka dziennika nie została zainicjowana!";
        return;
    }

    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    ui->logBrowser->append(QString("[%1] %2").arg(timestamp, message));
    ui->statusbar->showMessage(message, 3000);
}

void MainWindow::handleSearchNearby() {
    QString address = ui->addressInput->text().trimmed();
    double radius = ui->radiusSpinBox->value();

    if (address.isEmpty()) {
        QMessageBox::warning(this, "Błąd", "Proszę wprowadzić adres");
        return;
    }

    ui->statusbar->showMessage(QString("Wyszukiwanie stacji w promieniu %1 km od %2...").arg(radius).arg(address));
    m_apiHandler->findStationsNearAddress(address, radius);
}

void MainWindow::handleGeocodingResult(double lat, double lon) {
    double radius = ui->radiusSpinBox->value();
    m_apiHandler->findStationsInRadius(lat, lon, radius);

    ui->statusbar->showMessage(
        QString("Znaleziono lokalizację: %1, %2. Pokazuję stacje w promieniu %3 km")
            .arg(lat, 0, 'f', 6)
            .arg(lon, 0, 'f', 6)
            .arg(ui->radiusSpinBox->value()),
        5000
        );
}

void MainWindow::handleGeocodingError(const QString& message) {
    QMessageBox::warning(this, "Błąd geokodowania", message);
    ui->statusbar->showMessage("Błąd: " + message, 5000);
}


void MainWindow::handleDateRangeApplied()
{
    QDateEdit* dateFromEdit = ui->timeToolBar->findChild<QDateEdit*>("dateFromEdit");
    QDateEdit* dateToEdit = ui->timeToolBar->findChild<QDateEdit*>("dateToEdit");

    if (!dateFromEdit || !dateToEdit) {
        qWarning() << "Nie znaleziono widżetów danych!";
        return;
    }

    if (dateFromEdit->date() < QDate::currentDate().addDays(-2)) {
        QMessageBox::warning(this, "Błąd", "Możesz wybrać datę nie starszą niż 2 dni od dzisiejszej!");
        dateFromEdit->setDate(QDate::currentDate().addDays(-2)); //autokorekta
        return;
    }

    //otrzymujemy tylko datę (bez godziny)
    QDate fromDate = dateFromEdit->date();
    QDate toDate = dateToEdit->date();


    //tworzymy QDateTime z początkiem dnia (00:00:00) dla daty początkowej
    QDateTime from = QDateTime(fromDate, QTime(0, 0, 0));

    //tworzymy QDateTime z końcem dnia (23:59:59) dla daty końcowej
    QDateTime to = QDateTime(toDate, QTime(23, 59, 59));

    //sprawdzanie poprawności zakresu
    if (from > to) {
        QMessageBox::warning(this, "Błąd", "Data końcowa nie może być wcześniejsza niż data początkowa!");
        return;
    }

    if (m_currentMeasurement) {
        QVector<Measurement::DataPoint> filtered = m_currentMeasurement->filterByDateRange(from, to);
        updateChart(filtered, m_currentMeasurement->paramCode());
        updateTableWithFilteredData(filtered);

        Measurement::AnalysisResult analysis = analyzeFilteredData(filtered);
        displayAnalysis(analysis);
    }
}

void MainWindow::updateTableWithFilteredData(const QVector<Measurement::DataPoint>& data) {
    auto table = ui->measurementTable;
    table->clearContents();
    table->setRowCount(data.size());

    for (int row = 0; row < data.size(); ++row) {
        const auto& point = data[row];
        table->setItem(row, 0, new QTableWidgetItem(
                                   point.timestamp.toString("yyyy-MM-dd HH:mm")));

        QTableWidgetItem* valueItem = new QTableWidgetItem(
            point.isValid ? QString::number(point.value, 'f', 2) : "--");
        valueItem->setTextAlignment(Qt::AlignCenter);
        table->setItem(row, 1, valueItem);

        QTableWidgetItem* statusItem = new QTableWidgetItem(
            point.isValid ? "OK" : "Brak danych");
        statusItem->setTextAlignment(Qt::AlignCenter);
        table->setItem(row, 2, statusItem);
    }
}

void MainWindow::updateChart(const QVector<Measurement::DataPoint>& data, const QString& paramName)
{
    qDebug() << "Aktualizacja wykresu za pomocą" << data.size() << "punktów danych";

    //wyczyszczenie poprzednich danych
    if (m_axisX) {
        m_chart->removeAxis(m_axisX);
        delete m_axisX;
        m_axisX = nullptr;
    }
    if (m_axisY) {
        m_chart->removeAxis(m_axisY);
        delete m_axisY;
        m_axisY = nullptr;
    }
    m_chart->removeAllSeries();

    //sprawdzanie, czy mamy dane
    if (data.isEmpty()) {
        m_chart->setTitle("Brak danych");
        qDebug() << "Brak danych do wyświetlenia";
        return;
    }

    //tworzenie serii
    QLineSeries *series = new QLineSeries();
    series->setName(paramName);
    qDebug() << "Nazwa parametru:" << paramName;

    // min i max daty i wartości
    QDateTime minDate, maxDate;
    double minVal = std::numeric_limits<double>::max();
    double maxVal = std::numeric_limits<double>::lowest();
    bool hasValidData = false;

    for (const auto& point : data) {
        if (point.isValid && !std::isnan(point.value)) {
            qint64 msecs = point.timestamp.toMSecsSinceEpoch();
            series->append(msecs, point.value);
            hasValidData = true;

            // Update ranges
            if (minDate.isNull() || point.timestamp < minDate) {
                minDate = point.timestamp;
            }
            if (maxDate.isNull() || point.timestamp > maxDate) {
                maxDate = point.timestamp;
            }
            if (point.value < minVal) {
                minVal = point.value;
            }
            if (point.value > maxVal) {
                maxVal = point.value;
            }
        }
    }

    if (!hasValidData) {
        m_chart->setTitle("Brak ważnych danych");
        delete series;
        return;
    }

    //dodawanie serii do wykresu
    m_chart->addSeries(series);
    m_chart->setTitle("Chart: " + paramName);

    //konfiguracja osi X (czas)
    m_axisX = new QDateTimeAxis();
    m_axisX->setFormat("dd.MM.yyyy HH:mm");
    m_axisX->setTitleText("Time");
    m_axisX->setRange(minDate, maxDate);

    //konfiguracja osi Y (wartości)
    m_axisY = new QValueAxis();
    m_axisY->setTitleText(paramName);
    double padding = (maxVal - minVal) * 0.1;
    m_axisY->setRange(minVal - padding, maxVal + padding);

    //dodawanie osi
    m_chart->addAxis(m_axisX, Qt::AlignBottom);
    m_chart->addAxis(m_axisY, Qt::AlignLeft);
    series->attachAxis(m_axisX);
    series->attachAxis(m_axisY);

    //konfiguracja legendy
    m_chart->legend()->setVisible(true);
    m_chart->legend()->setAlignment(Qt::AlignBottom);

    //aktualizacja
    ui->chartsBrowser->repaint();
    qDebug() << "Aktualizacja wykresu zakończona. Zakres czasu:" << minDate << "-" << maxDate;
}

DatabaseManager* MainWindow::databaseManager() const {
    return m_apiHandler->databaseManager();
}

bool MainWindow::isOnline() const {
    try {
        //sprawdzanie aktywnych interfejsów
        bool hasActiveInterface = false;
        foreach (const QNetworkInterface& interface, QNetworkInterface::allInterfaces()) {
            if (interface.flags().testFlag(QNetworkInterface::IsUp) &&
                !interface.addressEntries().isEmpty()) {
                hasActiveInterface = true;
                break;
            }
        }
        if (!hasActiveInterface) return false;

        //szybkie sprawdzanie przez dns
        QTcpSocket dnsSocket;
        dnsSocket.connectToHost("8.8.8.8", 53); // google dns
        if (dnsSocket.waitForConnected(1000)) {
            dnsSocket.disconnectFromHost();
            return true;
        }

        //sprawdzanie dostępności HTTP
        QTcpSocket httpSocket;
        httpSocket.connectToHost("www.google.com", 80);
        return httpSocket.waitForConnected(1000);
    } catch (...) {
        return false; // w przypadku błędów uważamy, że nie ma internetu
    }
}

void MainWindow::showEvent(QShowEvent *event) {
    QMainWindow::showEvent(event);

    if (!isOnline()) {
        QMessageBox::information(this, "Informacja",
                                 "Brak połączenia z internetem. Aplikacja użyje danych z pamięci lokalnej.");
        handleRefreshClicked(); //automatycznie pobieramy dane lokalne
    }
}

void MainWindow::displayAnalysis(const Measurement::AnalysisResult& analysis)
{
    QString analysisText = QStringLiteral(
                               "<table border='1' cellpadding='5' width='100%' style='font-size:10pt'>"
                               "<tr><td>Wartość minimalna:</td><td>%1</td><td>%2</td></tr>"
                               "<tr><td>Wartość maksymalna:</td><td>%3</td><td>%4</td></tr>"
                               "<tr><td>Średnia wartość:</td><td colspan='2'>%5</td></tr>"
                               "<tr><td>Trend:</td><td colspan='2' style='color:%6'><b>%7</b></td></tr>"
                               "</table>")
                               .arg(analysis.minValue, 0, 'f', 2)
                               .arg(analysis.minTime.toString("dd.MM.yyyy HH:mm"))
                               .arg(analysis.maxValue, 0, 'f', 2)
                               .arg(analysis.maxTime.toString("dd.MM.yyyy HH:mm"))
                               .arg(analysis.avgValue, 0, 'f', 2)
                               .arg(analysis.trendValue >= 0 ? "red" : "green")
                               .arg(analysis.trend);

    ui->analysisBrowser->setHtml(analysisText);
}

Measurement::AnalysisResult MainWindow::analyzeFilteredData(const QVector<Measurement::DataPoint>& filteredData)
{
    Measurement::AnalysisResult result;
    result.minValue = std::numeric_limits<double>::max();
    result.maxValue = std::numeric_limits<double>::lowest();
    result.avgValue = 0;
    result.trend = "Za mało danych";
    result.trendValue = 0;

    if(filteredData.size() < 2) {
        return result; //niewystarczające dane do analizy trendu
    }

    //obliczenie min, max i średnią
    double sum = 0;
    int validPoints = 0;

    for(const auto& point : filteredData) {
        if(point.isValid && !std::isnan(point.value)) {
            //aktualizacja min i max
            if(point.value < result.minValue) {
                result.minValue = point.value;
                result.minTime = point.timestamp;
            }
            if(point.value > result.maxValue) {
                result.maxValue = point.value;
                result.maxTime = point.timestamp;
            }
            sum += point.value;
            validPoints++;
        }
    }

    //obliczamy średnią
    result.avgValue = validPoints > 0 ? sum / validPoints : NAN;

    //obliczamy trend (regresja liniowa)
    if(validPoints >= 2) {
        double sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0;
        int n = 0;

        for(const auto& point : filteredData) {
            if(point.isValid && !std::isnan(point.value)) {
                double x = point.timestamp.toSecsSinceEpoch() / 3600.0;
                double y = point.value;
                sumX += x;
                sumY += y;
                sumXY += x * y;
                sumX2 += x * x;
                n++;
            }
        }

        if(n > 1) {
            double denominator = n * sumX2 - sumX * sumX;
            if(denominator != 0) {
                double a = (n * sumXY - sumX * sumY) / denominator;
                result.trendValue = a;

                if(abs(a) < 0.01) {
                    result.trend = "<span style='color:black;'>Stabilne</span>";
                }
                else if(a > 0) {
                    result.trend = QString("<span style='color:green;'>Wzrost (%1/h)</span>")
                    .arg(a, 0, 'f', 3);
                }
                else {
                    result.trend = QString("<span style='color:red;'>Spadek (%1/h)</span>")
                    .arg(a, 0, 'f', 3);
                }
            }
        }
    }

    return result;
}

void MainWindow::initializeStations()
{
    JsonBaseManager jsonManager;

    //sprawdzanie czy stacje już istnieją w pliku
    QVector<Station> existingStations = jsonManager.loadStations();

    if (!existingStations.isEmpty()) {
        qDebug() << "Stacje już istnieją w pliku JSON. Liczba:" << existingStations.size();
        return;
    }

    //tworzenie przykładowych stacji
    QVector<Station> defaultStations = {
        createStation(1, "Warszawa-Centrum", 52.2297, 21.0122, "Marszałkowska", "Warszawa"),
        createStation(2, "Kraków-Rynek", 50.0614, 19.9372, "Rynek Główny", "Kraków"),
        createStation(3, "Wrocław-Rynek", 51.11, 17.0383, "Rynek", "Wrocław")
    };

    //zapis wszystkich stacji
    for (const auto& station : defaultStations) {
        if (!jsonManager.saveStation(station)) {
            qWarning() << "Nie udało się zapisać stacji:" << station.name();
        }
    }

    qDebug() << "Zapisano domyślne stacje do pliku:" << jsonManager.getDatabasePath();
}

Station MainWindow::createStation(int id, const QString& name, double lat, double lon,
                                  const QString& street, const QString& city)
{
    Station station;
    station.setId(id);
    station.setName(name);
    station.setLatitude(lat);
    station.setLongitude(lon);
    station.setAddressStreet(street);
    station.setCityName(city);
    return station;
}
