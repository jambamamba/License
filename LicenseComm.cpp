#include "LicenseComm.h"

#include <QApplication>
#include <QFile>
#include <QDir>
#include <QDesktopServices>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QVariantMap>
#include <QUuid>
#include <QUrl>

#include "NetworkRequest.h"

#include "License.h"

namespace
{
const char* getOS()
{
#if defined(Win32)
    return "win32";
#elif defined(Win64)
    return "win64";
#elif defined(Darwin)
    return "osx";
#elif defined(Linux)
    return "linux";
#endif
}
}//namespace

LicenseComm::LicenseComm(const QString app_name, int app_id, int version, QUuid &guid, QObject *parent)
    : QObject(parent)
    , m_app_id(app_id)
    , m_app_version(version)
    , m_guid(guid)
    , m_app_name(app_name)
    , m_network(new NetworkRequest(this))
{

}

LicenseComm::~LicenseComm()
{

}

void LicenseComm::checkLicense(const QString &email)
{
    QVariantMap map;
    map.insert("appid", m_app_id);
    map.insert("version", m_app_version);
    map.insert("cmd", (int)License::Cmd::CHECK_LICENSE);
    map.insert("guid", m_guid.toString());
    map.insert("email", email);
    map.insert("os", getOS());

    auto reply = m_network->post(QUrl("https://www.osletek.com/upgrade.php"),
                QJsonDocument(QJsonObject::fromVariantMap(map)).
                                    toJson(QJsonDocument::JsonFormat::Compact));
    connect(reply, &QNetworkReply::finished, [this,reply](){
        handleCheckLicenseResponse(reply);
    });
}


void LicenseComm::handleCheckLicenseResponse(QNetworkReply *reply)
{
    //TODO
    //if reply says license is valid, extract the license and save it in license.key file, then hide this button for good
    //if the guid is not registered, then open web browser with this link
    //
//        auto url = QUrl("https://www.osletek.com/?a=order&fileid=2203");
//        url = url.replace("&&", "&");
//        QDesktopServices::openUrl(QUrl(url));

    //based on email, lookup the row that matches with rt_transx->payer_email
    //and select from rt_order the row with rt_order->transxid matchind rt_transx-id
    //and find the rt_order->licensekey is non zero

    QString data = reply->readAll();
    reply->deleteLater();

    QJsonParseError error;
    auto obj = QJsonDocument::fromJson(data.toUtf8(), &error).object();
    qDebug() << "check license response" << data << "error" << error.errorString();
    if(error.error != QJsonParseError::ParseError::NoError)
    {
        qDebug() << "parse error in server reply:" << data
                 << "error" << error.errorString();
        return;
    }
    QString err = obj.contains("err") ? obj.value("err").toString() : "";
    License::Status status = obj.contains("status") ?
                (License::Status)obj.value("status").toInt() : License::Status::LICENSE_NONE;
    QString license = obj.value("license").toString();
    switch(status)
    {
    case License::Status::LICENSE_GOOD:
        if(!license.isEmpty())
        {
            validateLicense(license.toUtf8());
        }
        break;
    case License::Status::LICENSE_EXPIRED:
        openBrowserToPaymentPage();
        break;
    case License::Status::LICENSE_GUID_CHANGED:
        handleGuidEmailMismatch();
        break;
    case License::Status::LICENSE_TERMINATED:
        removeLicenseKeyFile(err);
        break;
    case License::Status::LICENSE_NONE:
    default:
        break;
    }
}

QString LicenseComm::licenseKeyPath() const
{
    QString path(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation));
    path = path.append(QDir::separator()).append(m_app_name);
    if(!QFile(path).exists())
    {
        QDir().mkpath(path);
    }
    if(!QFile(path).exists())
    {
        return "license.key";
    }
    return path.append(QDir::separator()).append("license.key");
}

void LicenseComm::removeLicenseKeyFile(const QString &err)
{
    QFile licenseFile(licenseKeyPath());

    QString msg = QString("Thank you for trying out %1.\n\n"
            "Your license has ended. Contact us at support@osletek.com\nif you have any questions.\n"
            ).
            arg(m_app_name);
    emit terminated(msg);
    if(!licenseFile.exists())
    {
        return;
    }
    licenseFile.remove();

    QMessageBox::question(QApplication::activeWindow(), "License Ended", msg,
          QMessageBox::Ok);
}

void LicenseComm::validateLicense(const QByteArray &license)
{
    emit validate(license.trimmed());
}
void LicenseComm::openBrowserToPaymentPage() const
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(QApplication::activeWindow(), "License Expired",
          QString("Thank you for using %1. "
                  "Your license has expired.\n\n"
                  "Would you purchase a license "
                  "so we can continue providing this software?").
                                  arg(m_app_name),
          QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::Yes)
    {
        openBrowser();
    }
}

void LicenseComm::openBrowser() const
{
    QDesktopServices::openUrl(QUrl(
                                  QString("https://www.osletek.com/?a=order&fileid=2203&guid=%1").
                                  arg(m_guid.toString())));
}
void LicenseComm::handleGuidEmailMismatch()
{
    return;//TODO
    QMessageBox::question(QApplication::activeWindow(), "License Moved",
                          "Your license is for a different computer.\n\n"
                          "Please contact us at support@osletek.com to resolve this issue.",
          QMessageBox::Ok);
}
