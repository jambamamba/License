#pragma once

#include <QObject>

class QNetworkReply;
class NetworkRequest;
class LicenseComm : public QObject
{
    Q_OBJECT
public:
    LicenseComm(const QString app_name, int app_id, int version, QUuid &guid, QObject *parent = nullptr);
    virtual ~LicenseComm();

    void checkLicense(const QString &email);
    QString licenseKeyPath() const;

signals:
    void validate(const QByteArray &license);
    void terminated(const QString &msg);

protected:
    void handleCheckLicenseResponse(QNetworkReply *reply);
    void openBrowserToPaymentPage() const;
    void openBrowser() const;
    void validateLicense(const QByteArray &license);
    void removeLicenseKeyFile(const QString &msg);
    void handleGuidEmailMismatch();

    int m_app_id;
    int m_app_version;
    QUuid &m_guid;
    const QString m_app_name;
    NetworkRequest *m_network;
};
