#include "Station.h"
#include <QDebug>
#include <QtMath>

Station::Station() {}

Station::Station(const QJsonObject &json) {
    //parsowanie podstawowych pól
    m_id = json["id"].toInt();
    m_name = json["stationName"].toString();
    m_latitude = json["gegrLat"].toString().toDouble(); // API zwraca string
    m_longitude = json["gegrLon"].toString().toDouble();

    //parsowanie zagnieżdżonej struktury adresu
    QJsonObject city = json["city"].toObject();
    m_address.cityId = city["id"].toInt();
    m_address.cityName = city["name"].toString();

    QJsonObject commune = city["commune"].toObject();
    m_address.communeName = commune["communeName"].toString();
    m_address.districtName = commune["districtName"].toString();
    m_address.provinceName = commune["provinceName"].toString();

    m_address.streetName = json["addressStreet"].toString();
}

QString Station::toString() const {
    return QString("Station %1: %2 (Lat: %3, Lon: %4)")
    .arg(id())
        .arg(name())
        .arg(latitude())
        .arg(longitude());
}

bool Station::isInCity(const QString& city) const {
    return m_address.cityName.compare(city, Qt::CaseInsensitive) == 0;
}

double Station::distanceTo(double lat, double lon) const {
    //(algorytm haversine)
    const double R = 6371.0; //promień ziemi w km
    double lat1 = qDegreesToRadians(m_latitude);
    double lon1 = qDegreesToRadians(m_longitude);
    double lat2 = qDegreesToRadians(lat);
    double lon2 = qDegreesToRadians(lon);

    double dlat = lat2 - lat1;
    double dlon = lon2 - lon1;

    double a = qSin(dlat / 2) * qSin(dlat / 2) +
               qCos(lat1) * qCos(lat2) *
                   qSin(dlon / 2) * qSin(dlon / 2);
    double c = 2 * qAtan2(qSqrt(a), qSqrt(1 - a));

    return R * c;
}

QString Station::toShortString() const {
    return QString("%1 (%2)").arg(m_name).arg(m_address.cityName);
}

QString Station::toFullString(double refLat, double refLon) const {
    QString str = QString("Stacja %1: %2\n"
                          "Lokalizacja: Lat %3, Lon %4\n")
                      .arg(m_id)
                      .arg(m_name)
                      .arg(m_latitude, 0, 'f', 6)
                      .arg(m_longitude, 0, 'f', 6);

    if (!std::isnan(refLat) && !std::isnan(refLon)) {
        str += QString("Odległość: %1\n")
                   .arg(distanceStringTo(refLat, refLon));
    }

    str += QString("Adres: %5, %6\n"
                   "Dystrykt: %7, Prowincja: %8")
               .arg(m_address.streetName)
               .arg(m_address.cityName)
               .arg(m_address.districtName)
               .arg(m_address.provinceName);

    return str;
}

QString Station::distanceStringTo(double lat, double lon) const {
    double dist = distanceTo(lat, lon);
    return QString("%1 km").arg(dist, 0, 'f', 2);
}

//json
void Station::setId(int id) { m_id = id; }
void Station::setName(const QString &name) { m_name = name; }
void Station::setLatitude(double lat) { m_latitude = lat; }
void Station::setLongitude(double lon) { m_longitude = lon; }
void Station::setAddressStreet(const QString &street) { m_addressStreet = street; }
void Station::setCityName(const QString &city) { m_cityName = city; }
QString Station::addressStreet() const { return m_addressStreet; }

