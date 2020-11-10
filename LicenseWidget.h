#ifndef GETLICENSEDIALOG_H
#define GETLICENSEDIALOG_H

#include <functional>

#include <QWidget>

#include "License.h"

namespace Ui {
class GetLicenseWidget;
}

class LicenseComm;
class LicenseWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LicenseWidget(const QString app_name, int app_id, int version, QUuid &guid,
                           const QString &order_page_url, QWidget *parent = 0);
    ~LicenseWidget();

    void checkLicense(const QString &label, std::function<void(int)>onLicenseValidated);
    QString userEmail() const;

protected:
    QString openLicenseWebPagePushButtonText();
    QByteArray getLicense();
    void setLicenseValid(const QString &buyerEmail);
    void setLicenseInvalid();
    void showLicense(const QString &license);
    void makePushButtonIntoWebLink();
    void saveLicenseToFile(const QByteArray &license);
    void addAboutAppWidget(QWidget *widget);
    void loadLicenseFromFile();

signals:
    void licenseValid(bool);

private slots:
    void validate(const QByteArray &license);
    void terminated(const QString &msg);
    void on_openLicenseWebPagePushButton_clicked();
    void on_validateLicensePushButton_clicked();

    void on_pushButtonDownloadLicense_clicked();

private:
    License m_license;
    LicenseComm *m_license_comm;
    std::function<void(int)> m_onLicenseValidated;
    QString m_order_page_url;
    Ui::GetLicenseWidget *ui;

};

#endif // GETLICENSEDIALOG_H
