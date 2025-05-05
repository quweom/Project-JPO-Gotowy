#include "ApiHandler.h"
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <QSslConfiguration>
#include <QUrlQuery>
#include <QtConcurrent>


ApiHandler::ApiHandler(QObject *parent) : QObject(parent),
    m_apiBaseUrl("https://api.gios.gov.pl/pjp-api/rest"),
    m_isBusy(false),
    m_dbManager(new DatabaseManager(this))
{
    //weryfikacja obsługi SSL
    if (!QSslSocket::supportsSsl()) {
        qCritical() << "SSL nie jest obsługiwany!";
        qDebug() << "Wersja kompilacji biblioteki SSL:" << QSslSocket::sslLibraryBuildVersionString();
    }
    m_manager.setTransferTimeout(10000);
    m_threadPool.setMaxThreadCount(4);
}

//podstawowe metody API
//wielowątkowość
void ApiHandler::fetchStations() {
    if (m_isBusy) return;

    m_isBusy = true;

    QFuture<void> future = QtConcurrent::run(&m_threadPool, [this]() {
        QUrl url = buildUrl("station/findAll");
        QNetworkRequest request = createRequest(url);

        QNetworkReply *reply = m_manager.get(request);

        QEventLoop loop;
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        handleStationsReplyImpl(reply);
        reply->deleteLater();
    });

    QFutureWatcher<void>* watcher = new QFutureWatcher<void>(this);
    connect(watcher, &QFutureWatcher<void>::finished, watcher, &QFutureWatcher<void>::deleteLater);
    watcher->setFuture(future);
}

//wielowątkowość dod metoda
void ApiHandler::handleStationsReplyImpl(QNetworkReply* reply) {
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseError);

    if (parseError.error != QJsonParseError::NoError || !doc.isArray()) {
        emit apiError("Błąd danych");
        return;
    }

    QVector<Station> stations;
    foreach (const QJsonValue &value, doc.array()) {
        if (value.isObject()) {
            stations.append(Station(value.toObject()));
        }
    }

    updateStations(stations);

    QMetaObject::invokeMethod(this, [this, stations]() {
        m_isBusy = false;
        emit stationsFetched(stations);
    }, Qt::QueuedConnection);
}

//wielowątkowość
void ApiHandler::fetchSensors(int stationId) {
    m_threadPool.start([this, stationId]() {
        QUrl url = buildUrl(QString("station/sensors/%1").arg(stationId));
        QNetworkRequest request = createRequest(url);

        QNetworkReply *reply = m_manager.get(request);

        QEventLoop loop;
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        handleSensorsReplyImpl(reply);
        reply->deleteLater();
    });
}

//wielowątkowość dod
void ApiHandler::handleSensorsReplyImpl(QNetworkReply* reply) {
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseError);

    if (parseError.error != QJsonParseError::NoError || !doc.isArray()) {
        emit apiError("Błąd danych sensorów");
        return;
    }

    QVector<Sensor> sensors;
    foreach (const QJsonValue &value, doc.array()) {
        if (value.isObject()) {
            try {
                sensors.append(Sensor(value.toObject()));
            } catch (...) {
            }
        }
    }

    QMetaObject::invokeMethod(this, [this, sensors]() {
        emit sensorsFetched(sensors);
    }, Qt::QueuedConnection);
}

//wielowątkowość
void ApiHandler::fetchMeasurements(int sensorId, const QDateTime& from, const QDateTime& to) {
    m_threadPool.start([this, sensorId, from, to]() {
        QString endpoint = QString("data/getData/%1").arg(sensorId);
        QUrl url = buildUrl(endpoint);

        QUrlQuery query;
        if (from.isValid()) query.addQueryItem("from", from.toString(Qt::ISODate));
        if (to.isValid()) query.addQueryItem("to", to.toString(Qt::ISODate));
        if (!query.isEmpty()) url.setQuery(query);

        QNetworkRequest request = createRequest(url);
        QNetworkReply *reply = m_manager.get(request);

        QEventLoop loop;
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        handleMeasurementsReplyImpl(reply, sensorId, from, to);
        reply->deleteLater();
    });
}

//wielowątkowość dod
void ApiHandler::handleMeasurementsReplyImpl(QNetworkReply* reply, int sensorId, const QDateTime& from, const QDateTime& to) {
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseError);

    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        emit apiError("Błąd danych pomiarów");
        return;
    }

    try {
        Measurement measurement(doc.object());

        QMetaObject::invokeMethod(this, [this, measurement]() {
            emit measurementsFetched(measurement);
        }, Qt::QueuedConnection);
    } catch (...) {
        emit apiError("Błąd przetwarzania pomiarów");
    }
}


void ApiHandler::fetchAirQualityIndex(int stationId) {
    QUrl url = buildUrl(QString("aqindex/getIndex/%1").arg(stationId));
    QNetworkRequest request = createRequest(url);

    QNetworkReply *reply = m_manager.get(request);
    connect(reply, &QNetworkReply::finished, this, &ApiHandler::handleAirQualityIndexReply);
    connect(reply, &QNetworkReply::errorOccurred, this, &ApiHandler::handleNetworkError);
}

//procedury obsługi odpowiedzi
void ApiHandler::handleStationsReply() {
    QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> reply(qobject_cast<QNetworkReply*>(sender()));
    m_isBusy = false;

    if (!reply || reply->error() != QNetworkReply::NoError) {
        emit networkError(reply ? reply->errorString() : "Nieprawidłowy obiekt odpowiedzi");
        return;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        emit apiError(QString("Błąd analizy JSON: %1").arg(parseError.errorString()));
        return;
    }

    if (!doc.isArray()) {
        emit apiError("Nieprawidłowy format odpowiedzi: oczekiwana tablica JSON");
        return;
    }

    QVector<Station> stations;
    foreach (const QJsonValue &value, doc.array()) {
        if (value.isObject()) {
            stations.append(Station(value.toObject()));
        }
    }

    if (stations.isEmpty()) {
        qDebug() << "Otrzymano listę pustych stacji";
    }

    updateStations(stations);

    if (!stations.isEmpty()) {
        for (const auto &station : stations) {
            m_dbManager->saveStation(station);
        }
    }
    emit stationsFetched(stations);

    qDebug() << "Odebrano odpowiedź API!";

}

void ApiHandler::handleSensorsReply() {
    QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> reply(qobject_cast<QNetworkReply*>(sender()));

    if (!reply || reply->error() != QNetworkReply::NoError) {
        emit networkError(reply ? reply->errorString() : "Nieprawidłowy obiekt odpowiedzi");
        return;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        emit apiError(QString("Błąd analizy JSON: %1").arg(parseError.errorString()));
        return;
    }

    if (!doc.isArray()) {
        emit apiError("Nieprawidłowy format odpowiedzi: oczekiwana tablica JSON");
        return;
    }

    QVector<Sensor> sensors;
    foreach (const QJsonValue &value, doc.array()) {
        if (value.isObject()) {
            try {
                sensors.append(Sensor(value.toObject()));
            } catch (const std::exception &e) {
                qWarning() << "Nie udało się utworzyć czujnika:" << e.what();
            }
        }
    }

    emit sensorsFetched(sensors);
}

void ApiHandler::handleMeasurementsReply() {
    QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> reply(qobject_cast<QNetworkReply*>(sender()));

    if (!reply || reply->error() != QNetworkReply::NoError) {
        emit networkError(reply ? reply->errorString() : "Nieprawidłowy obiekt odpowiedzi");
        return;
    }

    //get the json data from the reply
    QByteArray responseData = reply->readAll();
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        emit apiError(QString("JSON parse error: %1").arg(parseError.errorString()));
        return;
    }

    if (!doc.isObject()) {
        emit apiError("Nieprawidłowy format odpowiedzi: oczekiwany obiekt JSON");
        return;
    }

    // pobranie przekazanych właściwości
    int sensorId = reply->property("sensorId").toInt();
    QDateTime requestedFrom = reply->property("requestedFrom").toDateTime();
    QDateTime requestedTo = reply->property("requestedTo").toDateTime();

    try {
        Measurement measurement(doc.object());

        // dodatkowe info o przedziale czasowym
        if (!measurement.data().isEmpty()) {
            QDateTime actualFrom = measurement.data().first().timestamp;
            QDateTime actualTo = measurement.data().last().timestamp;

            qDebug() << "Są dane dla czujnika" << sensorId
                     << "| Żądany zakres:" << requestedFrom << "-" << requestedTo
                     << "| Rzeczywisty zakres:" << actualFrom << "-" << actualTo;
        }

        emit measurementsFetched(measurement);
    }
    catch (const std::exception& e) {
        emit apiError(QString("Błąd przetwarzania pomiaru: %1").arg(e.what()));
    }
    catch (...) {
        emit apiError("Nieznany błąd przetwarzania pomiarów");
    }
}

void ApiHandler::handleAirQualityIndexReply() {
    QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> reply(qobject_cast<QNetworkReply*>(sender()));

    if (!reply || reply->error() != QNetworkReply::NoError) {
        emit networkError(reply ? reply->errorString() : "Nieprawidłowy obiekt odpowiedzi");
        return;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        emit apiError(QString("Błąd analizy JSON: %1").arg(parseError.errorString()));
        return;
    }

    if (!doc.isObject()) {
        emit apiError("Nieprawidłowy format odpowiedzi: oczekiwany obiekt JSON");
        return;
    }

    try {
        AirQualityIndex index(doc.object());
        emit airQualityIndexFetched(index);
    } catch (const std::exception& e) {
        emit apiError(QString("Nie udało się przeanalizować indeksu jakości powietrza: %1").arg(e.what()));
        qCritical() << "Nieudane dane JSON:" << doc.toJson(QJsonDocument::Indented);
    }
}

//metody filtracji
//wątki
void ApiHandler::filterStationsByCity(const QString& city) {
    QFutureWatcher<Station>* watcher = new QFutureWatcher<Station>(this);

    connect(watcher, &QFutureWatcher<Station>::finished, this, [=]() {
        QVector<Station> filtered = watcher->future().results();
        emit stationsFiltered(filtered);
        watcher->deleteLater();  //sprzątanie
    });

    QFuture<Station> future = QtConcurrent::filtered(m_allStations,
                                                     [city](const Station& s) { return s.isInCity(city); });

    watcher->setFuture(future);
}

void ApiHandler::findStationsInRadius(double lat, double lon, double radiusKm) {
    //otrzymujemy bezpieczną kopię danych
    QVector<Station> stationsCopy = getAllStations();

    QVector<Station> result;
    std::copy_if(stationsCopy.begin(), stationsCopy.end(), std::back_inserter(result),
                 [lat, lon, radiusKm](const Station& s) {
                     return s.distanceTo(lat, lon) <= radiusKm;
                 });

    emit stationsFiltered(result);
}

//metody pomocnicze
QUrl ApiHandler::buildUrl(const QString &endpoint) const {
    return QUrl(m_apiBaseUrl + "/" + endpoint);
}

QNetworkRequest ApiHandler::createRequest(const QUrl &url) const {
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");

    QSslConfiguration sslConfig = request.sslConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyPeer);
    sslConfig.setProtocol(QSsl::TlsV1_2OrLater);
    request.setSslConfiguration(sslConfig);

    return request;
}

void ApiHandler::handleNetworkError(QNetworkReply::NetworkError code) {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    //ignor wszystkich błędów oprócz najważniejszych
    if (code != QNetworkReply::HostNotFoundError &&
        code != QNetworkReply::ConnectionRefusedError) {
        reply->deleteLater();
        return;
    }

    //wysyłamy jedną standardową wiadomość
    emit networkError("connection_error");
    reply->deleteLater();
    m_isBusy = false;
}

void ApiHandler::setApiUrl(const QString &url) {
    if (m_apiBaseUrl != url) {
        m_apiBaseUrl = url;
        qDebug() << "URL bazy API zmieniono na:" << m_apiBaseUrl;
    }
}

void ApiHandler::handleGeocodingReply() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        qWarning() << "Otrzymano nieprawidłową odpowiedź na geokodowanie";
        return;
    }

    QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> replyPtr(reply);

    if (reply->error() != QNetworkReply::NoError) {
        emit geocodingError(reply->errorString());
        return;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        emit geocodingError("Błąd analizy JSON: " + parseError.errorString());
        return;
    }

    try {
        QJsonObject obj = doc.object();
        QJsonArray results = obj["results"].toArray();

        if (!results.isEmpty()) {
            QJsonObject firstResult = results.first().toObject();
            double lat = firstResult["geometry"].toObject()["location"].toObject()["lat"].toDouble();
            double lng = firstResult["geometry"].toObject()["location"].toObject()["lng"].toDouble();

            emit geocodingFinished(lat, lng);
        } else {
            emit geocodingError("Nie znaleziono wyników");
        }
    } catch (...) {
        emit geocodingError("Nie udało się przeanalizować odpowiedzi geokodowania");
    }
}

void ApiHandler::findStationsByAddress(const QString& address, double radiusKm) {
    QString encodedAddress = QUrl::toPercentEncoding(address);
    QUrl url(QString("https://maps.googleapis.com/maps/api/geocode/json?address=%1&key=YOUR_API_KEY")
                 .arg(encodedAddress));

    QNetworkRequest request(url);
    QNetworkReply* reply = m_geocoderManager.get(request);

    connect(reply, &QNetworkReply::finished,
            this, &ApiHandler::handleGeocodingReply);
    connect(reply, &QNetworkReply::errorOccurred,
            this, [this](QNetworkReply::NetworkError error) {
                emit geocodingError("Błąd sieci: " + QString::number(error));
            });
}

void ApiHandler::findStationsNearAddress(const QString& address, double radiusKm) {
    if (address.isEmpty()) {
        emit geocodingError("Adres nie może być pusty");
        return;
    }

    performGeocoding(address);
}

void ApiHandler::performGeocoding(const QString& address) {
    QString encodedAddress = QUrl::toPercentEncoding(address);
    QString url = QString("https://nominatim.openstreetmap.org/search?format=json&q=%1").arg(encodedAddress);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "AirQualityApp/1.0");

    QNetworkReply* reply = m_geocoderManager.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            emit geocodingError(reply->errorString());
            return;
        }

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseError);

        if (parseError.error != QJsonParseError::NoError || !doc.isArray()) {
            emit geocodingError("Nie można przetworzyć odpowiedzi geokodowania");
            return;
        }

        QJsonArray results = doc.array();
        if (results.isEmpty()) {
            emit geocodingError("Nie znaleziono lokalizacji");
            return;
        }

        QJsonObject firstResult = results.first().toObject();
        double lat = firstResult["lat"].toString().toDouble();
        double lon = firstResult["lon"].toString().toDouble();

        emit geocodingFinished(lat, lon);
    });
}

bool ApiHandler::isInternetAvailable() const {
    QTcpSocket socket;
    socket.connectToHost("8.8.8.8", 53);
    return socket.waitForConnected(1000);
}


void ApiHandler::updateStations(const QVector<Station>& stations) {
    QMutexLocker locker(&m_dataMutex);
    m_allStations = stations;
}

QVector<Station> ApiHandler::getAllStations() const {
    QMutexLocker locker(&m_dataMutex);
    return m_allStations;
}
