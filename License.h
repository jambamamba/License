#pragma once


#include <QByteArray>
#include <QString>
#include <QPair>

#include "LicenseType.h"

class License
{
public:
    License();
    QPair<LicenseType, QString> validate(const QByteArray &license_) const;

    enum class Cmd : int
    {
        NONE = 0,
        GET_VERSION,
        GET_APP,
        CHECK_LICENSE,
    };

    enum class Status : int
    {
        LICENSE_NONE = 0,
        LICENSE_GOOD,
        LICENSE_EXPIRED,
        LICENSE_GUID_CHANGED,
        LICENSE_TERMINATED,
    };


private:
    QByteArray loadPrivateKey() const;
    int getRsaEncryptedSecret(const QByteArray &license, const QByteArray &privateKey) const;
    QString xmlDataFromLicense(int secret, const QByteArray& license) const;
    QPair<LicenseType, QString> parseDataFromLicenseXmlString(const QStringList &verificationString, const QString &xml) const;

};
